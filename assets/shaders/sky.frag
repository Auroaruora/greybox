#version 330 core

in vec2 uvCoord;

out vec4 fragColor;

uniform mat4 inverseViewProjection;
uniform vec3 cameraPos;
uniform vec3 zenith;
uniform vec3 horizon;

void main() {
    vec4 far = inverseViewProjection * vec4(uvCoord * 2.0 - 1.0, 1.0, 1.0);
    vec3 direction = normalize(far.xyz / far.w - cameraPos);
    vec3 sky = mix(horizon, zenith, sqrt(max(direction.y, 0.0)));

    fragColor = vec4(pow(clamp(sky, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
}
