//we will be using glsl version 4.5 syntax
#version 450
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_to_camera;
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
    vec4 world_position = figure_uniforms.model * vec4(in_position, 1.0f);
    gl_Position = scene.projection_view * world_position;
    out_color= in_color * figure_uniforms.main_color;
    out_normal = vec3(figure_uniforms.model * vec4(in_normal, 0.0f));
    out_to_camera = vec3(scene.lightning.view_position) - vec3(world_position);
}
