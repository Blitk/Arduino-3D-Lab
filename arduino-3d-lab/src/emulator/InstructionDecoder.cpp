
#include "emulator/InstructionDecoder.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace arduino3d::emulator {
namespace {

constexpr std::uint16_t kNop = 0x0000;

constexpr std::uint16_t kImmediateMask = 0xF000;
constexpr std::uint16_t kRegisterMask = 0xFC00;
constexpr std::uint16_t kBranchMask = 0xFC07;
constexpr std::uint16_t kJumpMask = 0xFE0E;
constexpr std::uint16_t kSkipMask = 0xFE08;

std::string formatWord(std::uint16_t word) {
    constexpr char hexDigits[] = "0123456789ABCDEF";
    std::string result(4, '0');

    for (int index = 3; index >= 0; --index) {
        result[static_cast<std::size_t>(index)] =
            hexDigits[word & 0x0FU];
        word = static_cast<std::uint16_t>(word >> 4U);
    }

    return result;
}

std::int16_t signExtend(std::uint16_t value, unsigned bits) {
    const auto signBit = static_cast<std::uint16_t>(1U << (bits - 1U));

    if ((value & signBit) == 0) {
        return static_cast<std::int16_t>(value);
    }

    return static_cast<std::int16_t>(
        static_cast<int>(value) - static_cast<int>(1U << bits)
    );
}

std::uint8_t decodeRegisterD(std::uint16_t word) {
    return static_cast<std::uint8_t>((word >> 4U) & 0x1FU);
}

std::uint8_t decodeRegisterR(std::uint16_t word) {
    return static_cast<std::uint8_t>(
        (word & 0x0FU) | ((word >> 5U) & 0x10U)
    );
}

std::uint8_t decodeImmediate(std::uint16_t word) {
    return static_cast<std::uint8_t>(
        (word & 0x0FU) | ((word >> 4U) & 0xF0U)
    );
}

std::uint8_t decodeIoAddress(std::uint16_t word) {
    return static_cast<std::uint8_t>(
        ((word >> 5U) & 0x30U) | (word & 0x0FU)
    );
}

DecodedInstruction makeRegisterInstruction(
    Opcode opcode,
    std::uint16_t word
) {
    DecodedInstruction instruction{};
    instruction.opcode = opcode;
    instruction.word = word;
    instruction.rd = decodeRegisterD(word);
    instruction.rr = decodeRegisterR(word);
    return instruction;
}

} // namespace

InvalidInstruction::InvalidInstruction(std::uint16_t word)
    : std::runtime_error("Opcode AVR invalido: 0x" + formatWord(word)) {
}

DecodedInstruction InstructionDecoder::decode(std::uint16_t word) {
    DecodedInstruction instruction{};
    instruction.word = word;

    if (word == kNop) {
        instruction.opcode = Opcode::NOP;
        return instruction;
    }

    // LDI: 1110 KKKK dddd KKKK
    if ((word & kImmediateMask) == 0xE000U) {
        instruction.opcode = Opcode::LDI;
        instruction.rd = static_cast<std::uint8_t>(
            16U + ((word >> 4U) & 0x0FU)
        );
        instruction.immediate = decodeImmediate(word);
        return instruction;
    }

    // CPI: 0011 KKKK dddd KKKK
    if ((word & kImmediateMask) == 0x3000U) {
        instruction.opcode = Opcode::CPI;
        instruction.rd = static_cast<std::uint8_t>(
            16U + ((word >> 4U) & 0x0FU)
        );
        instruction.immediate = decodeImmediate(word);
        return instruction;
    }

    // IN: 1011 0AAd dddd AAAA
    if ((word & 0xF800U) == 0xB000U) {
        instruction.opcode = Opcode::IN;
        instruction.rd = decodeRegisterD(word);
        instruction.ioAddress = decodeIoAddress(word);
        return instruction;
    }

    // OUT: 1011 1AAr rrrr AAAA
    if ((word & 0xF800U) == 0xB800U) {
        instruction.opcode = Opcode::OUT;
        instruction.rr = decodeRegisterD(word);
        instruction.ioAddress = decodeIoAddress(word);
        return instruction;
    }



    // MOV: 0010 11rd dddd rrrr
    if ((word & kRegisterMask) == 0x2C00U) {
        return makeRegisterInstruction(Opcode::MOV, word);
    }

    // ADD: 0000 11rd dddd rrrr
    if ((word & kRegisterMask) == 0x0C00U) {
        return makeRegisterInstruction(Opcode::ADD, word);
    }

    // SUB: 0001 10rd dddd rrrr
    if ((word & kRegisterMask) == 0x1800U) {
        return makeRegisterInstruction(Opcode::SUB, word);
    }

    // EOR: 0010 01rd dddd rrrr
    if ((word & kRegisterMask) == 0x2400U) {
        return makeRegisterInstruction(Opcode::EOR, word);
    }

    // RJMP: 1100 kkkk kkkk kkkk
    if ((word & kImmediateMask) == 0xC000U) {
        instruction.opcode = Opcode::RJMP;
        instruction.relative = signExtend(word & 0x0FFFU, 12);
        return instruction;
    }

    // BREQ / BRNE: desvios condicionais baseados na flag Zero.
    const auto branch = static_cast<std::uint16_t>(word & kBranchMask);

    if (branch == 0xF000U || branch == 0xF001U) {
        instruction.opcode =
            (branch == 0xF000U) ? Opcode::BREQ : Opcode::BRNE;
        instruction.relative = signExtend((word >> 3U) & 0x7FU, 7);
        return instruction;
    }

    // BRBC: Branch if Bit in SREG is Cleared.
    // Formato: 1111 01kk kkkk ksss
    if ((word & 0xFC00U) == 0xF400U) {
        instruction.opcode = Opcode::BRBC;
        instruction.bit = static_cast<std::uint8_t>(word & 0x07U);
        instruction.relative = signExtend((word >> 3U) & 0x7FU, 7);
        return instruction;
    }

    // JMP: instrução de dois words.
    if ((word & kJumpMask) == 0x940CU) {
        instruction.opcode = Opcode::JMP;
        return instruction;
    }

    // CALL: chamada absoluta, instrução de dois words.
    if ((word & kJumpMask) == 0x940EU) {
        instruction.opcode = Opcode::CALL;
        return instruction;
    }

    // RET: retorno de sub-rotina.
    if (word == 0x9508U) {
        instruction.opcode = Opcode::RET;
        return instruction;
    }

    // SBRC: Skip if Bit in Register is Cleared.
    // Formato: 1111 110r rrrr 0bbb
    if ((word & kSkipMask) == 0xFC00U) {
        instruction.opcode = Opcode::SBRC;
        instruction.rd = decodeRegisterD(word);
        instruction.bit = static_cast<std::uint8_t>(word & 0x07U);
        return instruction;
    }


        // SBI: Set Bit in I/O Register
    if ((word & 0xFF00U) == 0x9A00U) {
        instruction.opcode = Opcode::SBI;
        instruction.ioAddress =
            static_cast<std::uint8_t>((word >> 3U) & 0x1FU);
        instruction.bit = static_cast<std::uint8_t>(word & 0x07U);
        return instruction;
    }

    // CBI: Clear Bit in I/O Register
    if ((word & 0xFF00U) == 0x9800U) {
        instruction.opcode = Opcode::CBI;
        instruction.ioAddress =
            static_cast<std::uint8_t>((word >> 3U) & 0x1FU);
        instruction.bit = static_cast<std::uint8_t>(word & 0x07U);
        return instruction;
    }

    // MOVW: Copy Register Word
    // Formato: 0000 0001 dddd rrrr
    if ((word & 0xFF00U) == 0x0100U) {
        instruction.opcode = Opcode::MOVW;

        instruction.rd = static_cast<std::uint8_t>(
            ((word >> 4U) & 0x0FU) * 2U
        );

        instruction.rr = static_cast<std::uint8_t>(
            (word & 0x0FU) * 2U
        );

        return instruction;
    }

    // CP: Compare Registers
    // Formato: 0001 01rd dddd rrrr
    if ((word & 0xFC00U) == 0x1400U) {
        instruction.opcode = Opcode::CP;
        instruction.rd = decodeRegisterD(word);
        instruction.rr = decodeRegisterR(word);
        return instruction;
    }

    // CPC: Compare with Carry
    // Formato: 0000 01rd dddd rrrr
    if ((word & 0xFC00U) == 0x0400U) {
        instruction.opcode = Opcode::CPC;
        instruction.rd = decodeRegisterD(word);
        instruction.rr = decodeRegisterR(word);
        return instruction;
    }
    
    throw InvalidInstruction(word);
}

} // namespace arduino3d::emulator
