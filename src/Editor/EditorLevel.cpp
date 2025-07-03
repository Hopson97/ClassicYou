#include "EditorLevel.h"

#include <print>

Wall& EditorLevel::add_wall(const Wall& wall)
{
    auto wall_mesh =
        generate_wall_mesh(wall.parameters.start, wall.parameters.end,
                           wall.props.texture_side_1.value, wall.props.texture_side_2.value);

    // Ensure the Wall object is copyable or movable
    auto& itr = walls.emplace_back(wall, std::move(wall_mesh));
    itr.second.buffer();

    std::print("Added wall: start=({}, {}), end=({}, {}), texture1={}, texture2={}\n",
               wall.parameters.start.x, wall.parameters.start.y, wall.parameters.end.x,
               wall.parameters.end.y, wall.props.texture_side_1.value,
               wall.props.texture_side_2.value);

    return itr.first;
}
