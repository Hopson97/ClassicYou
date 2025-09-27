#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec3 in_normal;

out VS_OUT {
    vec3 normal;
} gs_in;

layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection; // unused but needed for the SSBO
    mat4 view;
} matrices;

uniform mat4 model_matrix;

// Source: https://learnopengl.com/Advanced-OpenGL/Geometry-Shader
void main()
{
    gl_Position = matrices.view * model_matrix * vec4(in_position, 1.0); 
    mat3 normal_matrix = mat3(transpose(inverse(matrices.view * model_matrix)));
    gs_in.normal = normalize(vec3(vec4(normal_matrix * in_normal, 0.0)));
}