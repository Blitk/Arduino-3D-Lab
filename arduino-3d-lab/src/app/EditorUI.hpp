#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GLFWwindow;

namespace arduino3d {

enum class SimulationStatus {
    Ready,
    Running,
    Paused,
    Error
};

struct EditorUIState {
    SimulationStatus status{SimulationStatus::Ready};
    std::string firmwarePath;
    std::vector<std::string> availableFirmwares;

    std::string compileError;
    std::string runtimeError;

    std::string microcontroller{"ATmega328P"};
    double cpuFrequencyHz{16'000'000.0};
    std::uint32_t programCounter{0};
    std::uint64_t executedCycles{0};
    double simulatedSeconds{0.0};
};

struct EditorUIActions {
    bool startRequested{false};
    bool pauseRequested{false};
    bool restartRequested{false};
    bool firmwareChanged{false};

    std::string selectedFirmware;
};

class EditorUI {
public:
    EditorUI() = default;
    ~EditorUI();

    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;

    bool initialize(GLFWwindow* window, std::string* error = nullptr);
    void shutdown();

    void beginFrame();
    void render(const EditorUIState& state);
    void endFrame();

    EditorUIActions consumeActions();

private:
    bool initialized_{false};
    EditorUIActions actions_{};
};

} // namespace arduino3d
