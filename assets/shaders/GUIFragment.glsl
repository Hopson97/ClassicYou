#version 460

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    vec2 texture_coord;
} fs_in;

uniform sampler2D gui_texture;

void main() 
{ 
    out_colour = texture(gui_texture, fs_in.texture_coord);
}