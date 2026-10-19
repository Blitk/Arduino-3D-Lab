
#include "emulator/Memory.hpp"

#include <algorithm>
#include <stdexcept>

namespace arduino3d::emulator {

Memory::Memory()
    : program_(FlashSize, 0xFF),
      data_(DataSize, 0x00) {
    eeprom_.fill(0xFF);
}

void Memory::reset() {
    std::fill(program_.begin(), program_.end(), 0xFF);
    std::fill(data_.begin(), data_.end(), 0x00);
    eeprom_.fill(0xFF);
}

std::uint8_t Memory::readProgramByte(std::uint16_t address) const {
    if (address >= FlashSize) {
        throw std::out_of_range("Endereco fora da Flash AVR.");
    }

    return program_[address];
}

void Memory::writeProgramByte(
    std::uint16_t address,
    std::uint8_t value
) {
    if (address >= FlashSize) {
        throw std::out_of_range("Endereco fora da Flash AVR.");
    }

    program_[address] = value;
}

std::uint8_t Memory::readData(std::uint16_t address) const {
    if (address >= DataSize) {
        throw std::out_of_range("Endereco fora do espaco de dados AVR.");
    }

    return data_[address];
}

void Memory::writeData(
    std::uint16_t address,
    std::uint8_t value
) {
    if (address >= DataSize) {
        throw std::out_of_range("Endereco fora do espaco de dados AVR.");
    }

    data_[address] = value;
}

std::uint8_t Memory::readEeprom(std::uint16_t address) const {
    if (address >= EepromSize) {
        throw std::out_of_range("Endereco fora da EEPROM AVR.");
    }

    return eeprom_[address];
}

void Memory::writeEeprom(
    std::uint16_t address,
    std::uint8_t value
) {
    if (address >= EepromSize) {
        throw std::out_of_range("Endereco fora da EEPROM AVR.");
    }

    eeprom_[address] = value;
}

void Memory::loadProgram(
    const std::vector<std::uint8_t>& bytes,
    std::size_t byteOffset
) {
    if (byteOffset > FlashSize ||
        bytes.size() > FlashSize - byteOffset) {
        throw std::out_of_range("Programa excede a capacidade da Flash.");
    }

    std::copy(
        bytes.begin(),
        bytes.end(),
        program_.begin() + static_cast<std::ptrdiff_t>(byteOffset)
    );
}

const std::vector<std::uint8_t>& Memory::program() const noexcept {
    return program_;
}

const std::vector<std::uint8_t>& Memory::data() const noexcept {
    return data_;
}

const std::array<std::uint8_t, Memory::EepromSize>&
Memory::eeprom() const noexcept {
    return eeprom_;
}

} // namespace arduino3d::emulator
