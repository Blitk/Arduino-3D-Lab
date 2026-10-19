
#include "circuit/Pin.hpp"

#include <utility>

namespace arduino3d {

Pin::Pin(PinId id, std::string name, PinDirection direction)
    : id_(id),
      name_(std::move(name)),
      direction_(direction) {}

PinId Pin::id() const noexcept {
    return id_;
}

const std::string& Pin::name() const noexcept {
    return name_;
}

PinDirection Pin::direction() const noexcept {
    return direction_;
}

DigitalState Pin::state() const noexcept {
    return resolvedState_;
}

DigitalState Pin::drivenState() const noexcept {
    return drivenState_;
}

} // namespace arduino3d
