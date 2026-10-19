
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
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace arduino3d {
namespace {

struct WindowDeleter {
    void operator()(GLFWwindow* window) const noexcept {
        if (window != nullptr) {
            glfwDestroyWindow(window);
        }
    }
};

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

} // namespace

int Application::run() const {
    try {
        GlfwRuntime glfwRuntime;

        glfwWindowHint(
            GLFW_CONTEXT_VERSION_MAJOR, 3
        );
        glfwWindowHint(
            GLFW_CONTEXT_VERSION_MINOR, 3
        );
        glfwWindowHint(
            GLFW_OPENGL_PROFILE,
            GLFW_OPENGL_CORE_PROFILE
        );

        std::unique_ptr<GLFWwindow, WindowDeleter> window(
            glfwCreateWindow(
                1280,
                720,
                "Arduino 3D Lab",
                nullptr,
                nullptr
            )
        );

        if (!window) {
            std::cerr
                << "Falha ao criar a janela GLFW.\n";
            return 1;
        }

        glfwMakeContextCurrent(window.get());
        glfwSwapInterval(1);

        if (gladLoadGL(
                reinterpret_cast<GLADloadfunc>(
                    glfwGetProcAddress
                )
            ) == 0) {
            std::cerr
                << "Falha ao inicializar GLAD.\n";
            return 1;
        }

        Camera camera;
        Scene scene;

        glfwSetWindowUserPointer(
            window.get(), &camera
        );

        glfwSetFramebufferSizeCallback(
            window.get(), framebufferSizeCallback
        );

        glfwSetScrollCallback(
            window.get(), scrollCallback
        );

        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(
            window.get(), &width, &height
        );

        if (width > 0 && height > 0) {
            glViewport(0, 0, width, height);
        }

        // --------------------------------------------------
        // Inicialização do emulador ATmega328P
        // --------------------------------------------------

        arduino3d::emulator::CPUState cpu;
        arduino3d::Circuit circuit;

        // LED integrado do Arduino Uno: pino digital D13.
        const auto d13 = circuit.addPin(
            "D13",
            arduino3d::PinDirection::Input
        );

        // Referência lógica de GND para o LED.
        const auto gnd = circuit.addPin(
            "GND",
            arduino3d::PinDirection::Output
        );

        circuit.setOutput(
            gnd,
            arduino3d::DigitalState::Low
        );

        arduino3d::emulator::GPIO::PinMap pinMap{};
        pinMap.fill(0);
        pinMap[13] = d13;

        arduino3d::emulator::GPIO gpio(
            cpu,
            circuit,
            pinMap
        );

        // Carrega o firmware gerado por firmware/build.sh.
        const std::string firmwarePath =
            "build/firmware/blink.hex";

        std::string firmwareError;

        if (!arduino3d::emulator::FirmwareLoader::loadIntelHex(
                firmwarePath,
                cpu.memory,
                &firmwareError
            )) {
            std::cerr
                << "Erro ao carregar firmware '"
                << firmwarePath << "': "
                << firmwareError << '\n';

            return 1;
        }

        cpu.pc = 0;

        arduino3d::emulator::AVR avr(
            cpu,
            &gpio
        );

        std::cout
            << "Firmware carregado: "
            << firmwarePath << '\n';

        // --------------------------------------------------
        // Inicialização da renderização
        // --------------------------------------------------

        // Destruído antes da janela OpenGL.
        Renderer renderer;

        std::cout
            << "Arduino 3D Lab v0.1.0\n";

        std::cout
            << "OpenGL: "
            << glGetString(GL_VERSION)
            << '\n';

        std::cout
            << "Mouse: orbitar e usar a roda para zoom.\n";

        std::cout
            << "WASD: mover | Q/E: vertical | Esc: sair.\n";

        float previousTime =
            static_cast<float>(glfwGetTime());

        // --------------------------------------------------
        // Loop principal
        // --------------------------------------------------

        while (
            glfwWindowShouldClose(window.get())
            == GLFW_FALSE
        ) {
            const float currentTime =
                static_cast<float>(glfwGetTime());

            const float deltaTime =
                currentTime - previousTime;

            previousTime = currentTime;

            camera.processInput(
                window.get(),
                deltaTime
            );

            // Limita o trabalho de emulação por frame para
            // evitar bloquear indefinidamente a interface.
            constexpr int InstructionsPerFrame = 1000;

            for (
                int i = 0;
                i < InstructionsPerFrame;
                ++i
            ) {
                const auto result = avr.step();

                if (
                    result !=
                    arduino3d::emulator::StepStatus::Executed
                ) {
                    std::cerr
                        << "Erro durante a execução do firmware. "
                        << "O emulador interrompeu o programa.\n";

                    glfwSetWindowShouldClose(
                        window.get(),
                        GLFW_TRUE
                    );

                    break;
                }
            }

            glfwGetFramebufferSize(
                window.get(),
                &width,
                &height
            );

            if (width > 0 && height > 0) {
                renderer.render(
                    scene,
                    camera,
                    width,
                    height
                );

                glfwSwapBuffers(window.get());
            }

            if (
                glfwGetKey(
                    window.get(),
                    GLFW_KEY_ESCAPE
                ) == GLFW_PRESS
            ) {
                glfwSetWindowShouldClose(
                    window.get(),
                    GLFW_TRUE
                );
            }

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
