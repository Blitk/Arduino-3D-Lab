#include "graphics/Camera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace arduino3d {
namespace {

constexpr float radiansPerDegree = 0.01745329251994329577F;

} // namespace

Camera::Camera() {
    updatePosition();
}

void Camera::updatePosition() {
    const float yaw = yaw_ * radiansPerDegree;
    const float pitch = pitch_ * radiansPerDegree;

    const glm::vec3 direction{
        std::cos(yaw) * std::cos(pitch),
        std::sin(pitch),
        std::sin(yaw) * std::cos(pitch)
    };

    position_ = target_ - direction * distance_;
}

void Camera::processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        if (!dragging_) {
            dragging_ = true;
            firstMouse_ = true;
        }

        if (firstMouse_) {
            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
            firstMouse_ = false;
        } else {
            const float deltaX = static_cast<float>(mouseX - lastMouseX_);
            const float deltaY = static_cast<float>(lastMouseY_ - mouseY);

            constexpr float sensitivity = 0.25F;

            yaw_ += deltaX * sensitivity;
            pitch_ += deltaY * sensitivity;
            pitch_ = std::clamp(pitch_, -85.0F, 85.0F);

            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
        }
    } else {
        dragging_ = false;
        firstMouse_ = true;
    }

    const float movement = 3.0F * deltaTime;

    const float yaw = yaw_ * radiansPerDegree;
    const glm::vec3 forward{
        std::cos(yaw), 0.0F, std::sin(yaw)
    };
    const glm::vec3 right{
        -std::sin(yaw), 0.0F, std::cos(yaw)
    };

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        target_ += forward * movement;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        target_ -= forward * movement;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        target_ += right * movement;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        target_ -= right * movement;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        target_.y += movement;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        target_.y -= movement;
    }

    updatePosition();
}

void Camera::zoom(float scrollOffset) {
    distance_ -= scrollOffset * 0.7F;
    distance_ = std::clamp(distance_, 2.0F, 30.0F);
    updatePosition();
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(
        position_,
        target_,
        glm::vec3{0.0F, 1.0F, 0.0F}
    );
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    const float safeAspect = std::max(aspectRatio, 0.001F);

    return glm::perspective(
        glm::radians(45.0F),
        safeAspect,
        0.1F,
        100.0F
    );
}

const glm::vec3& Camera::target() const noexcept {
    return target_;
}

} // namespace arduino3d
