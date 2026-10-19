#include "project/Serializer.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace arduino3d::project;

namespace {

Project makeSampleProject() {
    Project project;
    project.name = "Teste de serializacao";
    project.settings.board = "arduino-uno";
    project.settings.cpuFrequencyHz = 16000000.0;
    project.settings.gridSize = 0.5;
    project.settings.snapToGrid = true;

    Component board;
    board.id = "board-1";
    board.type = "arduino-uno";
    board.name = "Arduino Uno";
    board.position = {1.0, 2.0, 3.0};
    board.orientation = {0.0, 0.0, 0.0, 1.0};
    board.pins = {"D2", "D3", "GND", "5V"};
    board.properties = {
        {"microcontroller", "ATmega328P"},
        {"revision", 1}
    };

    Component led;
    led.id = "led-1";
    led.type = "led";
    led.name = "LED vermelho";
    led.position = {4.0, 0.0, -1.0};
    led.orientation = {0.0, 0.0, 0.382683432, 0.923879533};
    led.pins = {"anode", "cathode"};
    led.properties = {
        {"color", "#ff0000"},
        {"forward_voltage", 2.0}
    };

    project.components = {board, led};
    project.connections.push_back({
        "board-1", "D2",
        "led-1", "anode"
    });

    return project;
}

void testRoundTrip() {
    const Project original = makeSampleProject();
    const Json encoded = Serializer::toJson(original);

    Project decoded;
    std::string error;

    assert(Serializer::fromJson(encoded, decoded, &error));
    assert(error.empty());

    assert(decoded.name == original.name);
    assert(decoded.components.size() == 2);
    assert(decoded.connections.size() == 1);
    assert(decoded.components[1].properties["color"] == "#ff0000");
    assert(decoded.components[0].position.y == 2.0);
    assert(decoded.settings.gridSize == 0.5);
    assert(decoded.components[1].orientation.w ==
           original.components[1].orientation.w);
}

void testDuplicateComponentId() {
    Json invalid = Serializer::toJson(makeSampleProject());
    invalid["components"][1]["id"] =
        invalid["components"][0]["id"];

    Project destination = makeSampleProject();
    destination.name = "Projeto preservado";

    std::string error;

    assert(!Serializer::fromJson(invalid, destination, &error));
    assert(!error.empty());

    // A desserializacao falhou; o destino nao foi alterado.
    assert(destination.name == "Projeto preservado");
    assert(destination.components.size() == 2);
}

void testMissingConnectionReference() {
    Json invalid = Serializer::toJson(makeSampleProject());
    invalid["connections"][0]["to"]["component"] = "inexistente";

    Project destination;
    std::string error;

    assert(!Serializer::fromJson(invalid, destination, &error));
    assert(!error.empty());
}

void testInvalidOrientationAndProperties() {
    Project project = makeSampleProject();

    project.components[0].orientation = {0.0, 0.0, 0.0, 0.0};
    assert(!project.validate().empty());

    project = makeSampleProject();
    project.components[0].properties = Json::array();
    assert(!project.validate().empty());
}

void testSaveLoadAndInvalidFile() {
    const auto path =
        std::filesystem::temp_directory_path() /
        "arduino3dlab-project-test.json";

    const Project original = makeSampleProject();
    Project loaded;
    std::string error;

    assert(Serializer::save(original, path, &error));
    assert(Serializer::load(path, loaded, &error));

    assert(loaded.name == original.name);
    assert(loaded.components.size() == original.components.size());
    assert(loaded.connections.size() == original.connections.size());

    // Substitui o arquivo por JSON malformado.
    {
        std::ofstream output(path, std::ios::trunc);
        output << "{ JSON invalido";
    }

    loaded.name = "Nao sobrescrever";

    assert(!Serializer::load(path, loaded, &error));
    assert(!error.empty());
    assert(loaded.name == "Nao sobrescrever");

    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

void testUnsupportedVersion() {
    Json invalid = Serializer::toJson(makeSampleProject());
    invalid["format_version"] = 999;

    Project destination;
    std::string error;

    assert(!Serializer::fromJson(invalid, destination, &error));
    assert(!error.empty());
}

} // namespace

int main() {
    testRoundTrip();
    testDuplicateComponentId();
    testMissingConnectionReference();
    testInvalidOrientationAndProperties();
    testSaveLoadAndInvalidFile();
    testUnsupportedVersion();

    std::cout << "Todos os testes de serializacao passaram.\n";
    return 0;
}
