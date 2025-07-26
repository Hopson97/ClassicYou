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
[[nodiscard]] bool object_try_select_2d(const PillarObject& pillar, glm::vec2 selection_tile)

{
    const auto& params = pillar.parameters;
    const auto& props = pillar.properties;

    return selection_tile.x >= params.position.x &&
           selection_tile.x <= params.position.x + props.size * TILE_SIZE &&
           selection_tile.y >= params.position.y &&
           selection_tile.y <= params.position.y + props.size * TILE_SIZE;
}

template <>
SerialiseResponse object_serialise(const PillarObject& pillar)
{
    nlohmann::json object;

    auto& params = pillar.parameters;
    auto& props = pillar.properties;

    nlohmann::json json_params = {params.position.x, params.position.y};

    nlohmann::json json_props = {};
    serialise_texture(json_props, props.texture);
    json_props.insert(json_props.end(), {(int)props.style, props.size, props.base_height,
                                         props.height, props.angled});

    object = {json_params, json_props};

    return {object, "pillar"};
}

bool object_deserialise(PillarObject& pillar_object, const nlohmann::json& json)
{
    auto& params = pillar_object.parameters;
    auto& props = pillar_object.properties;

    auto jparams = json[0];
    auto jprops = json[1];
    if (jparams.size() < 2)
    {
        std::println("Invalid pillar parameters, expected 2 values");
        return false;
    }
    if (jprops.size() < 6)
    {
        std::println("Invalid pillar properties, expected 6 values");
        return false;
    }

    params.position = {jparams[0], jparams[1]};

    props.texture = deserialise_texture(jprops[0]);
    props.style = (PillarStyle)(jprops[1]);
    props.size = jprops[2];
    props.base_height = jprops[3];
    props.height = jprops[4];
    props.angled = jprops[5];

    return true;
}