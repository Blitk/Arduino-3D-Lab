#include "app/Application.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace arduino3d {
namespace {

struct GlfwDeleter {
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
            throw std::runtime_error("Falha ao inicializar o GLFW.");
        }
    }

    ~GlfwRuntime() {
        glfwTerminate();
    }

    GlfwRuntime(const GlfwRuntime&) = delete;
    GlfwRuntime& operator=(const GlfwRuntime&) = delete;
};

void framebufferSizeCallback(
    GLFWwindow* window,
    int width,
    int height
) {
    (void)window;

    if (width > 0 && height > 0) {
        glViewport(0, 0, width, height);
    }
}

} // namespace

int Application::run() const {
    try {
        GlfwRuntime glfwRuntime;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* rawWindow = glfwCreateWindow(
            1280,
            720,
            "Arduino 3D Lab",
            nullptr,
            nullptr
        );

        if (rawWindow == nullptr) {
            std::cerr << "Falha ao criar a janela OpenGL.\n";
            return 1;
        }

        std::unique_ptr<GLFWwindow, GlfwDeleter> window(rawWindow);

        glfwMakeContextCurrent(window.get());
        glfwSwapInterval(1);

        if (gladLoadGL(
                reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)
            ) == 0) {
            std::cerr << "Falha ao carregar as funcoes OpenGL pelo GLAD.\n";
            return 1;
        }

        glfwSetFramebufferSizeCallback(
            window.get(),
            framebufferSizeCallback
        );

        int framebufferWidth = 0;
        int framebufferHeight = 0;

        glfwGetFramebufferSize(
            window.get(),
            &framebufferWidth,
            &framebufferHeight
        );

        if (framebufferWidth > 0 && framebufferHeight > 0) {
            glViewport(0, 0, framebufferWidth, framebufferHeight);
        }

        std::cout << "Arduino 3D Lab v0.1.0\n";
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n';
        std::cout << "Pressione Esc para sair.\n";

        while (glfwWindowShouldClose(window.get()) == GLFW_FALSE) {
            if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
            }

            glClearColor(0.08F, 0.10F, 0.14F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);

            glfwSwapBuffers(window.get());
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
