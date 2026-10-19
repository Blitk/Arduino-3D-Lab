
#pragma once

#include <cstdint>
#include <string>

namespace arduino3d {

using PinId = std::uint32_t;

enum class PinDirection {
    Input,
    Output
};

enum class DigitalState {
    Floating,
    Low,
    High
};

class Circuit;

class Pin {
public:
    Pin(PinId id, std::string name, PinDirection direction);

    PinId id() const noexcept;
    const std::string& name() const noexcept;
    PinDirection direction() const noexcept;

    DigitalState state() const noexcept;
    DigitalState drivenState() const noexcept;

private:
    friend class Circuit;

    PinId id_;
    std::string name_;
    PinDirection direction_;
    DigitalState drivenState_{DigitalState::Floating};
    DigitalState resolvedState_{DigitalState::Floating};
};

} // namespace arduino3d
