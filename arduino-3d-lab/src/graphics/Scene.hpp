
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace arduino3d {

enum class Geometry {
    Cube,
    Cylinder,
    Sphere
};

struct RenderObject {
    Geometry geometry{Geometry::Cube};
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
