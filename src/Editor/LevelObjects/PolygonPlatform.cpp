#include "PolygonPlatform.h"

#include "../DrawingPad.h"

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
        "Props:\n Texture Top: {}\n Texture Bottom: {}\n Base: {:.2f}\n Visible: {}\n"
        "Parameters:\n Corner Top Left: ({:.2f}, {:.2f})\n Corner Top Right: ({:.2f}, {:.2f})\n "
        "Corner Bottom Right: ({:.2f}, {:.2f})\n Corner Bottom Left: ({:.2f}, {:.2f})",
        props.texture_top.id, props.texture_bottom.id, props.base, props.visible ? "true" : "false",
        params.corner_top_left.x, params.corner_top_left.y, params.corner_top_right.x,
        params.corner_top_right.y, params.corner_bottom_right.x, params.corner_bottom_right.y,
        params.corner_bottom_left.x, params.corner_bottom_left.y);
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
bool object_is_within(const PolygonPlatformObject& poly, const Rectangle& selection_area)
{
    auto& params = poly.parameters;

    return Rectangle{
        .position = {params.corner_top_left.x, params.corner_top_left.y},
        .size = {params.corner_top_right.x - params.corner_top_left.x,
                 params.corner_bottom_left.y - params.corner_top_left.y},
    }
        .is_entirely_within(selection_area);
}

template <>
void object_move(PolygonPlatformObject& poly, glm::vec2 offset)
{
    auto& params = poly.parameters;

    params.corner_top_left += offset;
    params.corner_top_right += offset;
    params.corner_bottom_right += offset;
    params.corner_bottom_left += offset;
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
    auto& params = poly.parameters;
    auto& props = poly.properties;

    nlohmann::json json_params = {params.corner_top_left.x,     params.corner_top_left.y,
                                  params.corner_top_right.x,    params.corner_top_right.y,
                                  params.corner_bottom_right.x, params.corner_bottom_right.y,
                                  params.corner_bottom_left.x,  params.corner_bottom_left.y};

    nlohmann::json json_props = {};
    serialise_texture(json_props, props.texture_top);
    serialise_texture(json_props, props.texture_bottom);
    json_props.push_back(props.base);
    json_props.push_back(props.visible);
    return {{json_params, json_props}, "polygon_platform"};
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
    if (jprops.size() < 4)
    {
        std::println("Invalid polygon_platform properties, expected 4 values");
        return false;
    }

    params.corner_top_left = {jparams[0], jparams[1]};
    params.corner_top_right = {jparams[2], jparams[3]};
    params.corner_bottom_right = {jparams[4], jparams[5]};
    params.corner_bottom_left = {jparams[6], jparams[7]};

    params.corner_top_left *= TILE_SIZE_F;
    params.corner_top_right *= TILE_SIZE_F;
    params.corner_bottom_right *= TILE_SIZE_F;
    params.corner_bottom_left *= TILE_SIZE_F;




    props.texture_top = deserialise_texture(jprops[0]);
    props.texture_bottom = deserialise_texture(jprops[1]);
    props.base = jprops[2];
    props.visible = jprops[3];
    return true;
}

LevelObjectsMesh3D generate_polygon_platform_mesh(const PolygonPlatformObject& polygon_platform,
                                                  int floor_number)
{
    const auto& params = polygon_platform.parameters;
    const auto& props = polygon_platform.properties;

    if (!props.visible)
    {
        return LevelObjectsMesh3D{};
    }

    auto ob = props.base * FLOOR_HEIGHT + floor_number * FLOOR_HEIGHT;

    auto texture_bottom = static_cast<float>(props.texture_bottom.id);
    auto texture_top = static_cast<float>(props.texture_top.id);
    auto colour_bottom = props.texture_bottom.colour;
    auto colour_top = props.texture_top.colour;

    auto p1 = glm::vec3{params.corner_top_left.x, 0, params.corner_top_left.y} / TILE_SIZE_F;
    auto p2 = glm::vec3{params.corner_top_right.x, 0, params.corner_top_right.y} / TILE_SIZE_F;
    auto p3 =
        glm::vec3{params.corner_bottom_right.x, 0, params.corner_bottom_right.y} / TILE_SIZE_F;
    auto p4 = glm::vec3{params.corner_bottom_left.x, 0, params.corner_bottom_left.y} / TILE_SIZE_F;

    LevelObjectsMesh3D mesh;

    // clang-format off
    mesh.vertices = {
            // Bottom
            {{p1.x, ob, p1.z,},  {0,                    0,                      texture_bottom},    {0, -1, 0}, colour_bottom},
            {{p2.x, ob, p2.z,},  {0,                    glm::length(p2 - p1),   texture_bottom},    {0, -1, 0}, colour_bottom},
            {{p3.x, ob, p3.z },  {glm::length(p3 - p2), glm::length(p3 - p2),   texture_bottom},    {0, -1, 0}, colour_bottom},
            {{p4.x, ob, p4.z,},  {glm::length(p4 - p1), 0,                      texture_bottom},    {0, -1, 0}, colour_bottom},

            // Top
            {{p1.x, ob, p1.z},  {0,                     0,                      texture_top},   {0, 1, 0}, colour_top},
            {{p2.x, ob, p2.z},  {0,                     glm::length(p2 - p1),   texture_top},   {0, 1, 0}, colour_top},
            {{p3.x, ob, p3.z},  {glm::length(p3 - p2),  glm::length(p3 - p2),   texture_top},   {0, 1, 0}, colour_top},
            {{p4.x, ob, p4.z},  {glm::length(p4 - p1),  0,                      texture_top},   {0, 1, 0}, colour_top},
        };
    // clang-format on

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
    // clang-format on
}
