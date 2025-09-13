#include "Tool.h"

#include <print>

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "../Graphics/OpenGL/GLUtils.h"
#include "../Graphics/OpenGL/Shader.h"
#include "Actions.h"
#include "DrawingPad.h"
#include "EditConstants.h"
#include "EditorLevel.h"
#include "EditorState.h"

// =======================================
//          CreateWallTool
// =======================================
void CreateWallTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                              ActionManager& actions, const LevelTextures& drawing_pad_texture_map)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {

            active_dragging_ = true;
            wall_line_.start = node;
            wall_line_.end = node;
            update_previews(state, drawing_pad_texture_map);
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        wall_line_.end = node;
        update_previews(state, drawing_pad_texture_map);
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

void CreateWallTool::render_preview_2d_v2(gl::Shader& scene_shader_2d)
{
    glLineWidth(3);

    scene_shader_2d.set_uniform("use_texture", false);
    scene_shader_2d.set_uniform("is_selected", true);
    scene_shader_2d.set_uniform("on_floor_below", false);

    if (active_dragging_ && wall_preview_2d_.has_buffered())
    {
        wall_preview_2d_.bind().draw_elements(gl::PrimitiveType::Lines);
    }
}

void CreateWallTool::render_preview_2d(DrawingPad& drawing_pad,
                                       [[maybe_unused]] const EditorState& state)
{
    if (active_dragging_)
    {
        render_object_2d(
            WallObject{.properties = state.wall_default, .parameters = {.line = wall_line_}},
            drawing_pad, Colour::RED);
    }
}

ToolType CreateWallTool::get_tool_type() const
{
    return ToolType::CreateWall;
}

void CreateWallTool::update_previews(const EditorState& state,
                                     const LevelTextures& drawing_pad_texture_map)
{
    WallObject wall{
        .properties = state.wall_default,
        .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
    };

    wall_preview_ = object_to_geometry(wall, state.current_floor);
    wall_preview_.update();

    wall_preview_2d_ = object_to_geometry_2d(wall, drawing_pad_texture_map).first;
    wall_preview_2d_.update();
}

// =======================================
//          UpdateWallTool
// =======================================
UpdateWallTool::UpdateWallTool(LevelObject object, WallObject& wall, int wall_floor)
    : object_(object)
    , wall_{wall}
    , wall_floor_(wall_floor)
{
}

void UpdateWallTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                              ActionManager& actions, const LevelTextures& drawing_pad_texture_map)
{
    // Walls should only be edited on the same floor
    if (state.current_floor != wall_floor_)
    {
        return;
    }

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
                wall_preview_.update();
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
            wall_preview_.update();
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

void UpdateWallTool::render_preview_2d(DrawingPad& drawing_pad, const EditorState& state)
{
    if (state.current_floor == wall_floor_)
    {
        constexpr static glm::vec2 OFFSET{16, 16};
        drawing_pad.render_quad(wall_.parameters.line.start - OFFSET, glm::vec2{32.0f},
                                Colour::RED);
        drawing_pad.render_quad(wall_.parameters.line.end - OFFSET, glm::vec2{32.0f}, Colour::RED);

        if (active_dragging_)
        {
            drawing_pad.render_line(wall_line_.start, wall_line_.end, {1, 0.5, 0.5, 0.75}, 4);

            drawing_pad.render_quad(wall_line_.start - OFFSET, glm::vec2{32.0f}, Colour::RED);
            drawing_pad.render_quad(wall_line_.end - OFFSET, glm::vec2{32.0f}, Colour::RED);
        }
    }
}

ToolType UpdateWallTool::get_tool_type() const
{
    return ToolType::UpdateWall;
}

// =======================================
//          CreateObjectTool
// =======================================
CreateObjectTool::CreateObjectTool(ObjectTypeName object_type)
    : object_type_(object_type)
{
}

void CreateObjectTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                                ActionManager& actions,
                                const LevelTextures& drawing_pad_texture_map)
{
    tile_ = node;
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
                    actions.push_action(std::make_unique<AddObjectAction>(
                        LevelObject{
                            PillarObject{
                                .properties = state.pillar_default,
                                .parameters = {.position = node},
                            },
                        },
                        state.current_floor));
                    break;

                case ObjectTypeName::Ramp:
                    actions.push_action(std::make_unique<AddObjectAction>(
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
        object_preview_.update();
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
                drawing_pad, Colour::RED);
            break;
        case ObjectTypeName::PolygonPlatform:
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
                drawing_pad, Colour::RED);

            break;

        case ObjectTypeName::Pillar:
            render_object_2d(
                PillarObject{
                    .properties = state.pillar_default,
                    .parameters = {.position = tile_},
                },
                drawing_pad, Colour::RED);
            break;

        case ObjectTypeName::Ramp:
            render_object_2d(
                RampObject{
                    .properties = state.ramp_default,
                    .parameters = {.position = tile_},
                },
                drawing_pad, Colour::RED);
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

// =======================================
//          AreaSelectTool
// =======================================
AreaSelectTool::AreaSelectTool(EditorLevel& level)
    : p_level_(&level)
{
}

void AreaSelectTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                              ActionManager& actions, const LevelTextures& drawing_pad_texture_map)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            state.selection.clear_selection();
            start_floor_ = state.current_floor;
            max_floor_ = state.current_floor;
            min_floor_ = state.current_floor;

            active_dragging_ = true;
            render_preview_mesh_ = true;
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
            render_preview_mesh_ = false;
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
            select(state);

            // Ensure the selection area is reset when nothing is selected
            if (state.selection.objects.size() == 0)
            {
                selection_area_ = Line{};
            }
        }
    }
}

void AreaSelectTool::render_preview()
{

    auto start = glm::ivec2{selection_area_.start};
    auto end = glm::ivec2{selection_area_.end};
    if (glm::length2(glm::vec2{end - start}) < HALF_TILE_SIZE_F * HALF_TILE_SIZE_F)
    {
        return;
    }

    // Ensure start is less than end
    if (start.x > end.x)
    {
        std::swap(start.x, end.x);
    }
    if (start.y > end.y)
    {
        std::swap(start.y, end.y);
    }

    glm::ivec3 cube_start{start.x / TILE_SIZE, min_floor_ * FLOOR_HEIGHT, start.y / TILE_SIZE};
    glm::ivec3 cube_end{end.x / TILE_SIZE, max_floor_ * FLOOR_HEIGHT + FLOOR_HEIGHT + 0.01,
                        end.y / TILE_SIZE};

    auto size = cube_end - selection_cube_start_;

    if (cube_start != selection_cube_start_ || size != selection_cube_size_)
    {
        selection_cube_start_ = cube_start;
        selection_cube_size_ = size;

        // 16 is the id for a "blank" feature
        selection_cube_ = generate_cube_mesh_level(selection_cube_start_, selection_cube_size_, 16,
                                                   {255, 255, 255, 150});

        selection_cube_.update();
    }

    gl::enable(gl::Capability::Blend);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    selection_cube_.bind().draw_elements();

    gl::disable(gl::Capability::Blend);
}

void AreaSelectTool::render_preview_2d(DrawingPad& drawing_pad, const EditorState& state)
{
    if (active_dragging_)
    {
        drawing_pad.render_quad(selection_area_.start, selection_area_.end - selection_area_.start,
                                Colour::RED);
    }
}

ToolType AreaSelectTool::get_tool_type() const
{
    return ToolType::AreaSelectTool;
}

void AreaSelectTool::show_gui(EditorState& state)
{

    if (state.selection.objects.size() > 0)
    {

        if (ImGui::Begin("Selection Options"))
        {
            ImGui::Text("Selected %u objects.", state.selection.objects.size());

            bool update = false;
            update |= ImGui::SliderInt("Max floors selection", &max_floor_, start_floor_,
                                       p_level_->get_max_floor());
            update |= ImGui::SliderInt("Minimum floors selection", &min_floor_,
                                       p_level_->get_min_floor(), start_floor_);

            if (update)
            {
                select(state);
            }
        }
        ImGui::End();
    }
}

void AreaSelectTool::select(EditorState& state)
{
    state.selection.clear_selection();
    for (int floor = min_floor_; floor <= max_floor_; floor++)
    {
        p_level_->select_within(selection_area_.to_bounds(), state.selection, floor);
    }
}
