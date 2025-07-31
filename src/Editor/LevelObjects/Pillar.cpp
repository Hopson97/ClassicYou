#include "Pillar.h"

#include <magic_enum/magic_enum.hpp>

#include "../DrawingPad.h"
#include "../EditConstants.h"

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

    return std::format("Props:\n Texture: {}\n Style: {}\n Size: {:.2f}\n Base Height: {:.2f}\n "
                       "Height: {:.2f}\n Angled: {}\n"
                       "Parameters:\n Position: ({:.2f}, {:.2f})",
                       props.texture.id, magic_enum::enum_name(props.style), props.size,
                       props.base_height, props.height, props.angled ? "true" : "false",
                       params.position.x, params.position.y);
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
bool object_is_within(const PillarObject& pillar, const Rectangle& selection_area)
{
    return Rectangle{
        .position = {pillar.parameters.position.x, pillar.parameters.position.y},
        .size = {pillar.properties.size * TILE_SIZE_F, pillar.properties.size * TILE_SIZE_F}}
        .is_entirely_within(selection_area);
}

template <>
void object_move(PillarObject& pillar, glm::vec2 offset)
{
    pillar.parameters.position += offset;
}

template <>
SerialiseResponse object_serialise(const PillarObject& pillar)
{
    auto& params = pillar.parameters;
    auto& props = pillar.properties;

    nlohmann::json json_params = {params.position.x / TILE_SIZE_F, params.position.y / TILE_SIZE_F};

    nlohmann::json json_props = {};
    serialise_texture(json_props, props.texture);
    json_props.insert(json_props.end(), {(int)props.style, props.size, props.base_height,
                                         props.height, props.angled});

    return {{json_params, json_props}, "pillar"};
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
    params.position *= TILE_SIZE_F;

    props.texture = deserialise_texture(jprops[0]);
    props.style = (PillarStyle)(jprops[1]);
    props.size = jprops[2];
    props.base_height = jprops[3];
    props.height = jprops[4];
    props.angled = jprops[5];

    return true;
}

LevelObjectsMesh3D generate_pillar_mesh(const PillarObject& platform, int floor_number)
{
    const auto& params = platform.parameters;
    const auto& props = platform.properties;

    auto texture = static_cast<float>(props.texture.id);
    auto colour = props.texture.colour;

    auto size = props.size;
    auto ob = props.base_height * FLOOR_HEIGHT;
    auto h = std::min(ob + props.height * FLOOR_HEIGHT, FLOOR_HEIGHT);

    ob += floor_number * FLOOR_HEIGHT;
    h += floor_number * FLOOR_HEIGHT;

    float o = size / 2;
    auto p = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F;

    LevelObjectsMesh3D mesh;

    auto min_x = p.x - o;
    auto max_x = p.x + o;

    auto min_z = p.z - o;
    auto max_z = p.z + o;

    if (props.angled)
    {
        // ???
    }

    mesh.vertices = {
        {{max_x, h, max_z}, {size, h, texture}, {0.0f, 0.0f, 1.0f}, colour},
        {{min_x, h, max_z}, {0.0f, h, texture}, {0.0f, 0.0f, 1.0f}, colour},
        {{min_x, ob, max_z}, {0.0f, ob, texture}, {0.0f, 0.0f, 1.0f}, colour},
        {{max_x, ob, max_z}, {size, ob, texture}, {0.0f, 0.0f, 1.0f}, colour},

        {{min_x, h, max_z}, {size, h, texture}, {-1.0f, 0.0f, 0.0f}, colour},
        {{min_x, h, min_z}, {0.0f, h, texture}, {-1.0f, 0.0f, 0.0f}, colour},
        {{min_x, ob, min_z}, {0.0f, ob, texture}, {-1.0f, 0.0f, 0.0f}, colour},
        {{min_x, ob, max_z}, {size, ob, texture}, {-1.0f, 0.0f, 0.0f}, colour},

        {{min_x, h, min_z}, {size, h, texture}, {0.0f, 0.0f, -1.0f}, colour},
        {{max_x, h, min_z}, {0.0f, h, texture}, {0.0f, 0.0f, -1.0f}, colour},
        {{max_x, ob, min_z}, {0.0f, ob, texture}, {0.0f, 0.0f, -1.0f}, colour},
        {{min_x, ob, min_z}, {size, ob, texture}, {0.0f, 0.0f, -1.0f}, colour},

        {{max_x, h, min_z}, {size, h, texture}, {1.0f, 0.0f, 0.0f}, colour},
        {{max_x, h, max_z}, {0.0f, h, texture}, {1.0f, 0.0f, 0.0f}, colour},
        {{max_x, ob, max_z}, {0.0f, ob, texture}, {1.0f, 0.0f, 0.0f}, colour},
        {{max_x, ob, min_z}, {size, ob, texture}, {1.0f, 0.0f, 0.0f}, colour},

        {{max_x, h, min_z}, {size, 0, texture}, {0.0f, 1.0f, 0.0f}, colour},
        {{min_x, h, min_z}, {0.0f, 0, texture}, {0.0f, 1.0f, 0.0f}, colour},
        {{min_x, h, max_z}, {0.0f, size, texture}, {0.0f, 1.0f, 0.0f}, colour},
        {{max_x, h, max_z}, {size, size, texture}, {0.0f, 1.0f, 0.0f}, colour},

        {{min_x, ob, min_z}, {size, 0, texture}, {0.0f, -1.0f, 0.0f}, colour},
        {{max_x, ob, min_z}, {0.0f, 0, texture}, {0.0f, -1.0f, 0.0f}, colour},
        {{max_x, ob, max_z}, {0.0f, size, texture}, {0.0f, -1.0f, 0.0f}, colour},
        {{min_x, ob, max_z}, {size, size, texture}, {0.0f, -1.0f, 0.0f}, colour},
    };

    int index = 0;
    for (int i = 0; i < 6; i++)
    {
        mesh.indices.push_back(index);
        mesh.indices.push_back(index + 1);
        mesh.indices.push_back(index + 2);
        mesh.indices.push_back(index + 2);
        mesh.indices.push_back(index + 3);
        mesh.indices.push_back(index);
        index += 4;
    }

    return mesh;
    // clang-format on
}