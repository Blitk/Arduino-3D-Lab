
#pragma once

#include "circuit/Pin.hpp"

namespace arduino3d {

class Led {
public:
    Led(PinId anode, PinId cathode);

    PinId anode() const noexcept;
    PinId cathode() const noexcept;
    bool isOn() const noexcept;

private:
    friend class Circuit;

    void update(DigitalState anodeState, DigitalState cathodeState) noexcept;

    PinId anode_;
    PinId cathode_;
    bool on_{false};
};

} // namespace arduino3d
