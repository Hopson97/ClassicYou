#version 460

vec2 vertexCoord[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

vec2 texture_coord[4] = vec2[](
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

uniform mat4 view_matrix;
uniform mat4 orthographic_matrix;
uniform mat4 transform;

void main() 
{
    int i = indices[gl_VertexID];
    gl_Position = orthographic_matrix * view_matrix * transform * vec4(vertexCoord[i], 0.0, 1.0);

   vs_out.texture_coord = texture_coord[i];
}