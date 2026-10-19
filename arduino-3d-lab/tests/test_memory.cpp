
#include "emulator/Memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

using arduino3d::emulator::Memory;

template <typename Function>
bool throwsOutOfRange(Function&& function) {
    try {
        function();
    } catch (const std::out_of_range&) {
        return true;
    }

    return false;
}

int main() {
    Memory memory;

    // Valores iniciais determinísticos.
    assert(memory.readProgramByte(0) == 0xFF);
    assert(memory.readData(0) == 0x00);
    assert(memory.readEeprom(0) == 0xFF);

    // Registradores gerais são mapeados no início do espaço de dados.
    memory.writeData(0x0005, 0xA5);
    assert(memory.readData(0x0005) == 0xA5);

    // SRAM: primeiro e último endereço válidos.
    memory.writeData(Memory::SramBegin, 0x12);
    memory.writeData(Memory::SramEnd, 0x34);

    assert(memory.readData(Memory::SramBegin) == 0x12);
    assert(memory.readData(Memory::SramEnd) == 0x34);

    // Flash e EEPROM são espaços independentes.
    memory.writeProgramByte(0x1234, 0xAB);
    memory.writeEeprom(0x0010, 0xCD);
    memory.writeData(0x0123, 0xCD);

    assert(memory.readProgramByte(0x1234) == 0xAB);
    assert(memory.readEeprom(0x0010) == 0xCD);
    assert(memory.readData(0x0123) == 0xCD);
    assert(memory.readData(0x0123) != 0xAB);

    // Carregamento de bytes de firmware.
    memory.loadProgram({0x0C, 0x94, 0x34, 0x12}, 0);

    assert(memory.readProgramByte(0) == 0x0C);
    assert(memory.readProgramByte(1) == 0x94);
    assert(memory.readProgramByte(2) == 0x34);
    assert(memory.readProgramByte(3) == 0x12);

    // Endereços inválidos devem falhar.
    assert(throwsOutOfRange([&] {
        memory.readProgramByte(static_cast<std::uint16_t>(Memory::FlashSize));
    }));

    assert(throwsOutOfRange([&] {
        memory.readData(static_cast<std::uint16_t>(Memory::DataSize));
    }));

    assert(throwsOutOfRange([&] {
        memory.readEeprom(static_cast<std::uint16_t>(Memory::EepromSize));
    }));

    // Uma carga inválida não deve copiar parcialmente o programa.
    const auto before = memory.readProgramByte(
        static_cast<std::uint16_t>(Memory::FlashSize - 1)
    );

    assert(throwsOutOfRange([&] {
        memory.loadProgram({0x11, 0x22}, Memory::FlashSize - 1);
    }));

    assert(memory.readProgramByte(
        static_cast<std::uint16_t>(Memory::FlashSize - 1)
    ) == before);

    // Reset restaura os padrões documentados.
    memory.reset();

    assert(memory.readProgramByte(0) == 0xFF);
    assert(memory.readData(0x0005) == 0x00);
    assert(memory.readEeprom(0x0010) == 0xFF);

    std::cout << "Memory tests passed.\n";
    return 0;
}
