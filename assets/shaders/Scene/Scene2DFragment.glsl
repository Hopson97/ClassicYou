#version 460

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    vec2 texture_coord;
    vec4 colour;
} fs_in;

uniform sampler2D diffuse;
uniform bool use_texture;

uniform bool selected;
uniform bool below;


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

    if (selected)
    {
         out_colour *= vec4(1.0, 0.1, 0.1, 1);
    }
    else if (below)
    {
         out_colour *= vec4(0.5, 0.5, 0.5, 1);
    }

    if (out_colour.a < 0.1) 
    {
        discard;
    }
}
