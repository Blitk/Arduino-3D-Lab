
#include "circuit/Circuit.hpp"

#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace arduino3d {

Pin& Circuit::requirePin(PinId id) {
    const auto it = pins_.find(id);

    if (it == pins_.end()) {
        throw std::invalid_argument("Pino inexistente: " + std::to_string(id));
    }

    return it->second;
}

const Pin& Circuit::requirePin(PinId id) const {
    const auto it = pins_.find(id);

    if (it == pins_.end()) {
        throw std::invalid_argument("Pino inexistente: " + std::to_string(id));
    }

    return it->second;
}

PinId Circuit::addPin(std::string name, PinDirection direction) {
    if (name.empty()) {
        throw std::invalid_argument("O nome do pino nao pode ser vazio.");
    }

    if (nextPinId_ == 0) {
        throw std::overflow_error("Identificadores de pino esgotados.");
    }

    const PinId id = nextPinId_++;
    pins_.emplace(id, Pin{id, std::move(name), direction});

    return id;
}

WireId Circuit::connect(PinId first, PinId second) {
    requirePin(first);
    requirePin(second);

    if (first == second) {
        throw std::invalid_argument("Um pino nao pode ser conectado a si mesmo.");
    }

    for (const Wire& wire : wires_) {
        const bool sameOrder =
            wire.first() == first && wire.second() == second;
        const bool reverseOrder =
            wire.first() == second && wire.second() == first;

        if (sameOrder || reverseOrder) {
            throw std::invalid_argument("Ja existe um fio entre esses pinos.");
        }
    }

    if (nextWireId_ == 0) {
        throw std::overflow_error("Identificadores de fio esgotados.");
    }

    const WireId id = nextWireId_++;
    wires_.emplace_back(id, first, second);

    return id;
}

void Circuit::addLed(PinId anode, PinId cathode) {
    requirePin(anode);
    requirePin(cathode);

    if (anode == cathode) {
        throw std::invalid_argument("O LED precisa de dois pinos distintos.");
    }

    leds_.emplace_back(anode, cathode);
}

void Circuit::setOutput(PinId pinId, DigitalState state) {
    Pin& pin = requirePin(pinId);

    if (pin.direction() != PinDirection::Output) {
        throw std::invalid_argument(
            "Somente pinos de saida podem dirigir um estado."
        );
    }

    pin.drivenState_ = state;
}

bool Circuit::update(std::string* error) {
    if (error != nullptr) {
        error->clear();
    }

    // Inicialmente, cada pino pertence ao seu próprio conjunto.
    std::map<PinId, PinId> parent;

    for (const auto& [id, pin] : pins_) {
        (void)pin;
        parent[id] = id;
    }

    std::function<PinId(PinId)> findRoot = [&](PinId id) -> PinId {
        PinId& root = parent.at(id);

        if (root != id) {
            root = findRoot(root);
        }

        return root;
    };

    const auto unite = [&](PinId a, PinId b) {
        const PinId rootA = findRoot(a);
        const PinId rootB = findRoot(b);

        if (rootA != rootB) {
            parent[rootB] = rootA;
        }
    };

    for (const Wire& wire : wires_) {
        unite(wire.first(), wire.second());
    }

    struct NetState {
        bool hasHigh{false};
        bool hasLow{false};
    };

    std::map<PinId, NetState> netStates;

    // Detecta conflitos antes de alterar qualquer estado resolvido.
    for (const auto& [id, pin] : pins_) {
        if (pin.direction() != PinDirection::Output) {
            continue;
        }

        const PinId root = findRoot(id);
        NetState& net = netStates[root];

        if (pin.drivenState() == DigitalState::High) {
            net.hasHigh = true;
        } else if (pin.drivenState() == DigitalState::Low) {
            net.hasLow = true;
        }
    }

    for (const auto& [root, net] : netStates) {
        if (net.hasHigh && net.hasLow) {
            if (error != nullptr) {
                *error =
                    "Conflito digital na rede que contem o pino " +
                    std::to_string(root) +
                    ": saidas HIGH e LOW conectadas.";
            }

            return false;
        }
    }

    // Calcula primeiro os novos estados; só depois os publica.
    std::map<PinId, DigitalState> resolved;

    for (const auto& [id, pin] : pins_) {
        (void)pin;

        const auto netIt = netStates.find(findRoot(id));
        DigitalState state = DigitalState::Floating;

        if (netIt != netStates.end()) {
            if (netIt->second.hasHigh) {
                state = DigitalState::High;
            } else if (netIt->second.hasLow) {
                state = DigitalState::Low;
            }
        }

        resolved.emplace(id, state);
    }

    for (const auto& [id, state] : resolved) {
        pins_.at(id).resolvedState_ = state;
    }

    for (Led& led : leds_) {
        led.update(
            pins_.at(led.anode()).state(),
            pins_.at(led.cathode()).state()
        );
    }

    return true;
}

DigitalState Circuit::pinState(PinId pin) const {
    return requirePin(pin).state();
}

bool Circuit::ledOn(std::size_t index) const {
    if (index >= leds_.size()) {
        throw std::out_of_range("Indice de LED invalido.");
    }

    return leds_[index].isOn();
}

std::size_t Circuit::ledCount() const noexcept {
    return leds_.size();
}

std::size_t Circuit::wireCount() const noexcept {
    return wires_.size();
}

} // namespace arduino3d
