//we will be using glsl version 4.5 syntax
#version 450
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 outColor;

layout(std140, set = 0, binding = 0) uniform SceneUniform{
    mat4 projection_view;
    vec3 ambient;
    float ambient_strength;
} scene_uniform;
layout(std140, set = 1, binding = 0) uniform FigureUniform{
    mat4 model;
    vec3 main_color;
} figure_uniforms;
void main()
{
    gl_Position = (scene_uniform.projection_view * figure_uniforms.model) * vec4(in_position, 1.0f);
    outColor = in_color * figure_uniforms.main_color;
}
