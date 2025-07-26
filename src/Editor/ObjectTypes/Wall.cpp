#include "Wall.h"

bool operator==(const WallProps& lhs, const WallProps& rhs)
{
    return lhs.texture_front == rhs.texture_front && lhs.texture_back == rhs.texture_back &&
           lhs.start_base_height == rhs.start_base_height && lhs.start_height == rhs.start_height &&
           lhs.end_base_height == rhs.end_base_height && lhs.end_height == rhs.end_height &&
           lhs.tri_wall == rhs.tri_wall && lhs.flip_wall == rhs.flip_wall;
}

bool operator!=(const WallProps& lhs, const WallProps& rhs)
{
    return !(lhs == rhs);
}

template <>
LevelObjectsMesh3D object_to_geometry(const WallObject& wall, int floor_number)

{
    return generate_wall_mesh(wall, floor_number);
}

template <>
std::string object_to_string(const WallObject& wall)

{
    auto& params = wall.parameters;
    auto& props = wall.properties;
    return std::format("Props:\n Texture 1/2: {} {}: \n Base: {}\n Height: {}\nParameters:\n "
                       "  Start position: ({:.2f}, {:.2f}) - End Position: ({:.2f}, {:.2f})",
                       props.texture_front.id, props.texture_back.id, props.start_base_height,
                       props.start_height, params.line.start.x, params.line.start.y,
                       params.line.end.x, params.line.end.y, props.start_height,
                       params.line.start.x, params.line.start.y, params.line.end.x,
                       params.line.end.y);
}

template <>
void render_object_2d(const WallObject& wall, DrawingPad& drawing_pad, const glm::vec4& colour,
                      bool is_selected)

{
    auto thickness = is_selected ? 3.0f : 2.0f;

    drawing_pad.render_line(wall.parameters.line.start, wall.parameters.line.end, colour,
                            thickness);
}

template <>
[[nodiscard]] bool object_try_select_2d(const WallObject& wall, glm::vec2 selection_tile)

{
    return distance_to_line(selection_tile, wall.parameters.line) < 15;
}

template <>
SerialiseResponse object_serialise(const WallObject& wall)
{
    nlohmann::json object;

    auto& params = wall.parameters;
    auto& props = wall.properties;

    nlohmann::json json_params = {params.line.start.x, params.line.start.y, params.line.end.x,
                                  params.line.end.y};

    nlohmann::json json_props = {};
    serialise_texture(json_props, props.texture_back);
    serialise_texture(json_props, props.texture_front);
    json_props.insert(json_props.end(),
                      {props.start_height, props.start_base_height, props.end_height,
                       props.end_base_height, props.tri_wall, props.flip_wall});

    object = {json_params, json_props};

    return {object, "wall"};
}

bool object_deserialise(WallObject& wall, const nlohmann::json& json)
{
    auto& params = wall.parameters;
    auto& props = wall.properties;

    auto jparams = json[0];
    auto jprops = json[1];
    if (jparams.size() < 4)
    {
        std::println("Invalid wall parameters, expected 4 values");
        return false;
    }
    if (jprops.size() < 8)
    {
        std::println("Invalid wall properties, expected 8 values");
        return false;
    }
    params.line.start = {jparams[0], jparams[1]};
    params.line.end = {jparams[2], jparams[3]};

    props.texture_back = deserialise_texture(jprops[0]);
    props.texture_front = deserialise_texture(jprops[1]);

    props.start_height = jprops[2];
    props.start_base_height = jprops[3];
    props.end_height = jprops[4];
    props.end_base_height = jprops[5];
    props.tri_wall = jprops[6];
    props.flip_wall = jprops[7];
    return true;
}