#include "Ramp.h"

#include <glm/gtx/rotate_vector.hpp>
#include <magic_enum/magic_enum.hpp>

#include "../../Util/Maths.h"
#include "../EditConstants.h"
#include "../LevelFileIO.h"
#include "../LevelTextures.h"

bool operator==(const RampProps& lhs, const RampProps& rhs)
{
    return lhs.texture_top.id == rhs.texture_top.id &&
           lhs.texture_bottom.id == rhs.texture_bottom.id && lhs.width == rhs.width &&
           lhs.depth == rhs.depth && lhs.start_height == rhs.start_height &&
           lhs.end_height == rhs.end_height && lhs.direction == rhs.direction &&
           lhs.style == rhs.style;
}

bool operator!=(const RampProps& lhs, const RampProps& rhs)
{
    return !(lhs == rhs);
}

template <>
std::string object_to_string(const RampObject& ramp)
{
    auto& params = ramp.parameters;
    auto& props = ramp.properties;

    return std::format(
        "Props:\n Texture Top: {}\n Texture Bottom: {}\n Width: {:.2f}\n Depth: {:.2f}\n"
        " Start Height: {:.2f}\n End Height: {:.2f}\n Direction: {}\n Style: {}\n"
        "Parameters:\n Position: ({:.2f}, {:.2f})",
        props.texture_top.id, props.texture_bottom.id, props.width, props.depth, props.start_height,
        props.end_height, magic_enum::enum_name(props.direction),
        magic_enum::enum_name(props.style), params.position.x, params.position.y);
}

template <>
[[nodiscard]] bool object_try_select_2d(const RampObject& ramp, glm::vec2 selection_tile)
{
    const auto& params = ramp.parameters;
    const auto& props = ramp.properties;

    return selection_tile.x >= params.position.x &&
           selection_tile.x <= params.position.x + props.width * TILE_SIZE &&
           selection_tile.y >= params.position.y &&
           selection_tile.y <= params.position.y + props.depth * TILE_SIZE;
}

template <>
bool object_is_within(const RampObject& ramp, const Rectangle& selection_area)
{
    return Rectangle{
        .position = {ramp.parameters.position.x, ramp.parameters.position.y},
        .size = {ramp.properties.width * TILE_SIZE_F, ramp.properties.depth * TILE_SIZE_F},
    }
        .is_entirely_within(selection_area);
}

template <>
void object_move(RampObject& ramp, glm::vec2 offset)
{
    ramp.parameters.position += offset;
}

template <>
void object_rotate(RampObject& ramp, glm::vec2 rotation_origin, float degrees)
{
    auto copy = ramp.properties;
    auto& props = ramp.properties;
    auto& position = ramp.parameters.position;

    position = rotate_around(position, rotation_origin, degrees);

    // Swap width and depth
    props.depth = copy.width;
    props.width = copy.depth;
    position.x -= props.width * TILE_SIZE_F;

    switch (props.direction)
    {
        case Direction::Right:
            props.direction = Direction::Back;
            break;

        case Direction::Back:
            props.direction = Direction::Left;
            break;

        case Direction::Left:
            props.direction = Direction::Forward;
            break;

        case Direction::Forward:
            props.direction = Direction::Right;
            break;
    }
}

template <>
[[nodiscard]] glm::vec2 object_get_position(const RampObject& ramp)
{
    return ramp.parameters.position;
}

template <>
SerialiseResponse object_serialise(const RampObject& ramp, LevelFileIO& level_file_io)
{
    auto& params = ramp.parameters;
    auto& props = ramp.properties;

    nlohmann::json json_params = {params.position.x / TILE_SIZE_F, params.position.y / TILE_SIZE_F};

    nlohmann::json json_props = {};
    level_file_io.serialise_texture(json_props, props.texture_top);
    level_file_io.serialise_texture(json_props, props.texture_bottom);
    json_props.insert(json_props.end(), {props.width, props.depth, props.start_height,
                                         props.end_height, (int)props.direction, (int)props.style});

    return {{json_params, json_props}, "ramp"};
}

bool object_deserialise(RampObject& ramp, const nlohmann::json& json,
                        const LevelFileIO& level_file_io)
{
    auto& params = ramp.parameters;
    auto& props = ramp.properties;
    auto jparams = json[0];
    auto jprops = json[1];
    if (jparams.size() != 2)
    {
        std::println("Invalid ramp parameters, expected 2 values");
    }
    if (jprops.size() != 8)
    {
        std::println("Invalid ramp properties, expected 8 values");
        return false;
    }

    params.position = {jparams[0], jparams[1]};
    params.position *= TILE_SIZE_F;

    props.texture_top = level_file_io.deserialise_texture(jprops[0]);
    props.texture_bottom = level_file_io.deserialise_texture(jprops[1]);
    props.width = jprops[2];
    props.depth = jprops[3];
    props.start_height = jprops[4];
    props.end_height = jprops[5];
    props.direction = (Direction)jprops[6];
    props.style = (RampStyle)jprops[7];

    return true;
}

template <>
std::pair<Mesh2D, gl::PrimitiveType>
object_to_geometry_2d(const RampObject& ramp, const LevelTextures& drawing_pad_texture_map)
{
    auto& props = ramp.properties;
    auto texture = static_cast<float>(*drawing_pad_texture_map.get_texture("Ramp"));

    return {generate_2d_quad_mesh(ramp.parameters.position,
                                  {props.width * TILE_SIZE_F, props.depth * TILE_SIZE_F}, texture,
                                  props.texture_top.id, props.texture_top.colour, props.direction),
            gl::PrimitiveType::Triangles};
}

template <>
LevelObjectsMesh3D object_to_geometry(const RampObject& ramp, int floor_number)
{
    LevelObjectsMesh3D mesh;

    const auto& params = ramp.parameters;
    const auto& props = ramp.properties;

    float width = props.width;
    float depth = props.depth;

    GLfloat texture_bottom = static_cast<float>(props.texture_bottom.id);
    GLfloat texture_top = static_cast<float>(props.texture_top.id);
    auto colour_bottom = props.texture_bottom.colour;
    auto colour_top = props.texture_top.colour;

    auto p = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F;

    // Direction is assumed left by default for height start/end
    auto hs = props.start_height * FLOOR_HEIGHT;
    auto he = props.end_height * FLOOR_HEIGHT;
    hs += floor_number * FLOOR_HEIGHT;
    he += floor_number * FLOOR_HEIGHT;

    //  Layout of ramp heights can be controlled using the corner values
    //   ha----hd
    //   |      |
    //   hb----hc
    auto ha = hs;
    auto hb = hs;
    auto hc = he;
    auto hd = he;

    bool corner = props.style == RampStyle::Corner;
    bool corner_style = corner || props.style == RampStyle::InvertedCorner;

    if (corner_style)
    {
        // For corners, one point is higher than the rest (vice vera for inverted ramp)
        ha = props.style == RampStyle::Corner ? hs : he;
        hb = props.style == RampStyle::Corner ? hs : he;
        hc = props.style == RampStyle::Corner ? hs : he;
        hd = props.style == RampStyle::Corner ? hs : he;
        switch (props.direction)
        {
            case Direction::Left:
                ha = corner ? he : hs;
                break;

            case Direction::Right:
                hb = corner ? he : hs;
                break;

            case Direction::Back:
                hc = corner ? he : hs;
                break;

            case Direction::Forward:
                hd = corner ? he : hs;
                break;

            default:
                break;
        }
    }
    else
    {
        switch (props.direction)
        {
            case Direction::Left:
                std::swap(ha, hc);
                std::swap(hb, hd);
                break;

            case Direction::Forward:
                ha = he;
                hb = hs;
                hc = hs;
                hd = he;
                break;

            case Direction::Back:
                ha = hs;
                hb = he;
                hc = he;
                hd = hs;
                break;

            default:
                break;
        }
    }

    // clang-format off
    mesh.vertices = {
            // Top
            {{p.x,          ha, p.z,        },  {0,     0,      texture_top},    {0, 1, 0}, colour_top},
            {{p.x,          hb, p.z + depth,},  {0,     depth,  texture_top},    {0, 1, 0}, colour_top},
            {{p.x + width,  hc, p.z + depth,},  {width, depth,  texture_top},    {0, 1, 0}, colour_top},
            {{p.x + width,  hd, p.z,},          {width, 0,      texture_top},    {0, 1, 0}, colour_top},

            // Bottom
            {{p.x,          ha, p.z,        },  {0,     0,      texture_bottom},   {0, -1, 0}, colour_bottom},
            {{p.x,          hb, p.z + depth,},  {0,     depth,  texture_bottom},   {0, -1, 0}, colour_bottom},
            {{p.x + width,  hc, p.z + depth,},  {width, depth,  texture_bottom},   {0, -1, 0}, colour_bottom},
            {{p.x + width,  hd, p.z,        },  {width, 0,      texture_bottom},   {0, -1, 0}, colour_bottom},
        };
    // clang-format on

    if (props.style == RampStyle::TriRamp)
    {
        mesh.indices = {// Front
                        2, 3, 0,
                        // Back
                        4, 7, 6};
    }
    else if (props.style == RampStyle::FlippedTriRamp)
    {
        mesh.indices = {// Front
                        0, 1, 2,
                        // Back
                        6, 5, 4};
    }
    else if (props.style == RampStyle::Full || corner_style)

    {
        if (corner_style &&
            (props.direction == Direction::Right || props.direction == Direction::Forward))
        {
            mesh.indices = {// Front
                            3, 1, 2, 0, 1, 3,
                            // Back
                            7, 5, 4, 6, 5, 7};
        }
        else
        {
            mesh.indices = {// Front
                            0, 1, 2, 2, 3, 0,
                            // Back
                            6, 5, 4, 4, 7, 6};
        }
    }

    return mesh;
}
