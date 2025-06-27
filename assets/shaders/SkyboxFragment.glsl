#version 330

layout(location = 0) out vec4 out_colour;

in VS_OUT {
    vec3 texture_coord;
} fs_in;

uniform samplerCube cube_sampler;

void main() {
    out_colour = texture(cube_sampler, fs_in.texture_coord);
}