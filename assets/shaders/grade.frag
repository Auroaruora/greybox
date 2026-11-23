#version 330 core

in vec2 uvCoord;

uniform sampler2D sourceTexture;
uniform sampler3D lut;
uniform float lutSize;

out vec4 fragColor;

void main() {
    vec4 color = texture(sourceTexture, uvCoord);
    float scale = (lutSize - 1.0) / lutSize;
    float offset = 1.0 / (2.0 * lutSize);
    vec3 coord = scale * clamp(color.rgb, 0.0, 1.0) + offset;
    fragColor = vec4(texture(lut, coord).rgb, color.a);
}
