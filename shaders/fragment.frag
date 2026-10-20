//glsl version 4.5
#version 450

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_to_camera;
//output write
layout(location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 0) uniform SceneUniform{
    mat4 projection_view;
    vec3 ambient;
    float ambient_strength;
    vec3 sunlight_direction;
    float a;
} scene_uniform;
layout(std140, set = 1, binding = 0) uniform FigureUniform{
    mat4 model;
    vec3 main_color;
    float a;
    vec3 albedo;
    float b;
    vec3 specular_strength;
    float shininess;
} figure_uniforms;
void main()
{
    vec3 ambient = scene_uniform.ambient * scene_uniform.ambient_strength;
    vec3 N = normalize(in_normal);
    vec3 to_light = normalize(-scene_uniform.sunlight_direction);
    vec3 H = normalize(to_light+normalize(in_to_camera));
    float diff = max(dot(N, to_light), 0);
    vec3 diffuse = diff * figure_uniforms.albedo;
    vec3 specular = figure_uniforms.specular_strength * pow(max(dot(N,H), 0), max(figure_uniforms.shininess, 0.1f));
    outFragColor = vec4(in_color * (ambient + diffuse + specular), 1.0f);
}
