/*
    For fragment shaders only.  This assumes the following:

    The following is defined before the file is included:

    in VS_OUT {
        vec3 fragment_coord;
    } fs_in;
*/

struct LightBase 
{
    vec4 colour;
    float ambient_intensity;
    float diffuse_intensity;
    float specular_intensity;
    float PADDING_;
};

struct Attenuation 
{
    float constant;
    float linear;
    float exponent;
    float PADDING_;
};

struct DirectionalLight 
{
    LightBase base;
    vec4 direction;
};

struct PointLight 
{
    LightBase base;
    Attenuation att;
    vec4 position;
};

struct SpotLight 
{
    LightBase base;
    Attenuation att;
    vec4 direction;
    vec4 position;

    float cutoff;
    float PADDING0_;
    float PADDING1_;
    float PADDING2_;
};

float calculate_attenuation(Attenuation attenuation, vec3 light_position) 
{
    float distance = length(light_position - fs_in.fragment_coord);
    return 1.0 / (attenuation.constant +
        attenuation.linear * distance +
        attenuation.exponent * (distance * distance));
}

vec3 calculate_base_lighting(LightBase light, vec3 normal, vec3 light_direction, vec3 eye_direction, float shadow) 
{

    // Diffuse lighting
    float diffuse_factor = max(dot(normal, light_direction), 0.0);
    vec3 diffuse_colour = light.colour.rgb * light.diffuse_intensity * diffuse_factor;

    // Specular lighting
    vec3 reflect_direction = reflect(-light_direction, normal);
    float specular_factor = pow(max(dot(eye_direction, reflect_direction), 0.0), material.shininess);
    vec3 specular_colour = light.specular_intensity * specular_factor * vec3(texture(material.specular, fs_in.texture_coord));

    // Ambient
    vec3 ambient_colour = light.colour.rgb * light.ambient_intensity;

    return ambient_colour + shadow * (diffuse_colour + specular_colour);
}

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 eye_direction, float shadow) 
{
    vec3 light_result = calculate_base_lighting(light.base, normalize(light.position.xyz - fs_in.fragment_coord), normal, eye_direction, shadow);
    float attenuation = calculate_attenuation(light.att, light.position.xyz);

    return light_result * attenuation;
}

vec3 calculate_spot_light(SpotLight light, vec3 normal, vec3 eye_direction) 
{
    // Get the direction from this light to the eye
    vec3 light_direction = normalize(light.position.xyz - fs_in.fragment_coord);
    vec3 light_result = calculate_base_lighting(light.base, light_direction, normal, eye_direction, 1);

    // Get the attenuation value
    float attenuation = calculate_attenuation(light.att, light.position.xyz);
    float cutoff = cos(radians(light.cutoff));

    // Smooth edges, creates the flashlight effect such that only centre pixels are lit
    float oco = cos(acos(cutoff) + radians(6));
    float theta = dot(light_direction, -light.direction.xyz);
    float epsilon = cutoff - oco;
    float intensity = clamp((theta - oco) / epsilon, 0.0, 1.0);

    // Apply the attenuation and the flashlight effect. Note the flashlight also effects
    // this light source's ambient light, so this will only allow light inside the "light cone" 
    // - this may need to be changed
    return light_result * intensity * attenuation;

}

vec3 calculate_directional_light(DirectionalLight light, vec3 normal, vec3 eye_direction, float shadow) 
{
    return calculate_base_lighting(light.base, normalize(-light.direction).xyz, normal, eye_direction, shadow);
}