#version 330 core

in vec3 vNormal;
in vec3 vWorldPosition;

uniform vec3 uColor;

out vec4 FragColor;

void main() {
    float normalLength = length(vNormal);
    float lighting = 0.65;

    if (normalLength > 0.001) {
        vec3 normal = vNormal / normalLength;
        vec3 lightDirection = normalize(vec3(-0.4, 1.0, 0.3));

        float diffuse = max(dot(normal, lightDirection), 0.0);
        lighting = 0.25 + 0.75 * diffuse;
    }

    FragColor = vec4(uColor * lighting, 1.0);
}
