#include "Tool.h"

#include <print>

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "Actions.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "EditorLevel.h"
#include "EditorState.h"

void CreateWallTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                              ActionManager& actions)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {

            active_dragging_ = true;
            wall_line_.start = node;
            wall_line_.end = node;
            wall_preview_ = generate_wall_mesh(
                {
                    .properties = state.wall_default,
                    .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
                },
                state.current_floor);
            wall_preview_.buffer();
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        wall_line_.end = node;
        wall_preview_ = generate_wall_mesh(
            {
                .properties = state.wall_default,
                .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
            },
            state.current_floor);
        wall_preview_.buffer();
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            active_dragging_ = false;
            if (glm::length(wall_line_.start - wall_line_.end) > 0.25f)
            {
                actions.push_action(std::make_unique<AddObjectActionV2>(
                    LevelObject{WallObject{
                        .properties = state.wall_default,
                        .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
                    }},
                    state.current_floor));
            }
            else
            {
                state.selection.clear_selection();
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

void CreateWallTool::render_preview_2d(DrawingPad& drawing_pad,
                                       [[maybe_unused]] const EditorState& state)
{
    if (active_dragging_)
    {
        render_object_2d(
            WallObject{.properties = state.wall_default, .parameters = {.line = wall_line_}},
            drawing_pad, Colour::RED, true);
    }
}

ToolType CreateWallTool::get_tool_type() const
{
    return ToolType::CreateWall;
}

UpdateWallTool::UpdateWallTool(LevelObject object, WallObject& wall)
    : object_(object)
    , wall_{wall}
{
}

void UpdateWallTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                              ActionManager& actions)
{
    const float MIN_DISTANCE = 32.0f;

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if (glm::distance(node, wall_.parameters.line.start) < MIN_DISTANCE)
            {
                active_dragging_ = true;
                target_ = DragTarget::Start;
            }
            else if (glm::distance(node, wall_.parameters.line.end) < MIN_DISTANCE)
            {
                active_dragging_ = true;
                target_ = DragTarget::End;
            }

            if (active_dragging_)
            {
                wall_line_.start = wall_.parameters.line.start;
                wall_line_.end = wall_.parameters.line.end;

                wall_preview_ = generate_wall_mesh(wall_, state.current_floor);
                wall_preview_.buffer();
            }
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (active_dragging_)
        {

            switch (target_)
            {
                case UpdateWallTool::DragTarget::Start:
                    wall_line_.start = node;
                    break;

                case UpdateWallTool::DragTarget::End:
                    wall_line_.end = node;
                    break;
                default:
                    break;
            }

            wall_preview_ = generate_wall_mesh(
                {
                    .properties = wall_.properties,
                    .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
                },
                state.current_floor);
            wall_preview_.buffer();
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left &&
            active_dragging_)
        {
            auto new_object = object_;
            auto& new_wall = std::get<WallObject>(new_object.object_type);
            new_wall.parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}};

            actions.push_action(
                std::make_unique<UpdateObjectAction>(object_, new_object, state.current_floor));

            object_ = new_object;
            wall_ = new_wall;

            active_dragging_ = false;
        }
    }
}

void UpdateWallTool::render_preview()
{
    if (wall_preview_.has_buffered() && active_dragging_)
    {
        wall_preview_.bind().draw_elements();
    }
}

void UpdateWallTool::render_preview_2d(DrawingPad& drawing_pad,
                                       [[maybe_unused]] const EditorState& state)
{

    constexpr static glm::vec2 OFFSET{8, 8};
    drawing_pad.render_quad(wall_.parameters.line.start - OFFSET, glm::vec2{16.0f}, Colour::RED);
    drawing_pad.render_quad(wall_.parameters.line.end - OFFSET, glm::vec2{16.0f}, Colour::RED);

    if (active_dragging_)
    {
        drawing_pad.render_line(wall_line_.start, wall_line_.end, {1, 0.5, 0.5, 0.75}, 4);

        drawing_pad.render_quad(wall_line_.start - OFFSET, glm::vec2{16.0f}, Colour::RED);
        drawing_pad.render_quad(wall_line_.end - OFFSET, glm::vec2{16.0f}, Colour::RED);
    }
}

ToolType UpdateWallTool::get_tool_type() const
{
    return ToolType::UpdateWall;
}

CreateObjectTool::CreateObjectTool(ObjectTypeName object_type)
    : object_type_(object_type)
{
}

void CreateObjectTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                                ActionManager& actions)
{

    if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            switch (object_type_)
            {
                case ObjectTypeName::Platform:
                    actions.push_action(std::make_unique<AddObjectActionV2>(
                        LevelObject{
                            PlatformObject{
                                .properties = state.platform_default,
                                .parameters = {.position = node},
                            },
                        },
                        state.current_floor));
                    break;

                case ObjectTypeName::PolygonPlatform:
                    actions.push_action(std::make_unique<AddObjectActionV2>(
                        LevelObject{
                            PolygonPlatformObject{
                                .properties = state.polygon_platform_default,
                                .parameters = {.corner_top_left = node,
                                               .corner_top_right =
                                                   node + glm::vec2{10.0f, 0} * TILE_SIZE_F,
                                               .corner_bottom_right =
                                                   node + glm::vec2{10.0f, 10.0f} * TILE_SIZE_F,
                                               .corner_bottom_left =
                                                   node + glm::vec2{0, 10.0f} * TILE_SIZE_F

                                },
                            },
                        },
                        state.current_floor));
                    break;

                case ObjectTypeName::Pillar:
                    actions.push_action(std::make_unique<AddObjectActionV2>(
                        LevelObject{
                            PillarObject{
                                .properties = state.pillar_default,
                                .parameters = {.position = node},
                            },
                        },
                        state.current_floor));
                    break;

                case ObjectTypeName::Ramp:
                    actions.push_action(std::make_unique<AddObjectActionV2>(
                        LevelObject{
                            RampObject{
                                .properties = state.ramp_default,
                                .parameters = {.position = node},
                            },
                        },
                        state.current_floor));
                    break;

                default:
                    std::println("Missing implementation for CreateObjectTool for {}",
                                 magic_enum::enum_name(object_type_));
                    break;
            }
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        switch (object_type_)
        {
            case ObjectTypeName::Platform:
                object_preview_ = generate_platform_mesh(
                    {
                        .properties = state.platform_default,
                        .parameters = {.position = node},
                    },
                    state.current_floor);
                break;

            case ObjectTypeName::PolygonPlatform:
                object_preview_ = generate_polygon_platform_mesh(
                    {.properties = state.polygon_platform_default,
                     .parameters = {.corner_top_left = node,
                                    .corner_top_right = node + glm::vec2{10.0f, 0} * TILE_SIZE_F,
                                    .corner_bottom_right =
                                        node + glm::vec2{10.0f, 10.0f} * TILE_SIZE_F,
                                    .corner_bottom_left =
                                        node + glm::vec2{0, 10.0f} * TILE_SIZE_F}},
                    state.current_floor);
                break;

            case ObjectTypeName::Pillar:
                object_preview_ = generate_pillar_mesh(
                    PillarObject{
                        .properties = state.pillar_default,
                        .parameters = {.position = node},
                    },
                    state.current_floor);
                break;

            case ObjectTypeName::Ramp:
                object_preview_ = generate_ramp_mesh(
                    RampObject{
                        .properties = state.ramp_default,
                        .parameters = {.position = node},
                    },
                    state.current_floor);
                break;

            default:
                std::println("Missing implemention for CreateObjectTool mouse move for {}",
                             magic_enum::enum_name(object_type_));
                break;
        }
        object_preview_.buffer();
        tile_ = node;
    }
}

void CreateObjectTool::render_preview()
{
    if (object_preview_.has_buffered())
    {
        object_preview_.bind().draw_elements();
    }
}

void CreateObjectTool::render_preview_2d(DrawingPad& drawing_pad, const EditorState& state)
{
    switch (object_type_)
    {
        case ObjectTypeName::Platform:

            render_object_2d(
                PlatformObject{
                    .properties = state.platform_default,
                    .parameters = {.position = tile_},

                },
                drawing_pad, Colour::RED, true);
            break;
        case ObjectTypeName::PolygonPlatform:
        {
            render_object_2d(
                PolygonPlatformObject{
                    .parameters =
                        {
                            .corner_top_left = tile_,
                            .corner_top_right = tile_ + glm::vec2{10.0f, 0} * TILE_SIZE_F,
                            .corner_bottom_right = tile_ + glm::vec2{10.0f, 10.0f} * TILE_SIZE_F,
                            .corner_bottom_left = tile_ + glm::vec2{0, 10.0f} * TILE_SIZE_F,
                        },
                },
                drawing_pad, Colour::RED, true);
        }
        break;

        case ObjectTypeName::Pillar:
            render_object_2d(
                PillarObject{
                    .properties = state.pillar_default,
                    .parameters = {.position = tile_},
                },
                drawing_pad, Colour::RED, true);
            break;

        case ObjectTypeName::Ramp:
            render_object_2d(
                RampObject{
                    .properties = state.ramp_default,
                    .parameters = {.position = tile_},
                },
                drawing_pad, Colour::RED, true);
            break;

        default:
            std::println("Missing implementation for CreateObjectTool for {}",
                         magic_enum::enum_name(object_type_));
            break;
    }
}

ToolType CreateObjectTool::get_tool_type() const
{
    return ToolType::CreateObject;
}

/*

SelectTool::SelectTool(EditorLevel& level)
    : p_level_(&level)
{
}

void SelectTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                          ActionManager& actions)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            selected_objects_.clear();
            active_dragging_ = true;
            selection_area_.start = node;
            selection_area_.end = node;
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (active_dragging_)
        {
            selection_area_.end = node;
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            active_dragging_ = false;
            auto start = glm::ivec2{selection_area_.start};
            auto end = glm::ivec2{selection_area_.end};

            // Ensure start is less than end
            if (start.x > end.x)
            {
                std::swap(start.x, end.x);
            }
            if (start.y > end.y)
            {
                std::swap(start.y, end.y);
            }

            p_level_->try_select_all(selection_area_.to_bounds(), state.current_floor,
                                     selected_objects_);

            std::println("Selected {}", selected_objects_.size());
        }
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Right)
        {
            auto selection = p_level_->try_select(node, nullptr, state.current_floor);

            if (selection)
            {
                if (selected_objects_.find(selection) != selected_objects_.end())
                {
                    selected_objects_.erase(selection);
                }
                else
                {
                    selected_objects_.emplace(selection);
                }
            }
        }
        std::println("Selected {}", selected_objects_.size());
    }
}

void SelectTool::render_preview()
{
}

void SelectTool::render_preview_2d(DrawingPad& drawing_pad, const EditorState& state)
{
    if (active_dragging_)
    {
        drawing_pad.render_quad(selection_area_.start, selection_area_.end - selection_area_.start,
                                Colour::RED);
    }
    else if (!selected_objects_.empty())
    {
        for (auto object : selected_objects_)
        {
            object->render_2d(drawing_pad, object, true);
        }
    }
}

void SelectTool::move_all(glm::vec2 offset, ActionManager& actions, int floor)
{
    for (auto object : selected_objects_)
    {
        auto new_object = *object;
        new_object.move(offset);

        actions.push_action(std::make_unique<UpdateObjectAction>(*object, new_object, floor),
                            false);
    }
}

bool SelectTool::has_selection() const
{
    return !selected_objects_.empty();
}
*/