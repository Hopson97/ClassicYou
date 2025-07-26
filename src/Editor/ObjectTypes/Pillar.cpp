#include "Pillar.h"

#include "../EditConstants.h"
#include <magic_enum/magic_enum.hpp>

bool operator==(const PillarProps& lhs, const PillarProps& rhs)

{
    return lhs.texture == rhs.texture && lhs.style == rhs.style && lhs.size == rhs.size &&
           lhs.base_height == rhs.base_height && lhs.height == rhs.height &&
           lhs.angled == rhs.angled;
}

bool operator!=(const PillarProps& lhs, const PillarProps& rhs)
{
    return !(lhs == rhs);
}

template <>
LevelObjectsMesh3D object_to_geometry(const PillarObject& pillar, int floor_number)

{
    return generate_pillar_mesh(pillar, floor_number);
}

template <>
std::string object_to_string(const PillarObject& pillar)

{
    auto& params = pillar.parameters;
    auto& props = pillar.properties;

    return std::format(
        "Props:\n Texture: {}\n Style: {}\n Size: {}\n Base Height: {}\n Height: {}\n "
        "Angled: {}\nParameters:\n Position: ({:.2f}, {:.2f})",
        props.texture.id, magic_enum::enum_name(props.style), props.size, props.base_height,
        props.height, props.angled, params.position.x, params.position.y);
}

template <>
void render_object_2d(const PillarObject& pillar, DrawingPad& drawing_pad, const glm::vec4& colour,
                      bool is_selected)

{
    const auto& position = pillar.parameters.position;
    const auto& size = pillar.properties.size * TILE_SIZE;

    auto offset = size / 2.0f;

    drawing_pad.render_quad(position - offset, {size, size}, colour);
}

template <>
bool object_try_select_2d(const PillarObject& pillar, glm::vec2 selection_tile)

{
    const auto& params = pillar.parameters;
    const auto& props = pillar.properties;

    return selection_tile.x >= params.position.x &&
           selection_tile.x <= params.position.x + props.size * TILE_SIZE &&
           selection_tile.y >= params.position.y &&
           selection_tile.y <= params.position.y + props.size * TILE_SIZE;
}
