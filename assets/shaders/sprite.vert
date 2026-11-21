#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

uniform vec4 rect;

out vec2 uvCoord;

void main() {
    uvCoord = uv;
    gl_Position = vec4(rect.xy + position.xy * rect.zw, 0.0, 1.0);
}
