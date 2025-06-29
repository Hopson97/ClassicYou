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
