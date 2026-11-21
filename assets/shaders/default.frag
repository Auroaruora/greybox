#version 330 core

const int MAX_LIGHTS = 8;
const int LIGHT_POINT = 0;
const int LIGHT_DIRECTIONAL = 1;
const int LIGHT_SPOT = 2;

struct Light {
    int type;
    vec3 color;
    vec3 pos;
    vec3 dir;
    vec3 function;
    float angle;
    float penumbra;
};

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

uniform int lightCount;
uniform Light lights[MAX_LIGHTS];

float attenuation(Light light, float distance) {
    float denom = light.function.x
                + light.function.y * distance
                + light.function.z * distance * distance;
    if (denom <= 0.0) {
        return 1.0;
    }
    return 1.0 / max(denom, 0.0001);
}

float spotFalloff(Light light, vec3 lightToPoint) {
    float outer = light.angle;
    float inner = light.angle - light.penumbra;
    float x = acos(clamp(dot(lightToPoint, normalize(light.dir)), -1.0, 1.0));

    if (x <= inner) {
        return 1.0;
    }
    if (x >= outer) {
        return 0.0;
    }
    float t = (x - inner) / (outer - inner);
    return 1.0 - (-2.0 * t * t * t + 3.0 * t * t);
}

vec3 diffuseColor() {
    vec3 texel = texture(textureMap, uvCoord * textureRepeat).rgb;
    return mix(cDiffuse, texel, textureBlend);
}

vec3 emissiveTerm() {
    return cEmissive * pow(texture(emissiveMap, uvCoord).rgb, vec3(2.2));
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

vec3 directionToLight(Light light) {
    if (light.type == LIGHT_DIRECTIONAL) {
        return -normalize(light.dir);
    }
    return normalize(light.pos - worldPos);
}

vec3 intensityAt(Light light, vec3 L) {
    float distance = length(light.pos - worldPos);

    switch (light.type) {
    case LIGHT_DIRECTIONAL:
        return light.color;
    case LIGHT_POINT:
        return light.color * attenuation(light, distance);
    case LIGHT_SPOT:
        return light.color * attenuation(light, distance) * spotFalloff(light, -L);
    }
    return vec3(0.0);
}

void main() {
    vec3 N = normalize(worldNormal);
    vec3 E = normalize(cameraPos - worldPos);

    vec3 diffuse = diffuseColor();
    vec3 color = ambientTerm(diffuse) + emissiveTerm();
    for (int i = 0; i < lightCount; i++) {
        Light light = lights[i];
        vec3 L = directionToLight(light);
        vec3 intensity = intensityAt(light, L);

        color += intensity * (diffuseTerm(N, L, diffuse) + specularTerm(N, L, E));
    }

    fragColor = vec4(pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
}