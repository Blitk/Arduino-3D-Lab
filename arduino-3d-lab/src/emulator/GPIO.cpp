#include "emulator/GPIO.hpp"

#include <exception>

namespace arduino3d::emulator {

GPIO::GPIO(
    CPUState& cpu,
    arduino3d::Circuit& circuit,
    PinMap pinMap
)
    : cpu_(cpu),
      circuit_(circuit),
      pinMap_(pinMap) {
}

const std::array<GPIO::Port, 3>& GPIO::ports() noexcept {
    static const std::array<Port, 3> definitions{{
        // PORTB: PB0..PB5 -> Arduino D8..D13.
        {PINB, DDRB, PORTB, {{8, 9, 10, 11, 12, 13, -1, -1}}},

        // PORTC: PC0..PC5 -> Arduino A0..A5.
        {PINC, DDRC, PORTC, {{14, 15, 16, 17, 18, 19, -1, -1}}},

        // PORTD: PD0..PD7 -> Arduino D0..D7.
        {PIND, DDRD, PORTD, {{0, 1, 2, 3, 4, 5, 6, 7}}}
    }};

    return definitions;
}

void GPIO::writeIo(
    std::uint8_t ioAddress,
    std::uint8_t value
) {
    const auto dataAddress = static_cast<std::uint16_t>(
        0x20U + ioAddress
    );

    // PINB, PINC e PIND: escrever 1 alterna o bit em PORTx.
    std::uint16_t portAddress = 0;

    switch (dataAddress) {
    case PINB:
        portAddress = PORTB;
        break;

    case PINC:
        portAddress = PORTC;
        break;

    case PIND:
        portAddress = PORTD;
        break;

    default:
        cpu_.memory.writeData(dataAddress, value);
        return;
    }

    const auto current = cpu_.memory.readData(portAddress);

    cpu_.memory.writeData(
        portAddress,
        static_cast<std::uint8_t>(current ^ value)
    );
}

bool GPIO::synchronize(std::string* error) {
    if (error != nullptr) {
        error->clear();
    }

    try {
        // Primeiro aplica direção e nível de saída às redes.
        for (const auto& port : ports()) {
            const auto ddr = cpu_.memory.readData(port.ddrRegister);
            const auto output = cpu_.memory.readData(port.portRegister);

            for (unsigned bit = 0; bit < 8; ++bit) {
                const int arduinoIndex = port.arduinoPins[bit];

                if (arduinoIndex < 0) {
                    continue;
                }

                const auto pinId = pinMap_[
                    static_cast<std::size_t>(arduinoIndex)
                ];

                if (pinId == 0) {
                    continue;
                }

                const auto mask = static_cast<std::uint8_t>(1U << bit);
                const bool isOutput = (ddr & mask) != 0;

                circuit_.setPinDirection(
                    pinId,
                    isOutput
                        ? arduino3d::PinDirection::Output
                        : arduino3d::PinDirection::Input
                );

                if (isOutput) {
                    circuit_.setOutput(
                        pinId,
                        (output & mask) != 0
                            ? arduino3d::DigitalState::High
                            : arduino3d::DigitalState::Low
                    );
                }
            }
        }

        // Resolve conexões e detecta conflitos entre saídas.
        if (!circuit_.update(error)) {
            return false;
        }

        // Depois reflete os níveis resolvidos nos registradores PINx.
        for (const auto& port : ports()) {
            std::uint8_t inputValue = 0;

            for (unsigned bit = 0; bit < 8; ++bit) {
                const int arduinoIndex = port.arduinoPins[bit];

                if (arduinoIndex < 0) {
                    continue;
                }

                const auto pinId = pinMap_[
                    static_cast<std::size_t>(arduinoIndex)
                ];

                if (pinId == 0) {
                    continue;
                }

                if (circuit_.pinState(pinId) ==
                    arduino3d::DigitalState::High) {
                    inputValue = static_cast<std::uint8_t>(
                        inputValue | (1U << bit)
                    );
                }
            }

            cpu_.memory.writeData(port.pinRegister, inputValue);
        }
    } catch (const std::exception& exception) {
        if (error != nullptr) {
            *error = exception.what();
        }
        return false;
    }

    return true;
}

} // namespace arduino3d::emulator
