#include "PolygonPlatform.h"

#include "../../Util/Maths.h"
#include "../../Util/Util.h"
#include "../LevelFileIO.h"
#include "../LevelTextures.h"

#include <earcut/earcut.hpp>

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
std::string object_to_string(const PolygonPlatformObject& poly)
{
    auto& params = poly.parameters;
    auto& props = poly.properties;

    std::string points_string;
    points_string.reserve(props.geometry.size() * 8);
    for (auto& point : props.geometry[0])
    {
        points_string += std::format("({:.2f}, {:.2f}), ", point.x, point.y);
    }

    std::string holes_string;

    for (auto hole : props.geometry | std::views::drop(1))
    {
        std::string hole_string;
        for (auto& point : hole)
        {
            hole_string += std::format("({:.2f}, {:.2f}), ", point.x, point.y);
        }
        holes_string += std::format("Hole: {}", hole_string);
    }

    return std::format(
        "Props:\n Texture Top: {}\n Texture Bottom: {}\n Base: {:.2f}\n Visible: {}\n"
        "Points: {}\nHoles: {}"
        "Parameters:\n Position: ({:.2f}, {:.2f})",
        props.texture_top.id, props.texture_bottom.id, props.base, props.visible ? "true" : "false",
        points_string, holes_string, params.position.x, params.position.y);
}

template <>
[[nodiscard]] bool object_try_select_2d(const PolygonPlatformObject& poly, glm::vec2 selection_tile)
{
    return point_in_polygon(selection_tile, poly.properties.geometry[0], poly.parameters.position);
}

template <>
bool object_is_within(const PolygonPlatformObject& poly, const Rectangle& selection_area)
{
    return selection_area.contains(poly.properties.geometry[0], poly.parameters.position);
}

template <>
void object_move(PolygonPlatformObject& poly, glm::vec2 offset)
{
    poly.parameters.position += offset;
}

template <>
void object_rotate(PolygonPlatformObject& poly, glm::vec2 rotation_origin, float degrees)
{
    auto& props = poly.properties;
    auto& position = poly.parameters.position;

    glm::vec2 new_position = rotate_around(position, rotation_origin, degrees);
    for (auto& point_array : props.geometry)
    {
        for (auto& point : point_array)
        {
            point = rotate_around(point + position, rotation_origin, degrees);
            point -= new_position;
        }
    }

    position = new_position;
}

template <>
[[nodiscard]] glm::vec2 object_get_position(const PolygonPlatformObject& poly)
{
    return poly.parameters.position;
}

template <>
SerialiseResponse object_serialise(const PolygonPlatformObject& poly, LevelFileIO& level_file_io)
{
    auto& params = poly.parameters;
    auto& props = poly.properties;

    nlohmann::json json_params = {params.position.x / TILE_SIZE_F, params.position.y / TILE_SIZE_F};

    nlohmann::json json_props = {};

    nlohmann::json points;
    for (auto& point : props.geometry[0])
    {
        points.push_back(point.x);
        points.push_back(point.y);
    }

    nlohmann::json holes;
    for (const auto& hole : props.geometry | std::views::drop(1))
    {
        nlohmann::json json_hole;
        for (auto& point : hole)
        {
            json_hole.push_back(point.x);
            json_hole.push_back(point.y);
        }
        holes.push_back(std::move(json_hole));
    }
    json_props.push_back(std::move(points));
    json_props.push_back(std::move(holes));

    level_file_io.serialise_texture(json_props, props.texture_top);
    level_file_io.serialise_texture(json_props, props.texture_bottom);
    json_props.push_back(props.base);
    json_props.push_back(props.visible);
    return {{json_params, json_props}, "polygon_platform"};
}

bool object_deserialise(PolygonPlatformObject& poly, const nlohmann::json& json,
                        const LevelFileIO& level_file_io)
{
    auto& params = poly.parameters;
    auto& props = poly.properties;

    auto jparams = json[0];
    auto jprops = json[1];

    if (jparams.size() != 2)
    {
        std::println("Invalid polygon platform parameters, expected 2 values");
        return false;
    }
    if (jprops.size() != 6)
    {
        std::println("Invalid polygon platform properties, expected 2 values");
        return false;
    }

    params.position = {jparams[0], jparams[1]};
    params.position *= TILE_SIZE_F;

    // Ensure a clean slate when reading points
    props.geometry.clear();

    // Read the edge points of the polygon
    auto& points = props.geometry.emplace_back();
    size_t points_size = jprops[0].size();
    for (size_t i = 0; i < points_size; i += 2)
    {
        points.push_back({jprops[0][i], jprops[0][i + 1]});
    }

    // Read the holes
    props.geometry.reserve(jprops[1].size() + 1);
    for (auto& hole : jprops[1])
    {
        size_t hole_size = hole.size();
        auto& hole_vector = props.geometry.emplace_back();
        hole_vector.reserve(hole_size / 2 + 1);
        for (size_t i = 0; i < hole_size; i += 2)
        {
            hole_vector.push_back({hole[i], hole[i + 1]});
        }
    }

    // Rwad the rest of the props
    props.texture_top = level_file_io.deserialise_texture(jprops[2]);
    props.texture_bottom = level_file_io.deserialise_texture(jprops[3]);
    props.base = jprops[4];
    props.visible = jprops[5];

    return true;
}

template <>
std::pair<Mesh2DWorld, gl::PrimitiveType>
object_to_geometry_2d(const PolygonPlatformObject& poly,
                      const LevelTextures& drawing_pad_texture_map)
{
    auto base_texture = static_cast<float>(*drawing_pad_texture_map.get_texture("PolygonPlatform"));
    const auto& params = poly.parameters;
    const auto& props = poly.properties;
    auto& position = params.position;

    auto texture_top = static_cast<float>(props.texture_top.id);
    auto colour_top = props.texture_top.colour;

    Mesh2DWorld mesh;
    auto earcut_indices = mapbox::earcut<>(props.geometry);

    // Top face
    for (auto& point_array : props.geometry)
    {
        for (auto& point : point_array)
        {
            glm::vec2 world_pos{point.x + position.x, point.y + position.y};
            glm::vec2 tex_coords = world_pos / TILE_SIZE_F;
            mesh.vertices.push_back({world_pos,
                                     {tex_coords.x, tex_coords.y, base_texture},
                                     {tex_coords.x, tex_coords.y, texture_top},
                                     colour_top});
        }
    }

    // The indices vector is given in clockwise order, so must be reversed to get anti-clockwise
    // ordering for the top face.
    for (auto i = earcut_indices.rbegin(); i != earcut_indices.rend(); ++i)
    {
        mesh.indices.push_back(*i);
    }
    return std::make_pair(std::move(mesh), gl::PrimitiveType::Triangles);
}

Mesh2DWorld object_to_outline_2d(const PolygonPlatformObject& poly)
{
    const auto& params = poly.parameters;
    const auto& props = poly.properties;
    auto& position = params.position;

    constexpr static glm::i8vec4 OL_COLOUR{255, 0, 0, 255};
    constexpr static glm::i8vec4 HOLE_COLOUR{255, 255, 0, 255};

    Mesh2DWorld mesh;

    // Create the lines, the first array is the outer polygon (outline), thereafter are the inner
    // hole polygons
    bool is_outline = true;
    for (auto& point_array : props.geometry)
    {
        for (size_t i = 0; i < point_array.size(); i++)
        {
            auto p1 = point_array[i];
            auto p2 = point_array[(i + 1) % point_array.size()];

            auto colour = is_outline ? OL_COLOUR : HOLE_COLOUR;

            add_line_to_mesh(mesh, p1 + position, p2 + position, colour);
        }
        is_outline = false;
    }
    return mesh;
}

template <>
LevelObjectsMesh3D object_to_geometry(const PolygonPlatformObject& poly, int floor_number)
{
    const auto& params = poly.parameters;
    const auto& props = poly.properties;

    if (!props.visible)
    {
        return LevelObjectsMesh3D{};
    }

    auto ob = props.base * FLOOR_HEIGHT + floor_number * FLOOR_HEIGHT;

    auto texture_bottom = static_cast<float>(props.texture_bottom.id);
    auto texture_top = static_cast<float>(props.texture_top.id);
    auto colour_bottom = props.texture_bottom.colour;
    auto colour_top = props.texture_top.colour;

    auto p = glm::vec3{params.position.x / TILE_SIZE_F, 0, params.position.y / TILE_SIZE_F};
    LevelObjectsMesh3D mesh;

    // Mapbox's Earcut triangulates the polygon's points and holes which gets a list of INDICES
    // within all of of the "props.points" vectors.
    auto earcut_indices = mapbox::earcut<>(props.geometry);

    // Top face
    for (auto& point_array : props.geometry)
    {
        for (auto& point : point_array)
        {
            glm::vec3 pos = {point.x / TILE_SIZE_F + p.x, ob, point.y / TILE_SIZE_F + p.z};
            mesh.vertices.push_back({pos, {pos.x, pos.z, texture_top}, {0, 1, 0}, colour_top});
        }
    }

    // The indices vector is given in clockwise order, so must be reversed to get anti-clockwise
    // ordering for the top face.
    for (auto i = earcut_indices.rbegin(); i != earcut_indices.rend(); ++i)
    {
        mesh.indices.push_back(*i);
    }

    // Bottom face
    for (auto& point_array : props.geometry)
    {
        for (auto& point : point_array)
        {
            glm::vec3 pos = {point.x / TILE_SIZE_F + p.x, ob, point.y / TILE_SIZE_F + p.z};
            mesh.vertices.push_back(
                {pos, {pos.x, pos.z, texture_bottom}, {0, 1, 0}, colour_bottom});
        }
    }

    for (auto& point_array : props.geometry)
    {
        for (auto& point : point_array)
        {
            glm::vec3 pos = {point.x / TILE_SIZE_F + p.x, ob, point.y / TILE_SIZE_F + p.z};
        }
    }

    auto max_index = *std::max_element(mesh.indices.cbegin(), mesh.indices.cend());
    for (auto& i : earcut_indices)
    {
        mesh.indices.push_back(i + max_index + 1);
    }
    return mesh;
}
