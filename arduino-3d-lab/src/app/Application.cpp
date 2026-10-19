#include "app/Application.hpp"
#include "app/EditorUI.hpp"
#include <cstdio>
#include "graphics/Camera.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Scene.hpp"

#include "circuit/Circuit.hpp"
#include "circuit/Pin.hpp"

#include "emulator/AVR.hpp"
#include "emulator/CPU.hpp"
#include "emulator/FirmwareLoader.hpp"
#include "emulator/GPIO.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace arduino3d {
namespace {

constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;

constexpr double CpuFrequencyHz = 16'000'000.0;
constexpr double MaxFrameDelta = 0.05;
constexpr double MaxPendingCycles =
    16'000'000.0 * 0.25;

// Limita o trabalho por frame para preservar a responsividade
// da interface, mesmo com firmware problemático.
constexpr std::uint64_t MaxInstructionsPerFrame = 50'000;

constexpr const char* WindowTitle = "Arduino 3D Lab";
constexpr const char* InitialFirmwarePath =
    "build/firmware/blink.hex";

struct WindowDeleter {
    void operator()(GLFWwindow* window) const noexcept {
        if (window != nullptr) {
            glfwDestroyWindow(window);
        }
    }
};

using WindowHandle = std::unique_ptr<GLFWwindow, WindowDeleter>;

class GlfwRuntime {
public:
    GlfwRuntime() {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error(
                "Falha ao inicializar GLFW."
            );
        }
    }

    ~GlfwRuntime() {
        glfwTerminate();
    }

    GlfwRuntime(const GlfwRuntime&) = delete;
    GlfwRuntime& operator=(const GlfwRuntime&) = delete;
};

void framebufferSizeCallback(
    GLFWwindow*,
    int width,
    int height
) {
    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }
}

void scrollCallback(
    GLFWwindow* window,
    double,
    double yOffset
) {
    auto* camera = static_cast<Camera*>(
        glfwGetWindowUserPointer(window)
    );

    if (camera != nullptr) {
        camera->zoom(static_cast<float>(yOffset));
    }
}

WindowHandle createWindow() {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    WindowHandle window(
        glfwCreateWindow(
            WindowWidth,
            WindowHeight,
            WindowTitle,
            nullptr,
            nullptr
        )
    );

    if (!window) {
        throw std::runtime_error(
            "Falha ao criar a janela GLFW."
        );
    }

    glfwMakeContextCurrent(window.get());
    glfwSwapInterval(1);

    if (gladLoadGL(
            reinterpret_cast<GLADloadfunc>(
                glfwGetProcAddress
            )
        ) == 0) {
        throw std::runtime_error(
            "Falha ao inicializar GLAD."
        );
    }

    return window;
}

void configureWindowCallbacks(
    GLFWwindow* window,
    Camera& camera
) {
    glfwSetWindowUserPointer(window, &camera);

    glfwSetFramebufferSizeCallback(
        window,
        framebufferSizeCallback
    );

    glfwSetScrollCallback(
        window,
        scrollCallback
    );

    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);

    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }
}

class EmulatorRuntime {
public:
    explicit EmulatorRuntime(const std::string& firmwarePath)
        : cpu{},
          circuit{},
          d13(circuit.addPin(
              "D13",
              PinDirection::Input
          )),
          gnd(circuit.addPin(
              "GND",
              PinDirection::Output
          )),
          pinMap(makePinMap(d13)),
          gpio(cpu, circuit, pinMap),
          avr(cpu, &gpio) {

        circuit.setOutput(gnd, DigitalState::Low);
        circuit.addLed(d13, gnd);

        std::string error;

        if (!emulator::FirmwareLoader::loadIntelHex(
                firmwarePath,
                cpu.memory,
                &error
            )) {
            throw std::runtime_error(
                "Falha ao carregar firmware '" +
                firmwarePath + "': " + error
            );
        }

        cpu.pc = 0;

        if (!gpio.synchronize(&error)) {
            throw std::runtime_error(
                "Falha ao inicializar GPIO: " + error
            );
        }

        std::cout
            << "Firmware carregado: "
            << firmwarePath << '\n';
    }

    emulator::AVR& processor() noexcept {
        return avr;
    }

    std::uint32_t programCounter() const noexcept {
        return static_cast<std::uint32_t>(cpu.pc);
    }

    bool ledOn() const {
        return circuit.ledOn(0);
    }

private:
    static emulator::GPIO::PinMap makePinMap(PinId ledPin) {
        emulator::GPIO::PinMap map{};
        map.fill(0);
        map[13] = ledPin;
        return map;
    }

    emulator::CPUState cpu;
    Circuit circuit;

    PinId d13;
    PinId gnd;

    emulator::GPIO::PinMap pinMap;
    emulator::GPIO gpio;
    emulator::AVR avr;
};

void printStartupInformation() {
    std::cout << "Arduino 3D Lab v0.1.0\n";
    std::cout
        << "OpenGL: "
        << glGetString(GL_VERSION)
        << '\n';
    std::cout
        << "Mouse: orbitar e usar a roda para zoom.\n";
    std::cout
        << "WASD: mover | Q/E: vertical | Esc: sair.\n";
    std::cout
        << "Espaço: pausar/retomar a simulação.\n";
}

void processInput(
    GLFWwindow* window,
    Camera& camera,
    float deltaTime
) {
    camera.processInput(window, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void updatePauseState(
    GLFWwindow* window,
    bool& paused,
    int& previousSpaceState
) {
    const int spaceState =
        glfwGetKey(window, GLFW_KEY_SPACE);

    if (
        spaceState == GLFW_PRESS &&
        previousSpaceState == GLFW_RELEASE
    ) {
        paused = !paused;

        std::cout
            << (paused
                ? "Simulação pausada.\n"
                : "Simulação retomada.\n");
    }

    previousSpaceState = spaceState;
}

bool executeFirmwareFrame(
    emulator::AVR& avr,
    double deltaTime,
    double& pendingCycles,
    double& simulatedSeconds,
    std::uint64_t& executedCycles,
    bool& firmwareRunning,
    std::string& runtimeError
) {
    if (!firmwareRunning) {
        return false;
    }

    const double elapsed = std::clamp(
        deltaTime,
        0.0,
        MaxFrameDelta
    );

    pendingCycles = std::min(
        pendingCycles + elapsed * CpuFrequencyHz,
        MaxPendingCycles
    );

    const auto cycleBudget =
        static_cast<std::uint64_t>(pendingCycles);

    std::uint64_t consumedCycles = 0;
    std::uint64_t instructionCount = 0;

    while (
        consumedCycles < cycleBudget &&
        instructionCount < MaxInstructionsPerFrame
    ) {
        const emulator::StepResult result = avr.step();
        ++instructionCount;

        if (!result.ok()) {
            runtimeError =
                "Falha na execução do firmware. PC: 0x";

            char pcBuffer[16]{};
            std::snprintf(
                pcBuffer,
                sizeof(pcBuffer),
                "%04X",
                static_cast<unsigned int>(result.pcBefore)
            );

            runtimeError += pcBuffer;
            runtimeError += "\nInstrução: ";

            char wordBuffer[16]{};
            std::snprintf(
                wordBuffer,
                sizeof(wordBuffer),
                "0x%04X",
                static_cast<unsigned int>(result.word)
            );

            runtimeError += wordBuffer;
            runtimeError += "\n";
            runtimeError += result.message;

            std::cerr << runtimeError << '\n';

            firmwareRunning = false;
            return false;
        }

        if (result.cycles == 0) {
            runtimeError =
                "A instrução executada não contabilizou ciclos.";

            std::cerr << runtimeError << '\n';

            firmwareRunning = false;
            return false;
        }

        consumedCycles += result.cycles;
    }

    // Mantém o saldo de ciclos, inclusive quando o limite
    // de instruções impede consumir todo o orçamento do frame.
    pendingCycles -= static_cast<double>(consumedCycles);

    executedCycles += consumedCycles;
    simulatedSeconds +=
        static_cast<double>(consumedCycles) / CpuFrequencyHz;

    return true;
}

void renderScene(
    GLFWwindow* window,
    Renderer& renderer,
    Scene& scene,
    const Camera& camera,
    bool ledOn
) {
    scene.setLedOn(ledOn);

    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);

    if (width <= 0 || height <= 0) {
        return;
    }

    renderer.render(scene, camera, width, height);
}

} // namespace

int Application::run() const {
    try {
        GlfwRuntime glfwRuntime;
        WindowHandle window = createWindow();

        Camera camera;
        Scene scene;
        configureWindowCallbacks(window.get(), camera);

        // O ImGui é inicializado após a criação do contexto OpenGL.
        EditorUI editorUI;
        std::string uiError;

        if (!editorUI.initialize(window.get(), &uiError)) {
            throw std::runtime_error(
                "Falha ao inicializar EditorUI: " + uiError
            );
        }

        Renderer renderer;

        std::string firmwarePath = InitialFirmwarePath;
        std::string compileError;
        std::string runtimeError;

        std::unique_ptr<EmulatorRuntime> runtime;

        const auto loadFirmware =
            [&](const std::string& path) -> bool {
                try {
                    // Cria primeiro a nova instância. Se falhar,
                    // a instância anterior não é destruída.
                    auto replacement =
                        std::make_unique<EmulatorRuntime>(path);

                    runtime = std::move(replacement);
                    firmwarePath = path;

                    runtimeError.clear();
                    return true;
                } catch (const std::exception& exception) {
                    runtimeError = exception.what();
                    std::cerr << runtimeError << '\n';
                    return false;
                }
            };

        // A ausência ou falha do firmware inicial é apresentada
        // na interface em vez de fechar imediatamente a janela.
        const bool initiallyLoaded = loadFirmware(firmwarePath);

        bool paused = !initiallyLoaded;
        bool firmwareRunning = initiallyLoaded;

        double previousTime = glfwGetTime();
        double pendingCycles = 0.0;
        double simulatedSeconds = 0.0;
        std::uint64_t executedCycles = 0;

        int previousSpaceState = GLFW_RELEASE;

        printStartupInformation();

        while (
            glfwWindowShouldClose(window.get()) == GLFW_FALSE
        ) {
            const double currentTime = glfwGetTime();
            const double deltaTime = std::max(
                0.0,
                currentTime - previousTime
            );

            previousTime = currentTime;

            processInput(
                window.get(),
                camera,
                static_cast<float>(
                    std::min(deltaTime, MaxFrameDelta)
                )
            );

            updatePauseState(
                window.get(),
                paused,
                previousSpaceState
            );

            if (!paused && firmwareRunning && runtime) {
                executeFirmwareFrame(
                    runtime->processor(),
                    deltaTime,
                    pendingCycles,
                    simulatedSeconds,
                    executedCycles,
                    firmwareRunning,
                    runtimeError
                );
            }

            // Cena 3D primeiro.
            renderScene(
                window.get(),
                renderer,
                scene,
                camera,
                runtime ? runtime->ledOn() : false
            );

            // Interface por cima da cena 3D.
            editorUI.beginFrame();

            EditorUIState uiState;
            uiState.status =
                !runtime || !runtimeError.empty()
                    ? SimulationStatus::Error
                    : !firmwareRunning
                        ? SimulationStatus::Error
                        : paused
                            ? SimulationStatus::Paused
                            : SimulationStatus::Running;

            uiState.firmwarePath = firmwarePath;
            uiState.microcontroller = "ATmega328P";
            uiState.cpuFrequencyHz = CpuFrequencyHz;
            uiState.programCounter =
                runtime ? runtime->programCounter() : 0;
            uiState.executedCycles = executedCycles;
            uiState.simulatedSeconds = simulatedSeconds;
            uiState.compileError = compileError;
            uiState.runtimeError = runtimeError;

            editorUI.render(uiState);
            const EditorUIActions actions =
                editorUI.consumeActions();

            // A interface apenas emite comandos. A aplicação
            // decide como executá-los.
            if (actions.firmwareChanged) {
                if (loadFirmware(actions.selectedFirmware)) {
                    pendingCycles = 0.0;
                    simulatedSeconds = 0.0;
                    executedCycles = 0;
                    firmwareRunning = true;
                    paused = false;
                } else {
                    firmwareRunning = false;
                    paused = true;
                }
            } else if (actions.restartRequested) {
                if (loadFirmware(firmwarePath)) {
                    pendingCycles = 0.0;
                    simulatedSeconds = 0.0;
                    executedCycles = 0;
                    firmwareRunning = true;
                    paused = false;
                } else {
                    firmwareRunning = false;
                    paused = true;
                }
            }

            if (actions.pauseRequested) {
                paused = true;
            }

            if (actions.startRequested) {
                if (runtime && runtimeError.empty()) {
                    paused = false;
                    firmwareRunning = true;
                }
            }

            editorUI.endFrame();

            glfwSwapBuffers(window.get());
            glfwPollEvents();
        }

        std::cout
            << "Ciclos executados: "
            << executedCycles
            << "\nTempo simulado: "
            << simulatedSeconds
            << " s\n";

        return 0;
    } catch (const std::exception& exception) {
        std::cerr
            << "Erro na aplicação: "
            << exception.what()
            << '\n';

        return 1;
    }
}

} // namespace arduino3d
