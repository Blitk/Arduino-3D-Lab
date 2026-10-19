#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace arduino3d::project {

using Json = nlohmann::json;

struct Vector3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct Quaternion {
    double x{0.0};
    double y{0.0};
    double z{0.0};
    double w{1.0};
};

struct Component {
    std::string id;
    std::string type;
    std::string name;

    Vector3 position;
    Quaternion orientation;

    // Nomes lógicos dos pinos, não ponteiros para objetos da aplicação.
    std::vector<std::string> pins;

    // Propriedades específicas: resistência, cor, tensão, modelo etc.
    Json properties{Json::object()};
};

struct Connection {
    std::string fromComponent;
    std::string fromPin;
    std::string toComponent;
    std::string toPin;
};

struct ProjectSettings {
    std::string board{"arduino-uno"};
    double cpuFrequencyHz{16000000.0};
    double gridSize{1.0};
    bool snapToGrid{true};
};

struct Project {
    static constexpr int CurrentFormatVersion = 1;

    int formatVersion{CurrentFormatVersion};
    std::string name{"Untitled Project"};
    ProjectSettings settings;
    std::vector<Component> components;
    std::vector<Connection> connections;

    // Retorna uma mensagem vazia se válido.
    [[nodiscard]] std::string validate() const;
};

} // namespace arduino3d::project
