//we will be using glsl version 4.5 syntax
#version 450
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 outColor;

layout(std140, set = 0, binding = 0) uniform GlobalUniforms {
    mat4 local_coords_matrix;
    mat4 transform_proj_matrix;
    vec3 main_color;
} global_uniforms;
vec3 color_from_coords(vec3 local_coords){
    return abs(normalize(local_coords));
}
void main()
{
    vec4 local_coords = global_uniforms.local_coords_matrix * vec4(in_position, 1.0f);
    gl_Position = global_uniforms.transform_proj_matrix* local_coords;
    outColor = color_from_coords(vec3(local_coords));
}
