#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texture_coord;
layout(location = 2) in vec3 in_normal;

out VS_OUT {
    vec2 texture_coord;
    vec3 normal;
    vec3 fragment_coord;
} vs_out;

// Example of a read-only SSBO
layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;



uniform mat4 model_matrix;

void main() 
{
    vs_out.fragment_coord = vec3(model_matrix * vec4(in_position, 1.0));
    vs_out.normal = mat3(transpose(inverse(model_matrix))) * in_normal;
    vs_out.texture_coord = in_texture_coord;
    gl_Position = matrices.projection * matrices.view * vec4(vs_out.fragment_coord, 1.0);

}
