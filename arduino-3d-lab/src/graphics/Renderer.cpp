
#include "graphics/Renderer.hpp"

#include "graphics/Camera.hpp"
#include "graphics/Scene.hpp"
#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef ARDUINO3D_ASSET_DIR
#define ARDUINO3D_ASSET_DIR "assets"
#endif

namespace arduino3d {
namespace {

constexpr float pi = 3.14159265358979323846F;

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

// Adiciona um vértice no formato posição XYZ + normal XYZ.
void addVertex(
    std::vector<float>& vertices,
    const glm::vec3& position,
    const glm::vec3& normal
) {
    vertices.insert(vertices.end(), {
        position.x, position.y, position.z,
        normal.x, normal.y, normal.z
    });
}

void addTriangle(
    std::vector<float>& vertices,
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& normalA,
    const glm::vec3& normalB,
    const glm::vec3& normalC
) {
    addVertex(vertices, a, normalA);
    addVertex(vertices, b, normalB);
    addVertex(vertices, c, normalC);
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
            throw std::runtime_error("Uniforms dos shaders nao encontrados.");
        }

        createCube();
        createCylinder();
        createSphere();
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
        GL_VERTEX_SHADER, readTextFile(vertexPath)
    );

    GLuint fragmentShader = 0;
    GLuint program = 0;

    try {
        fragmentShader = compileShader(
            GL_FRAGMENT_SHADER, readTextFile(fragmentPath)
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

Renderer::Mesh Renderer::createMesh(const std::vector<float>& vertices) {
    Mesh mesh;
    mesh.vertexCount = static_cast<GLsizei>(vertices.size() / 6);

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        6 * static_cast<GLsizei>(sizeof(float)), nullptr
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        6 * static_cast<GLsizei>(sizeof(float)),
        reinterpret_cast<void*>(3 * sizeof(float))
    );

    glBindVertexArray(0);
    return mesh;
}

void Renderer::createCube() {
    std::vector<float> vertices;

    struct Face {
        glm::vec3 normal;
        glm::vec3 a, b, c, d;
    };

    const std::vector<Face> faces{
        {{0, 0, 1},  {-0.5F,-0.5F, 0.5F}, {0.5F,-0.5F, 0.5F},
                     {0.5F, 0.5F, 0.5F}, {-0.5F, 0.5F, 0.5F}},
        {{0, 0,-1},  {0.5F,-0.5F,-0.5F}, {-0.5F,-0.5F,-0.5F},
                    {-0.5F, 0.5F,-0.5F}, {0.5F, 0.5F,-0.5F}},
        {{1, 0, 0},  {0.5F,-0.5F, 0.5F}, {0.5F,-0.5F,-0.5F},
                    {0.5F, 0.5F,-0.5F}, {0.5F, 0.5F, 0.5F}},
        {{-1,0, 0},  {-0.5F,-0.5F,-0.5F}, {-0.5F,-0.5F, 0.5F},
                    {-0.5F, 0.5F, 0.5F}, {-0.5F, 0.5F,-0.5F}},
        {{0, 1, 0},  {-0.5F, 0.5F, 0.5F}, {0.5F, 0.5F, 0.5F},
                    {0.5F, 0.5F,-0.5F}, {-0.5F, 0.5F,-0.5F}},
        {{0,-1, 0},  {-0.5F,-0.5F,-0.5F}, {0.5F,-0.5F,-0.5F},
                    {0.5F,-0.5F, 0.5F}, {-0.5F,-0.5F, 0.5F}}
    };

    for (const Face& f : faces) {
        addTriangle(vertices, f.a, f.b, f.c, f.normal, f.normal, f.normal);
        addTriangle(vertices, f.a, f.c, f.d, f.normal, f.normal, f.normal);
    }

    cube_ = createMesh(vertices);
}

void Renderer::createCylinder() {
    std::vector<float> vertices;
    constexpr int segments = 20;

    // Cilindro unitário: raio 0,5 e altura 1, alinhado ao eixo Y.
    for (int i = 0; i < segments; ++i) {
        const float a0 = 2.0F * pi * static_cast<float>(i) / segments;
        const float a1 = 2.0F * pi * static_cast<float>(i + 1) / segments;

        const glm::vec3 p0{0.5F * std::cos(a0), -0.5F, 0.5F * std::sin(a0)};
        const glm::vec3 p1{0.5F * std::cos(a1), -0.5F, 0.5F * std::sin(a1)};
        const glm::vec3 p2{p1.x, 0.5F, p1.z};
        const glm::vec3 p3{p0.x, 0.5F, p0.z};

        const glm::vec3 n0{std::cos(a0), 0.0F, std::sin(a0)};
        const glm::vec3 n1{std::cos(a1), 0.0F, std::sin(a1)};

        addTriangle(vertices, p0, p1, p2, n0, n1, n1);
        addTriangle(vertices, p0, p2, p3, n0, n1, n0);

        const glm::vec3 bottom{0.0F, -0.5F, 0.0F};
        const glm::vec3 top{0.0F, 0.5F, 0.0F};

        addTriangle(
            vertices, bottom, p1, p0,
            {0,-1,0}, {0,-1,0}, {0,-1,0}
        );
        addTriangle(
            vertices, top, p3, p2,
            {0,1,0}, {0,1,0}, {0,1,0}
        );
    }

    cylinder_ = createMesh(vertices);
}

void Renderer::createSphere() {
    std::vector<float> vertices;
    constexpr int rings = 12;
    constexpr int segments = 20;

    for (int ring = 0; ring < rings; ++ring) {
        const float phi0 = pi * static_cast<float>(ring) / rings;
        const float phi1 = pi * static_cast<float>(ring + 1) / rings;

        for (int segment = 0; segment < segments; ++segment) {
            const float theta0 = 2.0F * pi * segment / segments;
            const float theta1 = 2.0F * pi * (segment + 1) / segments;

            const auto point = [](float phi, float theta) {
                return glm::vec3{
                    0.5F * std::sin(phi) * std::cos(theta),
                    0.5F * std::cos(phi),
                    0.5F * std::sin(phi) * std::sin(theta)
                };
            };

            const glm::vec3 a = point(phi0, theta0);
            const glm::vec3 b = point(phi1, theta0);
            const glm::vec3 c = point(phi1, theta1);
            const glm::vec3 d = point(phi0, theta1);

            addTriangle(vertices, a, b, c,
                        glm::normalize(a), glm::normalize(b), glm::normalize(c));
            addTriangle(vertices, a, c, d,
                        glm::normalize(a), glm::normalize(c), glm::normalize(d));
        }
    }

    sphere_ = createMesh(vertices);
}

void Renderer::createGrid() {
    std::vector<float> vertices;
    constexpr int extent = 10;
    constexpr float y = 0.002F;

    for (int i = -extent; i <= extent; ++i) {
        const float p = static_cast<float>(i);

        vertices.insert(vertices.end(), {
            -static_cast<float>(extent), y, p, 0,1,0,
             static_cast<float>(extent), y, p, 0,1,0,
             p, y, -static_cast<float>(extent), 0,1,0,
             p, y,  static_cast<float>(extent), 0,1,0
        });
    }

    gridVertexCount_ = static_cast<GLsizei>(vertices.size() / 6);

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
        0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
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
    const glm::mat4 projection = camera.projectionMatrix(
        static_cast<float>(width) / static_cast<float>(height)
    );

    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(viewLocation_, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(
        projectionLocation_, 1, GL_FALSE, glm::value_ptr(projection)
    );

    glBindVertexArray(gridVAO_);
    glUniformMatrix4fv(
        modelLocation_, 1, GL_FALSE, glm::value_ptr(glm::mat4{1.0F})
    );
    glUniform3f(colorLocation_, 0.20F, 0.27F, 0.34F);
    glDrawArrays(GL_LINES, 0, gridVertexCount_);

    for (const RenderObject& object : scene.objects()) {
        const Mesh* mesh = &cube_;

        switch (object.geometry) {
        case Geometry::Cube:
            mesh = &cube_;
            break;
        case Geometry::Cylinder:
            mesh = &cylinder_;
            break;
        case Geometry::Sphere:
            mesh = &sphere_;
            break;
        }

        glUniformMatrix4fv(
            modelLocation_, 1, GL_FALSE, glm::value_ptr(object.model)
        );
        glUniform3fv(
            colorLocation_, 1, glm::value_ptr(object.color)
        );

        glBindVertexArray(mesh->vao);
        glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);
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

    for (Mesh* mesh : {&cube_, &cylinder_, &sphere_}) {
        if (mesh->vbo != 0) {
            glDeleteBuffers(1, &mesh->vbo);
            mesh->vbo = 0;
        }
        if (mesh->vao != 0) {
            glDeleteVertexArrays(1, &mesh->vao);
            mesh->vao = 0;
        }
        mesh->vertexCount = 0;
    }

    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }
}

} // namespace arduino3d
