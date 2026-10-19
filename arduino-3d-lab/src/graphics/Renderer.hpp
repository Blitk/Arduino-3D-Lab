#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>

namespace arduino3d {

class Camera;
class Scene;

class Renderer {
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void render(const Scene& scene, const Camera& camera,
                int width, int height);

private:
    GLuint createShaderProgram(
        const std::string& vertexPath,
        const std::string& fragmentPath
    );

    void createCube();
    void createGrid();
    void destroy() noexcept;

    GLuint shaderProgram_{0};

    GLuint cubeVAO_{0};
    GLuint cubeVBO_{0};

    GLuint gridVAO_{0};
    GLuint gridVBO_{0};

    GLsizei gridVertexCount_{0};

    GLint modelLocation_{-1};
    GLint viewLocation_{-1};
    GLint projectionLocation_{-1};
    GLint colorLocation_{-1};
};

} // namespace arduino3d
