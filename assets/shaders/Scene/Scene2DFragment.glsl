#version 460

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    vec2 texture_coord;
    vec4 colour;
} fs_in;

uniform sampler2D diffuse;
uniform bool use_texture;

void main() 
{
    if (use_texture) 
    {
        out_colour = texture(diffuse, fs_in.texture_coord);
    }
    else 
    {
        out_colour = fs_in.colour;
    }

    if (out_colour.a < 0.1) 
    {
        discard;
    }
}
