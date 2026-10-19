
#include "circuit/Wire.hpp"

namespace arduino3d {

Wire::Wire(WireId id, PinId first, PinId second)
    : id_(id), first_(first), second_(second) {}

WireId Wire::id() const noexcept {
    return id_;
}

PinId Wire::first() const noexcept {
    return first_;
}

PinId Wire::second() const noexcept {
    return second_;
}

} // namespace arduino3d
