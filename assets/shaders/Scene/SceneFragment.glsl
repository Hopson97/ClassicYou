#version 460

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    vec2 texture_coord;
    vec3 normal;
    vec3 fragment_coord;
} fs_in;

uniform sampler2D diffuse;

void main() 
{
    out_colour = texture(diffuse, fs_in.texture_coord);
}
