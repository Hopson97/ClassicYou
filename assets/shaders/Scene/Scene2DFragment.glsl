#version 460

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    vec3 texture_coord;
    vec3 world_texture_coord;
    vec4 colour;
} fs_in;

uniform sampler2DArray base_texture;
uniform sampler2DArray world_texture;

uniform bool use_texture;
uniform bool use_world_texture;
uniform bool is_selected;
uniform bool on_floor_below;
uniform bool use_texture_alpha_channel;

uniform float texture_mix;

#define HIGHLIGHT_FACTOR 1.5
#define OBSCURE_FACTOR 0.5

void main() 
{
    out_colour = fs_in.colour;
    if (use_texture) 
    {
        vec4 texture_base = texture(base_texture, fs_in.texture_coord);
        if (use_world_texture) 
        {
            out_colour = mix(
                texture_base, 
                texture(world_texture, fs_in.world_texture_coord) * fs_in.colour, 
                texture_mix
            );
        }
        else 
        {
            out_colour *= texture_base;
        }

        if (!use_texture_alpha_channel) 
        {
            out_colour.a = fs_in.colour.a;
        }
    }

    if (is_selected)
    {
        // When not using a texture, it looks better to just go full red
        // Otherwise, it just highlights the object.
        if (use_texture) 
        {
            out_colour *= vec4(HIGHLIGHT_FACTOR, HIGHLIGHT_FACTOR, HIGHLIGHT_FACTOR, 1);
        }      
        else
        {
            out_colour = vec4(1.0, 0.1, 0.1, 1);
        }
    }
    else if (on_floor_below)
    {
         out_colour *= vec4(OBSCURE_FACTOR, OBSCURE_FACTOR, OBSCURE_FACTOR, 0.8);
    }

    if (out_colour.a < 0.1) 
    {
        discard;
    }
}
