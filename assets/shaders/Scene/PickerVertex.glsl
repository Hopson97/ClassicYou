#version 450 core

layout(location = 0) in vec3 in_position;

layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;

void main() 
{
    gl_Position = matrices.projection * matrices.view * vec4(in_position, 1.0);
}
