#include "app/Application.hpp"

#include "graphics/Camera.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/Scene.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

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
            throw std::runtime_error("Falha ao inicializar GLFW.");
        }
    }

    ~GlfwRuntime() {
        glfwTerminate();
    }

    GlfwRuntime(const GlfwRuntime&) = delete;
    GlfwRuntime& operator=(const GlfwRuntime&) = delete;
};

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
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

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
            std::cerr << "Falha ao criar a janela GLFW.\n";
            return 1;
        }

        glfwMakeContextCurrent(window.get());
        glfwSwapInterval(1);

        if (gladLoadGL(
                reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)
            ) == 0) {
            std::cerr << "Falha ao inicializar GLAD.\n";
            return 1;
        }

        Camera camera;
        Scene scene;

        glfwSetWindowUserPointer(window.get(), &camera);
        glfwSetFramebufferSizeCallback(
            window.get(), framebufferSizeCallback
        );
        glfwSetScrollCallback(window.get(), scrollCallback);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window.get(), &width, &height);

        if (width > 0 && height > 0) {
            glViewport(0, 0, width, height);
        }

        // Declarado depois da janela para ser destruído antes dela.
        Renderer renderer;

        std::cout << "Arduino 3D Lab v0.1.0\n";
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n';
        std::cout << "Mouse: orbitar e usar a roda para zoom.\n";
        std::cout << "WASD: mover | Q/E: vertical | Esc: sair.\n";

        float previousTime = static_cast<float>(glfwGetTime());

        while (glfwWindowShouldClose(window.get()) == GLFW_FALSE) {
            const float currentTime =
                static_cast<float>(glfwGetTime());

            const float deltaTime = currentTime - previousTime;
            previousTime = currentTime;

            camera.processInput(window.get(), deltaTime);

            glfwGetFramebufferSize(
                window.get(), &width, &height
            );

            if (width > 0 && height > 0) {
                renderer.render(scene, camera, width, height);
                glfwSwapBuffers(window.get());
            }

            if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
            }

            glfwPollEvents();
        }

        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Erro na aplicacao: "
                  << exception.what() << '\n';
        return 1;
    }
}

} // namespace arduino3d
