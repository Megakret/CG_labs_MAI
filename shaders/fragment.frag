//glsl version 4.5
#version 450

layout(location = 0) in vec3 inColor;
//output write
layout(location = 0) out vec4 outFragColor;

layout(std140, set = 0, binding = 0) uniform GlobalUniforms {
    mat4 a;
    mat4 b;
    vec3 main_color;
} global_uniforms;
void main()
{
    outFragColor = vec4(inColor * global_uniforms.main_color, 1.0f);
}
