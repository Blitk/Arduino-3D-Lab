#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace arduino3d {

struct RenderObject {
    glm::mat4 model{1.0F};
    glm::vec3 color{1.0F};
};

class Scene {
public:
    Scene();

    const std::vector<RenderObject>& objects() const noexcept;

private:
    std::vector<RenderObject> objects_;
};

} // namespace arduino3d
