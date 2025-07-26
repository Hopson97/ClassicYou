#include "LevelObject.h"

#include "EditConstants.h"

#include "ObjectTypes/Wall.h"
namespace
{
    ///
    /// Platform Style UIs
    ///
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

LevelObjectsMesh3D generate_wall_mesh(const WallObject& wall, int floor_number)
{

    const auto& params = wall.parameters;
    const auto& props = wall.properties;
    // Begin
    auto b = glm::vec3{params.line.start.x, 0, params.line.start.y} / TILE_SIZE_F;

    // End
    auto e = glm::vec3{params.line.end.x, 0, params.line.end.y} / TILE_SIZE_F;

    // Offset x, y
    auto ox = 0.0f;
    auto oz = 0.0f;

    auto obs = props.start_base_height * FLOOR_HEIGHT;
    auto hs = std::min(obs + props.start_height * FLOOR_HEIGHT, FLOOR_HEIGHT);
    obs += floor_number * FLOOR_HEIGHT;
    hs += floor_number * FLOOR_HEIGHT;

    auto obe = props.end_base_height * FLOOR_HEIGHT;
    auto he = std::min(obe + props.end_height * FLOOR_HEIGHT, FLOOR_HEIGHT);
    obe += floor_number * FLOOR_HEIGHT;
    he += floor_number * FLOOR_HEIGHT;

    const auto length = glm::length(b - e);

    GLfloat texture_back = static_cast<float>(props.texture_back.id);
    GLfloat texture_front = static_cast<float>(props.texture_front.id);
    auto colour_back = props.texture_back.colour;
    auto colour_front = props.texture_front.colour;

    LevelObjectsMesh3D mesh;

    auto front_normal = glm::cross(glm::normalize(e - b), {0, 1, 0});
    auto back_normal = glm::cross(glm::normalize(b - e), {0, 1, 0});

    // clang-format off
    mesh.vertices = {
        // Back
        {{b.x + ox, obs, b.z + oz}, {0.0f,   obs, texture_back},  back_normal, colour_back},
        {{b.x + ox, hs,  b.z + oz}, {0.0,    hs,  texture_back},  back_normal, colour_back},
        {{e.x + ox, he,  e.z + oz}, {length, he,  texture_back},  back_normal, colour_back},
        {{e.x + ox, obe, e.z + oz}, {length, obe, texture_back},  back_normal, colour_back},

        // Front 
        {{b.x - ox, obs, b.z - oz}, {0.0f,   obs, texture_front}, front_normal, colour_front},
        {{b.x - ox, hs,  b.z - oz}, {0.0,    hs,  texture_front}, front_normal, colour_front},
        {{e.x - ox, he,  e.z - oz}, {length, he,  texture_front}, front_normal, colour_front},
        {{e.x - ox, obe, e.z - oz}, {length, obe, texture_front}, front_normal, colour_front},
    };
    // clang-format on

    if (props.tri_wall)
    {
        if (props.flip_wall)
        {
            mesh.indices = {// Front
                            0, 1, 2,
                            // Back
                            6, 5, 4};
        }
        else
        {
            mesh.indices = {// Front
                            2, 3, 0,
                            // Back
                            4, 7, 6};
        }

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

LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform, int floor_number)
{
    const auto& props = platform.properties;

    // Offset platform heights by a hair to prevent Z-fighting with PolygonPlatforms which can go
    // underneath
    float ob = props.base * FLOOR_HEIGHT + floor_number * FLOOR_HEIGHT + 0.00025;
    LevelObjectsMesh3D mesh;
    mesh.vertices = [&]()
    {
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

    auto texture_bottom = static_cast<float>(props.texture_top.id);
    auto texture_top = static_cast<float>(props.texture_bottom.id);
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
            // Top
            {{p1.x, ob, p1.z,},  {0,                    0,                      texture_top},    {0, 1, 0}, colour_bottom},
            {{p2.x, ob, p2.z,},  {0,                    glm::length(p2 - p1),   texture_top},    {0, 1, 0}, colour_bottom},
            {{p3.x, ob, p3.z },  {glm::length(p3 - p2), glm::length(p3 - p2),   texture_top},    {0, 1, 0}, colour_bottom},
            {{p4.x, ob, p4.z,},  {glm::length(p4 - p1), 0,                      texture_top},    {0, 1, 0}, colour_bottom},

            // Bottom
            {{p1.x, ob, p1.z},  {0,                     0,                      texture_bottom},   {0, -1, 0}, colour_top},
            {{p2.x, ob, p2.z},  {0,                     glm::length(p2 - p1),   texture_bottom},   {0, -1, 0}, colour_top},
            {{p3.x, ob, p3.z},  {glm::length(p3 - p2),  glm::length(p3 - p2),   texture_bottom},   {0, -1, 0}, colour_top},
            {{p4.x, ob, p4.z},  {glm::length(p4 - p1),  0,                      texture_bottom},   {0, -1, 0}, colour_top},
        };
    // clang-format on

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
    // clang-format on
}
