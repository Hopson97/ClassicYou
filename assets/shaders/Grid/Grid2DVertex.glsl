#version 450 core

layout(location = 0) in vec2 in_position;
layout(location = 2) in vec4 in_colour;

out VS_OUT {
    vec4 colour;
} vs_out;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform vec3 camera_position;

int tile_size = 32;

void main() 
{
    vec2 position = in_position + vec2(
            floor(camera_position.x / tile_size) * tile_size, 
            floor(camera_position.y / tile_size) * tile_size
       );

    vs_out.colour = in_colour;
    gl_Position = projection_matrix * view_matrix * vec4(position, 0.0, 1.0);

}
