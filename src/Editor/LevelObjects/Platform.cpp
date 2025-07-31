#include "Platform.h"

#include <magic_enum/magic_enum.hpp>

#include "../DrawingPad.h"
#include "../EditConstants.h"

bool operator==(const PlatformProps& lhs, const PlatformProps& rhs)

{
    return lhs.texture_top == rhs.texture_top && lhs.texture_bottom == rhs.texture_bottom &&
           lhs.width == rhs.width && lhs.depth == rhs.depth && lhs.base == rhs.base &&
           lhs.style == rhs.style;
    // &&lhs.direction == rhs.direction;
}

bool operator!=(const PlatformProps& lhs, const PlatformProps& rhs)
{
    return !(lhs == rhs);
}

template <>
LevelObjectsMesh3D object_to_geometry(const PlatformObject& platform, int floor_number)

{
    return generate_platform_mesh(platform, floor_number);
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
void render_object_2d(const PlatformObject& platform, DrawingPad& drawing_pad,
                      const glm::vec4& colour, bool is_selected)

{
    const auto& position = platform.parameters.position;
    const auto& width = platform.properties.width * TILE_SIZE;
    const auto& depth = platform.properties.depth * TILE_SIZE;

    if (platform.properties.style == PlatformStyle::Quad)
    {
        drawing_pad.render_quad(position, {width, depth}, colour);
    }
    else if (platform.properties.style == PlatformStyle::Diamond)
    {
        drawing_pad.render_diamond(position, {width, depth}, colour);
    }
}

template <>
[[nodiscard]] bool object_try_select_2d(const PlatformObject& platform, glm::vec2 selection_tile)

{
    const auto& params = platform.parameters;
    const auto& props = platform.properties;

    return selection_tile.x >= params.position.x &&
           selection_tile.x <= params.position.x + props.width * TILE_SIZE &&
           selection_tile.y >= params.position.y &&
           selection_tile.y <= params.position.y + props.depth * TILE_SIZE;
}

template <>
bool object_is_within(const PlatformObject& platform, const Rectangle& selection_area)
{
    return Rectangle{
        .position = {platform.parameters.position.x, platform.parameters.position.y},
        .size = {platform.properties.width * TILE_SIZE_F, platform.properties.depth * TILE_SIZE_F},
    }
        .is_entirely_within(selection_area);
}

template <>
void object_move(PlatformObject& platform, glm::vec2 offset)
{
    platform.parameters.position += offset;
}

template <>
SerialiseResponse object_serialise(const PlatformObject& platform)
{
    auto& params = platform.parameters;
    auto& props = platform.properties;

    nlohmann::json json_params = {params.position.x / TILE_SIZE_F, params.position.y / TILE_SIZE_F};

    nlohmann::json json_props = {};
    serialise_texture(json_props, props.texture_top);
    serialise_texture(json_props, props.texture_bottom);
    json_props.insert(json_props.end(), {props.width, props.depth, props.base, (int)props.style});

    return {{json_params, json_props}, "platform"};
}

bool object_deserialise(PlatformObject& platform, const nlohmann::json& json)
{
    auto& params = platform.parameters;
    auto& props = platform.properties;
    auto jparams = json[0];
    auto jprops = json[1];
    if (jparams.size() < 2)
    {
        std::println("Invalid platform parameters, expected 2 values");
    }
    if (jprops.size() < 6)
    {
        std::println("Invalid platform properties, expected 6 values");
        return false;
    }

    params.position = {jparams[0], jparams[1]};
    params.position *= TILE_SIZE_F;

    props.texture_top = deserialise_texture(jprops[0]);
    props.texture_bottom = deserialise_texture(jprops[1]);
    props.width = jprops[2];
    props.depth = jprops[3];
    props.base = jprops[4];
    props.style = (PlatformStyle)(jprops[5]);

    return true;
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

        GLfloat texture_bottom = static_cast<float>(props.texture_bottom.id);
        GLfloat texture_top = static_cast<float>(props.texture_top.id);
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

        GLfloat texture_bottom = static_cast<float>(props.texture_bottom.id);
        GLfloat texture_top = static_cast<float>(props.texture_top.id);
        auto colour_bottom = props.texture_bottom.colour;
        auto colour_top = props.texture_top.colour;

        auto p = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F;

        // clang-format off
        return {
            // Bottom
            {{p.x + width / 2,  ob, p.z             }, {width / 2,  0,          texture_bottom}, {0, -1, 0}, colour_bottom},
            {{p.x + width,      ob, p.z + depth / 2 }, {width,      depth / 2,  texture_bottom}, {0, -1, 0}, colour_bottom},
            {{p.x + width / 2,  ob, p.z + depth     }, {width / 2,  depth,      texture_bottom}, {0, -1, 0}, colour_bottom},
            {{p.x,              ob, p.z + depth / 2 }, {0,          depth / 2,  texture_bottom}, {0, -1, 0}, colour_bottom},


            // Top
            {{p.x + width / 2,  ob, p.z             }, {width / 2,  0,          texture_top}, {0, 1, 0}, colour_top},
            {{p.x + width,      ob, p.z + depth / 2 }, {width,      depth / 2,  texture_top}, {0, 1, 0}, colour_top},
            {{p.x + width / 2,  ob, p.z + depth     }, {width / 2,  depth,      texture_top}, {0, 1, 0}, colour_top},
            {{p.x,              ob, p.z + depth / 2 }, {0,          depth / 2,  texture_top}, {0, 1, 0}, colour_top}
        };
        // clang-format on
    }
} // namespace

LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform, int floor_number)
{
    const auto& props = platform.properties;

    // Offset platform heights by a hair to prevent Z-fighting with PolygonPlatforms which can go
    // underneath
    float ob = props.base * FLOOR_HEIGHT + floor_number * FLOOR_HEIGHT + 0.00025;
    LevelObjectsMesh3D mesh;
    mesh.vertices = [&]()
    {
        // TODO: triangle platforms
        switch (props.style)
        {
            case PlatformStyle::Quad:
                return create_quad_platform_vertices(platform, ob);

            case PlatformStyle::Diamond:
                return create_diamond_platform_vertices(platform, ob);
        }
        return std::vector<VertexLevelObjects>{};
    }();

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
}