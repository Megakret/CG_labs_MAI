//we will be using glsl version 4.5 syntax
#version 450
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec3 out_normal;

layout(std140, set = 0, binding = 0) uniform SceneUniform{
    mat4 projection_view;
    vec3 ambient;
    float ambient_strength;
    vec3 sunlight_direction;
    float a;
    vec3 view_position;
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
    gl_Position = (scene_uniform.projection_view * figure_uniforms.model) * vec4(in_position, 1.0f);
    out_color= in_color * figure_uniforms.main_color;
    out_normal = mat3(figure_uniforms.model)*in_normal;
}
