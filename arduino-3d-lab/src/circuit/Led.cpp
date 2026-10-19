
#include "circuit/Led.hpp"

namespace arduino3d {

Led::Led(PinId anode, PinId cathode)
    : anode_(anode), cathode_(cathode) {}

PinId Led::anode() const noexcept {
    return anode_;
}

PinId Led::cathode() const noexcept {
    return cathode_;
}

bool Led::isOn() const noexcept {
    return on_;
}

void Led::update(
    DigitalState anodeState,
    DigitalState cathodeState
) noexcept {
    on_ = anodeState == DigitalState::High &&
          cathodeState == DigitalState::Low;
}

} // namespace arduino3d
