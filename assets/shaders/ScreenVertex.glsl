#version 460

vec2 vertex_positions[4] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2(-1.0,  1.0),
    vec2( 1.0,  1.0)
);

vec2 texture_coords[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

int indices[6] = int[](
    0, 1, 2, 2, 1, 3
);

out VS_OUT {
    vec2 texture_coord;
} vs_out;

void main() 
{
    int i = indices[gl_VertexID];
    gl_Position = vec4(vertex_positions[i], 0.0, 1.0);
    
    vs_out.texture_coord = texture_coords[i];
}