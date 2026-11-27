#version 330 core

#include "lighting.glsl"

in vec3 worldPos;
in vec3 worldNormal;
in vec2 uvCoord;

out vec4 fragColor;

uniform vec3 ambientLight;
uniform vec3 shadeColor;

uniform vec3 cDiffuse;
uniform vec3 cEmissive;

uniform sampler2D textureMap;
uniform float textureBlend;
uniform vec2 textureRepeat;

uniform sampler2D emissiveMap;

vec3 diffuseColor() {
    vec3 texel = texture(textureMap, uvCoord * textureRepeat).rgb;
    return mix(cDiffuse, texel, textureBlend);
}

vec3 emissiveTerm() {
    return cEmissive * texture(emissiveMap, uvCoord).rgb;
}

void main() {
    vec3 N = normalize(worldNormal);
    vec3 diffuse = diffuseColor();
    vec3 color = ambientLight * diffuse + emissiveTerm();
    for (int i = 0; i < lightCount; i++) {
        Light light = lights[i];
        vec3 L = directionToLight(light, worldPos);
        float lit = smoothstep(0.0, 0.1, dot(N, L));
        color += diffuse * intensityAt(light, L, worldPos) * mix(shadeColor, vec3(1.0), lit);
    }

    fragColor = vec4(pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
}
