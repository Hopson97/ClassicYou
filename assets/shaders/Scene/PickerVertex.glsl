#version 450 core

layout(location = 0) in vec3 in_position;

layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;

uniform mat4 model_matrix;

void main() 
{
    gl_Position = matrices.projection * matrices.view * model_matrix * vec4(in_position, 1.0);
}
