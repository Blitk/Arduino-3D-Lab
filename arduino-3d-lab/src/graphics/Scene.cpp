#include "graphics/Scene.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace arduino3d {

Scene::Scene() {
    // Plataforma central.
    objects_.push_back({
        glm::translate(
            glm::mat4{1.0F},
            glm::vec3{0.0F, -0.15F, 0.0F}
        ) * glm::scale(
            glm::mat4{1.0F},
            glm::vec3{4.0F, 0.3F, 4.0F}
        ),
        glm::vec3{0.16F, 0.22F, 0.30F}
    });

    // Cubo principal.
    objects_.push_back({
        glm::translate(
            glm::mat4{1.0F},
            glm::vec3{0.0F, 0.65F, 0.0F}
        ),
        glm::vec3{0.10F, 0.72F, 0.86F}
    });

    // Objetos auxiliares.
    objects_.push_back({
        glm::translate(
            glm::mat4{1.0F},
            glm::vec3{-1.5F, 0.35F, -1.0F}
        ) * glm::scale(
            glm::mat4{1.0F},
            glm::vec3{0.7F}
        ),
        glm::vec3{0.95F, 0.42F, 0.20F}
    });

    objects_.push_back({
        glm::translate(
            glm::mat4{1.0F},
            glm::vec3{1.4F, 0.45F, 1.0F}
        ) * glm::scale(
            glm::mat4{1.0F},
            glm::vec3{0.9F}
        ),
        glm::vec3{0.32F, 0.82F, 0.48F}
    });
}

const std::vector<RenderObject>& Scene::objects() const noexcept {
    return objects_;
}

} // namespace arduino3d
