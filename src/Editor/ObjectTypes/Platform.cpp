#include "Platform.h"

#include <magic_enum/magic_enum.hpp>

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

    return std::format("Props:\n Texture Top: {}\n Texture Bottom: {}\n Width: {}\n Depth: "
                       "{} \n Height: {} \n Style: {}\n"
                       "Parameters:\n "
                       " Position: ({:.2f}, {:.2f})",
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
bool object_try_select_2d(const PlatformObject& platform, glm::vec2 selection_tile)

{
    const auto& params = platform.parameters;
    const auto& props = platform.properties;

    return selection_tile.x >= params.position.x &&
           selection_tile.x <= params.position.x + props.width * TILE_SIZE &&
           selection_tile.y >= params.position.y &&
           selection_tile.y <= params.position.y + props.depth * TILE_SIZE;
}
