
#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

namespace arduino3d {

class Camera;
class Scene;

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void render(
        const Scene& scene,
        const Camera& camera,
        int width,
        int height
    );

private:
    struct Mesh {
        GLuint vao{0};
        GLuint vbo{0};
        GLsizei vertexCount{0};
    };

    GLuint createShaderProgram(
        const std::string& vertexPath,
        const std::string& fragmentPath
    );

    Mesh createMesh(const std::vector<float>& vertices);
    void createCube();
    void createCylinder();
    void createSphere();
    void createGrid();
    void destroy() noexcept;

    GLuint shaderProgram_{0};

    Mesh cube_;
    Mesh cylinder_;
    Mesh sphere_;

    GLuint gridVAO_{0};
    GLuint gridVBO_{0};
    GLsizei gridVertexCount_{0};

    GLint modelLocation_{-1};
    GLint viewLocation_{-1};
    GLint projectionLocation_{-1};
    GLint colorLocation_{-1};
};

} // namespace arduino3d
