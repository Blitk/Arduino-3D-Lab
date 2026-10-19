
#include "circuit/Circuit.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace arduino3d;

int main() {
    Circuit circuit;

    const PinId output = circuit.addPin("D13", PinDirection::Output);
    const PinId input = circuit.addPin("LED_ANODE", PinDirection::Input);
    const PinId ground = circuit.addPin("GND", PinDirection::Output);
    const PinId cathode = circuit.addPin("LED_CATHODE", PinDirection::Input);

    circuit.connect(output, input);
    circuit.connect(ground, cathode);
    circuit.addLed(input, cathode);

    // Estado inicial: nenhuma saída dirige um nível.
    assert(circuit.pinState(input) == DigitalState::Floating);
    assert(!circuit.ledOn(0));

    // HIGH na saída propaga-se até o ânodo.
    circuit.setOutput(output, DigitalState::High);
    circuit.setOutput(ground, DigitalState::Low);

    std::string error;
    assert(circuit.update(&error));
    assert(error.empty());
    assert(circuit.pinState(input) == DigitalState::High);
    assert(circuit.pinState(cathode) == DigitalState::Low);
    assert(circuit.ledOn(0));

    // LOW no pino de controle apaga o LED logicamente.
    circuit.setOutput(output, DigitalState::Low);
    assert(circuit.update(&error));
    assert(!circuit.ledOn(0));

    // Floating não equivale a LOW.
    circuit.setOutput(output, DigitalState::Floating);
    assert(circuit.update(&error));
    assert(circuit.pinState(input) == DigitalState::Floating);
    assert(!circuit.ledOn(0));

    // Conectar um pino a si mesmo deve falhar.
    bool rejectedSelfConnection = false;

    try {
        circuit.connect(output, output);
    } catch (const std::invalid_argument&) {
        rejectedSelfConnection = true;
    }

    assert(rejectedSelfConnection);

    // Um pino inexistente deve ser rejeitado.
    bool rejectedUnknownPin = false;

    try {
        circuit.connect(output, 999999);
    } catch (const std::invalid_argument&) {
        rejectedUnknownPin = true;
    }

    assert(rejectedUnknownPin);

    // Uma entrada não pode dirigir diretamente um estado.
    bool rejectedInputDrive = false;

    try {
        circuit.setOutput(input, DigitalState::High);
    } catch (const std::invalid_argument&) {
        rejectedInputDrive = true;
    }

    assert(rejectedInputDrive);

    // Duas saídas em conflito na mesma rede causam falha de update.
    const PinId secondOutput =
        circuit.addPin("D12", PinDirection::Output);

    circuit.connect(output, secondOutput);
    circuit.setOutput(output, DigitalState::High);
    circuit.setOutput(secondOutput, DigitalState::Low);

    assert(!circuit.update(&error));
    assert(!error.empty());

    std::cout << "Todos os testes do circuito passaram.\n";
    return 0;
}
