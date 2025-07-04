#include "Tool.h"

#include "DrawingPad.h"

#include "EditorLevel.h"

CreateWallTool::CreateWallTool(EditorLevel& level)
    : p_level_(&level)
{
}

void CreateWallTool::on_event(sf::Event event, glm::vec2 node, EditorState& state)
{
    const auto& default_props = state.wall_default;

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouse->button == sf::Mouse::Button::Left)
        {

            active_dragging_ = true;
            start_ = node;
            end_ = node;
            wall_preview_ = generate_wall_mesh(start_, end_, default_props.texture_side_1.value,
                                               default_props.texture_side_2.value);
            wall_preview_.buffer();
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        end_ = node;
        wall_preview_ = generate_wall_mesh(start_, end_, default_props.texture_side_1.value,
                                           default_props.texture_side_2.value);
        wall_preview_.buffer();
    }
    else if (event.is<sf::Event::MouseButtonReleased>())
    {
        active_dragging_ = false;
        if (glm::length(start_ - end_) > 0.25f)
        {
            state.p_active_object_ = &p_level_->add_wall({.start = start_, .end = end_});
        }
    }
}

void CreateWallTool::render_preview()
{
    if (active_dragging_)
    {
        wall_preview_.bind().draw_elements();
    }
}

void CreateWallTool::render_preview_2d(DrawingPad& drawing_pad)
{
    if (active_dragging_)
    {
        drawing_pad.render_line(start_, end_, {1, 0, 0, 1}, 4);
    }
}
