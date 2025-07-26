#version 460

vec3 vertex_positions[4] = vec3[](
    vec3(-1.0, 0.0, -1.0),
    vec3( 1.0, 0.0, -1.0),
    vec3(-1.0, 0.0,  1.0),
    vec3( 1.0, 0.0,  1.0)
);

int indices[6] = int[](
    0, 1, 2, 2, 1, 3
);

layout(std430, binding = 0) readonly buffer Matrices 
{
    mat4 projection;
    mat4 view;
} matrices;

out VS_OUT {
    vec3 world_position;
} vs_out;

uniform vec3 camera_position;
uniform int floor_number;
uniform float floor_height;

void main() 
{
    int i = indices[gl_VertexID];

    // Offset the X/Z coords by the camera to prevent grid from moving
    vec3 position = vertex_positions[i] * 128.0;
    position.x += camera_position.x;
    position.z += camera_position.z;
    position.y += floor_number * floor_height - 0.01;


    gl_Position = matrices.projection * matrices.view * vec4(position, 1.0);

    vs_out.world_position = position;
}
