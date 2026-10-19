
#include "app/Application.hpp"

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

#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace arduino3d {
namespace {

constexpr int WindowWidth = 1280;
constexpr int WindowHeight = 720;
constexpr int InstructionsPerFrame = 1000;

constexpr const char* WindowTitle = "Arduino 3D Lab";
constexpr const char* FirmwarePath = "build/firmware/blink.hex";

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

// Agrupa os componentes que precisam compartilhar
// as mesmas instâncias de CPU, circuito e GPIO.
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

        circuit.setOutput(
            gnd,
            DigitalState::Low
        );

        // Registra o LED integrado no circuito lógico.
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

    const Circuit& circuitState() const noexcept {
        return circuit;
    }

private:
    static emulator::GPIO::PinMap makePinMap(PinId ledPin) {
        emulator::GPIO::PinMap map{};
        map.fill(0);
        map[13] = ledPin;
        return map;
    }

    // A ordem dos membros é importante: as referências
    // usadas por GPIO e AVR precisam continuar válidas.
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
    std::cout << "OpenGL: "
              << glGetString(GL_VERSION)
              << '\n';
    std::cout
        << "Mouse: orbitar e usar a roda para zoom.\n";
    std::cout
        << "WASD: mover | Q/E: vertical | Esc: sair.\n";
}

bool executeFirmwareFrame(
    emulator::AVR& avr,
    bool& firmwareRunning
) {
    if (!firmwareRunning) {
        return false;
    }

    for (int i = 0; i < InstructionsPerFrame; ++i) {
        const emulator::StepResult result = avr.step();

        if (!result.ok()) {
            std::cerr
                << "Erro durante a execução do firmware.\n"
                << "PC: 0x"
                << std::hex << result.pcBefore
                << "\nInstrução: 0x"
                << result.word
                << std::dec
                << "\nDetalhes: "
                << result.message
                << '\n';

            firmwareRunning = false;
            return false;
        }
    }

    return true;
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

void renderFrame(
    GLFWwindow* window,
    Renderer& renderer,
    const Scene& scene,
    const Camera& camera
) {
    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);

    if (width <= 0 || height <= 0) {
        return;
    }

    renderer.render(scene, camera, width, height);
    glfwSwapBuffers(window);
}

} // namespace

int Application::run() const {
    try {
        GlfwRuntime glfwRuntime;
        WindowHandle window = createWindow();

        Camera camera;
        Scene scene;

        configureWindowCallbacks(window.get(), camera);

        // A instância permanece viva durante todo o loop.
        EmulatorRuntime runtime(FirmwarePath);

        // Declarado depois da janela para ser destruído antes dela.
        Renderer renderer;

        printStartupInformation();

        float previousTime =
            static_cast<float>(glfwGetTime());

        bool firmwareRunning = true;

        while (
            glfwWindowShouldClose(window.get())
            == GLFW_FALSE
        ) {
            const float currentTime =
                static_cast<float>(glfwGetTime());

            const float deltaTime =
                currentTime - previousTime;

            previousTime = currentTime;

            processInput(
                window.get(),
                camera,
                deltaTime
            );

            executeFirmwareFrame(
                runtime.processor(),
                firmwareRunning
            );

            renderFrame(
                window.get(),
                renderer,
                scene,
                camera
            );

            glfwPollEvents();
        }

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
