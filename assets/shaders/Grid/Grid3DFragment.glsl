#version 330

// https://github.com/emeiri/ogldev/blob/master/DemoLITION/Framework/Shaders/GL/infinite_grid.fs

layout(location = 0) out vec4 out_colour;

uniform samplerCube cube_sampler;

in VS_OUT {
    vec3 world_position;
} fs_in;


float min_pixels_between_cells = 2.0;
float grid_size = 1.0;

float log10(float x)
{
    float f = log(x) / log(10.0);
    return f;
}

float max2(vec2 v)
{
    float f = max(v.x, v.y);
    return f;
}

vec2 satv(vec2 x)
{
    vec2 v = clamp(x, vec2(0.0), vec2(1.0));
    return v;
}

float satf(float x)
{
    float f = clamp(x, 0.0, 1.0);
    return f;
}

#define WHITE vec4(1,1, 1, 1)

void main() 
{

    float length_x = length(vec2(dFdx(fs_in.world_position.x), dFdy(fs_in.world_position.x)));
    float length_y = length(vec2(dFdx(fs_in.world_position.z), dFdy(fs_in.world_position.z)));
    vec2 dudv = vec2(length_x, length_y);


    float lod = max(0.0, log10(length(dudv) * min_pixels_between_cells / grid_size) + 1.0);
    float grid_size_lod0 = grid_size * pow(10.0, floor(lod));
    float grid_size_lod1 = grid_size_lod0 * 10.0;
    float grid_size_lod2 = grid_size_lod1 * 10.0;
    float grid_size_half = grid_size_lod0 * 0.5;

    dudv *= 4.0;

    vec2 mod_half = mod(fs_in.world_position.xz, grid_size_half) / dudv;
    float lodHalf = max2(vec2(1.0) - abs(satv(mod_half) * 2.0 - vec2(1.0)));

    vec2 mod_div_dudv = mod(fs_in.world_position.xz, grid_size_lod0) / dudv;
    float lod0a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    mod_div_dudv = mod(fs_in.world_position.xz, grid_size_lod1) / dudv;
    float lod1a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    mod_div_dudv = mod(fs_in.world_position.xz, grid_size_lod2) / dudv;
    float lod2a = max2(vec2(1.0) - abs(satv(mod_div_dudv) * 2.0 - vec2(1.0)));

    float lod_fade = fract(lod);
    out_colour = WHITE;

    if (lod2a > 0.0) {
        out_colour.a = lod2a;
    } else if (lod1a > 0.0) {
        out_colour.a = lod1a;
    } else if (lod0a > 0.0) {
        out_colour.a = lod0a * (1.0 - lod_fade);
    } else if (lodHalf > 0.0) {
        out_colour.a = (lodHalf * 0.25) * (1.0 - lod_fade); 
    } else {
        out_colour.a = 0.0;
    }

    if (out_colour.a < 0.01)
    {
        discard;
    }
}