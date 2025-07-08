#include "EditorLevel.h"

#include "DrawingPad.h"
#include "EditConstants.h"

EditorLevel::EditorLevel(const EditorState& state)
    : p_editor_state_(&state)
{
}

Wall& EditorLevel::add_wall(const WallParameters& parameters)
{
    Wall wall{current_id_++};
    wall.parameters = parameters;
    wall.props = p_editor_state_->wall_default;

    LevelMesh level_mesh = {
        .id = wall.object_id,
        .mesh =
            generate_wall_mesh(wall.parameters.start, wall.parameters.end,
                               wall.props.texture_side_1.value, wall.props.texture_side_2.value),
    };
    level_mesh.mesh.buffer();
    wall_meshes_.push_back(std::move(level_mesh));

    return walls_.emplace_back(wall);
}

void EditorLevel::update_object(const Wall& wall)
{
    for (auto& w : walls_)
    {
        if (w.object_id == wall.object_id)
        {
            w = wall;
            break;
        }
    }

    for (auto& wall_mesh : wall_meshes_)
    {
        if (wall_mesh.id == wall.object_id)
        {
            auto new_mesh = generate_wall_mesh(wall.parameters.start, wall.parameters.end,
                                               wall.props.texture_side_1.value,
                                               wall.props.texture_side_2.value);
            new_mesh.buffer();
            wall_mesh.mesh = std::move(new_mesh);
        }
    }
}

void EditorLevel::remove_object(std::size_t id)
{
    std::erase_if(wall_meshes_, [id](const LevelMesh& mesh) { return mesh.id == id; });
    std::erase_if(walls_, [id](const Wall& wall) { return wall.object_id == id; });
}

void EditorLevel::render()
{
    for (auto& wall : wall_meshes_)
    {
        wall.mesh.bind().draw_elements();
    }
}

void EditorLevel::render_2d(DrawingPad& drawing_pad)
{
    for (auto& wall : walls_)
    {
        auto is_selected = p_editor_state_->p_active_object_ &&
                           p_editor_state_->p_active_object_->object_id == wall.object_id;

        auto colour = is_selected ? Colour::RED : Colour::WHITE;
        auto thickness = is_selected ? 3 : 2;

        drawing_pad.render_line(wall.parameters.start, wall.parameters.end, colour, thickness);
    }
}

LevelObject* EditorLevel::try_select(glm::vec2 selection_tile)
{
    for (auto& wall : walls_)
    {
        if (wall.try_select_2d(selection_tile))
        {
            // Allow selecting objects that may be overlapping
            if (!p_editor_state_->p_active_object_ ||
                p_editor_state_->p_active_object_->object_id != wall.object_id)
            {
                return &wall;
            }
        }
    }
    return nullptr;
}
