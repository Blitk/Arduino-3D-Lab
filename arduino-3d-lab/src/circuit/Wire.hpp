
#pragma once

#include "circuit/Pin.hpp"

#include <cstdint>

namespace arduino3d {

using WireId = std::uint32_t;

class Wire {
public:
    Wire(WireId id, PinId first, PinId second);

    WireId id() const noexcept;
    PinId first() const noexcept;
    PinId second() const noexcept;

private:
    WireId id_;
    PinId first_;
    PinId second_;
};

} // namespace arduino3d
