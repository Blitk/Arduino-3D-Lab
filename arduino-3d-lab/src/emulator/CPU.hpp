
#pragma once

#include "emulator/Memory.hpp"

#include <cstdint>
#include <string>

namespace arduino3d::emulator {

struct GeneralRegisters {
    static constexpr std::uint8_t Count = 32;

    std::uint8_t read(const Memory& memory, std::uint8_t index) const;
    void write(Memory& memory, std::uint8_t index, std::uint8_t value) const;
};

struct StatusRegister {
    // Bits do SREG AVR: I T H S V N Z C.
    static constexpr std::uint8_t Carry = 1U << 0U;
    static constexpr std::uint8_t Zero = 1U << 1U;
    static constexpr std::uint8_t Negative = 1U << 2U;
    static constexpr std::uint8_t Overflow = 1U << 3U;
    static constexpr std::uint8_t Sign = 1U << 4U;
    static constexpr std::uint8_t HalfCarry = 1U << 5U;
    static constexpr std::uint8_t Transfer = 1U << 6U;
    static constexpr std::uint8_t InterruptEnable = 1U << 7U;

    std::uint8_t value{0};

    bool flag(std::uint8_t mask) const noexcept;
    void setFlag(std::uint8_t mask, bool enabled) noexcept;
    void reset() noexcept;
};

struct CPUState {
    static constexpr std::uint16_t ProgramWordAddressMax = 0x3FFF;
    static constexpr std::uint16_t InitialStackPointer = 0x08FF;

    Memory memory;
    GeneralRegisters registers;
    std::uint16_t pc{0}; // Endereço em palavras de instrução.
    std::uint16_t sp{InitialStackPointer}; // Endereço em bytes.
    StatusRegister sreg;

    void reset();
    bool validate(std::string* error = nullptr) const;
};

} // namespace arduino3d::emulator
