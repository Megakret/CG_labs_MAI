//glsl version 4.5
#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_to_camera;
//output write
layout(location = 0) out vec4 outFragColor;

struct Material{
    vec3 albedo;
    float b;
    vec3 specular_strength;
    float shininess;
};
struct SceneLightning{
    vec3 view_position;
    float a;
    vec3 ambient;
    float ambient_strength;
    vec3 sunlight_direction;
    float sunlight_intensity;
    vec3 sunlight_color;
    float b;
};
layout(std140, set = 0, binding = 0) uniform SceneUniform{
    mat4 projection_view;
    SceneLightning lightning;
} scene;
layout(std140, set = 1, binding = 0) uniform FigureUniform{
    mat4 model;
    vec3 main_color;
    float a;
    Material material;
} figure_uniforms;
void main()
{
    vec3 ambient = scene.lightning.ambient * scene.lightning.ambient_strength;
    vec3 N = normalize(in_normal);
    vec3 to_light = normalize(-scene.lightning.sunlight_direction);
    vec3 H = normalize(to_light+normalize(in_to_camera));
    float diff = max(dot(N, to_light), 0);
    vec3 sunlight_color = scene.lightning.sunlight_color * scene.lightning.sunlight_intensity;
    vec3 diffuse = sunlight_color * (diff * figure_uniforms.material.albedo);
    vec3 specular = sunlight_color * figure_uniforms.material.specular_strength * pow(max(dot(N,H), 0), max(figure_uniforms.material.shininess, 0.1f));
    outFragColor = vec4(in_color * (ambient + diffuse + specular), 1.0f);
}
