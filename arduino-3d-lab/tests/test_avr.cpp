#include "emulator/AVR.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace arduino3d::emulator;

namespace {

void loadWords(CPUState& cpu, const std::vector<std::uint16_t>& words) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(words.size() * 2);

    for (const auto word : words) {
        bytes.push_back(static_cast<std::uint8_t>(word & 0xFF));
        bytes.push_back(static_cast<std::uint8_t>(word >> 8));
    }

    cpu.memory.loadProgram(bytes);
}

void testNop() {
    CPUState cpu;
    AVR avr(cpu);

    loadWords(cpu, {0x0000});

    const auto result = avr.step();

    assert(result.ok());
    assert(result.word == 0x0000);
    assert(cpu.pc == 1);
}

void testLdi() {
    CPUState cpu;
    AVR avr(cpu);

    // LDI R16, 0x2A
    loadWords(cpu, {0xE20A});

    assert(avr.step().ok());
    assert(cpu.registers.read(cpu.memory, 16) == 0x2A);
    assert(cpu.pc == 1);
}

// void testMov() {
//     CPUState cpu;
//     AVR avr(cpu);

//     // LDI R16, 0x2A; MOV R18, R16
//     loadWords(cpu, {0xE20A, 0x2F20});

//     assert(avr.step().ok());
//     assert(avr.step().ok());

//     assert(cpu.registers.read(cpu.memory, 18) == 0x2A);
//     assert(cpu.pc == 2);
// }



void testMov() {
    CPUState cpu;
    AVR avr(cpu);

    // LDI R16, 0x2A; MOV R18, R16
    loadWords(cpu, {0xE20A, 0x2F20});

    assert(avr.step().ok());
    assert(cpu.registers.read(cpu.memory, 16) == 0x2A);

    assert(avr.step().ok());
    assert(cpu.registers.read(cpu.memory, 18) == 0x2A);
    assert(cpu.pc == 2);
}



void testAddAndFlags() {
    CPUState cpu;
    AVR avr(cpu);

    // LDI R16, 0xFF; LDI R17, 0x01; ADD R16, R17
    loadWords(cpu, {0xEF0F, 0xE011, 0x0F01});

    assert(avr.run(3).instructionsExecuted == 3);
    assert(cpu.registers.read(cpu.memory, 16) == 0x00);
    assert(cpu.sreg.flag(StatusRegister::Carry));
    assert(cpu.sreg.flag(StatusRegister::Zero));
    assert(!cpu.sreg.flag(StatusRegister::Negative));
    assert(!cpu.sreg.flag(StatusRegister::Overflow));
    assert(!cpu.sreg.flag(StatusRegister::Sign));
}

void testSubAndFlags() {
    CPUState cpu;
    AVR avr(cpu);

    // LDI R16, 0x00; LDI R17, 0x01; SUB R16, R17
    loadWords(cpu, {0xE000, 0xE011, 0x1B01});

    assert(avr.run(3).instructionsExecuted == 3);
    assert(cpu.registers.read(cpu.memory, 16) == 0xFF);
    assert(cpu.sreg.flag(StatusRegister::Carry));
    assert(!cpu.sreg.flag(StatusRegister::Zero));
    assert(cpu.sreg.flag(StatusRegister::Negative));
    assert(!cpu.sreg.flag(StatusRegister::Overflow));
    assert(cpu.sreg.flag(StatusRegister::Sign));
}

void testCpi() {
    CPUState cpu;
    AVR avr(cpu);

    // LDI R16, 0x2A; CPI R16, 0x2A
    loadWords(cpu, {0xE20A, 0x320A});

    assert(avr.run(2).instructionsExecuted == 2);
    assert(cpu.registers.read(cpu.memory, 16) == 0x2A);
    assert(cpu.sreg.flag(StatusRegister::Zero));
    assert(cpu.pc == 2);
}

void testRjmp() {
    CPUState cpu;
    AVR avr(cpu);

    // RJMP +2: pula duas palavras após a próxima posição.
    loadWords(cpu, {0xC002, 0xE001, 0xE002, 0xE003});

    assert(avr.step().ok());
    assert(cpu.pc == 3);

    assert(avr.step().ok());
    assert(cpu.registers.read(cpu.memory, 16) == 3);
}

void testBreqAndBrne() {
    {
        CPUState cpu;
        AVR avr(cpu);

        // CPI R16, 0; BREQ +1; LDI R16, 1
        loadWords(cpu, {0x3000, 0xF008, 0xE001});

        assert(avr.step().ok());
        assert(cpu.sreg.flag(StatusRegister::Zero));

        assert(avr.step().ok());
        assert(cpu.pc == 3);

        // A instrução pulada não deve alterar R16.
        assert(cpu.registers.read(cpu.memory, 16) == 0);
    }

    {
        CPUState cpu;
        AVR avr(cpu);

        // CPI R16, 1; BRNE +1; LDI R16, 1
        loadWords(cpu, {0x3001, 0xF009, 0xE001});

        assert(avr.step().ok());
        assert(!cpu.sreg.flag(StatusRegister::Zero));

        assert(avr.step().ok());
        assert(cpu.pc == 3);
        assert(cpu.registers.read(cpu.memory, 16) == 0);
    }
}

void testInvalidOpcode() {
    CPUState cpu;
    AVR avr(cpu);

    // 0xFFFF não pertence ao subconjunto implementado.
    loadWords(cpu, {0xFFFF});

    const auto result = avr.step();

    assert(result.status == StepStatus::InvalidInstruction);
    assert(!result.message.empty());
    assert(cpu.pc == 0);
}

void testExecutionLimit() {
    CPUState cpu;
    AVR avr(cpu);

    // Loop infinito: RJMP -1.
    loadWords(cpu, {0xCFFF});

    const auto result = avr.run(10);

    assert(result.status == RunStatus::LimitReached);
    assert(result.instructionsExecuted == 10);
    assert(cpu.pc == 0);
}

void testFlashBoundary() {
    CPUState cpu;
    AVR avr(cpu);

    cpu.pc = CPUState::ProgramWordAddressMax;

    // A última palavra da Flash é válida e está inicialmente em 0xFFFF,
    // que não é uma instrução suportada.
    const auto invalid = avr.step();
    assert(invalid.status == StepStatus::InvalidInstruction);

    cpu.pc = static_cast<std::uint16_t>(
        CPUState::ProgramWordAddressMax + 1U);

    const auto outOfRange = avr.step();
    assert(outOfRange.status == StepStatus::ProgramCounterOutOfRange);
}

} // namespace

int main() {
    testNop();
    testLdi();
    testMov();
    testAddAndFlags();
    testSubAndFlags();
    testCpi();
    testRjmp();
    testBreqAndBrne();
    testInvalidOpcode();
    testExecutionLimit();
    testFlashBoundary();

    std::cout << "Todos os testes do emulador AVR passaram.\n";
    return 0;
}
