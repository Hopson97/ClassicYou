#include "Tool.h"

#include <print>

#include <imgui.h>

#include "../../Graphics/MeshGeneration.h"
#include "../../Graphics/OpenGL/GLUtils.h"
#include "../../Graphics/OpenGL/Shader.h"
#include "../Actions.h"
#include "../EditConstants.h"
#include "../EditorLevel.h"
#include "../EditorState.h"

AreaSelectTool::AreaSelectTool(EditorLevel& level)
    : p_level_(&level)
{
}

bool AreaSelectTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map,
                              [[maybe_unused]] bool mouse_in_2d_view)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            start_floor_ = state.current_floor;
            max_floor_ = state.current_floor;
            min_floor_ = state.current_floor;

            active_dragging_ = true;
            selection_area_.start = state.node_hovered;
            selection_area_.end = state.node_hovered;
            update_previews();
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (active_dragging_)
        {
            selection_area_.end = state.node_hovered;
            update_previews();
        }

        glm::vec3 selection_cube_start{
            state.node_hovered.x / TILE_SIZE_F,
            state.current_floor * FLOOR_HEIGHT,
            state.node_hovered.y / TILE_SIZE_F,
        };
        selection_mesh_ = generate_cube_mesh_level(selection_cube_start, {0.1f, 1.0f, 0.1f}, 16);
        selection_mesh_.update();
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
            select(state);

            // Ensure the selection area is reset when nothing is selected
            if (state.selection.objects.size() == 0)
            {
                selection_area_ = Line{};
            }
        }
    }
    return false;
}

void AreaSelectTool::render_preview()
{
    if (selection_quad_.has_buffered() && render_preview_mesh_)
    {
        gl::enable(gl::Capability::Blend);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        selection_cube_.bind().draw_elements();

        gl::disable(gl::Capability::Blend);
    }

    if (selection_mesh_.has_buffered())
    {
        selection_mesh_.bind().draw_elements();
    }
}

void AreaSelectTool::render_preview_2d(gl::Shader& scene_shader_2d)
{
    if (active_dragging_ && selection_quad_.has_buffered() && render_preview_mesh_)
    {
        scene_shader_2d.set_uniform("use_texture", false);
        scene_shader_2d.set_uniform("is_selected", true);
        scene_shader_2d.set_uniform("on_floor_below", false);
        selection_quad_.bind().draw_elements(gl::PrimitiveType::Lines);
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
            ImGui::Text("Selected %zu objects.", state.selection.objects.size());

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
    update_previews();
}

void AreaSelectTool::update_previews()
{
    auto start = glm::vec2{selection_area_.start};
    auto end = glm::vec2{selection_area_.end};
    if (glm::length2(glm::vec2{end - start}) < HALF_TILE_SIZE_F * HALF_TILE_SIZE_F)
    {
        render_preview_mesh_ = false;
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

    glm::vec3 cube_start{start.x / TILE_SIZE_F, min_floor_ * FLOOR_HEIGHT, start.y / TILE_SIZE_F};
    glm::vec3 cube_end{end.x / TILE_SIZE_F, max_floor_ * FLOOR_HEIGHT + FLOOR_HEIGHT + 0.01,
                       end.y / TILE_SIZE_F};

    glm::vec3 current_size = cube_end - cube_start;
    if (cube_start != selection_cube_start_ || current_size != selection_cube_size_)
    {
        selection_cube_start_ = cube_start;
        selection_cube_size_ = current_size;

        selection_cube_ = generate_cube_mesh_level(selection_cube_start_, selection_cube_size_, 16,
                                                   {255, 255, 255, 150});
        selection_cube_.update();
    }

    selection_quad_ = generate_2d_outline_quad_mesh(selection_area_.start,
                                                    selection_area_.end - selection_area_.start);
    selection_quad_.update();
    render_preview_mesh_ = true;
}