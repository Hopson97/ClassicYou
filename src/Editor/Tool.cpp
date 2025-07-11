#include "Tool.h"

#include <imgui.h>

#include "Actions.h"
#include "DrawingPad.h"
#include "LevelObjects.h"
#include "EditConstants.h"

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
            wall_preview_ = generate_wall_mesh({
                .properties = state.wall_default,
                .parameters = {.start = start_, .end = end_},
            });
            wall_preview_.buffer();
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        end_ = node;
        wall_preview_ = generate_wall_mesh({
            .properties = state.wall_default,
            .parameters = {.start = start_, .end = end_},
        });
        wall_preview_.buffer();
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            active_dragging_ = false;
            if (glm::length(start_ - end_) > 0.25f)
            {

                actions.push_action(std::make_unique<AddObjectAction>(WallObject{
                    .properties = state.wall_default,
                    .parameters = {.start = start_, .end = end_},
                }));
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
    if (active_dragging_ && wall_preview_.has_buffered())
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

void CreatePlatformTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                                  ActionManager& actions)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            actions.push_action(std::make_unique<AddObjectAction>(PlatformObject{
                .properties = state.platform_default,
                .parameters = {.position = node},
            }));
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        platform_preview_ = generate_platform_mesh({
            .properties = state.platform_default,
            .parameters = {.position = node},
        });
        platform_preview_.buffer();
        tile_ = node;
    }
}

void CreatePlatformTool::render_preview()
{
    if (platform_preview_.has_buffered())
    {
        platform_preview_.bind().draw_elements();
    }
}

void CreatePlatformTool::render_preview_2d(DrawingPad& drawing_pad)
{
    // TODO Pass in the default here
    drawing_pad.render_quad({tile_.x, tile_.y}, {TILE_SIZE, TILE_SIZE}, {1, 0, 0, 1});
}
