uniform sampler2DShadow shadow_map;
layout (binding = 4) uniform samplerCubeShadow shadow_map_point_light;

float calculate_shadows(vec4 lightspace_fragment_coord)
{
    // NDC ranged from [-1.0, 1.0]
    vec3 projection_coords = lightspace_fragment_coord.xyz / lightspace_fragment_coord.w;

    // Convert to [0.0, 1.0]
    projection_coords = projection_coords * 0.5 + 0.5;
    if (projection_coords.x < 0 || projection_coords.y < 0 || projection_coords.x >= 1024 || projection_coords.x >= 1024) 
    {
        return 1;
    }
    if (projection_coords.z > 1.0)
    {
        return 0;
    }

    float x_offset = 1.0 / 1920.0;
    float y_offset = 1.0 / 1080.0;

    float shadow = 0.0;

    for (int y = -1 ; y <= 1 ; y++) {
        for (int x = -1 ; x <= 1 ; x++) {
            vec3 uv = vec3(projection_coords.xy + vec2(x * x_offset, y * y_offset), projection_coords.z - 0.001);
            shadow += texture(shadow_map, uv);
        }
    }

    return (0.5 + (shadow / 18.0));
}


float calculate_shadows_cubemap(vec3 frag_position, PointLight pointlight)
{
    // get vector between fragment position and light position
    vec3 frag_to_light = frag_position - pointlight.position.xyz;

    // use the light to fragment vector to sample from the depth map    
    return texture(shadow_map_point_light, vec4(frag_to_light, 1.0));
}