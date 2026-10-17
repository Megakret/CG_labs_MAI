//glsl version 4.5
#version 450

layout(location = 0) in vec3 inColor;
//output write
layout(location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 0) uniform SceneUniform{
    mat4 projection_view;
} scene_uniform;
layout(std140, set = 1, binding = 0) uniform FigureUniform{
    mat4 model;
    vec3 main_color;
} figure_uniforms;
void main()
{
    outFragColor = vec4(inColor, 1.0f);
}
