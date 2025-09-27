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

const vec3 LIGHT_POS = vec3(60, 35, 70);

void generate_line(int index)
{
    gl_Position = matrices.projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = matrices.projection * (gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

void generate_line_to_light(int index)
{
    gl_Position = matrices.projection * gl_in[index].gl_Position;
    EmitVertex();
    gl_Position = matrices.projection * (gl_in[index].gl_Position + normalize(vec4(LIGHT_POS, 0.0) - gl_in[index].gl_Position) * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

void main()
{
    generate_line(0); 
    generate_line(1); 
    generate_line(2); 

    generate_line_to_light(0); 
    generate_line_to_light(1); 
    generate_line_to_light(2); 
}