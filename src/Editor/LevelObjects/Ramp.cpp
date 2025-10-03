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

    // clang-format off
    if (props.style == RampStyle::InvertedCorner || props.style == RampStyle::Corner)
    {
        switch (props.direction)
        {
            case Direction::Right:      props.direction = Direction::Left;      break;
            case Direction::Back:       props.direction = Direction::Right;     break;
            case Direction::Left:       props.direction = Direction::Forward;   break;
            case Direction::Forward:    props.direction = Direction::Back;      break;
        }
    }
    else
    {
        switch (props.direction)
        {
            case Direction::Right:      props.direction = Direction::Back;      break;
            case Direction::Back:       props.direction = Direction::Left;      break;
            case Direction::Left:       props.direction = Direction::Forward;   break;
            case Direction::Forward:    props.direction = Direction::Right;     break;
        }
    }
    // clang-format on
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
std::pair<Mesh2DWorld, gl::PrimitiveType>
object_to_geometry_2d(const RampObject& ramp, const LevelTextures& drawing_pad_texture_map)
{
    auto& props = ramp.properties;
    auto texture = static_cast<float>(*drawing_pad_texture_map.get_texture("Ramp"));

    return {generate_2d_quad_mesh(ramp.parameters.position,
                                  {props.width * TILE_SIZE_F, props.depth * TILE_SIZE_F}, texture,
                                  props.texture_top.id, props.texture_top.colour, props.direction),
            gl::PrimitiveType::Triangles};
}

namespace
{
    struct RampMeshParams
    {
        // The ramps position in the world
        glm::vec3 pos;

        // Size
        float width = 0.0f;
        float depth = 0.0f;

        // Textures and colour
        GLfloat texture_top;
        GLfloat texture_bottom;

        glm::u8vec4 colour_top;
        glm::u8vec4 colour_bottom;
    };

    struct RampCornerHeights
    {
        // Layout of ramp heights can be controlled using the corner values
        // These are heights of the 4 corners
        // a----d
        // |    |
        // b----c
        float a = 0.0f;
        float b = 0.0f;
        float c = 0.0f;
        float d = 0.0f;
    };

    auto generate_vertex_positions(const RampCornerHeights& heights, float x, float z, float width,
                                   float depth)
    {
        // clang-format off
        return std::make_tuple(
            glm::vec3{x, heights.a, z},
            glm::vec3{x, heights.b, z + depth},
            glm::vec3{x + width, heights.c, z + depth},
            glm::vec3{x + width, heights.d, z}
        );
        // clang-format on
    }

    // To prevent stretching the UVs, ramp texture coords must be mapped using planar
    // mappping
    auto generate_ramp_texture_coords(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                                      const glm::vec3& d)
    {
        auto tangent = glm::normalize(c - b);
        auto bitangent = glm::normalize(a - b);

        auto make_uv = [&](const glm::vec3& pos)
        {
            return glm::vec2{
                glm::dot(b - pos, tangent),
                glm::dot(pos - b, bitangent),
            };
        };

        // clang-format off
        return std::make_tuple(
            make_uv(a),
            make_uv(b),
            make_uv(c),
            make_uv(d)
        );
        // clang-format on
    }

    auto generate_corner_ramp_texture_coords(const glm::vec3& a, const glm::vec3& b,
                                             const glm::vec3& c)
    {
        auto [uv_a, uv_b, uv_c, _] = generate_ramp_texture_coords(a, b, c, c);
        return std::make_tuple(uv_a, uv_b, uv_c);
    }

    LevelObjectsMesh3D generate_flat_ramp_mesh(const RampMeshParams& p, RampCornerHeights& heights,
                                               Direction direction, RampStyle style,
                                               float start_height, float end_height)
    {
        switch (direction)
        {
            case Direction::Left:
                std::swap(heights.a, heights.c);
                std::swap(heights.b, heights.d);
                break;

            case Direction::Forward:
                heights.a = end_height;
                heights.b = start_height;
                heights.c = start_height;
                heights.d = end_height;
                break;

            case Direction::Back:
                heights.a = start_height;
                heights.b = end_height;
                heights.c = end_height;
                heights.d = start_height;
                break;

            default:
                break;
        }

        // The 4 corners positions and texture coords
        auto [a, b, c, d] = generate_vertex_positions(heights, p.pos.x, p.pos.z, p.width, p.depth);
        auto [uv_a, uv_b, uv_c, uv_d] = generate_ramp_texture_coords(a, b, c, d);

        // Normal is the cross product between the directions of 3 points on the "plane"
        auto normal = glm::normalize(glm::cross(b - a, c - a));

        LevelObjectsMesh3D mesh;
        // clang-format off
        // UV Coords are intentially swapped otherwise they appear upside-down
        mesh.vertices = {
            // Top
            {a, {uv_c.x, uv_c.y, p.texture_top}, normal, p.colour_top},
            {b, {uv_d.x, uv_d.y, p.texture_top}, normal, p.colour_top},
            {c, {uv_a.x, uv_a.y, p.texture_top}, normal, p.colour_top},
            {d, {uv_b.x, uv_b.y,  p.texture_top}, normal, p.colour_top},
             
            // Bottom
            {a, {uv_c.x, uv_c.y, p.texture_bottom}, -normal, p.colour_bottom},
            {b, {uv_d.x, uv_d.y, p.texture_bottom}, -normal, p.colour_bottom},
            {c, {uv_a.x, uv_a.y, p.texture_bottom}, -normal, p.colour_bottom},
            {d, {uv_b.x, uv_b.y, p.texture_bottom}, -normal, p.colour_bottom},
        };
        // clang-format on

        if (style == RampStyle::TriRamp)
        {
            mesh.indices = {
                2, 3, 0, // Top
                4, 7, 6, // Bottom
            };
        }
        else if (style == RampStyle::FlippedTriRamp)
        {
            mesh.indices = {
                0, 1, 2, // Top
                6, 5, 4, // Bottom
            };
        }
        else
        {
            mesh.indices = {
                0, 1, 2, 2, 3, 0, // Top
                6, 5, 4, 4, 7, 6, // Bottom
            };
        }
        return mesh;
    }

    LevelObjectsMesh3D generate_corner_ramp_mesh(const RampMeshParams& p,
                                                 RampCornerHeights& heights, Direction direction,
                                                 RampStyle style, float start_height,
                                                 float end_height)
    {
        bool corner = style == RampStyle::Corner;
        // For corners, one point is higher than the rest (vice versa for inverted ramp), aka
        // 'prominent_corner'
        heights.a = corner ? start_height : end_height;
        heights.b = corner ? start_height : end_height;
        heights.c = corner ? start_height : end_height;
        heights.d = corner ? start_height : end_height;
        float* prominent_corner = [&]()
        {
            // clang-format off
            switch (direction)
            {
                case Direction::Left:    return &heights.a;
                case Direction::Right:   return &heights.b;
                case Direction::Back:    return &heights.c;
                case Direction::Forward: return &heights.d;
            }
            // clang-format on
        }();
        *prominent_corner = corner ? end_height : start_height;

        // The 4 corners
        auto [a, b, c, d] = generate_vertex_positions(heights, p.pos.x, p.pos.z, p.width, p.depth);

        LevelObjectsMesh3D mesh;

        // For the lighting to be correct for corners, the two faces of the corner must be their own
        // triangle face such that they have their own normal vectors
        // clang-format off
        if (direction == Direction::Right || direction == Direction::Forward)
        {
            // Texture coords for each of the faces
            auto [uv_b1, uv_c1, uv_d1] = generate_corner_ramp_texture_coords(b, c, d);
            auto [uv_b2, uv_a2, uv_d2] = generate_corner_ramp_texture_coords(b, a, d);

            // Normals for each a/b face of the corner
            auto na = glm::normalize(glm::cross(c - b, d - c));
            auto nb = glm::normalize(glm::cross(a - b, d - a));

            mesh.vertices = {
                // Top
                {b, {uv_b1.x, uv_b1.y, p.texture_top}, na, p.colour_top},
                {c, {uv_c1.x, uv_c1.y, p.texture_top}, na, p.colour_top},
                {d, {uv_d1.x, uv_d1.y, p.texture_top}, na, p.colour_top},

                {b, {uv_b2.x, uv_b2.y, p.texture_top}, -nb, p.colour_top},
                {a, {uv_a2.x, uv_a2.y, p.texture_top}, -nb, p.colour_top},
                {d, {uv_d2.x, uv_d2.y, p.texture_top}, -nb, p.colour_top},
             
                // Bottom
                {b, {uv_b1.x, uv_b1.y, p.texture_bottom}, -na, p.colour_bottom},
                {c, {uv_c1.x, uv_c1.y, p.texture_bottom}, -na, p.colour_bottom},
                {d, {uv_d1.x, uv_d1.y, p.texture_bottom}, -na, p.colour_bottom},
                
                {b, {uv_b2.x, uv_b2.y, p.texture_bottom}, nb, p.colour_bottom},
                {a, {uv_a2.x, uv_a2.y, p.texture_bottom}, nb, p.colour_bottom},
                {d, {uv_d2.x, uv_d2.y, p.texture_bottom}, nb, p.colour_bottom},
            };

            mesh.indices = {
                0, 1, 2, 5, 4,  3, // Top
                8, 7, 6, 9, 10, 11, // Bottom
            };
        }
        else 
        {
            // Texture coords for each of the faces
            auto [uv_a1, uv_b1, uv_c1] = generate_corner_ramp_texture_coords(a, b, c);
            auto [uv_c2, uv_d2, uv_a2] = generate_corner_ramp_texture_coords(c, d, a);

            // Normals for each a/b face of the corner
            auto na = glm::normalize(glm::cross(b - a, c - b));
            auto nb = glm::normalize(glm::cross(d - c, a - d));

            mesh.vertices = {
                // Top
                {a, {uv_a1.x, uv_a1.y, p.texture_top}, na, p.colour_top},
                {b, {uv_b1.x, uv_b1.y, p.texture_top}, na, p.colour_top},
                {c, {uv_c1.x, uv_c1.y, p.texture_top}, na, p.colour_top},

                {c, {uv_c2.x, uv_c2.y, p.texture_top}, nb, p.colour_top},
                {d, {uv_d2.x, uv_d2.y, p.texture_top}, nb, p.colour_top},
                {a, {uv_a2.x, uv_a2.y, p.texture_top}, nb, p.colour_top},
             
                // Bottom
                {a, {uv_a1.x, uv_a1.y, p.texture_bottom}, -na, p.colour_bottom},
                {b, {uv_b1.x, uv_b1.y, p.texture_bottom}, -na, p.colour_bottom},
                {c, {uv_c1.x, uv_c1.y, p.texture_bottom}, -na, p.colour_bottom},

                {c, {uv_c2.x, uv_c2.y, p.texture_bottom}, -nb, p.colour_bottom},
                {d, {uv_d2.x, uv_d2.y, p.texture_bottom}, -nb, p.colour_bottom},
                {a, {uv_a2.x, uv_a2.y, p.texture_bottom}, -nb, p.colour_bottom},
            };
            mesh.indices = {
                0, 1, 2, 3,  4,  5, // Top
                8, 7, 6, 11, 10, 9, // Bottom
            };
        }
        // clang-format on

        return mesh;
    }
} // namespace

template <>
LevelObjectsMesh3D object_to_geometry(const RampObject& ramp, int floor_number)
{

    LevelObjectsMesh3D mesh;

    const auto& params = ramp.parameters;
    const auto& props = ramp.properties;

    // Direction is assumed left by default for height start/end
    auto hs = props.start_height * FLOOR_HEIGHT;
    auto he = props.end_height * FLOOR_HEIGHT;
    hs += floor_number * FLOOR_HEIGHT;
    he += floor_number * FLOOR_HEIGHT;

    RampMeshParams ramp_mesh_params{
        .pos = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F,

        // Size
        .width = props.width,
        .depth = props.depth,

        // Textures and colour
        .texture_top = static_cast<GLfloat>(props.texture_top.id),
        .texture_bottom = static_cast<GLfloat>(props.texture_bottom.id),

        .colour_top = props.texture_top.colour,
        .colour_bottom = props.texture_bottom.colour,
    };

    RampCornerHeights corner_heights = {
        .a = hs,
        .b = hs,
        .c = he,
        .d = he,
    };

    if (props.style == RampStyle::InvertedCorner || props.style == RampStyle::Corner)
    {
        return generate_corner_ramp_mesh(ramp_mesh_params, corner_heights, props.direction,
                                         props.style, hs, he);
    }
    else
    {
        return generate_flat_ramp_mesh(ramp_mesh_params, corner_heights, props.direction,
                                       props.style, hs, he);
    }
}
