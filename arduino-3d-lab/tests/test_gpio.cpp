#include "circuit/Circuit.hpp"
#include "emulator/AVR.hpp"
#include "emulator/GPIO.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace arduino3d;
using namespace arduino3d::emulator;

namespace {

void loadWords(
    CPUState& cpu,
    const std::vector<std::uint16_t>& words
) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(words.size() * 2);

    for (const auto word : words) {
        bytes.push_back(static_cast<std::uint8_t>(word & 0xFFU));
        bytes.push_back(static_cast<std::uint8_t>(word >> 8U));
    }

    cpu.memory.loadProgram(bytes);
}

void testGpioToLedPropagation() {
    CPUState cpu;
    Circuit circuit;

    // O pino D13 começa como entrada.
    const PinId d13 = circuit.addPin(
        "Arduino D13 / PB5",
        PinDirection::Input
    );

    // Referência lógica de GND no circuito virtual.
    const PinId ground = circuit.addPin(
        "GND",
        PinDirection::Output
    );

    circuit.setOutput(ground, DigitalState::Low);
    circuit.addLed(d13, ground);

    std::string error;
    assert(circuit.update(&error));

    GPIO::PinMap pinMap{};
    pinMap.fill(0);
    pinMap[13] = d13;

    GPIO gpio(cpu, circuit, pinMap);
    AVR avr(cpu, &gpio);

    loadWords(cpu, {
        0xE200, // LDI R16, 0x20
        0xB904, // OUT DDRB, R16
        0xB905, // OUT PORTB, R16
        0xB113, // IN R17, PINB
        0xB903  // OUT PINB, R16: alterna PB5
    });

    // LDI: carrega a máscara do bit PB5.
    assert(avr.step().ok());
    assert(cpu.registers.read(cpu.memory, 16) == 0x20);
    assert(!circuit.ledOn(0));

    // DDRB: D13 passa a ser uma saída digital.
    assert(avr.step().ok());
    assert((cpu.memory.readData(GPIO::DDRB) & 0x20U) != 0);

    // PORTB: nível alto chega ao circuito e acende o LED.
    assert(avr.step().ok());
    assert((cpu.memory.readData(GPIO::PORTB) & 0x20U) != 0);
    assert(circuit.pinState(d13) == DigitalState::High);
    assert(circuit.ledOn(0));

    // IN lê o valor refletido por PINB no espaço de dados.
    assert(avr.step().ok());
    assert(cpu.registers.read(cpu.memory, 17) == 0x20);

    // Escrever 1 em PINB alterna o bit de PORTB.
    assert(avr.step().ok());
    assert((cpu.memory.readData(GPIO::PORTB) & 0x20U) == 0);
    assert(circuit.pinState(d13) == DigitalState::Low);
    assert(!circuit.ledOn(0));
}

void testArduinoUnoPortMapping() {
    CPUState cpu;
    Circuit circuit;

    const auto d8 = circuit.addPin("D8", PinDirection::Input);
    const auto d13 = circuit.addPin("D13", PinDirection::Input);
    const auto a0 = circuit.addPin("A0", PinDirection::Input);
    const auto d7 = circuit.addPin("D7", PinDirection::Input);

    GPIO::PinMap pinMap{};
    pinMap.fill(0);
    pinMap[8] = d8;
    pinMap[13] = d13;
    pinMap[14] = a0;
    pinMap[7] = d7;

    GPIO gpio(cpu, circuit, pinMap);

    // PB0, PB5, PC0 e PD7 configurados como saídas.
    cpu.memory.writeData(GPIO::DDRB, 0x21);
    cpu.memory.writeData(GPIO::PORTB, 0x21);

    cpu.memory.writeData(GPIO::DDRC, 0x01);
    cpu.memory.writeData(GPIO::PORTC, 0x01);

    cpu.memory.writeData(GPIO::DDRD, 0x80);
    cpu.memory.writeData(GPIO::PORTD, 0x80);

    std::string error;
    assert(gpio.synchronize(&error));

    assert(circuit.pinState(d8) == DigitalState::High);
    assert(circuit.pinState(d13) == DigitalState::High);
    assert(circuit.pinState(a0) == DigitalState::High);
    assert(circuit.pinState(d7) == DigitalState::High);
}

void testCircuitConflictIsReported() {
    CPUState cpu;
    Circuit circuit;

    const auto pin = circuit.addPin("D13", PinDirection::Input);
    const auto conflictingOutput = circuit.addPin(
        "External output",
        PinDirection::Output
    );

    // Duas saídas opostas na mesma rede lógica.
    const auto wire = circuit.connect(pin, conflictingOutput);
    (void)wire;

    circuit.setOutput(conflictingOutput, DigitalState::Low);

    GPIO::PinMap pinMap{};
    pinMap.fill(0);
    pinMap[13] = pin;

    GPIO gpio(cpu, circuit, pinMap);

    cpu.memory.writeData(GPIO::DDRB, 0x20);
    cpu.memory.writeData(GPIO::PORTB, 0x20);

    std::string error;
    assert(!gpio.synchronize(&error));
    assert(!error.empty());
}

} // namespace

int main() {
    testGpioToLedPropagation();
    testArduinoUnoPortMapping();
    testCircuitConflictIsReported();

    std::cout << "Testes de GPIO e circuito concluidos.\n";
    return 0;
}
