#version 330 core

in vec3 vNormal;
in vec3 vWorldPosition;

uniform vec3 uColor;

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(vec3(-0.4, 1.0, 0.3));

    float diffuse = max(dot(normal, lightDirection), 0.0);
    float lighting = 0.25 + 0.75 * diffuse;

    vec3 color = uColor * lighting;

    FragColor = vec4(color, 1.0);
}
