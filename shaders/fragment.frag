//glsl version 4.5
#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_world_position;
//output write
layout(location = 0) out vec4 outFragColor;

struct Material {
    vec3 albedo;
    float b;
    vec3 specular_strength;
    float shininess;
};
struct PointLight {
    vec3 light_color;
    float intensity;
    vec3 position;
    float linear_term;
    float quadratic_term;
};
struct Spotlight {
    vec3 light_color;
    float intensity;
    vec3 position;
    float inner_angle;
    vec3 spot_direction;
    float outer_angle;
    float linear_term;
    float quadratic_term;
    vec2 padding;
};
struct SceneLightning {
    Spotlight flashlight;
    vec3 view_position;
    float a;
    vec3 ambient;
    float ambient_strength;
    vec3 sunlight_direction;
    float sunlight_intensity;
    vec3 sunlight_color;
    int point_light_count;
    vec3 padding;
    int spotlight_count;
};
layout(std140, set = 0, binding = 0) uniform SceneUniform {
    mat4 projection_view;
    SceneLightning lightning;
} scene;
layout(std430, set = 0, binding = 1) readonly buffer PointLightBuffer {
    PointLight[] lights;
} point_lights;
layout(std430, set = 0, binding = 2) readonly buffer SpotlightBuffer {
    Spotlight[] lights;
} spotlights;
layout(std140, set = 1, binding = 0) uniform FigureUniform {
    mat4 model;
    vec3 main_color;
    float a;
    Material material;
} figure_uniforms;
vec3 CalculateLight(vec3 N, vec3 to_camera, vec3 to_light) {
    vec3 H = normalize(to_light + to_camera);
    float diff = max(dot(N, to_light), 0);
    vec3 diffuse = (diff * figure_uniforms.material.albedo);
    vec3 specular = figure_uniforms.material.specular_strength * pow(max(dot(N, H), 0), max(figure_uniforms.material.shininess, 0.1f));
    return diffuse + specular;
}
vec3 CalculateSunlight(vec3 N, vec3 to_camera) {
    vec3 to_light = normalize(-scene.lightning.sunlight_direction);
    return scene.lightning.sunlight_intensity * scene.lightning.sunlight_color * CalculateLight(N, to_camera, to_light);
}
vec3 CalculatePointLight(vec3 N, vec3 to_camera, PointLight light) {
    vec3 to_light = normalize(light.position - in_world_position);
    float distance = length(light.position - in_world_position);
    vec3 phong = CalculateLight(N, to_camera, to_light);
    float attenuation = 1 / (1 + light.linear_term * distance + light.quadratic_term * distance * distance);
    return attenuation * light.intensity * light.light_color * phong;
}
vec3 CalculateSpotlight(vec3 N, vec3 to_camera, Spotlight light) {
    vec3 to_light = normalize(light.position - in_world_position);
    float cos_angle = dot(-to_light, normalize(light.spot_direction));
    if (cos_angle < light.outer_angle) {
        return vec3(0, 0, 0);
    }
    float distance = length(light.position - in_world_position);
    float attenuation = 1 / (1 + light.linear_term * distance + light.quadratic_term * distance * distance);
    vec3 phong = CalculateLight(N, to_camera, to_light);
    float smooth_coef = clamp((cos_angle - light.outer_angle) / (light.inner_angle - light.outer_angle), 0.0, 1.0);
    return light.intensity * attenuation * smooth_coef * light.light_color * phong;
}
void main()
{
    vec3 ambient = scene.lightning.ambient * scene.lightning.ambient_strength;
    vec3 output_light = ambient;
    vec3 N = normalize(in_normal);
    vec3 to_camera = normalize(scene.lightning.view_position - in_world_position);
    output_light += CalculateSunlight(N, to_camera);
    for (uint i = 0u; i < scene.lightning.point_light_count; ++i) {
        PointLight light = point_lights.lights[i];
        output_light += CalculatePointLight(N, to_camera, light);
    }
    for (uint i = 0u; i < scene.lightning.spotlight_count; ++i) {
        Spotlight light = spotlights.lights[i];
        output_light += CalculateSpotlight(N, to_camera, light);
    }
    output_light += CalculateSpotlight(N, to_camera, scene.lightning.flashlight);
    outFragColor = vec4(in_color * output_light, 1.0f);
}
