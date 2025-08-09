#include "Wall.h"

#include "../DrawingPad.h"
#include "../EditConstants.h"

namespace
{
    // Minimum distance the selection is required to be to select a wall.
    constexpr float SELECTION_DISTANCE = 8.0f;
} // namespace

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
    return std::format(
        "Props:\n Texture Front: {}\n Texture Back: {}\n Start Base Height: {:.2f}\n Start Height: "
        "{:.2f}\n End Base Height: {:.2f}\n End Height: {:.2f}\n Tri Wall: {}\n Flip Wall: {}\n"
        "Parameters:\n Start Position: ({:.2f}, {:.2f})\n End Position: ({:.2f}, {:.2f})",
        props.texture_front.id, props.texture_back.id, props.start_base_height, props.start_height,
        props.end_base_height, props.end_height, props.tri_wall ? "true" : "false",
        props.flip_wall ? "true" : "false", params.line.start.x, params.line.start.y,
        params.line.end.x, params.line.end.y);
}

template <>
void render_object_2d(const WallObject& wall, DrawingPad& drawing_pad, const glm::vec4& colour,
                      const glm::vec2& selected_offset)

{
    drawing_pad.render_line(wall.parameters.line.start + selected_offset,
                            wall.parameters.line.end + selected_offset, colour, 2.0f);
}

template <>
[[nodiscard]] bool object_try_select_2d(const WallObject& wall, glm::vec2 selection_tile)
{
    return distance_to_line(selection_tile, wall.parameters.line) < SELECTION_DISTANCE;
}

template <>
bool object_is_within(const WallObject& wall, const Rectangle& selection_area)
{
    return wall.parameters.line.to_bounds().is_entirely_within(selection_area);
}

template <>
void object_move(WallObject& wall, glm::vec2 offset)
{
    wall.parameters.line.start += offset;
    wall.parameters.line.end += offset;
}

template <>
void object_rotate(WallObject& wall, glm::vec2 rotation_origin, float degrees)
{
    auto& begin = wall.parameters.line.start;
    auto& end = wall.parameters.line.end;

    begin = rotate_around(begin, rotation_origin, degrees);
    end = rotate_around(end, rotation_origin, degrees);
}

template <>
[[nodiscard]] glm::vec2 object_get_position(const WallObject& wall)
{
    return wall.parameters.line.start;

    /*
        auto x = std::midpoint(line.start.x, line.end.x);
        auto y = std::midpoint(line.start.y, line.end.y);
    */
}

template <>
SerialiseResponse object_serialise(const WallObject& wall)
{
    auto& params = wall.parameters;
    auto& props = wall.properties;

    nlohmann::json json_params = {
        params.line.start.x / TILE_SIZE_F,
        params.line.start.y / TILE_SIZE_F,
        params.line.end.x / TILE_SIZE_F,
        params.line.end.y / TILE_SIZE_F,
    };

    nlohmann::json json_props = {};
    serialise_texture(json_props, props.texture_back);
    serialise_texture(json_props, props.texture_front);
    json_props.insert(json_props.end(),
                      {props.start_height, props.start_base_height, props.end_height,
                       props.end_base_height, props.tri_wall, props.flip_wall});

    return {{json_params, json_props}, "wall"};
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
    params.line.start *= TILE_SIZE_F;
    params.line.end *= TILE_SIZE_F;

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

LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall, int floor_number)
{

    const auto& params = wall.parameters;
    const auto& props = wall.properties;
    // Begin
    auto b = glm::vec3{params.line.start.x, 0, params.line.start.y} / TILE_SIZE_F;

    // End
    auto e = glm::vec3{params.line.end.x, 0, params.line.end.y} / TILE_SIZE_F;

    // Offset x, y
    auto ox = 0.0f;
    auto oz = 0.0f;

    auto obs = props.start_base_height * FLOOR_HEIGHT;
    // auto hs = std::min(obs + props.start_height * FLOOR_HEIGHT, FLOOR_HEIGHT);
    auto hs = obs + props.start_height * FLOOR_HEIGHT;
    obs += floor_number * FLOOR_HEIGHT;
    hs += floor_number * FLOOR_HEIGHT;

    auto obe = props.end_base_height * FLOOR_HEIGHT;
    // auto he = std::min(obe + props.end_height * FLOOR_HEIGHT, FLOOR_HEIGHT);
    auto he = obe + props.end_height * FLOOR_HEIGHT;
    obe += floor_number * FLOOR_HEIGHT;
    he += floor_number * FLOOR_HEIGHT;

    const auto length = glm::length(b - e);

    GLfloat texture_back = static_cast<float>(props.texture_back.id);
    GLfloat texture_front = static_cast<float>(props.texture_front.id);
    auto colour_back = props.texture_back.colour;
    auto colour_front = props.texture_front.colour;

    LevelObjectsMesh3D mesh;

    auto front_normal = glm::cross(glm::normalize(e - b), {0, 1, 0});
    auto back_normal = glm::cross(glm::normalize(b - e), {0, 1, 0});

    // clang-format off
    mesh.vertices = {
        // Back
        {{b.x + ox, obs, b.z + oz}, {0.0f,   obs, texture_back},  back_normal, colour_back},
        {{b.x + ox, hs,  b.z + oz}, {0.0,    hs,  texture_back},  back_normal, colour_back},
        {{e.x + ox, he,  e.z + oz}, {length, he,  texture_back},  back_normal, colour_back},
        {{e.x + ox, obe, e.z + oz}, {length, obe, texture_back},  back_normal, colour_back},

        // Front 
        {{b.x - ox, obs, b.z - oz}, {0.0f,   obs, texture_front}, front_normal, colour_front},
        {{b.x - ox, hs,  b.z - oz}, {0.0,    hs,  texture_front}, front_normal, colour_front},
        {{e.x - ox, he,  e.z - oz}, {length, he,  texture_front}, front_normal, colour_front},
        {{e.x - ox, obe, e.z - oz}, {length, obe, texture_front}, front_normal, colour_front},
    };
    // clang-format on

    if (props.tri_wall)
    {
        if (props.flip_wall)
        {
            mesh.indices = {// Front
                            0, 1, 2,
                            // Back
                            6, 5, 4};
        }
        else
        {
            mesh.indices = {// Front
                            2, 3, 0,
                            // Back
                            4, 7, 6};
        }
    }
    else
    {
        mesh.indices = {// Front
                        0, 1, 2, 2, 3, 0,
                        // Back
                        6, 5, 4, 4, 7, 6};
    }

    return mesh;
}