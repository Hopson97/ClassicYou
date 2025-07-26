#version 460

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    vec4 colour;
} fs_in;


void main() 
{

    out_colour = fs_in.colour;
}
