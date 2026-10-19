
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace arduino3d::emulator {

class Memory {
public:
    static constexpr std::size_t FlashSize = 32 * 1024;
    static constexpr std::size_t EepromSize = 1024;
    static constexpr std::size_t DataSize = 0x900;

    static constexpr std::uint16_t GeneralRegistersBegin = 0x0000;
    static constexpr std::uint16_t GeneralRegistersEnd = 0x001F;
    static constexpr std::uint16_t IoBegin = 0x0020;
    static constexpr std::uint16_t IoEnd = 0x005F;
    static constexpr std::uint16_t ExtendedIoBegin = 0x0060;
    static constexpr std::uint16_t ExtendedIoEnd = 0x00FF;
    static constexpr std::uint16_t SramBegin = 0x0100;
    static constexpr std::uint16_t SramEnd = 0x08FF;

    Memory();

    void reset();

    std::uint8_t readProgramByte(std::uint16_t address) const;
    void writeProgramByte(std::uint16_t address, std::uint8_t value);

    std::uint8_t readData(std::uint16_t address) const;
    void writeData(std::uint16_t address, std::uint8_t value);

    std::uint8_t readEeprom(std::uint16_t address) const;
    void writeEeprom(std::uint16_t address, std::uint8_t value);

    void loadProgram(
        const std::vector<std::uint8_t>& bytes,
        std::size_t byteOffset = 0
    );

    const std::vector<std::uint8_t>& program() const noexcept;
    const std::vector<std::uint8_t>& data() const noexcept;
    const std::array<std::uint8_t, EepromSize>& eeprom() const noexcept;

private:
    std::vector<std::uint8_t> program_;
    std::vector<std::uint8_t> data_;
    std::array<std::uint8_t, EepromSize> eeprom_{};
};

} // namespace arduino3d::emulator
