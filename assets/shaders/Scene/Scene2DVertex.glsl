#version 450 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_texture_coord;
layout(location = 2) in vec3 in_world_texture_coord;
layout(location = 3) in vec4 in_colour;

out VS_OUT {
    vec3 texture_coord;
    vec3 world_texture_coord;
    vec4 colour;
} vs_out;

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;

void main() 
{
    vs_out.texture_coord = in_texture_coord;
    vs_out.world_texture_coord = in_world_texture_coord;
    vs_out.colour = in_colour;
    gl_Position = projection_matrix * view_matrix * model_matrix * vec4(in_position, 0.0, 1.0);
}
