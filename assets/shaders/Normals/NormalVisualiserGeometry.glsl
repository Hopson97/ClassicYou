#version 450 core

layout (triangles) in;
layout (line_strip, max_vertices = 12) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

  
layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;

const float MAGNITUDE = 0.5;

void generate_line(int index)
{
    gl_Position = matrices.projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = matrices.projection * (gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}


void main()
{
    generate_line(0); 
    generate_line(1); 
    generate_line(2); 
}