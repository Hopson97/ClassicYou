#include "LevelObject.h"

#include "EditConstants.h"

namespace
{
    ///
    /// Platform Style UIs
    ///
    std::vector<VertexLevelObjects> create_quad_platform_vertices(const PlatformObject& platform,
                                                                  int floor_number)
    {
        const auto& params = platform.parameters;
        const auto& props = platform.properties;

        float width = props.width;
        float depth = props.depth;
        auto ob = props.base * FLOOR_HEIGHT + floor_number * FLOOR_HEIGHT;

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
                                                                     int floor_number)
    {
        const auto& params = platform.parameters;
        const auto& props = platform.properties;

        float width = props.width;
        float depth = props.depth;
        auto ob = props.base * FLOOR_HEIGHT + floor_number * FLOOR_HEIGHT;

        GLfloat texture_bottom = static_cast<float>(props.texture_bottom.id);
        GLfloat texture_top = static_cast<float>(props.texture_top.id);
        auto colour_bottom = props.texture_bottom.colour;
        auto colour_top = props.texture_top.colour;

        auto p = glm::vec3{params.position.x, 0, params.position.y} / TILE_SIZE_F;

        // clang-format off
        return {
            // Bottom
            {{p.x + width / 2,  ob, p.z             }, {width / 2,  0,          texture_bottom}, {0, 1, 0}, colour_bottom},
            {{p.x + width,      ob, p.z + depth / 2 }, {width,      depth / 2,  texture_bottom}, {0, 1, 0}, colour_bottom},
            {{p.x + width / 2,  ob, p.z + depth     }, {width / 2,  depth,      texture_bottom}, {0, 1, 0}, colour_bottom},
            {{p.x,              ob, p.z + depth / 2 }, {0,          depth / 2,  texture_bottom}, {0, 1, 0}, colour_bottom},


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
    auto ob = props.base_height * FLOOR_HEIGHT;
    auto h = std::min(ob + props.wall_height * 2, FLOOR_HEIGHT);

    ob += floor_number * FLOOR_HEIGHT;
    h += floor_number * FLOOR_HEIGHT;

    const auto length = glm::length(b - e);

    GLfloat texture_back = static_cast<float>(props.texture_back.id);
    GLfloat texture_front = static_cast<float>(props.texture_front.id);
    auto colour_back = props.texture_back.colour;
    auto colour_front = props.texture_front.colour;

    LevelObjectsMesh3D mesh;

    // clang-format off
    mesh.vertices = {
        // Back
        {{b.x + ox, ob, b.z + oz}, {0.0f,   ob, texture_back},  {0, 0, 1}, colour_back},
        {{b.x + ox, h,  b.z + oz}, {0.0,    h,  texture_back},  {0, 0, 1}, colour_back},
        {{e.x + ox, h,  e.z + oz}, {length, h,  texture_back},  {0, 0, 1}, colour_back},
        {{e.x + ox, ob, e.z + oz}, {length, ob, texture_back},  {0, 0, 1}, colour_back},

        // Front 
        {{b.x - ox, ob, b.z - oz}, {0.0f,   ob, texture_front}, {0, 0, 1}, colour_front},
        {{b.x - ox, h,  b.z - oz}, {0.0,    h,  texture_front}, {0, 0, 1}, colour_front},
        {{e.x - ox, h,  e.z - oz}, {length, h,  texture_front}, {0, 0, 1}, colour_front},
        {{e.x - ox, ob, e.z - oz}, {length, ob, texture_front}, {0, 0, 1}, colour_front},
    };
    // clang-format on

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
}

LevelObjectsMesh3D generate_platform_mesh(const PlatformObject& platform, int floor_number)
{
    const auto& props = platform.properties;

    LevelObjectsMesh3D mesh;
    mesh.vertices = [&]()
    {
        switch (props.style)
        {
            case PlatformStyle::Quad:
                return create_quad_platform_vertices(platform, floor_number);

            case PlatformStyle::Diamond:
                return create_diamond_platform_vertices(platform, floor_number);
        }
        return std::vector<VertexLevelObjects>{};
    }();

    mesh.indices = {// Front
                    0, 1, 2, 2, 3, 0,
                    // Back
                    6, 5, 4, 4, 7, 6};

    return mesh;
}

LevelObjectsMesh3D generate_polygon_platform_mesh(const PolygonPlatformObject& polygon_platform,
                                                  int floor_number)
{
    const auto& params = polygon_platform.parameters;
    const auto& props = polygon_platform.properties;

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
