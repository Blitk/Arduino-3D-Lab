#include "emulator/InstructionDecoder.hpp"
#include <cstddef>
#include <string>

namespace arduino3d::emulator {

InvalidInstruction::InvalidInstruction(std::uint16_t word)
    : std::runtime_error(
          "Opcode AVR invalido: 0x" + [] (std::uint16_t value) {
              constexpr char hex[] = "0123456789ABCDEF";
              std::string result(4, '0');

              for (int i = 3; i >= 0; --i) {
                  result[static_cast<std::size_t>(i)] =
                      hex[value & 0x0F];
                  value = static_cast<std::uint16_t>(value >> 4);
              }

              return result;
          }(word)) {
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
            16 + ((word >> 4) & 0x0F));
        instruction.immediate = static_cast<std::uint8_t>(
            (word & 0x000F) | ((word >> 4) & 0x00F0));
        return instruction;
    }

    // CPI: 0011 KKKK dddd KKKK
    if ((word & 0xF000) == 0x3000) {
        instruction.opcode = Opcode::CPI;
        instruction.rd = static_cast<std::uint8_t>(
            16 + ((word >> 4) & 0x0F));
        instruction.immediate = static_cast<std::uint8_t>(
            (word & 0x000F) | ((word >> 4) & 0x00F0));
        return instruction;
    }

    // MOV: 0010 11rd dddd rrrr
    // ADD: 0000 11rd dddd rrrr
    // SUB: 0001 10rd dddd rrrr
    const auto destination = static_cast<std::uint8_t>(
        (word >> 4) & 0x1F);

    const auto source = static_cast<std::uint8_t>(
        (word & 0x0F) | ((word >> 5) & 0x10));

    if ((word & 0xFC00) == 0x2C00) {
        instruction.opcode = Opcode::MOV;
    } else if ((word & 0xFC00) == 0x0C00) {
        instruction.opcode = Opcode::ADD;
    } else if ((word & 0xFC00) == 0x1800) {
        instruction.opcode = Opcode::SUB;
    } else {
        // RJMP: 1100 kkkk kkkk kkkk
        if ((word & 0xF000) == 0xC000) {
            instruction.opcode = Opcode::RJMP;

            int offset = word & 0x0FFF;
            if ((offset & 0x0800) != 0) {
                offset -= 0x1000;
            }

            instruction.relative = static_cast<std::int8_t>(offset);
            return instruction;
        }

        // BREQ: 1111 00kk kkkk 0000
        // BRNE: 1111 00kk kkkk 0001
        const auto branch = static_cast<std::uint16_t>(word & 0xFC07);

        if (branch == 0xF000 || branch == 0xF001) {
            instruction.opcode =
                branch == 0xF000 ? Opcode::BREQ : Opcode::BRNE;

            int offset = (word >> 3) & 0x7F;
            if ((offset & 0x40) != 0) {
                offset -= 0x80;
            }

            instruction.relative = static_cast<std::int8_t>(offset);
            return instruction;
        }

        throw InvalidInstruction(word);
    }

    instruction.rd = destination;
    instruction.rr = source;
    return instruction;
}

} // namespace arduino3d::emulator
