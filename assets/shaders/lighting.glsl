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

vec3 directionToLight(Light light, vec3 position) {
    if (light.type == LIGHT_DIRECTIONAL) {
        return -normalize(light.dir);
    }
    return normalize(light.pos - position);
}

vec3 intensityAt(Light light, vec3 L, vec3 position) {
    float distance = length(light.pos - position);

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
