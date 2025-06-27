#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texture_coord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in mat4 in_model_matrix;

out VS_OUT {
    vec2 texture_coord;
    vec3 normal;
    vec3 fragment_coord;
    vec4 lightspace_fragment_coord;
} vs_out;

// Example of a read-only SSBO
layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;

uniform mat4 light_space_matrix;

void main() 
{
    vs_out.fragment_coord = vec3(in_model_matrix * vec4(in_position, 1.0));
    vs_out.normal = mat3(transpose(inverse(in_model_matrix))) * in_normal;
    vs_out.texture_coord = in_texture_coord;
    vs_out.lightspace_fragment_coord = light_space_matrix * vec4(vs_out.fragment_coord, 1.0);

    gl_Position = matrices.projection * matrices.view * vec4(vs_out.fragment_coord, 1.0);

}
