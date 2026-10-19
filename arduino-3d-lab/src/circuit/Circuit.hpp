
#pragma once

#include "circuit/Led.hpp"
#include "circuit/Pin.hpp"
#include "circuit/Wire.hpp"

#include <map>
#include <string>
#include <vector>

namespace arduino3d {

class Circuit {
public:
    PinId addPin(std::string name, PinDirection direction);

    WireId connect(PinId first, PinId second);
    void addLed(PinId anode, PinId cathode);

    void setOutput(PinId pin, DigitalState state);

    // Retorna false se houver conflito entre saídas na mesma rede.
    // Em caso de conflito, os estados resolvidos anteriores são preservados.
    bool update(std::string* error = nullptr);

    DigitalState pinState(PinId pin) const;
    bool ledOn(std::size_t index) const;
    std::size_t ledCount() const noexcept;
    std::size_t wireCount() const noexcept;

private:
    Pin& requirePin(PinId id);
    const Pin& requirePin(PinId id) const;

    std::map<PinId, Pin> pins_;
    std::vector<Wire> wires_;
    std::vector<Led> leds_;

    PinId nextPinId_{1};
    WireId nextWireId_{1};
};

} // namespace arduino3d
