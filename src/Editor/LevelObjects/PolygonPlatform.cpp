#include "PolygonPlatform.h"

#include "../../Util/Maths.h"
#include "../../Util/Util.h"
#include "../LevelFileIO.h"

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
    points_string.reserve(props.points.size() * 8);
    for (auto& point : props.points[0])
    {
        points_string += std::format("({:.2f}, {:.2f}), ", point.x, point.y);
    }

    std::string holes_string;

    for (int i = 1; i < props.points.size(); i++)
    {
        std::string hole_string;
        for (auto& point : props.points[i])
        {
            hole_string += std::format("({:.2f}, {:.2f}), ", point.x, point.y);
        }
        holes_string += std::format("Hole: {}", hole_string);
    }

    return std::format(
        "Props:\n Texture Top: {}\n Texture Bottom: {}\n Base: {:.2f}\n Visible: {}\n"
        "Parameters:\n Points: {}\nHoles: {}",
        props.texture_top.id, props.texture_bottom.id, props.base, props.visible ? "true" : "false",
        points_string, holes_string);
}

template <>
[[nodiscard]] bool object_try_select_2d(const PolygonPlatformObject& poly, glm::vec2 selection_tile)
{
    const auto& params = poly.parameters;
    // TODO implment
    return false;
}

template <>
bool object_is_within(const PolygonPlatformObject& poly, const Rectangle& selection_area)
{
    auto& params = poly.parameters;
    // TODO implment
    return false;
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
    position = rotate_around(position, rotation_origin, degrees);

    for (auto& point : props.points[0])
    {
        point = rotate_around(point, rotation_origin, degrees);
    }
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
    for (auto& point : props.points[0])
    {
        points.push_back(point.x);
        points.push_back(point.y);
    }

    nlohmann::json holes;
    for (int i = 1; i < props.points.size(); i++)
    {
        nlohmann::json hole;
        for (auto& point : props.points[i])
        {
            hole.push_back(point.x);
            hole.push_back(point.y);
        }
        holes.push_back(hole);
    }

    json_props.push_back(points);
    json_props.push_back(holes);

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
    props.points.clear();

    // Read the edge points of the polygon
    auto& points = props.points.emplace_back();
    size_t points_size = jprops[0].size();
    for (size_t i = 0; i < points_size; i += 2)
    {
        points.push_back({jprops[0][i], jprops[0][i + 1]});
    }

    // Read the holes
    props.points.reserve(jprops[1].size() + 1);
    for (auto& hole : jprops[1])
    {
        size_t hole_size = hole.size();
        auto& hole_vector = props.points.emplace_back();
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
    const auto& params = poly.parameters;
    const auto& props = poly.properties;

    Mesh2DWorld mesh;

    for (auto& point : props.points[0])
    {
        add_line_to_mesh(mesh, point + params.position,
                         point + glm::vec2(TILE_SIZE, 0) + params.position,
                         poly.properties.texture_top.colour);
    }

    return std::make_pair(std::move(mesh), gl::PrimitiveType::Lines);
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
    // When creating the mesh,this enables simply adding all points to the mesh, and then let the
    // order of the indices returned do the rest.
    auto earcut_indices = mapbox::earcut<>(props.points);

    // Top face
    for (auto& point_array : props.points)
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
    for (auto& point_array : props.points)
    {
        for (auto& point : point_array)
        {
            glm::vec3 pos = {point.x / TILE_SIZE_F + p.x, ob, point.y / TILE_SIZE_F + p.z};
            mesh.vertices.push_back(
                {pos, {pos.x, pos.z, texture_bottom}, {0, 1, 0}, colour_bottom});
        }
    }

    std::println("=#=#=#=#=");
    for (auto& point_array : props.points)
    {
        for (auto& point : point_array)
        {
            glm::vec3 pos = {point.x / TILE_SIZE_F + p.x, ob, point.y / TILE_SIZE_F + p.z};
            std::print(" ({} {} {}) ", pos.x, pos.y, pos.z);
        }
        std::println("");
    }
    std::println("=#=#=#=#=\n");

    auto max_index = *std::max_element(mesh.indices.cbegin(), mesh.indices.cend());
    for (auto& i : earcut_indices)
    {
        mesh.indices.push_back(i + max_index + 1);
    }
    return mesh;
}
