#include "project/Serializer.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace arduino3d::project {
namespace {

constexpr std::uintmax_t MaxFileBytes = 10 * 1024 * 1024;

void setError(std::string* error, const std::string& message) {
    if (error) {
        *error = message;
    }
}

template <typename T>
T required(const Json& object, const char* key) {
    if (!object.is_object() || !object.contains(key)) {
        throw std::runtime_error(
            std::string("Campo obrigatorio ausente: ") + key
        );
    }

    return object.at(key).get<T>();
}

Json vectorToJson(const Vector3& v) {
    return {
        {"x", v.x},
        {"y", v.y},
        {"z", v.z}
    };
}

Json quaternionToJson(const Quaternion& q) {
    return {
        {"x", q.x},
        {"y", q.y},
        {"z", q.z},
        {"w", q.w}
    };
}

Vector3 vectorFromJson(const Json& json) {
    if (!json.is_object()) {
        throw std::runtime_error("'position' deve ser um objeto.");
    }

    return {
        required<double>(json, "x"),
        required<double>(json, "y"),
        required<double>(json, "z")
    };
}

Quaternion quaternionFromJson(const Json& json) {
    if (!json.is_object()) {
        throw std::runtime_error("'orientation' deve ser um objeto.");
    }

    return {
        required<double>(json, "x"),
        required<double>(json, "y"),
        required<double>(json, "z"),
        required<double>(json, "w")
    };
}

} // namespace

Json Serializer::toJson(const Project& project) {
    const std::string validationError = project.validate();

    if (!validationError.empty()) {
        throw std::invalid_argument(
            "Projeto invalido: " + validationError
        );
    }

    Json components = Json::array();

    for (const auto& component : project.components) {
        components.push_back({
            {"id", component.id},
            {"type", component.type},
            {"name", component.name},
            {"position", vectorToJson(component.position)},
            {"orientation", quaternionToJson(component.orientation)},
            {"pins", component.pins},
            {"properties", component.properties}
        });
    }

    Json connections = Json::array();

    for (const auto& connection : project.connections) {
        connections.push_back({
            {"from", {
                {"component", connection.fromComponent},
                {"pin", connection.fromPin}
            }},
            {"to", {
                {"component", connection.toComponent},
                {"pin", connection.toPin}
            }}
        });
    }

    return {
        {"format", "arduino-3d-lab-project"},
        {"format_version", project.formatVersion},
        {"name", project.name},
        {"settings", {
            {"board", project.settings.board},
            {"cpu_frequency_hz", project.settings.cpuFrequencyHz},
            {"grid_size", project.settings.gridSize},
            {"snap_to_grid", project.settings.snapToGrid}
        }},
        {"components", std::move(components)},
        {"connections", std::move(connections)}
    };
}

bool Serializer::fromJson(
    const Json& json,
    Project& destination,
    std::string* error
) {
    try {
        if (!json.is_object()) {
            setError(error, "A raiz do JSON deve ser um objeto.");
            return false;
        }

        if (required<std::string>(json, "format") !=
            "arduino-3d-lab-project") {
            setError(error, "Formato de projeto desconhecido.");
            return false;
        }

        // Constrói um candidato separado para preservar destination
        // caso qualquer etapa falhe.
        Project candidate;

        candidate.formatVersion =
            required<int>(json, "format_version");

        candidate.name = required<std::string>(json, "name");

        const Json& settings = json.at("settings");

        if (!settings.is_object()) {
            setError(error, "'settings' deve ser um objeto.");
            return false;
        }

        candidate.settings.board =
            required<std::string>(settings, "board");

        candidate.settings.cpuFrequencyHz =
            required<double>(settings, "cpu_frequency_hz");

        candidate.settings.gridSize =
            required<double>(settings, "grid_size");

        candidate.settings.snapToGrid =
            required<bool>(settings, "snap_to_grid");

        const Json& components = json.at("components");

        if (!components.is_array()) {
            setError(error, "'components' deve ser uma lista.");
            return false;
        }

        if (components.size() > 10000) {
            setError(error, "Limite de componentes excedido.");
            return false;
        }

        for (const Json& item : components) {
            if (!item.is_object()) {
                setError(error, "Cada componente deve ser um objeto.");
                return false;
            }

            Component component;

            component.id = required<std::string>(item, "id");
            component.type = required<std::string>(item, "type");
            component.name = required<std::string>(item, "name");

            if (!item.contains("position") ||
                !item.contains("orientation")) {
                setError(error, "Componente sem position ou orientation.");
                return false;
            }

            component.position =
                vectorFromJson(item.at("position"));

            component.orientation =
                quaternionFromJson(item.at("orientation"));

            component.pins =
                required<std::vector<std::string>>(item, "pins");

            component.properties =
                required<Json>(item, "properties");

            candidate.components.push_back(std::move(component));
        }

        const Json& connections = json.at("connections");

        if (!connections.is_array()) {
            setError(error, "'connections' deve ser uma lista.");
            return false;
        }

        if (connections.size() > 50000) {
            setError(error, "Limite de conexoes excedido.");
            return false;
        }

        for (const Json& item : connections) {
            if (!item.is_object() ||
                !item.contains("from") ||
                !item.contains("to")) {
                setError(error, "Conexao sem origem ou destino.");
                return false;
            }

            const Json& from = item.at("from");
            const Json& to = item.at("to");

            if (!from.is_object() || !to.is_object()) {
                setError(error, "Endpoints da conexao devem ser objetos.");
                return false;
            }

            candidate.connections.push_back({
                required<std::string>(from, "component"),
                required<std::string>(from, "pin"),
                required<std::string>(to, "component"),
                required<std::string>(to, "pin")
            });
        }

        const std::string validationError = candidate.validate();

        if (!validationError.empty()) {
            setError(error, validationError);
            return false;
        }

        destination = std::move(candidate);

        if (error) {
            error->clear();
        }

        return true;
    } catch (const std::exception& exception) {
        setError(
            error,
            std::string("JSON de projeto invalido: ") + exception.what()
        );
        return false;
    }
}

bool Serializer::save(
    const Project& project,
    const std::filesystem::path& file,
    std::string* error
) {
    const std::filesystem::path temporary =
        file.string() + ".tmp";

    try {
        const Json json = toJson(project);

        {
            std::ofstream output(
                temporary,
                std::ios::binary | std::ios::trunc
            );

            if (!output) {
                setError(error, "Nao foi possivel criar o arquivo temporario.");
                return false;
            }

            output << json.dump(2) << '\n';
            output.flush();

            if (!output) {
                setError(error, "Falha ao gravar o arquivo do projeto.");
                output.close();

                std::error_code ignored;
                std::filesystem::remove(temporary, ignored);
                return false;
            }
        }

        // No Linux, rename substitui atomicamente o destino no mesmo
        // sistema de arquivos, evitando deixar um JSON parcialmente escrito.
        std::error_code ec;
        std::filesystem::rename(temporary, file, ec);

        if (ec) {
            setError(error, "Nao foi possivel finalizar o salvamento: " +
                            ec.message());

            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return false;
        }

        if (error) {
            error->clear();
        }

        return true;
    } catch (const std::exception& exception) {
        setError(error, exception.what());

        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        return false;
    }
}

bool Serializer::load(
    const std::filesystem::path& file,
    Project& destination,
    std::string* error
) {
    try {
        std::error_code ec;
        const auto size = std::filesystem::file_size(file, ec);

        if (ec) {
            setError(error, "Nao foi possivel acessar o arquivo: " +
                            file.string());
            return false;
        }

        if (size > MaxFileBytes) {
            setError(error, "Arquivo excede o limite de 10 MiB.");
            return false;
        }

        std::ifstream input(file, std::ios::binary);

        if (!input) {
            setError(error, "Nao foi possivel abrir o arquivo.");
            return false;
        }

        Json json;
        input >> json;

        Project candidate;

        if (!fromJson(json, candidate, error)) {
            return false;
        }

        destination = std::move(candidate);

        if (error) {
            error->clear();
        }

        return true;
    } catch (const std::exception& exception) {
        setError(
            error,
            std::string("Falha ao carregar projeto: ") + exception.what()
        );
        return false;
    }
}

} // namespace arduino3d::project
