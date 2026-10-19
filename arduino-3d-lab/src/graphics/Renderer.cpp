#include "graphics/Renderer.hpp"

#include "graphics/Camera.hpp"
#include "graphics/Scene.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

#ifndef ARDUINO3D_ASSET_DIR
#define ARDUINO3D_ASSET_DIR "assets"
#endif

namespace arduino3d {
namespace {

std::string readTextFile(const std::string& path) {
    std::ifstream file(path);

    if (!file) {
        throw std::runtime_error("Nao foi possivel abrir: " + path);
    }

    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

GLuint compileShader(GLenum type, const std::string& source) {
    const GLuint shader = glCreateShader(type);

    if (shader == 0) {
        throw std::runtime_error("Falha ao criar shader OpenGL.");
    }

    const char* sourcePointer = source.c_str();
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        std::string log(static_cast<std::size_t>(length), '\0');

        if (length > 0) {
            glGetShaderInfoLog(shader, length, nullptr, log.data());
        }

        glDeleteShader(shader);
        throw std::runtime_error("Erro ao compilar shader: " + log);
    }

    return shader;
}

} // namespace

Renderer::Renderer() {
    try {
        shaderProgram_ = createShaderProgram(
            std::string{ARDUINO3D_ASSET_DIR} + "/shaders/basic.vert",
            std::string{ARDUINO3D_ASSET_DIR} + "/shaders/basic.frag"
        );

        modelLocation_ = glGetUniformLocation(shaderProgram_, "uModel");
        viewLocation_ = glGetUniformLocation(shaderProgram_, "uView");
        projectionLocation_ =
            glGetUniformLocation(shaderProgram_, "uProjection");
        colorLocation_ = glGetUniformLocation(shaderProgram_, "uColor");

        if (modelLocation_ < 0 || viewLocation_ < 0 ||
            projectionLocation_ < 0 || colorLocation_ < 0) {
            throw std::runtime_error(
                "Uniforms necessarios nao encontrados nos shaders."
            );
        }

        createCube();
        createGrid();

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    } catch (...) {
        destroy();
        throw;
    }
}

Renderer::~Renderer() {
    destroy();
}

GLuint Renderer::createShaderProgram(
    const std::string& vertexPath,
    const std::string& fragmentPath
) {
    const GLuint vertexShader = compileShader(
        GL_VERTEX_SHADER,
        readTextFile(vertexPath)
    );

    GLuint fragmentShader = 0;
    GLuint program = 0;

    try {
        fragmentShader = compileShader(
            GL_FRAGMENT_SHADER,
            readTextFile(fragmentPath)
        );

        program = glCreateProgram();

        if (program == 0) {
            throw std::runtime_error("Falha ao criar programa OpenGL.");
        }

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (success != GL_TRUE) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

            std::string log(static_cast<std::size_t>(length), '\0');

            if (length > 0) {
                glGetProgramInfoLog(program, length, nullptr, log.data());
            }

            throw std::runtime_error("Erro ao vincular shaders: " + log);
        }
    } catch (...) {
        if (program != 0) {
            glDeleteProgram(program);
        }

        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
        }

        glDeleteShader(vertexShader);
        throw;
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void Renderer::createCube() {
    // Cada vertice contem posicao XYZ e normal XYZ.
    // Cada face tem quatro vertices e dois triangulos.
    constexpr float vertices[] = {
        // Frente (+Z)
        -0.5F,-0.5F, 0.5F, 0,0,1,
         0.5F,-0.5F, 0.5F, 0,0,1,
         0.5F, 0.5F, 0.5F, 0,0,1,
        -0.5F,-0.5F, 0.5F, 0,0,1,
         0.5F, 0.5F, 0.5F, 0,0,1,
        -0.5F, 0.5F, 0.5F, 0,0,1,

        // Tras (-Z)
         0.5F,-0.5F,-0.5F, 0,0,-1,
        -0.5F,-0.5F,-0.5F, 0,0,-1,
        -0.5F, 0.5F,-0.5F, 0,0,-1,
         0.5F,-0.5F,-0.5F, 0,0,-1,
        -0.5F, 0.5F,-0.5F, 0,0,-1,
         0.5F, 0.5F,-0.5F, 0,0,-1,

        // Direita (+X)
         0.5F,-0.5F, 0.5F, 1,0,0,
         0.5F,-0.5F,-0.5F, 1,0,0,
         0.5F, 0.5F,-0.5F, 1,0,0,
         0.5F,-0.5F, 0.5F, 1,0,0,
         0.5F, 0.5F,-0.5F, 1,0,0,
         0.5F, 0.5F, 0.5F, 1,0,0,

        // Esquerda (-X)
        -0.5F,-0.5F,-0.5F, -1,0,0,
        -0.5F,-0.5F, 0.5F, -1,0,0,
        -0.5F, 0.5F, 0.5F, -1,0,0,
        -0.5F,-0.5F,-0.5F, -1,0,0,
        -0.5F, 0.5F, 0.5F, -1,0,0,
        -0.5F, 0.5F,-0.5F, -1,0,0,

        // Topo (+Y)
        -0.5F, 0.5F, 0.5F, 0,1,0,
         0.5F, 0.5F, 0.5F, 0,1,0,
         0.5F, 0.5F,-0.5F, 0,1,0,
        -0.5F, 0.5F, 0.5F, 0,1,0,
         0.5F, 0.5F,-0.5F, 0,1,0,
        -0.5F, 0.5F,-0.5F, 0,1,0,

        // Base (-Y)
        -0.5F,-0.5F,-0.5F, 0,-1,0,
         0.5F,-0.5F,-0.5F, 0,-1,0,
         0.5F,-0.5F, 0.5F, 0,-1,0,
        -0.5F,-0.5F,-0.5F, 0,-1,0,
         0.5F,-0.5F, 0.5F, 0,-1,0,
        -0.5F,-0.5F, 0.5F, 0,-1,0
    };

    glGenVertexArrays(1, &cubeVAO_);
    glGenBuffers(1, &cubeVBO_);

    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );

    glBindVertexArray(0);
}

void Renderer::createGrid() {
    std::vector<float> vertices;

    constexpr int extent = 10;
    constexpr float y = 0.002F;

    for (int i = -extent; i <= extent; ++i) {
        const float p = static_cast<float>(i);

        // Linhas paralelas aos eixos X e Z.
        vertices.insert(vertices.end(), {
            -static_cast<float>(extent), y, p,
             static_cast<float>(extent), y, p,
             p, y, -static_cast<float>(extent),
             p, y,  static_cast<float>(extent)
        });
    }

    gridVertexCount_ = static_cast<GLsizei>(vertices.size() / 3);

    glGenVertexArrays(1, &gridVAO_);
    glGenBuffers(1, &gridVBO_);

    glBindVertexArray(gridVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr
    );

    glBindVertexArray(0);
}

void Renderer::render(
    const Scene& scene,
    const Camera& camera,
    int width,
    int height
) {
    if (width <= 0 || height <= 0) {
        return;
    }

    glViewport(0, 0, width, height);
    glClearColor(0.035F, 0.055F, 0.085F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection =
        camera.projectionMatrix(
            static_cast<float>(width) / static_cast<float>(height)
        );

    glUseProgram(shaderProgram_);

    glUniformMatrix4fv(
        viewLocation_, 1, GL_FALSE, glm::value_ptr(view)
    );
    glUniformMatrix4fv(
        projectionLocation_, 1, GL_FALSE, glm::value_ptr(projection)
    );

    // Grade de referencia.
    glUniformMatrix4fv(
        modelLocation_, 1, GL_FALSE,
        glm::value_ptr(glm::mat4{1.0F})
    );
    glUniform3f(colorLocation_, 0.20F, 0.27F, 0.34F);

    glBindVertexArray(gridVAO_);
    glDrawArrays(GL_LINES, 0, gridVertexCount_);

    // Objetos sólidos da cena.
    glBindVertexArray(cubeVAO_);

    for (const RenderObject& object : scene.objects()) {
        glUniformMatrix4fv(
            modelLocation_, 1, GL_FALSE, glm::value_ptr(object.model)
        );
        glUniform3fv(
            colorLocation_, 1, glm::value_ptr(object.color)
        );

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::destroy() noexcept {
    if (gridVBO_ != 0) {
        glDeleteBuffers(1, &gridVBO_);
        gridVBO_ = 0;
    }

    if (gridVAO_ != 0) {
        glDeleteVertexArrays(1, &gridVAO_);
        gridVAO_ = 0;
    }

    if (cubeVBO_ != 0) {
        glDeleteBuffers(1, &cubeVBO_);
        cubeVBO_ = 0;
    }

    if (cubeVAO_ != 0) {
        glDeleteVertexArrays(1, &cubeVAO_);
        cubeVAO_ = 0;
    }

    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }
}

} // namespace arduino3d
