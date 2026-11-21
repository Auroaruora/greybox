#version 330 core

layout(location = 0) in vec3 objectPos;
layout(location = 1) in vec3 objectNormal;
layout(location = 2) in vec2 uv;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 uvCoord;

uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main() {
    worldPos = vec3(modelMatrix * vec4(objectPos, 1.0));
    worldNormal = normalMatrix * objectNormal;
    uvCoord = uv;
    gl_Position = projMatrix * viewMatrix * vec4(worldPos, 1.0);
}