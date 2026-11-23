#version 330 core

in vec2 uvCoord;

uniform sampler2D sourceTexture;

out vec4 fragColor;

void main() {
    fragColor = texture(sourceTexture, uvCoord);
}
