#include "Tool.h"

#include <print>

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "Actions.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "LevelObject.h"

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

                actions.push_action(std::make_unique<AddObjectAction>(
                    LevelObject{WallObject{
                        .properties = state.wall_default,
                        .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
                    }},
                    state.current_floor));
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

void CreateWallTool::render_preview_2d(DrawingPad& drawing_pad,
                                       [[maybe_unused]] const EditorState& state)
{
    if (active_dragging_)
    {
        drawing_pad.render_line(wall_line_.start, wall_line_.end, {1, 0, 0, 1}, 4);
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
    float TS = TILE_SIZE;

    if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            switch (object_type_)
            {
                case ObjectTypeName::Platform:
                    actions.push_action(std::make_unique<AddObjectAction>(
                        LevelObject{
                            PlatformObject{
                                .properties = state.platform_default,
                                .parameters = {.position = node},
                            },
                        },
                        state.current_floor));
                    break;

                case ObjectTypeName::PolygonPlatform:
                    actions.push_action(std::make_unique<AddObjectAction>(
                        LevelObject{
                            PolygonPlatformObject{
                                .properties = state.polygon_platform_default,
                                .parameters = {.corner_top_left = node,
                                               .corner_top_right = node + glm::vec2{10.0f, 0} * TS,
                                               .corner_bottom_right =
                                                   node + glm::vec2{10.0f, 10.0f} * TS,
                                               .corner_bottom_left = node + glm::vec2{0, 10.0f} * TS

                                },
                            },
                        },
                        state.current_floor));
                    break;

                default:
                    std::println("Missing implemention for CreateObjectTool for {}",
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
                                    .corner_top_right = node + glm::vec2{10.0f, 0} * TS,
                                    .corner_bottom_right = node + glm::vec2{10.0f, 10.0f} * TS,
                                    .corner_bottom_left = node + glm::vec2{0, 10.0f} * TS}},
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
            if (state.platform_default.style == PlatformStyle::Quad)
            {
                drawing_pad.render_quad(tile_,
                                        {TILE_SIZE * state.platform_default.width,
                                         TILE_SIZE * state.platform_default.depth},
                                        Colour::RED);
            }
            else if (state.platform_default.style == PlatformStyle::Diamond)
            {
                drawing_pad.render_diamond(tile_,
                                           {TILE_SIZE * state.platform_default.width,
                                            TILE_SIZE * state.platform_default.depth},
                                           Colour::RED);
            }
            break;

        case ObjectTypeName::PolygonPlatform:
        {
            float TS = TILE_SIZE;
            auto tl = tile_;
            auto tr = tile_ + glm::vec2{10.0f, 0} * TS;
            auto br = tile_ + glm::vec2{10.0f, 10.0f} * TS;
            auto bl = tile_ + glm::vec2{0, 10.0f} * TS;

            drawing_pad.render_line(tl, tl + glm::vec2(TILE_SIZE, 0), Colour::RED, 5);
            drawing_pad.render_line(tl, tl + glm::vec2(0, TILE_SIZE), Colour::RED, 5);

            drawing_pad.render_line(tr, tr - glm::vec2(TILE_SIZE, 0), Colour::RED, 5);
            drawing_pad.render_line(tr, tr + glm::vec2(0, TILE_SIZE), Colour::RED, 5);

            drawing_pad.render_line(br, br - glm::vec2(TILE_SIZE, 0), Colour::RED, 5);
            drawing_pad.render_line(br, br - glm::vec2(0, TILE_SIZE), Colour::RED, 5);

            drawing_pad.render_line(bl, bl + glm::vec2(TILE_SIZE, 0), Colour::RED, 5);
            drawing_pad.render_line(bl, bl - glm::vec2(0, TILE_SIZE), Colour::RED, 5);
        }
        break;

        default:
            std::println("Missing implemention for CreateObjectTool for {}",
                         magic_enum::enum_name(object_type_));
            break;
    }
}

ToolType CreateObjectTool::get_tool_type() const
{
    return ToolType();
}
