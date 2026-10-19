#pragma once

#include <cstdint>
#include <stdexcept>

namespace arduino3d::emulator {

enum class Opcode {
    NOP,
    LDI,
    MOV,
    ADD,
    SUB,
    EOR,
    CPI,
    RJMP,
    JMP,
    CALL,
    RET,
    BREQ,
    BRBC,
    BRNE,
    IN,
    SBRC,
    OUT,
    SBI,
    CBI,
    MOVW,
    CP,
    CPC,
};

struct DecodedInstruction {
    Opcode opcode{};
    std::uint16_t word{0};
    std::uint8_t bit{0};
    std::uint8_t rd{0};
    std::uint8_t rr{0};
    std::uint8_t immediate{0};
    std::int16_t relative{0};
    std::uint8_t ioAddress{0};
};

class InvalidInstruction final : public std::runtime_error {
public:
    explicit InvalidInstruction(std::uint16_t word);
};

class InstructionDecoder {
public:
    static DecodedInstruction decode(std::uint16_t word);
};

} // namespace arduino3d::emulator
