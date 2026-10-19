#pragma once

#include "circuit/Circuit.hpp"
#include "emulator/CPU.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace arduino3d::emulator {

class GPIO {
public:
    static constexpr std::size_t ArduinoPinCount = 20;

    // Endereços no espaço de dados AVR.
    static constexpr std::uint16_t PINB  = 0x23;
    static constexpr std::uint16_t DDRB  = 0x24;
    static constexpr std::uint16_t PORTB = 0x25;

    static constexpr std::uint16_t PINC  = 0x26;
    static constexpr std::uint16_t DDRC  = 0x27;
    static constexpr std::uint16_t PORTC = 0x28;

    static constexpr std::uint16_t PIND  = 0x29;
    static constexpr std::uint16_t DDRD  = 0x2A;
    static constexpr std::uint16_t PORTD = 0x2B;

    using PinMap = std::array<arduino3d::PinId, ArduinoPinCount>;

    GPIO(CPUState& cpu, arduino3d::Circuit& circuit, PinMap pinMap);

    // Propaga DDRx/PORTx ao circuito e atualiza PINx.
    bool synchronize(std::string* error = nullptr);

    // Recebe um endereço de I/O de 0x00 a 0x3F, como usado por OUT.
    // A escrita em PINx segue a semântica de toggle do ATmega328P.
    void writeIo(std::uint8_t ioAddress, std::uint8_t value);

private:
    struct Port {
        std::uint16_t pinRegister;
        std::uint16_t ddrRegister;
        std::uint16_t portRegister;

        // Para cada bit, índice do pino Arduino correspondente.
        std::array<int, 8> arduinoPins;
    };

    static const std::array<Port, 3>& ports() noexcept;

    CPUState& cpu_;
    arduino3d::Circuit& circuit_;
    PinMap pinMap_;
};

} // namespace arduino3d::emulator
