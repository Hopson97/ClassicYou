#version 460

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    TEX_COORD_LENGTH texture_coord;
    vec3 normal;
    vec3 fragment_coord;
    vec4 colour;
} fs_in;

uniform SAMPLER_TYPE diffuse;
uniform bool use_texture;

uniform bool selected;

void main() 
{
    out_colour = fs_in.colour;
    if (use_texture) 
    {
        out_colour *= texture(diffuse, fs_in.texture_coord);

        if (selected)
        {
           out_colour += vec4(0.25, 0.25, 0.25, 0.1);
        }
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
