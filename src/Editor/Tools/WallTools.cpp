#include "Tool.h"

#include <imgui.h>

#include "../../Graphics/OpenGL/GLUtils.h"
#include "../../Graphics/OpenGL/Shader.h"
#include "../Actions.h"
#include "../EditConstants.h"
#include "../EditorLevel.h"
#include "../EditorState.h"
#include "../LevelTextures.h"

namespace
{
    constexpr glm::vec2 WALL_NODE_ICON_SIZE{8, 8};

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
    selection_node_ = generate_2d_quad_mesh(
        {0, 0}, WALL_NODE_ICON_SIZE,
        static_cast<float>(*drawing_pad_texture_map.get_texture("Selection")));
    selection_node_.buffer();
}

void CreateWallTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map)
{
    selected_node_ = state.node_hovered;
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {

            active_dragging_ = true;
            wall_line_.start = state.node_hovered;
            wall_line_.end = state.node_hovered;
            update_previews(state, drawing_pad_texture_map);
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        wall_line_.end = state.node_hovered;
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

void CreateWallTool::render_preview_2d(gl::Shader& scene_shader_2d)
{
    render_wall(active_dragging_, wall_preview_2d_, scene_shader_2d);

    scene_shader_2d.set_uniform("use_texture", true);
    scene_shader_2d.set_uniform("is_selected", false);
    scene_shader_2d.set_uniform("on_floor_below", false);
    scene_shader_2d.set_uniform("use_texture_alpha_channel", true);
    scene_shader_2d.set_uniform("use_world_texture", false);

    scene_shader_2d.set_uniform(
        "model_matrix",
        create_model_matrix(
            {.position = glm::vec3{selected_node_, 0} - glm::vec3(WALL_NODE_ICON_SIZE / 2.0f, 0)}));

    selection_node_.bind().draw_elements();
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
UpdateWallTool::UpdateWallTool(LevelObject object, WallObject& wall, int wall_floor,
                               const LevelTextures& drawing_pad_texture_map)
    : object_(object)
    , wall_{wall}
    , wall_floor_(wall_floor)
{
    edge_mesh_ = generate_2d_quad_mesh(
        {0, 0}, {32, 32}, static_cast<float>(*drawing_pad_texture_map.get_texture("SelectCircle")));
    edge_mesh_.buffer();

    wall_line_ = wall.parameters.line;
}

void UpdateWallTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map)
{
    state_floor_ = state.current_floor;
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
            if (glm::distance(state.world_position_hovered, wall_.parameters.line.start) <
                MIN_DISTANCE)
            {
                active_dragging_ = true;
                target_ = DragTarget::Start;
            }
            else if (glm::distance(state.world_position_hovered, wall_.parameters.line.end) <
                     MIN_DISTANCE)
            {
                active_dragging_ = true;
                target_ = DragTarget::End;
            }

            if (active_dragging_)
            {
                wall_line_.start = wall_.parameters.line.start;
                wall_line_.end = wall_.parameters.line.end;

                update_previews(state, drawing_pad_texture_map);
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
                    wall_line_.start = state.node_hovered;
                    break;

                case UpdateWallTool::DragTarget::End:
                    wall_line_.end = state.node_hovered;
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
}

void UpdateWallTool::render_preview()
{
    if (wall_preview_.has_buffered() && active_dragging_)
    {
        // Use polygon offset to prevent Z-Fighting the existing wall
        gl::enable(gl::Capability::PolygonOffsetFill);
        glPolygonOffset(-1.0f, -1.0f);

        wall_preview_.bind().draw_elements();

        gl::disable(gl::Capability::PolygonOffsetFill);
    }
}

constexpr glm::vec2 OFFSET{16, 16};
void UpdateWallTool::render_preview_2d(gl::Shader& scene_shader_2d)
{
    if (state_floor_ == wall_floor_)
    {
        render_wall(active_dragging_, wall_preview_2d_, scene_shader_2d);

        scene_shader_2d.set_uniform("use_texture", true);
        scene_shader_2d.set_uniform("use_world_texture", false);
        scene_shader_2d.set_uniform("use_texture_alpha_channel", true);
        scene_shader_2d.set_uniform("is_selected", false);

        glm::vec3 start{wall_line_.start - OFFSET, 0};
        glm::vec3 end{wall_line_.end - OFFSET, 0};

        scene_shader_2d.set_uniform("model_matrix", create_model_matrix({.position = start}));
        edge_mesh_.bind().draw_elements();

        scene_shader_2d.set_uniform("model_matrix", create_model_matrix({.position = end}));
        edge_mesh_.bind().draw_elements();
    }
}

ToolType UpdateWallTool::get_tool_type() const
{
    return ToolType::UpdateWall;
}

void UpdateWallTool::update_previews(const EditorState& state,
                                     const LevelTextures& drawing_pad_texture_map)
{
    WallObject wall{
        .properties = wall_.properties,
        .parameters = {Line{.start = wall_line_.start, .end = wall_line_.end}},
    };

    wall_preview_ = object_to_geometry(wall, state.current_floor);
    wall_preview_.update();

    wall_preview_2d_ = object_to_geometry_2d(wall, drawing_pad_texture_map).first;
    wall_preview_2d_.update();
}