#include "Tool.h"

#include <imgui.h>

#include "../../Graphics/Camera.h"
#include "../../Graphics/MeshGeneration.h"
#include "../../Graphics/OpenGL/GLUtils.h"
#include "../../Graphics/OpenGL/Shader.h"
#include "../Actions.h"
#include "../EditConstants.h"
#include "../EditorLevel.h"
#include "../EditorState.h"
#include "../EditorUtils.h"
#include "../LevelTextures.h"

namespace
{
    constexpr glm::vec2 WALL_NODE_ICON_SIZE{8, 8};
    constexpr float SELECT_AREA_SIZE = 16.0f;
    constexpr float MIN_SELECT_DISTANCE = SELECT_AREA_SIZE / 2.0f;

    void render_wall(bool should_draw, const Mesh2DWorld& wall_mesh, gl::Shader& scene_shader_2d)
    {
        if (should_draw && wall_mesh.has_buffered())
        {
            glLineWidth(3);

            scene_shader_2d.set_uniform("use_texture", false);
            scene_shader_2d.set_uniform("is_selected", true);
            scene_shader_2d.set_uniform("on_floor_below", false);

            wall_mesh.bind().draw_elements(gl::PrimitiveType::Lines);
        }
    }
} // namespace

// =======================================
//          CreateWallTool
// =======================================
CreateWallTool::CreateWallTool(const LevelTextures& drawing_pad_texture_map)
{
    vertex_selector_mesh_ = generate_2d_quad_mesh(
        {0, 0}, WALL_NODE_ICON_SIZE,
        static_cast<float>(*drawing_pad_texture_map.get_texture("Selection")));
    vertex_selector_mesh_.buffer();
}

bool CreateWallTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map, const Camera& camera_3d,
                              bool mouse_in_2d_view)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if (mouse_in_2d_view)
            {
                wall_line_.start = state.node_hovered;
                wall_line_.end = state.node_hovered;
                active_dragging_ = true;
            }
            else
            {
                wall_line_.start = get_mouse_floor_snapped_intersect(camera_3d, mouse->position,
                                                                     state.current_floor);
                wall_line_.end = wall_line_.start;
                active_dragging_3d_ = true;
            }
            update_previews(state, drawing_pad_texture_map);
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        tile_hovered_ = mouse_in_2d_view ? glm::vec2{state.node_hovered}
                                         : get_mouse_floor_snapped_intersect(
                                               camera_3d, mouse->position, state.current_floor);

        wall_line_.end = tile_hovered_;
        update_previews(state, drawing_pad_texture_map);
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {

        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left &&
            (active_dragging_ || active_dragging_3d_))
        {
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
        active_dragging_ = false;
        active_dragging_3d_ = false;
    }
    return false;
}

void CreateWallTool::render_preview([[maybe_unused]] bool always_show_gizmos)
{
    if ((active_dragging_ || active_dragging_3d_) && wall_preview_.has_buffered())
    {
        wall_preview_.bind().draw_elements();
    }

    if (selection_mesh_.has_buffered())
    {
        selection_mesh_.bind().draw_elements();
    }
}

void CreateWallTool::render_preview_2d(gl::Shader& scene_shader_2d)
{
    render_wall(active_dragging_ || active_dragging_3d_, wall_preview_2d_, scene_shader_2d);

    scene_shader_2d.set_uniform("use_texture", true);
    scene_shader_2d.set_uniform("is_selected", false);
    scene_shader_2d.set_uniform("on_floor_below", false);
    scene_shader_2d.set_uniform("use_texture_alpha_channel", true);
    scene_shader_2d.set_uniform("use_world_texture", false);

    scene_shader_2d.set_uniform(
        "model_matrix",
        create_model_matrix(
            {.position = glm::vec3{tile_hovered_, 0} - glm::vec3(WALL_NODE_ICON_SIZE / 2.0f, 0)}));
    vertex_selector_mesh_.bind().draw_elements();
}

ToolType CreateWallTool::get_tool_type() const
{
    return ToolType::CreateWall;
}

void CreateWallTool::cancel_events()
{
    active_dragging_ = false;
    active_dragging_3d_ = false;
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

    auto selection_cube_start =
        glm::vec3{tile_hovered_.x / TILE_SIZE_F,
                  state.wall_default.start_base_height * 2 + state.current_floor * FLOOR_HEIGHT,
                  tile_hovered_.y / TILE_SIZE_F};

    selection_mesh_ = generate_cube_mesh_level(selection_cube_start,
                                               {0.1, state.wall_default.end_height * 2, 0.1}, 16);
    selection_mesh_.update();
}

// =======================================
//          UpdateWallTool
// =======================================
UpdateWallTool::UpdateWallTool(LevelObject object, WallObject& wall, int wall_floor,
                               const LevelTextures& drawing_pad_texture_map)
    : object_(object)
    , wall_{wall}
    , wall_floor_(wall_floor)
{
    vertex_selector_mesh_ = generate_2d_quad_mesh(
        {0, 0}, {SELECT_AREA_SIZE, SELECT_AREA_SIZE},
        static_cast<float>(*drawing_pad_texture_map.get_texture("SelectCircle")));
    vertex_selector_mesh_.buffer();

    wall_line_ = wall.parameters.line;

    update_3d_previews(wall);
}

bool UpdateWallTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map, const Camera& camera_3d,
                              bool mouse_in_2d_view)
{
    state_floor_ = state.current_floor;
    // Walls should only be edited on the same floor
    if (state.current_floor != wall_floor_)
    {
        return false;
    }

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if (drag_target_ != DragTarget::None)
            {
                active_dragging_ = true;

                wall_line_.start = wall_.parameters.line.start;
                wall_line_.end = wall_.parameters.line.end;

                update_previews(state, drawing_pad_texture_map);
                return true;
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {

        if (!active_dragging_)
        {
            if (!mouse_over_edge_3d_)
            {
                drag_target_ = DragTarget::None;
            }

            auto tile_hovered = state.world_position_hovered;
            if (glm::distance(tile_hovered, wall_.parameters.line.start) < MIN_SELECT_DISTANCE)
            {
                drag_target_ = DragTarget::Start;
            }
            else if (glm::distance(tile_hovered, wall_.parameters.line.end) < MIN_SELECT_DISTANCE)
            {
                drag_target_ = DragTarget::End;
            }
        }

        if (active_dragging_)
        {
            auto tile_hovered = mouse_in_2d_view
                                    ? glm::vec2{state.node_hovered}
                                    : get_mouse_floor_snapped_intersect(camera_3d, mouse->position,
                                                                        state.current_floor);
            switch (drag_target_)
            {
                case UpdateWallTool::DragTarget::Start:
                    wall_line_.start = tile_hovered;
                    break;

                case UpdateWallTool::DragTarget::End:
                    wall_line_.end = tile_hovered;
                    break;
                default:
                    break;
            }
            update_previews(state, drawing_pad_texture_map);
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
    return false;
}

void UpdateWallTool::render_preview(bool always_show_gizmos)
{
    if (wall_preview_.has_buffered() && active_dragging_)
    {
        // Use polygon offset to prevent Z-Fighting the existing wall
        gl::enable(gl::Capability::PolygonOffsetFill);
        glPolygonOffset(-1.0f, -1.0f);

        wall_preview_.bind().draw_elements();

        gl::disable(gl::Capability::PolygonOffsetFill);
    }

    auto render_wall_line = [&](Mesh3D& mesh, DragTarget required_target)
    {
        if (mesh.has_buffered())
        {
            if (drag_target_ == required_target || always_show_gizmos)
            {
                glLineWidth(drag_target_ == required_target ? 5 : 3);

                mesh.bind().draw_elements(gl::PrimitiveType::Lines);
            }
        }
    };
    render_wall_line(wall_begin_preview_3d_, DragTarget::Start);
    render_wall_line(wall_end_preview_3d_, DragTarget::End);

    glLineWidth(2);
}

void UpdateWallTool::render_preview_2d(gl::Shader& scene_shader_2d)
{
    auto draw_selection_point = [&](const glm::vec2& position, float scale = 1.0f)
    {
        scene_shader_2d.set_uniform(
            "model_matrix",
            create_model_matrix(
                {.position = glm::vec3{position - glm::vec2{MIN_SELECT_DISTANCE * scale}, 0.0},
                 .scale = glm::vec3{scale}}));

        vertex_selector_mesh_.bind().draw_elements();
    };

    if (state_floor_ == wall_floor_)
    {
        render_wall(active_dragging_, wall_preview_2d_, scene_shader_2d);

        scene_shader_2d.set_uniform("use_texture", true);
        scene_shader_2d.set_uniform("use_world_texture", false);
        scene_shader_2d.set_uniform("use_texture_alpha_channel", true);
        scene_shader_2d.set_uniform("is_selected", false);

        glm::vec3 start{wall_line_.start - MIN_SELECT_DISTANCE, 0};
        glm::vec3 end{wall_line_.end - MIN_SELECT_DISTANCE, 0};

        draw_selection_point(wall_line_.start, drag_target_ == DragTarget::Start ? 1.5f : 1.0f);
        draw_selection_point(wall_line_.end, drag_target_ == DragTarget::End ? 1.5f : 1.0f);
    }
}

ToolType UpdateWallTool::get_tool_type() const
{
    return ToolType::UpdateWall;
}

void UpdateWallTool::render_to_picker_mouse_over(const MousePickingState& picker_state,
                                                 gl::Shader& picker_shader)
{
    if ((state_floor_ != wall_floor_) || active_dragging_)
    {
        return;
    }

    picker_shader.set_uniform("model_matrix", create_model_matrix({}));

    // A thicker line is used to ensure easier selection
    glLineWidth(15);

    picker_shader.set_uniform("object_id", (int)DragTarget::Start);
    wall_begin_preview_3d_.bind().draw_elements(gl::PrimitiveType::Lines);

    picker_shader.set_uniform("object_id", (int)DragTarget::End);
    wall_end_preview_3d_.bind().draw_elements(gl::PrimitiveType::Lines);

    // Check which one was picked
    GLint picked_id = 0;
    glReadPixels(picker_state.point.x, picker_state.point.y, 1, 1, GL_RED_INTEGER, GL_INT,
                 &picked_id);

    mouse_over_edge_3d_ = false;
    if (picked_id > -1)
    {
        mouse_over_edge_3d_ = true;
        drag_target_ = static_cast<DragTarget>(picked_id);
    }
    glLineWidth(2);
}

void UpdateWallTool::update_previews(const EditorState& state,
                                     const LevelTextures& drawing_pad_texture_map)
{
    WallObject wall{
        .properties = wall_.properties,
        .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
    };
    update_3d_previews(wall);

    wall_preview_ = object_to_geometry(wall, state.current_floor);
    wall_preview_.update();

    wall_preview_2d_ = object_to_geometry_2d(wall, drawing_pad_texture_map).first;
    wall_preview_2d_.update();
}

void UpdateWallTool::update_3d_previews(const WallObject& wall)
{

    auto [begin, top] = wall_to_lines(wall, wall_floor_);
    generate_line_mesh(wall_begin_preview_3d_, {begin.start, top.start}, Colour::MAGENTA);
    generate_line_mesh(wall_end_preview_3d_, {begin.end, top.end}, Colour::MAGENTA);
    wall_begin_preview_3d_.update();
    wall_end_preview_3d_.update();
}