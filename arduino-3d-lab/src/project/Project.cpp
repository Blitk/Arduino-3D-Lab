#include "project/Project.hpp"

#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace arduino3d::project {
namespace {

bool finite(double value) {
    return std::isfinite(value);
}

bool validText(const std::string& value, std::size_t maxLength) {
    return !value.empty() && value.size() <= maxLength;
}

bool validQuaternion(const Quaternion& q) {
    if (!finite(q.x) || !finite(q.y) ||
        !finite(q.z) || !finite(q.w)) {
        return false;
    }

    const double normSquared =
        q.x * q.x + q.y * q.y +
        q.z * q.z + q.w * q.w;

    return finite(normSquared) && normSquared > 1e-12;
}

} // namespace

std::string Project::validate() const {
    if (formatVersion != CurrentFormatVersion) {
        return "Versao de formato nao suportada.";
    }

    if (!validText(name, 512)) {
        return "Nome do projeto invalido.";
    }

    if (!validText(settings.board, 128)) {
        return "Identificador da placa invalido.";
    }

    if (!finite(settings.cpuFrequencyHz) ||
        settings.cpuFrequencyHz <= 0.0 ||
        settings.cpuFrequencyHz > 1.0e12) {
        return "Frequencia da CPU invalida.";
    }

    if (!finite(settings.gridSize) ||
        settings.gridSize <= 0.0 ||
        settings.gridSize > 1.0e6) {
        return "Tamanho da grade invalido.";
    }

    if (components.size() > 10000) {
        return "Limite de 10000 componentes excedido.";
    }

    if (connections.size() > 50000) {
        return "Limite de 50000 conexoes excedido.";
    }

    std::unordered_map<std::string, const Component*> byId;
    byId.reserve(components.size());

    for (const auto& component : components) {
        if (!validText(component.id, 128)) {
            return "ID de componente invalido.";
        }

        if (!validText(component.type, 128)) {
            return "Tipo invalido no componente " + component.id;
        }

        if (!validText(component.name, 256)) {
            return "Nome invalido no componente " + component.id;
        }

        if (!byId.emplace(component.id, &component).second) {
            return "ID de componente duplicado: " + component.id;
        }

        const auto& p = component.position;

        if (!finite(p.x) || !finite(p.y) || !finite(p.z)) {
            return "Posicao invalida no componente " + component.id;
        }

        if (!validQuaternion(component.orientation)) {
            return "Orientacao invalida no componente " + component.id;
        }

        if (!component.properties.is_object()) {
            return "As propriedades devem formar um objeto JSON: " +
                   component.id;
        }

        if (component.pins.size() > 256) {
            return "Limite de 256 pinos excedido: " + component.id;
        }

        std::unordered_set<std::string> pins;

        for (const auto& pin : component.pins) {
            if (!validText(pin, 128)) {
                return "Nome de pino invalido: " + component.id;
            }

            if (!pins.insert(pin).second) {
                return "Pino duplicado no componente " + component.id +
                       ": " + pin;
            }
        }
    }

    std::unordered_set<std::string> seenConnections;

    for (const auto& connection : connections) {
        const auto from = byId.find(connection.fromComponent);
        const auto to = byId.find(connection.toComponent);

        if (from == byId.end()) {
            return "Componente de origem inexistente: " +
                   connection.fromComponent;
        }

        if (to == byId.end()) {
            return "Componente de destino inexistente: " +
                   connection.toComponent;
        }

        const auto hasPin = [](const Component& component,
                               const std::string& pin) {
            for (const auto& candidate : component.pins) {
                if (candidate == pin) {
                    return true;
                }
            }
            return false;
        };

        if (!hasPin(*from->second, connection.fromPin)) {
            return "Pino de origem inexistente: " + connection.fromPin;
        }

        if (!hasPin(*to->second, connection.toPin)) {
            return "Pino de destino inexistente: " + connection.toPin;
        }

        const std::string key =
            connection.fromComponent + "\x1f" + connection.fromPin +
            "\x1f" + connection.toComponent + "\x1f" + connection.toPin;

        if (!seenConnections.insert(key).second) {
            return "Conexao duplicada.";
        }
    }

    return {};
}

} // namespace arduino3d::project
