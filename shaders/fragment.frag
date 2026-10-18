//glsl version 4.5
#version 450

layout(location = 0) in vec3 inColor;
//output write
layout(location = 0) out vec4 outFragColor;

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
    vec3 ambient = scene_uniform.ambient * scene_uniform.ambient_strength;
    outFragColor = vec4(inColor * ambient, 1.0f);
}
