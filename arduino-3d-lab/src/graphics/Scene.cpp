
#include "graphics/Scene.hpp"

#include <cmath>
#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace arduino3d {
namespace {

constexpr float pi = 3.14159265358979323846F;

void addBox(
    std::vector<RenderObject>& objects,
    const glm::vec3& position,
    const glm::vec3& size,
    const glm::vec3& color
) {
    const glm::mat4 model =
        glm::translate(glm::mat4{1.0F}, position) *
        glm::scale(glm::mat4{1.0F}, size);

    objects.push_back({Geometry::Cube, model, color});
}

void addCylinder(
    std::vector<RenderObject>& objects,
    const glm::vec3& position,
    const glm::vec3& size,
    const glm::vec3& color
) {
    const glm::mat4 model =
        glm::translate(glm::mat4{1.0F}, position) *
        glm::scale(glm::mat4{1.0F}, size);

    objects.push_back({Geometry::Cylinder, model, color});
}

void addSphere(
    std::vector<RenderObject>& objects,
    const glm::vec3& position,
    const glm::vec3& size,
    const glm::vec3& color
) {
    const glm::mat4 model =
        glm::translate(glm::mat4{1.0F}, position) *
        glm::scale(glm::mat4{1.0F}, size);

    objects.push_back({Geometry::Sphere, model, color});
}

// O cilindro unitário tem seu eixo longitudinal no eixo Y.
void addWire(
    std::vector<RenderObject>& objects,
    const glm::vec3& start,
    const glm::vec3& end,
    const glm::vec3& color,
    float radius = 0.025F
) {
    const glm::vec3 delta = end - start;
    const float length = glm::length(delta);

    if (length < 0.001F) {
        return;
    }

    const glm::vec3 direction = delta / length;
    const glm::vec3 up{0.0F, 1.0F, 0.0F};

    const float dot = glm::clamp(glm::dot(up, direction), -1.0F, 1.0F);
    const float angle = std::acos(dot);

    glm::mat4 rotation{1.0F};

    if (dot < -0.9999F) {
        rotation = glm::rotate(rotation, pi, glm::vec3{1.0F, 0.0F, 0.0F});
    } else if (dot < 0.9999F) {
        const glm::vec3 axis = glm::normalize(glm::cross(up, direction));
        rotation = glm::rotate(rotation, angle, axis);
    }

    const glm::mat4 model =
        glm::translate(glm::mat4{1.0F}, (start + end) * 0.5F) *
        rotation *
        glm::scale(glm::mat4{1.0F}, glm::vec3{radius, length, radius});

    objects.push_back({Geometry::Cylinder, model, color});
}

void addHeaderPins(
    std::vector<RenderObject>& objects,
    float x,
    float z,
    int count,
    float spacing,
    float y
) {
    for (int i = 0; i < count; ++i) {
        const float px = x + static_cast<float>(i) * spacing;

        addCylinder(
            objects,
            {px, y, z},
            {0.045F, 0.20F, 0.045F},
            {0.80F, 0.68F, 0.35F}
        );
    }
}

} // namespace

Scene::Scene() {
    // Superfície de bancada.
    addBox(objects_, {0.0F, -0.22F, 0.0F},
           {11.0F, 0.30F, 6.5F}, {0.15F, 0.18F, 0.22F});

    // ---------------------------------------------------------
    // Arduino estilizado, à esquerda.
    // ---------------------------------------------------------
    addBox(objects_, {-2.75F, 0.03F, 0.0F},
           {2.8F, 0.16F, 1.85F}, {0.04F, 0.34F, 0.72F});

    // Portas e conectores.
    addBox(objects_, {-4.05F, 0.16F, 0.0F},
           {0.40F, 0.24F, 0.58F}, {0.68F, 0.70F, 0.72F});

    addBox(objects_, {-1.48F, 0.15F, 0.45F},
           {0.28F, 0.22F, 0.42F}, {0.13F, 0.14F, 0.16F});

    addBox(objects_, {-1.48F, 0.15F, -0.48F},
           {0.28F, 0.22F, 0.38F}, {0.13F, 0.14F, 0.16F});

    // Microcontrolador e componentes na placa.
    addBox(objects_, {-2.65F, 0.15F, 0.0F},
           {0.72F, 0.12F, 0.62F}, {0.10F, 0.11F, 0.13F});

    addBox(objects_, {-3.40F, 0.14F, -0.38F},
           {0.35F, 0.10F, 0.25F}, {0.17F, 0.18F, 0.20F});

    addBox(objects_, {-2.00F, 0.14F, -0.38F},
           {0.30F, 0.10F, 0.22F}, {0.17F, 0.18F, 0.20F});

    // Fileiras de pinos.
    addHeaderPins(objects_, -3.75F, -0.73F, 8, 0.22F, 0.20F);
    addHeaderPins(objects_, -3.75F,  0.73F, 8, 0.22F, 0.20F);

    // ---------------------------------------------------------
    // Protoboard, à direita.
    // ---------------------------------------------------------
    addBox(objects_, {2.15F, 0.02F, 0.0F},
           {3.65F, 0.18F, 2.30F}, {0.86F, 0.87F, 0.84F});

    // Trilhos de alimentação: vermelho e azul.
    addBox(objects_, {2.15F, 0.115F, -0.94F},
           {3.35F, 0.015F, 0.035F}, {0.83F, 0.13F, 0.12F});

    addBox(objects_, {2.15F, 0.115F, -0.84F},
           {3.35F, 0.015F, 0.035F}, {0.13F, 0.30F, 0.78F});

    addBox(objects_, {2.15F, 0.115F, 0.84F},
           {3.35F, 0.015F, 0.035F}, {0.83F, 0.13F, 0.12F});

    addBox(objects_, {2.15F, 0.115F, 0.94F},
           {3.35F, 0.015F, 0.035F}, {0.13F, 0.30F, 0.78F});

    // Furos representados por pequenos cilindros escuros.
    // Há uma separação central típica de uma protoboard.
    for (int row = 0; row < 10; ++row) {
        const float z = -0.65F + static_cast<float>(row) * 0.145F;

        for (int column = 0; column < 24; ++column) {
            const float x = 0.62F + static_cast<float>(column) * 0.133F;

            if (column >= 10 && column <= 12) {
                continue;
            }

            addCylinder(
                objects_,
                {x, 0.116F, z},
                {0.040F, 0.012F, 0.040F},
                {0.20F, 0.22F, 0.22F}
            );
        }
    }

    // ---------------------------------------------------------
    // LED visual com encapsulamento vermelho e terminais.
    // ---------------------------------------------------------
    const glm::vec3 ledBase{2.05F, 0.28F, 0.12F};

    addCylinder(
        objects_, {ledBase.x, ledBase.y, ledBase.z},
        {0.11F, 0.22F, 0.11F}, {0.72F, 0.73F, 0.70F}
    );

    ledLensIndex_ = objects_.size();

    addSphere(
        objects_, {ledBase.x, 0.43F, ledBase.z},
        {0.19F, 0.20F, 0.19F}, {0.92F, 0.045F, 0.055F}
    );

    addWire(
        objects_, {ledBase.x - 0.045F, 0.18F, ledBase.z},
        {ledBase.x - 0.045F, 0.05F, ledBase.z - 0.15F},
        {0.72F, 0.73F, 0.75F}, 0.018F
    );

    addWire(
        objects_, {ledBase.x + 0.045F, 0.18F, ledBase.z},
        {ledBase.x + 0.045F, 0.05F, ledBase.z + 0.15F},
        {0.72F, 0.73F, 0.75F}, 0.018F
    );

    // ---------------------------------------------------------
    // Fios entre a região dos pinos e a protoboard.
    // Formas geométricas, sem conectividade elétrica.
    // ---------------------------------------------------------
    addWire(objects_, {-1.95F, 0.22F, -0.73F},
            {-0.85F, 0.30F, -1.05F}, {0.85F, 0.12F, 0.10F});

    addWire(objects_, {-1.73F, 0.22F, -0.73F},
            {-0.55F, 0.36F, -1.25F}, {0.12F, 0.25F, 0.85F});

    addWire(objects_, {-1.95F, 0.22F, 0.73F},
            {-0.75F, 0.30F, 1.15F}, {0.88F, 0.72F, 0.12F});

    addWire(objects_, {-1.73F, 0.22F, 0.73F},
            {-0.35F, 0.42F, 1.35F}, {0.15F, 0.72F, 0.30F});

    addWire(objects_, {-0.85F, 0.30F, -1.05F},
            {0.75F, 0.22F, -0.94F}, {0.85F, 0.12F, 0.10F});

    addWire(objects_, {-0.55F, 0.36F, -1.25F},
            {0.75F, 0.22F, -0.84F}, {0.12F, 0.25F, 0.85F});

    addWire(objects_, {-0.75F, 0.30F, 1.15F},
            {0.75F, 0.22F, 0.84F}, {0.88F, 0.72F, 0.12F});

    addWire(objects_, {-0.35F, 0.42F, 1.35F},
            {0.75F, 0.22F, 0.94F}, {0.15F, 0.72F, 0.30F});
}

const std::vector<RenderObject>& Scene::objects() const noexcept {
    return objects_;
}

void Scene::setLedOn(bool on) noexcept {
    if (ledLensIndex_ >= objects_.size()) {
        return;
    }

    objects_[ledLensIndex_].color = on
        ? glm::vec3{1.0F, 0.08F, 0.02F}
        : glm::vec3{0.24F, 0.04F, 0.04F};
}

} // namespace arduino3d
