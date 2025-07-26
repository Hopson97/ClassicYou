#include "PolygonPlatform.h"

#include "../EditConstants.h"

bool operator==(const PolygonPlatformProps& lhs, const PolygonPlatformProps& rhs)

{
    return lhs.texture_top == rhs.texture_top && lhs.texture_bottom == rhs.texture_bottom &&
           lhs.visible == rhs.visible && lhs.base == rhs.base;
}

bool operator!=(const PolygonPlatformProps& lhs, const PolygonPlatformProps& rhs)
{
    return !(lhs == rhs);
}

template <>
LevelObjectsMesh3D object_to_geometry(const PolygonPlatformObject& poly, int floor_number)

{
    return generate_polygon_platform_mesh(poly, floor_number);
}

template <>
std::string object_to_string(const PolygonPlatformObject& poly)

{
    auto& params = poly.parameters;
    auto& props = poly.properties;

    return std::format(
        "Props:\n Texture Top: {}\n Texture Bottom: {}\n Visible: {}\nParameters:\n "
        "Corner Top Left: ({:.2f}, {:.2f})\n - Corner Top Right: ({:.2f}, {:.2f})\n "
        "Corner Bottom Right: ({:.2f}, {:.2f})\n - Corner Bottom Left: ({:.2f}, {:.2f})",
        props.texture_top.id, props.texture_bottom.id, props.visible, params.corner_top_left.x,
        params.corner_top_left.y, params.corner_top_right.x, params.corner_top_right.y,
        params.corner_bottom_right.x, params.corner_bottom_right.y, params.corner_bottom_left.x,
        params.corner_bottom_left.y);
}

template <>
void render_object_2d(const PolygonPlatformObject& poly, DrawingPad& drawing_pad,
                      const glm::vec4& colour, bool is_selected)

{
    const auto& params = poly.parameters;
    auto& tl = params.corner_top_left;
    auto& tr = params.corner_top_right;
    auto& br = params.corner_bottom_right;
    auto& bl = params.corner_bottom_left;

    drawing_pad.render_line(tl, tl + glm::vec2(TILE_SIZE, 0), colour, 5);
    drawing_pad.render_line(tl, tl + glm::vec2(0, TILE_SIZE), colour, 5);

    drawing_pad.render_line(tr, tr - glm::vec2(TILE_SIZE, 0), colour, 5);
    drawing_pad.render_line(tr, tr + glm::vec2(0, TILE_SIZE), colour, 5);

    drawing_pad.render_line(br, br - glm::vec2(TILE_SIZE, 0), colour, 5);
    drawing_pad.render_line(br, br - glm::vec2(0, TILE_SIZE), colour, 5);

    drawing_pad.render_line(bl, bl + glm::vec2(TILE_SIZE, 0), colour, 5);
    drawing_pad.render_line(bl, bl - glm::vec2(0, TILE_SIZE), colour, 5);
}

template <>
[[nodiscard]] bool object_try_select_2d(const PolygonPlatformObject& poly, glm::vec2 selection_tile)

{
    const auto& params = poly.parameters;

    return selection_tile.x >= params.corner_top_left.x &&
           selection_tile.x <= params.corner_top_right.x &&
           selection_tile.y >= params.corner_top_left.y &&
           selection_tile.y <= params.corner_bottom_left.y;
}

template <>
SerialiseResponse object_serialise(const PolygonPlatformObject& poly)
{
    nlohmann::json object;

    auto& params = poly.parameters;
    auto& props = poly.properties;

    nlohmann::json json_params = {params.corner_top_left.x,     params.corner_top_left.y,
                                  params.corner_top_right.x,    params.corner_top_right.y,
                                  params.corner_bottom_right.x, params.corner_bottom_right.y,
                                  params.corner_bottom_left.x,  params.corner_bottom_left.y};

    nlohmann::json json_props = {};
    serialise_texture(json_props, props.texture_top);
    serialise_texture(json_props, props.texture_bottom);
    json_props.push_back(props.visible);
    return {object, "polygon_platform"};
}

bool object_deserialise(PolygonPlatformObject& poly, const nlohmann::json& json)
{
    auto& params = poly.parameters;
    auto& props = poly.properties;

    auto jparams = json[0];
    auto jprops = json[1];
    if (jparams.size() < 8)
    {
        std::println("Invalid polygon_platform parameters, expected 8 values");
        return false;
    }
    if (jprops.size() < 3)
    {
        std::println("Invalid polygon_platform properties, expected 3 values");
        return false;
    }

    params.corner_top_left = {jparams[0], jparams[1]};
    params.corner_top_right = {jparams[2], jparams[3]};
    params.corner_bottom_right = {jparams[4], jparams[5]};
    params.corner_bottom_left = {jparams[6], jparams[7]};

    props.texture_top = deserialise_texture(jprops[0]);
    props.texture_bottom = deserialise_texture(jprops[1]);
    props.visible = jprops[2];
    return true;
}
