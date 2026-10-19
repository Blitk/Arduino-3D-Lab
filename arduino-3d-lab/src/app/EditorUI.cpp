#include "app/EditorUI.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace arduino3d {
namespace {

const char* statusLabel(SimulationStatus status)
{
    switch (status) {
    case SimulationStatus::Ready:
        return "Pronta";
    case SimulationStatus::Running:
        return "Em execução";
    case SimulationStatus::Paused:
        return "Pausada";
    case SimulationStatus::Error:
        return "Erro";
    }

    return "Desconhecido";
}

std::vector<std::string> discoverFirmwares()
{
    namespace fs = std::filesystem;

    const std::vector<fs::path> directories{
        "build/firmware",
        "firmware/build",
        "firmware"
    };

    std::vector<std::string> firmwares;

    for (const auto& directory : directories) {
        std::error_code error;

        if (!fs::is_directory(directory, error) || error) {
            continue;
        }

        fs::directory_iterator it(directory, error);
        const fs::directory_iterator end;

        for (; !error && it != end; it.increment(error)) {
            std::error_code fileError;

            if (!it->is_regular_file(fileError) || fileError) {
                continue;
            }

            if (it->path().extension() == ".hex") {
                firmwares.push_back(it->path().string());
            }
        }
    }

    std::sort(firmwares.begin(), firmwares.end());
    firmwares.erase(
        std::unique(firmwares.begin(), firmwares.end()),
        firmwares.end()
    );

    return firmwares;
}

bool containsFirmware(
    const std::vector<std::string>& firmwares,
    const std::string& path)
{
    return std::find(firmwares.begin(), firmwares.end(), path)
        != firmwares.end();
}

} // namespace

EditorUI::~EditorUI()
{
    shutdown();
}

bool EditorUI::initialize(GLFWwindow* window, std::string* error)
{
    if (initialized_) {
        return true;
    }

    if (window == nullptr) {
        if (error) {
            *error = "Não é possível iniciar o EditorUI: janela nula.";
        }
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();

        if (error) {
            *error = "Falha ao inicializar o backend GLFW do Dear ImGui.";
        }
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (error) {
            *error = "Falha ao inicializar o backend OpenGL3 do Dear ImGui.";
        }
        return false;
    }

    initialized_ = true;
    return true;
}

void EditorUI::shutdown()
{
    if (!initialized_) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    initialized_ = false;
}

void EditorUI::beginFrame()
{
    if (!initialized_) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EditorUI::render(const EditorUIState& state)
{
    if (!initialized_) {
        return;
    }

    ImGui::SetNextWindowSize(
        ImVec2(360.0F, 520.0F),
        ImGuiCond_FirstUseEver
    );

    if (!ImGui::Begin("Arduino 3D Lab")) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Controle da simulacao");
    ImGui::Separator();

    ImGui::Text("Estado: %s", statusLabel(state.status));

    const bool canStart =
        state.status == SimulationStatus::Ready ||
        state.status == SimulationStatus::Paused;

    if (!canStart) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Iniciar", ImVec2(100.0F, 0.0F))) {
        actions_.startRequested = true;
    }

    if (!canStart) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    const bool canPause = state.status == SimulationStatus::Running;

    if (!canPause) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Pausar", ImVec2(100.0F, 0.0F))) {
        actions_.pauseRequested = true;
    }

    if (!canPause) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    if (ImGui::Button("Reiniciar", ImVec2(100.0F, 0.0F))) {
        actions_.restartRequested = true;
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Firmware");
    ImGui::Separator();

    std::vector<std::string> firmwares = state.availableFirmwares;

    if (firmwares.empty()) {
        firmwares = discoverFirmwares();
    }

    std::string selected = state.firmwarePath;

    if (!containsFirmware(firmwares, selected) && !firmwares.empty()) {
        selected = firmwares.front();
    }

    const char* preview = selected.empty()
        ? "Nenhum firmware encontrado"
        : selected.c_str();

    if (ImGui::BeginCombo("Arquivo HEX", preview)) {
        for (const auto& firmware : firmwares) {
            const bool isSelected = firmware == selected;

            if (ImGui::Selectable(firmware.c_str(), isSelected)) {
                if (firmware != state.firmwarePath) {
                    actions_.firmwareChanged = true;
                    actions_.selectedFirmware = firmware;
                }
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    if (firmwares.empty()) {
        ImGui::TextWrapped(
            "Nenhum .hex encontrado. Compile o firmware primeiro."
        );
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Microcontrolador");
    ImGui::Separator();

    ImGui::Text("Modelo: %s", state.microcontroller.c_str());
    ImGui::Text("Clock: %.2f MHz", state.cpuFrequencyHz / 1'000'000.0);
    ImGui::Text("PC: 0x%04X",
        static_cast<unsigned int>(state.programCounter));
    ImGui::Text("Ciclos executados: %llu",
        static_cast<unsigned long long>(state.executedCycles));
    ImGui::Text("Tempo simulado: %.3f s", state.simulatedSeconds);

    ImGui::Spacing();
    ImGui::TextUnformatted("Mensagens");
    ImGui::Separator();

    if (state.compileError.empty() && state.runtimeError.empty()) {
        ImGui::TextDisabled("Nenhum erro registrado.");
    } else {
        ImGui::BeginChild(
            "ErrorLog",
            ImVec2(0.0F, 110.0F),
            ImGuiChildFlags_Borders
        );

        if (!state.compileError.empty()) {
            ImGui::TextUnformatted("Erro de compilacao:");
            ImGui::TextWrapped("%s", state.compileError.c_str());
            ImGui::Spacing();
        }

        if (!state.runtimeError.empty()) {
            ImGui::TextUnformatted("Erro de execucao:");
            ImGui::TextWrapped("%s", state.runtimeError.c_str());
        }

        ImGui::EndChild();
    }

    ImGui::End();
}

void EditorUI::endFrame()
{
    if (!initialized_) {
        return;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

EditorUIActions EditorUI::consumeActions()
{
    EditorUIActions result = std::move(actions_);
    actions_ = {};
    return result;
}

} // namespace arduino3d
