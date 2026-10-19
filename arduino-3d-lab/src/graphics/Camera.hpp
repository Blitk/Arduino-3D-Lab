#pragma once

#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace arduino3d {

class Camera {
public:
    Camera();

    void processInput(GLFWwindow* window, float deltaTime);
    void zoom(float scrollOffset);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

    const glm::vec3& target() const noexcept;

private:
    void updatePosition();

    glm::vec3 target_{0.0F, 0.5F, 0.0F};
    glm::vec3 position_{0.0F, 3.0F, 8.0F};

    float yaw_{-90.0F};
    float pitch_{-18.0F};
    float distance_{8.0F};

    double lastMouseX_{0.0};
    double lastMouseY_{0.0};

    bool dragging_{false};
    bool firstMouse_{true};
};

} // namespace arduino3d
