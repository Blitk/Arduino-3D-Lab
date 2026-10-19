
#include "emulator/AVR.hpp"

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>

namespace arduino3d::emulator {
namespace {

constexpr std::uint32_t kInstructionSizeBytes = 2U;
constexpr std::uint32_t kIoRegisterOffset = 0x20U;

std::uint16_t readProgramWord(const CPUState& cpu, std::uint32_t wordAddress) {
    const auto byteAddress = wordAddress * kInstructionSizeBytes;

    if (byteAddress + 1U >= Memory::FlashSize) {
        throw std::runtime_error("Leitura fora dos limites da Flash.");
    }

    const auto low = cpu.memory.readProgramByte(
        static_cast<std::uint16_t>(byteAddress)
    );
    const auto high = cpu.memory.readProgramByte(
        static_cast<std::uint16_t>(byteAddress + 1U)
    );

    return static_cast<std::uint16_t>(
        low | (static_cast<std::uint16_t>(high) << 8U)
    );
}

void validateProgramAddress(std::int32_t address) {
    if (address < 0 ||
        static_cast<std::uint32_t>(address) >
            CPUState::ProgramWordAddressMax) {
        throw std::runtime_error("Destino de salto fora dos limites da Flash.");
    }
}

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

    const bool halfCarry = subtraction
        ? ((!d3 && r3) || (r3 && q3) || (q3 && !d3))
        : ((d3 && r3) || (r3 && !q3) || (!q3 && d3));

    const bool carry = subtraction
        ? ((!d7 && r7) || (r7 && q7) || (q7 && !d7))
        : ((d7 && r7) || (r7 && !q7) || (!q7 && d7));

    const bool overflow = subtraction
        ? ((d7 && !r7 && !q7) || (!d7 && r7 && q7))
        : ((d7 && r7 && !q7) || (!d7 && !r7 && q7));

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

void setLogicalFlags(StatusRegister& sreg, std::uint8_t value) {
    const bool negative = (value & 0x80U) != 0;

    sreg.setFlag(StatusRegister::Zero, value == 0);
    sreg.setFlag(StatusRegister::Negative, negative);
    sreg.setFlag(StatusRegister::Overflow, false);
    sreg.setFlag(StatusRegister::Sign, negative);
}

bool isTwoWordInstruction(Opcode opcode) {
    return opcode == Opcode::JMP || opcode == Opcode::CALL;
}

std::uint8_t instructionCycles(
    const DecodedInstruction& instruction,
    const CPUState& cpu,
    std::uint16_t pcBeforeExecution,
    bool zeroBefore,
    bool bitBranchTaken
) {
    switch (instruction.opcode) {
    case Opcode::JMP:
        return 3;
    case Opcode::CALL:
        return 4;
    case Opcode::RET:
        return 4;
    case Opcode::RJMP:
        return 2;

    case Opcode::BREQ:
        return zeroBefore ? 2 : 1;
    case Opcode::BRNE:
        return zeroBefore ? 1 : 2;

    case Opcode::BRBC:
        return bitBranchTaken ? 2 : 1;

    case Opcode::SBI:
    case Opcode::CBI:
        return 2;

    case Opcode::SBRC: {
        const auto value = cpu.registers.read(
            cpu.memory, instruction.rd
        );

        if ((value & (1U << instruction.bit)) != 0) {
            return 1;
        }

        const auto skippedAddress =
            static_cast<std::uint32_t>(pcBeforeExecution) + 1U;

        if (skippedAddress > CPUState::ProgramWordAddressMax) {
            return 2;
        }

        try {
            const auto skipped = InstructionDecoder::decode(
                readProgramWord(cpu, skippedAddress)
            );
            return isTwoWordInstruction(skipped.opcode) ? 3 : 2;
        } catch (const InvalidInstruction&) {
            return 2;
        }
    }

    default:
        return 1;
    }
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
        return RunStatus::LimitReached;
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

    DecodedInstruction instruction{};

    try {
        result.word = readProgramWord(cpu_, cpu_.pc);
        instruction = InstructionDecoder::decode(result.word);

        if (isTwoWordInstruction(instruction.opcode) &&
            cpu_.pc >= CPUState::ProgramWordAddressMax) {
            result.status = StepStatus::ProgramCounterOutOfRange;
            result.message = "Instrucao de dois words ultrapassa a Flash.";
            return result;
        }
    } catch (const InvalidInstruction& exception) {
        result.status = StepStatus::InvalidInstruction;
        result.message = exception.what();
        return result;
    } catch (const std::exception& exception) {
        result.status = StepStatus::ProgramCounterOutOfRange;
        result.message = exception.what();
        return result;
    }

    const auto pcBeforeExecution = cpu_.pc;
    const bool zeroBefore = cpu_.sreg.flag(StatusRegister::Zero);
    bool bitBranchTaken = false;

    if (instruction.opcode == Opcode::BRBC) {
        const auto mask = static_cast<std::uint8_t>(
            1U << instruction.bit
        );
        bitBranchTaken = !cpu_.sreg.flag(mask);
    }

    try {
        execute(instruction);
    } catch (const InvalidInstruction& exception) {
        result.status = StepStatus::InvalidInstruction;
        result.message = exception.what();
        return result;
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

    result.cycles = instructionCycles(
        instruction, cpu_, pcBeforeExecution, zeroBefore, bitBranchTaken
    );
    result.status = StepStatus::Executed;
    return result;
}

void AVR::execute(const DecodedInstruction& instruction) {
    const auto nextPc = static_cast<std::int32_t>(cpu_.pc) + 1;

    switch (instruction.opcode) {
    case Opcode::NOP:
        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;

    case Opcode::EOR: {
        const auto rd = cpu_.registers.read(cpu_.memory, instruction.rd);
        const auto rr = cpu_.registers.read(cpu_.memory, instruction.rr);
        const auto value = static_cast<std::uint8_t>(rd ^ rr);

        cpu_.registers.write(cpu_.memory, instruction.rd, value);
        setLogicalFlags(cpu_.sreg, value);
        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;
    }

    case Opcode::SBRC: {
        const auto value = cpu_.registers.read(cpu_.memory, instruction.rd);
        const bool bitClear =
            (value & (1U << instruction.bit)) == 0;

        if (!bitClear) {
            cpu_.pc = static_cast<std::uint16_t>(nextPc);
            return;
        }

        const auto skippedAddress =
            static_cast<std::uint32_t>(cpu_.pc) + 1U;

        if (skippedAddress > CPUState::ProgramWordAddressMax) {
            throw std::runtime_error("SBRC ultrapassou o limite da Flash.");
        }

        const auto skipped = InstructionDecoder::decode(
            readProgramWord(cpu_, skippedAddress)
        );

        const auto destination = static_cast<std::int32_t>(cpu_.pc) +
            (isTwoWordInstruction(skipped.opcode) ? 3 : 2);

        validateProgramAddress(destination);
        cpu_.pc = static_cast<std::uint16_t>(destination);
        return;
    }

    case Opcode::LDI:
        cpu_.registers.write(
            cpu_.memory, instruction.rd, instruction.immediate
        );
        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;

    case Opcode::MOV: {
        const auto value = cpu_.registers.read(
            cpu_.memory, instruction.rr
        );
        cpu_.registers.write(cpu_.memory, instruction.rd, value);
        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;
    }
    case Opcode::MOVW: {
        const auto low = cpu_.registers.read(
            cpu_.memory, instruction.rr
        );

        const auto high = cpu_.registers.read(
            cpu_.memory,
            static_cast<std::uint8_t>(instruction.rr + 1U)
        );

        cpu_.registers.write(
            cpu_.memory, instruction.rd, low
        );

        cpu_.registers.write(
            cpu_.memory,
            static_cast<std::uint8_t>(instruction.rd + 1U),
            high
        );

        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;
    }

    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::CPI: 
    case Opcode::CP: {
        const auto rd = cpu_.registers.read(
            cpu_.memory, instruction.rd
        );

        const auto rr =
            (instruction.opcode == Opcode::CPI)
            ? instruction.immediate
            : cpu_.registers.read(cpu_.memory, instruction.rr);

        const bool subtraction = instruction.opcode != Opcode::ADD;

        const auto result = subtraction
            ? static_cast<std::uint8_t>(
                  static_cast<unsigned int>(rd) -
                  static_cast<unsigned int>(rr)
              )
            : static_cast<std::uint8_t>(
                  static_cast<unsigned int>(rd) +
                  static_cast<unsigned int>(rr)
              );

        setArithmeticFlags(cpu_.sreg, rd, rr, result, subtraction);

        if (instruction.opcode != Opcode::CPI &&
            instruction.opcode != Opcode::CP) {
            cpu_.registers.write(cpu_.memory, instruction.rd, result);
        }

        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;
    }

    case Opcode::JMP:
    case Opcode::CALL: {
        const auto secondWord = readProgramWord(
            cpu_, static_cast<std::uint32_t>(cpu_.pc) + 1U
        );

        const auto target =
            (static_cast<std::uint32_t>(instruction.word & 0x01F0U) << 13U) |
            (static_cast<std::uint32_t>(instruction.word & 0x0001U) << 16U) |
            secondWord;

        validateProgramAddress(static_cast<std::int32_t>(target));

        if (instruction.opcode == Opcode::CALL) {
            if (cpu_.sp < Memory::SramBegin + 1U) {
                throw std::runtime_error("Stack overflow durante CALL.");
            }

            const auto returnAddress =
                static_cast<std::uint16_t>(cpu_.pc + 2U);

            cpu_.memory.writeData(
                cpu_.sp,
                static_cast<std::uint8_t>((returnAddress >> 8U) & 0xFFU)
            );
            --cpu_.sp;

            cpu_.memory.writeData(
                cpu_.sp,
                static_cast<std::uint8_t>(returnAddress & 0xFFU)
            );
            --cpu_.sp;
        }

        cpu_.pc = static_cast<std::uint16_t>(target);
        return;
    }

    case Opcode::RET: {
        if (cpu_.sp > Memory::SramEnd - 2U) {
            throw std::runtime_error("Stack underflow durante RET.");
        }

        ++cpu_.sp;
        const auto low = cpu_.memory.readData(cpu_.sp);
        ++cpu_.sp;
        const auto high = cpu_.memory.readData(cpu_.sp);

        const auto returnAddress = static_cast<std::uint16_t>(
            low | (static_cast<std::uint16_t>(high) << 8U)
        );

        validateProgramAddress(returnAddress);
        cpu_.pc = returnAddress;
        return;
    }

    case Opcode::RJMP:
    case Opcode::BREQ:
    case Opcode::BRNE:
    case Opcode::BRBC: {
        bool takeBranch = true;

        switch (instruction.opcode) {
        case Opcode::BREQ:
            takeBranch = cpu_.sreg.flag(StatusRegister::Zero);
            break;
        case Opcode::BRNE:
            takeBranch = !cpu_.sreg.flag(StatusRegister::Zero);
            break;
        case Opcode::BRBC: {
            const auto mask = static_cast<std::uint8_t>(
                1U << instruction.bit
            );
            takeBranch = !cpu_.sreg.flag(mask);
            break;
        }
        default:
            break;
        }

        const auto destination = takeBranch
            ? nextPc + instruction.relative
            : nextPc;

        validateProgramAddress(destination);
        cpu_.pc = static_cast<std::uint16_t>(destination);
        return;
    }

    case Opcode::IN: {
        const auto dataAddress = static_cast<std::uint16_t>(
            kIoRegisterOffset + instruction.ioAddress
        );
        const auto value = cpu_.memory.readData(dataAddress);

        cpu_.registers.write(cpu_.memory, instruction.rd, value);
        cpu_.pc = static_cast<std::uint16_t>(nextPc);
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
                kIoRegisterOffset + instruction.ioAddress
            );
            cpu_.memory.writeData(dataAddress, value);
        }

        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;
    }

    case Opcode::SBI:
    case Opcode::CBI: {
        const auto dataAddress = static_cast<std::uint16_t>(
            kIoRegisterOffset + instruction.ioAddress
        );

        const auto mask = static_cast<std::uint8_t>(
            1U << instruction.bit
        );

        auto value = cpu_.memory.readData(dataAddress);

        if (instruction.opcode == Opcode::SBI) {
            value = static_cast<std::uint8_t>(value | mask);
        } else {
            value = static_cast<std::uint8_t>(
                value & static_cast<std::uint8_t>(~mask)
            );
        }

        if (gpio_ != nullptr) {
            gpio_->writeIo(instruction.ioAddress, value);
        } else {
            cpu_.memory.writeData(dataAddress, value);
        }

        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;
    }
    case Opcode::CPC: {
        const auto rd = cpu_.registers.read(
            cpu_.memory, instruction.rd
        );
        const auto rr = cpu_.registers.read(
            cpu_.memory, instruction.rr
        );

        const bool carryIn =
            cpu_.sreg.flag(StatusRegister::Carry);
        const bool zeroBefore =
            cpu_.sreg.flag(StatusRegister::Zero);

        const unsigned int carry =
            carryIn ? 1U : 0U;

        const auto result = static_cast<std::uint8_t>(
            static_cast<unsigned int>(rd) -
            static_cast<unsigned int>(rr) -
            carry
        );

        const unsigned int difference =
            static_cast<unsigned int>(rd) -
            static_cast<unsigned int>(rr) -
            carry;

        const bool halfCarry =
            (rd & 0x0FU) <
            (static_cast<unsigned int>(rr & 0x0FU) + carry);

        const bool carryOut =
            static_cast<unsigned int>(rd) <
            (static_cast<unsigned int>(rr) + carry);

        const bool overflow =
            ((rd & 0x80U) != 0) != ((rr & 0x80U) != 0)
                ? ((result & 0x80U) != 0) != ((rd & 0x80U) != 0)
                : false;

        const bool negative = (result & 0x80U) != 0;
        const bool zero = zeroBefore && result == 0;
        const bool sign = negative != overflow;

        (void)difference;

        cpu_.sreg.setFlag(StatusRegister::HalfCarry, halfCarry);
        cpu_.sreg.setFlag(StatusRegister::Carry, carryOut);
        cpu_.sreg.setFlag(StatusRegister::Overflow, overflow);
        cpu_.sreg.setFlag(StatusRegister::Negative, negative);
        cpu_.sreg.setFlag(StatusRegister::Zero, zero);
        cpu_.sreg.setFlag(StatusRegister::Sign, sign);

        cpu_.pc = static_cast<std::uint16_t>(nextPc);
        return;
    }
    }

    throw std::runtime_error("Opcode reconhecido, mas nao implementado.");
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
