
#version 450 core
out vec4 out_colour;

uniform bool selected;

void main()
{
    if (selected) 
    {
        out_colour = vec4(0.0, 1.0, 1.0, 1.0);
    }
    else 
    {
        out_colour = vec4(1.0, 1.0, 0.0, 1.0);
    }
}