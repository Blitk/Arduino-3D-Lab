
#include "emulator/CPU.hpp"
#include <stdexcept>
#include <string>

namespace arduino3d::emulator {

std::uint8_t GeneralRegisters::read(
    const Memory& memory,
    std::uint8_t index
) const {
    if (index >= Count) {
        throw std::out_of_range("Indice de registrador AVR invalido.");
    }

    return memory.readData(index);
}

void GeneralRegisters::write(
    Memory& memory,
    std::uint8_t index,
    std::uint8_t value
) const {
    if (index >= Count) {
        throw std::out_of_range("Indice de registrador AVR invalido.");
    }

    memory.writeData(index, value);
}

bool StatusRegister::flag(std::uint8_t mask) const noexcept {
    return (value & mask) != 0;
}

void StatusRegister::setFlag(
    std::uint8_t mask,
    bool enabled
) noexcept {
    if (enabled) {
        value = static_cast<std::uint8_t>(value | mask);
    } else {
        value = static_cast<std::uint8_t>(
            value & static_cast<std::uint8_t>(~mask)
        );
    }
}

void StatusRegister::reset() noexcept {
    value = 0;
}

void CPUState::reset() {
    memory.reset();
    pc = 0;
    sp = InitialStackPointer;
    sreg.reset();
}

bool CPUState::validate(std::string* error) const {
    if (error != nullptr) {
        error->clear();
    }

    if (pc > ProgramWordAddressMax) {
        if (error != nullptr) {
            *error = "PC fora do intervalo de palavras da Flash.";
        }
        return false;
    }

    if (sp < Memory::SramBegin || sp > Memory::SramEnd) {
        if (error != nullptr) {
            *error = "SP fora do intervalo da SRAM interna.";
        }
        return false;
    }

    return true;
}

} // namespace arduino3d::emulator
