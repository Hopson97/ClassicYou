#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 3) in vec4 in_colour;

out VS_OUT {
    vec4 colour;
} vs_out;

// Example of a read-only SSBO
layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;

uniform vec3 camera_position;
uniform int floor_number;
uniform float floor_height;


void main() 
{
    vec3 position = in_position + vec3(
            floor(camera_position.x), 
            floor_number * floor_height - 0.01, 
            floor(camera_position.z)
       );

    vs_out.colour = in_colour;
    gl_Position = matrices.projection * matrices.view * vec4(position, 1.0);
}