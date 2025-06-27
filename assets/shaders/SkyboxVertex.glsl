#version 460

layout(location = 0) in vec3 in_position;

out VS_OUT 
{
    vec3 texture_coord;
} vs_out;

// Example of a read-only SSBO
layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;

void main() 
{
    // Ensure the Skybox's position never moves
    mat4 view_matrix_no_translation = matrices.view;
    view_matrix_no_translation[3][0] = 0;
    view_matrix_no_translation[3][1] = 0;
    view_matrix_no_translation[3][2] = 0;

    gl_Position = matrices.projection * view_matrix_no_translation * vec4(in_position, 1.0);

    vs_out.texture_coord = in_position;
}