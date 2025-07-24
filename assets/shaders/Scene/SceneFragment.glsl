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

const vec3 LIGHT_POS = vec3(60, 35, 70);

void main() 
{
    vec3 light_direction = normalize(LIGHT_POS - fs_in.fragment_coord);
    vec3 norm = normalize(fs_in.normal);
    float diffuse_light  = max(dot(fs_in.normal, light_direction), 0.25);

    out_colour = fs_in.colour;
    if (use_texture) 
    {
        out_colour *= texture(diffuse, fs_in.texture_coord) * fs_in.colour;

        if (selected)
        {
           out_colour += vec4(0.25, 0.25, 0.25, 0.1);
        }
        out_colour.rgb *= diffuse_light;
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
