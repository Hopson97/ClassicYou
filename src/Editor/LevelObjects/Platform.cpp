#include "Platform.h"

#include <magic_enum/magic_enum.hpp>

#include "../LevelFileIO.h"
#include "../LevelTextures.h"
#include "LevelObjectHelpers.h"

bool operator==(const PlatformProps& lhs, const PlatformProps& rhs)
{
    return lhs.texture_top == rhs.texture_top && lhs.texture_bottom == rhs.texture_bottom &&
           lhs.width == rhs.width && lhs.depth == rhs.depth && lhs.base == rhs.base &&
           lhs.style == rhs.style && lhs.direction == rhs.direction;
}

bool operator!=(const PlatformProps& lhs, const PlatformProps& rhs)
{
    return !(lhs == rhs);
}

template <>
std::string object_to_string(const PlatformObject& platform)
{
    auto& params = platform.parameters;
    auto& props = platform.properties;

    return std::format("Props:\n Texture Top: {}\n Texture Bottom: {}\n Width: {:.2f}\n Depth: "
                       "{:.2f}\n Base: {:.2f}\n Style: {}\n"
                       "Parameters:\n Position: ({:.2f}, {:.2f})",
                       props.texture_top.id, props.texture_bottom.id, props.width, props.depth,
                       props.base, magic_enum::enum_name(props.style), params.position.x,
                       params.position.y);
}

template <>
[[nodiscard]] bool object_try_select_2d(const PlatformObject& platform, glm::vec2 selection_tile)
{
    const auto& params = platform.parameters;
    const auto& props = platform.properties;

    // Do the AABB check first - This works for all shapes as a broadphase before diamond and
    // triangle checks are performed
    if (!object_to_rectangle(platform).contains(selection_tile))
    {
        return false;
    }

    if (props.style == PlatformStyle::Diamond)
    {
        auto half_width = props.width * TILE_SIZE / 2.0f;
        auto half_depth = props.depth * TILE_SIZE / 2.0f;

        auto center = params.position + glm::vec2{half_width, half_depth};
        auto dx = std::abs(selection_tile.x - center.x);
        auto dy = std::abs(selection_tile.y - center.y);

        return (dx / half_width) + (dy / half_depth) <= 1.0f;
    }
    else if (props.style == PlatformStyle::Triangle)
    {
        glm::vec2 size{props.width * TILE_SIZE_F, props.depth * TILE_SIZE_F};
        return point_in_triangle(selection_tile, generate_2d_triangle_vertex_positions(
                                                     params.position, size, props.direction));
    }

    // Successful quad-shape selection just uses the AABB at the function start
    return true;
}

template <>
bool object_is_within(const PlatformObject& platform, const Rectangle& selection_area)
{
    return object_to_rectangle(platform).is_entirely_within(selection_area);
}

template <>
void object_move(PlatformObject& platform, glm::vec2 offset)
{
    platform.parameters.position += offset;
}

template <>
void object_rotate(PlatformObject& platform, glm::vec2 rotation_origin, float degrees)
{
    auto copy = platform.properties;
    auto& props = platform.properties;
    auto& position = platform.parameters.position;

    position = rotate_around(position, rotation_origin, degrees);

    // Swap width and depth
    props.depth = copy.width;
    props.width = copy.depth;
    position.x -= props.width * TILE_SIZE_F;

    if (props.style == PlatformStyle::Triangle)
    {
        // clang-format off
        switch (props.direction)
        {
            case Direction::Right:      props.direction = Direction::Back;      break;
            case Direction::Back:       props.direction = Direction::Forward;   break;
            case Direction::Left:       props.direction = Direction::Right;      break;
            case Direction::Forward:    props.direction = Direction::Left;     break;
        }
        // clang-format on
    }
}

template <>
[[nodiscard]] glm::vec2 object_get_position(const PlatformObject& platform)
{
    return platform.parameters.position;
}

template <>
SerialiseResponse object_serialise(const PlatformObject& platform, LevelFileIO& level_file_io)
{
    auto& params = platform.parameters;
    auto& props = platform.properties;

    nlohmann::json json_params = {params.position.x / TILE_SIZE_F, params.position.y / TILE_SIZE_F};

    nlohmann::json json_props = {};
    level_file_io.serialise_texture(json_props, props.texture_top);
    level_file_io.serialise_texture(json_props, props.texture_bottom);
    json_props.insert(json_props.end(), {props.width, props.depth, props.base, (int)props.style,
                                         (int)props.direction});

    return {{json_params, json_props}, "platform"};
}

bool object_deserialise(PlatformObject& platform, const nlohmann::json& json,
                        const LevelFileIO& level_file_io)
{
    auto& params = platform.parameters;
    auto& props = platform.properties;
    auto jparams = json[0];
    auto jprops = json[1];
    if (jparams.size() != 2)
    {
        std::println("Invalid platform parameters, expected 2 values");
    }
    if (jprops.size() != 7)
    {
        std::println("Invalid platform properties, expected 7 values");
        return false;
    }

    params.position = {jparams[0], jparams[1]};
    params.position *= TILE_SIZE_F;

    props.texture_top = level_file_io.deserialise_texture(jprops[0]);
    props.texture_bottom = level_file_io.deserialise_texture(jprops[1]);
    props.width = jprops[2];
    props.depth = jprops[3];
    props.base = jprops[4];
    props.style = (PlatformStyle)(jprops[5]);
    props.direction = (Direction)(jprops[6]);

    return true;
}

template <>
std::pair<Mesh2DWorld, gl::PrimitiveType>
object_to_geometry_2d(const PlatformObject& platform, const LevelTextures& drawing_pad_texture_map)
{
    auto& params = platform.parameters;
    auto& props = platform.properties;
    auto texture = static_cast<float>(*drawing_pad_texture_map.get_texture("Platform"));
    glm::vec2 size{props.width * TILE_SIZE_F, props.depth * TILE_SIZE_F};

    switch (props.style)
    {
        case PlatformStyle::Triangle:
            return {generate_2d_triangle_mesh(params.position, size, texture, props.texture_top.id,
                                              props.texture_top.colour, props.direction),
                    gl::PrimitiveType::Triangles};

        case PlatformStyle::Diamond:
            return {generate_2d_diamond_mesh(params.position, size, texture, props.texture_top.id,
                                             props.texture_top.colour, Direction::Forward),
                    gl::PrimitiveType::Triangles};

        // Default to quad
        default:
            return {generate_2d_quad_mesh(params.position, size, texture, props.texture_top.id,
                                          props.texture_top.colour, Direction::Forward),
                    gl::PrimitiveType::Triangles};
    }
}

namespace
{
    std::vector<VertexLevelObjects> create_quad_platform_vertices(const PlatformObject& platform,
                                                                  float ob)
    {
        const auto& params = platform.parameters;
        const auto& props = platform.properties;

        float width = props.width;
        float depth = props.depth;

        auto texture_bottom = static_cast<GLfloat>(props.texture_bottom.id);
        auto texture_top = static_cast<GLfloat>(props.texture_top.id);
        auto colour_bottom = props.texture_bottom.colour;
        auto colour_top = props.texture_top.colour;

        auto p = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F;
        // clang-format off
        return {
            // Top
            {{p.x,          ob, p.z,        },  {0,     0,      texture_top},    {0, 1, 0}, colour_top},
            {{p.x,          ob, p.z + depth,},  {0,     depth,  texture_top},    {0, 1, 0}, colour_top},
            {{p.x + width,  ob, p.z + depth,},  {width, depth,  texture_top},    {0, 1, 0}, colour_top},
            {{p.x + width,  ob, p.z,},          {width, 0,      texture_top},    {0, 1, 0}, colour_top},

            // Bottom
            {{p.x,          ob, p.z,        },  {0,     0,      texture_bottom},   {0, -1, 0}, colour_bottom},
            {{p.x,          ob, p.z + depth,},  {0,     depth,  texture_bottom},   {0, -1, 0}, colour_bottom},
            {{p.x + width,  ob, p.z + depth,},  {width, depth,  texture_bottom},   {0, -1, 0}, colour_bottom},
            {{p.x + width,  ob, p.z,        },  {width, 0,      texture_bottom},   {0, -1, 0}, colour_bottom},
        };
        // clang-format on
    }

    std::vector<VertexLevelObjects> create_diamond_platform_vertices(const PlatformObject& platform,
                                                                     float ob)
    {
        const auto& params = platform.parameters;
        const auto& props = platform.properties;

        float width = props.width;
        float depth = props.depth;

        auto texture_bottom = static_cast<GLfloat>(props.texture_bottom.id);
        auto texture_top = static_cast<GLfloat>(props.texture_top.id);
        auto colour_bottom = props.texture_bottom.colour;
        auto colour_top = props.texture_top.colour;

        auto p = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F;

        // clang-format off
        return {
            // Top
            {{p.x + width,     ob, p.z + depth / 2}, {width,     depth / 2, texture_top}, {0, 1, 0}, colour_top},
            {{p.x + width / 2, ob, p.z            }, {width / 2, 0,         texture_top}, {0, 1, 0}, colour_top},
            {{p.x,             ob, p.z + depth / 2}, {0,         depth / 2, texture_top}, {0, 1, 0}, colour_top},
            {{p.x + width / 2, ob, p.z + depth    }, {width / 2, depth,     texture_top}, {0, 1, 0}, colour_top},

            // Bottom
            {{p.x + width,     ob, p.z + depth / 2}, {width,     depth / 2, texture_bottom}, {0, -1, 0}, colour_bottom},
            {{p.x + width / 2, ob, p.z            }, {width / 2, 0,         texture_bottom}, {0, -1, 0}, colour_bottom}, 
            {{p.x,             ob, p.z + depth / 2}, {0,         depth / 2, texture_bottom}, {0, -1, 0}, colour_bottom}, 
            {{p.x + width / 2, ob, p.z + depth    }, {width / 2, depth,     texture_bottom}, {0, -1, 0}, colour_bottom}, 
        };
        // clang-format on
    }

    std::vector<VertexLevelObjects>
    create_triangle_platform_vertices(const PlatformObject& platform, float ob)
    {
        const auto& params = platform.parameters;
        const auto& props = platform.properties;

        float width = props.width;
        float depth = props.depth;

        auto texture_top = static_cast<GLfloat>(props.texture_top.id);
        auto texture_bottom = static_cast<GLfloat>(props.texture_bottom.id);
        auto colour_top = props.texture_top.colour;
        auto colour_bottom = props.texture_bottom.colour;
        auto direction = props.direction;

        auto p = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F;

        // clang-format off
        // Construct the triangle by removing a vertex
        auto v = direction_to_triangle_vertices<glm::vec3>(
            NamedQuadVertices<glm::vec3>{.top_left      = {p.x,         ob, p.z},
                                         .bottom_left   = {p.x,         ob, p.z + depth},
                                         .bottom_right  = {p.x + width, ob, p.z + depth},
                                         .top_right     = {p.x + width, ob, p.z}},
            direction);

        // Calculate the UVs using planar mapping to avoid stretching
        auto make_uv = [&](const glm::vec3& pos, float texture)
        { return glm::vec3{std::abs(pos.x - p.x), std::abs(pos.z - p.z), texture}; };
        return {
            // Top
            {v[0], make_uv(v[0], texture_top), {0, 1, 0}, colour_top},
            {v[1], make_uv(v[1], texture_top), {0, 1, 0}, colour_top},
            {v[2], make_uv(v[2], texture_top), {0, 1, 0}, colour_top},
            // Bottom
            {v[0], make_uv(v[0], texture_bottom), {0, -1, 0}, colour_bottom},
            {v[1], make_uv(v[1], texture_bottom), {0, -1, 0}, colour_bottom},
            {v[2], make_uv(v[2], texture_bottom), {0, -1, 0}, colour_bottom},
        };
        // clang-format on
    }
} // namespace

template <>
LevelObjectsMesh3D object_to_geometry(const PlatformObject& platform, int floor_number)
{
    const auto& props = platform.properties;

    // Offset platform heights by a hair to prevent Z-fighting with PolygonPlatforms which can go
    // underneath
    float ob = props.base * FLOOR_HEIGHT + floor_number * FLOOR_HEIGHT + 0.00025f;
    LevelObjectsMesh3D mesh;
    mesh.vertices = [&]()
    {
        switch (props.style)
        {
            case PlatformStyle::Triangle:
                return create_triangle_platform_vertices(platform, ob);

            case PlatformStyle::Quad:
                return create_quad_platform_vertices(platform, ob);

            case PlatformStyle::Diamond:
                return create_diamond_platform_vertices(platform, ob);
        }
        return std::vector<VertexLevelObjects>{};
    }();

    if (props.style == PlatformStyle::Triangle)
    {
        mesh.indices = {// Front
                        0, 1, 2,
                        // Back
                        5, 4, 3};
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
