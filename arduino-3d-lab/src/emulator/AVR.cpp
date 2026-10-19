#include "emulator/AVR.hpp"

#include <cstdint>
#include <exception>
#include <string>

namespace arduino3d::emulator {
namespace {

void setArithmeticFlags(
    StatusRegister& sreg,
    std::uint8_t rd,
    std::uint8_t rr,
    std::uint8_t result,
    bool subtraction
) {
    const bool d3 = (rd & 0x08U) != 0;
    const bool r3 = (rr & 0x08U) != 0;
    const bool q3 = (result & 0x08U) != 0;

    const bool d7 = (rd & 0x80U) != 0;
    const bool r7 = (rr & 0x80U) != 0;
    const bool q7 = (result & 0x80U) != 0;

    bool halfCarry;
    bool carry;
    bool overflow;

    if (subtraction) {
        halfCarry = (!d3 && r3) || (r3 && q3) || (q3 && !d3);
        carry = (!d7 && r7) || (r7 && q7) || (q7 && !d7);
        overflow = (d7 && !r7 && !q7) || (!d7 && r7 && q7);
    } else {
        halfCarry = (d3 && r3) || (r3 && !q3) || (!q3 && d3);
        carry = (d7 && r7) || (r7 && !q7) || (!q7 && d7);
        overflow = (d7 && r7 && !q7) || (!d7 && !r7 && q7);
    }

    const bool negative = q7;
    const bool zero = result == 0;
    const bool sign = negative != overflow;

    sreg.setFlag(StatusRegister::HalfCarry, halfCarry);
    sreg.setFlag(StatusRegister::Carry, carry);
    sreg.setFlag(StatusRegister::Overflow, overflow);
    sreg.setFlag(StatusRegister::Negative, negative);
    sreg.setFlag(StatusRegister::Zero, zero);
    sreg.setFlag(StatusRegister::Sign, sign);
}

RunStatus toRunStatus(StepStatus status) {
    switch (status) {
    case StepStatus::InvalidInstruction:
        return RunStatus::InvalidInstruction;
    case StepStatus::ProgramCounterOutOfRange:
        return RunStatus::ProgramCounterOutOfRange;
    case StepStatus::InvalidCpuState:
        return RunStatus::InvalidCpuState;
    case StepStatus::PeripheralError:
        return RunStatus::PeripheralError;
    case StepStatus::Executed:
        break;
    }

    return RunStatus::LimitReached;
}

} // namespace

AVR::AVR(CPUState& cpu, GPIO* gpio) noexcept
    : cpu_(cpu), gpio_(gpio) {
}

CPUState& AVR::cpu() noexcept {
    return cpu_;
}

const CPUState& AVR::cpu() const noexcept {
    return cpu_;
}

StepResult AVR::step() {
    StepResult result;
    result.pcBefore = cpu_.pc;

    if (cpu_.pc > CPUState::ProgramWordAddressMax) {
        result.status = StepStatus::ProgramCounterOutOfRange;
        result.message = "PC ultrapassou o limite da Flash.";
        return result;
    }

    std::string validationError;

    if (!cpu_.validate(&validationError)) {
        result.status = StepStatus::InvalidCpuState;
        result.message = validationError;
        return result;
    }

    const auto byteAddress =
        static_cast<std::uint32_t>(cpu_.pc) * 2U;

    if (byteAddress + 1U >= Memory::FlashSize) {
        result.status = StepStatus::ProgramCounterOutOfRange;
        result.message = "Instrucao ultrapassa o limite da Flash.";
        return result;
    }

    const auto low = cpu_.memory.readProgramByte(
        static_cast<std::uint16_t>(byteAddress)
    );
    const auto high = cpu_.memory.readProgramByte(
        static_cast<std::uint16_t>(byteAddress + 1U)
    );

    result.word = static_cast<std::uint16_t>(
        low | (static_cast<std::uint16_t>(high) << 8U)
    );

    DecodedInstruction instruction{};

    try {
        instruction = InstructionDecoder::decode(result.word);
    } catch (const InvalidInstruction& exception) {
        result.status = StepStatus::InvalidInstruction;
        result.message = exception.what();
        return result;
    }

    try {
        const bool zeroBefore = cpu_.sreg.flag(StatusRegister::Zero);
        execute(instruction);
    } catch (const std::exception& exception) {
        result.status = StepStatus::InvalidCpuState;
        result.message = exception.what();
        return result;
    }

    if (gpio_ != nullptr) {
        std::string gpioError;

        if (!gpio_->synchronize(&gpioError)) {
            result.status = StepStatus::PeripheralError;
            result.message = gpioError;
            return result;
        }
    }
    switch (instruction.opcode) {
    case Opcode::RJMP:
            result.cycles = 2;
            break;

    case Opcode::BREQ:
            result.cycles = zeroBefore ? 2 : 1;
            break;

    case Opcode::BRNE:
            result.cycles = zeroBefore ? 1 : 2;
            break;

    default:
            result.cycles = 1;
            break;
    }
    result.status = StepStatus::Executed;
    return result;
}

void AVR::execute(const DecodedInstruction& instruction) {
    const auto nextPc = static_cast<std::uint16_t>(cpu_.pc + 1U);

    switch (instruction.opcode) {
    case Opcode::NOP:
        cpu_.pc = nextPc;
        return;

    case Opcode::LDI:
        cpu_.registers.write(
            cpu_.memory, instruction.rd, instruction.immediate
        );
        cpu_.pc = nextPc;
        return;

    case Opcode::MOV: {
        const auto value = cpu_.registers.read(
            cpu_.memory, instruction.rr
        );
        cpu_.registers.write(
            cpu_.memory, instruction.rd, value
        );
        cpu_.pc = nextPc;
        return;
    }

    case Opcode::ADD: {
        const auto rd = cpu_.registers.read(
            cpu_.memory, instruction.rd
        );
        const auto rr = cpu_.registers.read(
            cpu_.memory, instruction.rr
        );

        const auto sum = static_cast<std::uint16_t>(rd) + rr;
        const auto value = static_cast<std::uint8_t>(sum & 0xFFU);

        setArithmeticFlags(cpu_.sreg, rd, rr, value, false);
        cpu_.registers.write(cpu_.memory, instruction.rd, value);
        cpu_.pc = nextPc;
        return;
    }

    case Opcode::SUB: {
        const auto rd = cpu_.registers.read(
            cpu_.memory, instruction.rd
        );
        const auto rr = cpu_.registers.read(
            cpu_.memory, instruction.rr
        );

        const auto value = static_cast<std::uint8_t>(
            static_cast<unsigned int>(rd) -
            static_cast<unsigned int>(rr)
        );

        setArithmeticFlags(cpu_.sreg, rd, rr, value, true);
        cpu_.registers.write(cpu_.memory, instruction.rd, value);
        cpu_.pc = nextPc;
        return;
    }

    case Opcode::CPI: {
        const auto rd = cpu_.registers.read(
            cpu_.memory, instruction.rd
        );
        const auto value = static_cast<std::uint8_t>(
            static_cast<unsigned int>(rd) -
            static_cast<unsigned int>(instruction.immediate)
        );

        setArithmeticFlags(
            cpu_.sreg, rd, instruction.immediate, value, true
        );
        cpu_.pc = nextPc;
        return;
    }

    case Opcode::RJMP:
        cpu_.pc = static_cast<std::uint16_t>(
            static_cast<int>(nextPc) + instruction.relative
        );
        return;

    case Opcode::BREQ:
        cpu_.pc = cpu_.sreg.flag(StatusRegister::Zero)
            ? static_cast<std::uint16_t>(
                  static_cast<int>(nextPc) + instruction.relative
              )
            : nextPc;
        return;

    case Opcode::BRNE:
        cpu_.pc = !cpu_.sreg.flag(StatusRegister::Zero)
            ? static_cast<std::uint16_t>(
                  static_cast<int>(nextPc) + instruction.relative
              )
            : nextPc;
        return;

    case Opcode::IN: {
        const auto dataAddress = static_cast<std::uint16_t>(
            0x20U + instruction.ioAddress
        );
        const auto value = cpu_.memory.readData(dataAddress);

        cpu_.registers.write(cpu_.memory, instruction.rd, value);
        cpu_.pc = nextPc;
        return;
    }

    case Opcode::OUT: {
        const auto value = cpu_.registers.read(
            cpu_.memory, instruction.rr
        );

        if (gpio_ != nullptr) {
            gpio_->writeIo(instruction.ioAddress, value);
        } else {
            const auto dataAddress = static_cast<std::uint16_t>(
                0x20U + instruction.ioAddress
            );
            cpu_.memory.writeData(dataAddress, value);
        }

        cpu_.pc = nextPc;
        return;
    }
    }
}

RunResult AVR::run(std::size_t instructionLimit) {
    RunResult result;

    while (result.instructionsExecuted < instructionLimit) {
        const auto stepResult = step();
        result.lastStep = stepResult;

        if (!stepResult.ok()) {
            result.status = toRunStatus(stepResult.status);
            return result;
        }

        ++result.instructionsExecuted;
    }

    result.status = RunStatus::LimitReached;
    return result;
}

} // namespace arduino3d::emulator
