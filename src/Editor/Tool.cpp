#include "Tool.h"

#include <imgui.h>

#include "DrawingPad.h"
#include "WorldGeometry.h"
#include "Actions.h"

void CreateWallTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                              ActionManager& actions)
{
    const auto& default_props = state.wall_default;

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
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
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            active_dragging_ = false;
            if (glm::length(start_ - end_) > 0.25f)
            {
                actions.push_action(
                    std::make_unique<AddWallAction>(WallParameters{.start = start_, .end = end_}));
            }
            else
            {
                state.p_active_object_ = nullptr;
            }
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
