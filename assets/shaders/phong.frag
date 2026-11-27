#version 330 core

#include "lighting.glsl"

in vec3 worldPos;
in vec3 worldNormal;
in vec2 uvCoord;

out vec4 fragColor;

uniform float ka;
uniform float kd;
uniform float ks;

uniform vec3 ambientLight;

uniform vec3 cDiffuse;
uniform vec3 cSpecular;
uniform vec3 cEmissive;
uniform float shininess;

uniform sampler2D textureMap;
uniform float textureBlend;
uniform vec2 textureRepeat;

uniform sampler2D emissiveMap;

uniform vec3 cameraPos;

vec3 diffuseColor() {
    vec3 texel = texture(textureMap, uvCoord * textureRepeat).rgb;
    return mix(cDiffuse, texel, textureBlend);
}

vec3 emissiveTerm() {
    return cEmissive * texture(emissiveMap, uvCoord).rgb;
}

vec3 ambientTerm(vec3 diffuse) {
    return ka * ambientLight * diffuse;
}

vec3 diffuseTerm(vec3 N, vec3 L, vec3 diffuse) {
    float facing = clamp(dot(N, L), 0.0, 1.0);
    return kd * diffuse * facing;
}

vec3 specularTerm(vec3 N, vec3 L, vec3 E) {
    if (shininess <= 0.0) {
        return vec3(0.0);
    }
    vec3 R = reflect(-L, N);
    float highlight = pow(clamp(dot(R, E), 0.0, 1.0), shininess);
    return ks * cSpecular * highlight;
}

void main() {
    vec3 N = normalize(worldNormal);
    vec3 E = normalize(cameraPos - worldPos);

    vec3 diffuse = diffuseColor();
    vec3 color = ambientTerm(diffuse) + emissiveTerm();
    for (int i = 0; i < lightCount; i++) {
        Light light = lights[i];
        vec3 L = directionToLight(light, worldPos);
        vec3 intensity = intensityAt(light, L, worldPos);

        color += intensity * (diffuseTerm(N, L, diffuse) + specularTerm(N, L, E));
    }

    fragColor = vec4(pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
}
