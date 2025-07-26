#include "Wall.h"

bool operator==(const WallProps& lhs, const WallProps& rhs)
{
    return lhs.texture_front == rhs.texture_front && lhs.texture_back == rhs.texture_back &&
           lhs.start_base_height == rhs.start_base_height && lhs.start_height == rhs.start_height &&
           lhs.end_base_height == rhs.end_base_height && lhs.end_height == rhs.end_height &&
           lhs.tri_wall == rhs.tri_wall && lhs.flip_wall == rhs.flip_wall;
}

bool operator!=(const WallProps& lhs, const WallProps& rhs)
{
    return !(lhs == rhs);
}

template <>
LevelObjectsMesh3D object_to_geometry(const WallObject& wall, int floor_number)

{
    return generate_wall_mesh(wall, floor_number);
}

template <>
std::string object_to_string(const WallObject& wall)

{
    auto& params = wall.parameters;
    auto& props = wall.properties;
    return std::format("Props:\n Texture 1/2: {} {}: \n Base: {}\n Height: {}\nParameters:\n "
                       "  Start position: ({:.2f}, {:.2f}) - End Position: ({:.2f}, {:.2f})",
                       props.texture_front.id, props.texture_back.id, props.start_base_height,
                       props.start_height, params.line.start.x, params.line.start.y,
                       params.line.end.x, params.line.end.y, props.start_height,
                       params.line.start.x, params.line.start.y, params.line.end.x,
                       params.line.end.y);
}

template <>
void render_object_2d(const WallObject& wall, DrawingPad& drawing_pad, const glm::vec4& colour,
                      bool is_selected)

{
    auto thickness = is_selected ? 3.0f : 2.0f;

    drawing_pad.render_line(wall.parameters.line.start, wall.parameters.line.end, colour,
                            thickness);
}

template <>
bool object_try_select_2d(const WallObject& wall, glm::vec2 selection_tile)

{
    return distance_to_line(selection_tile, wall.parameters.line) < 15;
}
