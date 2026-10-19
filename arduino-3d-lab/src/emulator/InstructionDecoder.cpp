#include "emulator/InstructionDecoder.hpp"

#include <cstddef>
#include <string>

namespace arduino3d::emulator {
namespace {

std::string formatWord(std::uint16_t word) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result(4, '0');

    for (int i = 3; i >= 0; --i) {
        result[static_cast<std::size_t>(i)] = hex[word & 0x0F];
        word = static_cast<std::uint16_t>(word >> 4);
    }

    return result;
}

std::int16_t signExtend(std::uint16_t value, unsigned bits) {
    const auto signBit = static_cast<std::uint16_t>(1U << (bits - 1U));

    if ((value & signBit) != 0) {
        return static_cast<std::int16_t>(
            static_cast<int>(value) - static_cast<int>(1U << bits)
        );
    }

    return static_cast<std::int16_t>(value);
}

} // namespace

InvalidInstruction::InvalidInstruction(std::uint16_t word)
    : std::runtime_error("Opcode AVR invalido: 0x" + formatWord(word)) {
}

DecodedInstruction InstructionDecoder::decode(std::uint16_t word) {
    DecodedInstruction instruction{};
    instruction.word = word;

    if (word == 0x0000) {
        instruction.opcode = Opcode::NOP;
        return instruction;
    }

    // LDI: 1110 KKKK dddd KKKK
    if ((word & 0xF000) == 0xE000) {
        instruction.opcode = Opcode::LDI;
        instruction.rd = static_cast<std::uint8_t>(
            16U + ((word >> 4U) & 0x0FU)
        );
        instruction.immediate = static_cast<std::uint8_t>(
            (word & 0x000FU) | ((word >> 4U) & 0x00F0U)
        );
        return instruction;
    }

    // CPI: 0011 KKKK dddd KKKK
    if ((word & 0xF000) == 0x3000) {
        instruction.opcode = Opcode::CPI;
        instruction.rd = static_cast<std::uint8_t>(
            16U + ((word >> 4U) & 0x0FU)
        );
        instruction.immediate = static_cast<std::uint8_t>(
            (word & 0x000FU) | ((word >> 4U) & 0x00F0U)
        );
        return instruction;
    }

    // IN: 1011 0AAd dddd AAAA
    if ((word & 0xF800) == 0xB000) {
        instruction.opcode = Opcode::IN;
        instruction.rd = static_cast<std::uint8_t>(
            (word >> 4U) & 0x1FU
        );
        instruction.ioAddress = static_cast<std::uint8_t>(
            ((word >> 5U) & 0x30U) | (word & 0x0FU)
        );
        return instruction;
    }

    // OUT: 1011 1AAr rrrr AAAA
    if ((word & 0xF800) == 0xB800) {
        instruction.opcode = Opcode::OUT;
        instruction.rr = static_cast<std::uint8_t>(
            (word >> 4U) & 0x1FU
        );
        instruction.ioAddress = static_cast<std::uint8_t>(
            ((word >> 5U) & 0x30U) | (word & 0x0FU)
        );
        return instruction;
    }

    const auto destination = static_cast<std::uint8_t>(
        (word >> 4U) & 0x1FU
    );

    const auto source = static_cast<std::uint8_t>(
        (word & 0x0FU) | ((word >> 5U) & 0x10U)
    );

    // MOV: 0010 11rd dddd rrrr
    if ((word & 0xFC00) == 0x2C00) {
        instruction.opcode = Opcode::MOV;
        instruction.rd = destination;
        instruction.rr = source;
        return instruction;
    }

    // ADD: 0000 11rd dddd rrrr
    if ((word & 0xFC00) == 0x0C00) {
        instruction.opcode = Opcode::ADD;
        instruction.rd = destination;
        instruction.rr = source;
        return instruction;
    }

    // SUB: 0001 10rd dddd rrrr
    if ((word & 0xFC00) == 0x1800) {
        instruction.opcode = Opcode::SUB;
        instruction.rd = destination;
        instruction.rr = source;
        return instruction;
    }

    // RJMP: 1100 kkkk kkkk kkkk
    if ((word & 0xF000) == 0xC000) {
        instruction.opcode = Opcode::RJMP;
        instruction.relative = signExtend(word & 0x0FFFU, 12);
        return instruction;
    }

    // BREQ: 1111 00kk kkkk 0000
    // BRNE: 1111 00kk kkkk 0001
    const auto branch = static_cast<std::uint16_t>(word & 0xFC07U);

    if (branch == 0xF000 || branch == 0xF001) {
        instruction.opcode =
            branch == 0xF000 ? Opcode::BREQ : Opcode::BRNE;
        instruction.relative = signExtend((word >> 3U) & 0x7FU, 7);
        return instruction;
    }

    throw InvalidInstruction(word);
}

} // namespace arduino3d::emulator
