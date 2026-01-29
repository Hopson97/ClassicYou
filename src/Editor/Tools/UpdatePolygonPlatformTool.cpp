#include "Tool.h"

#include <imgui.h>

#include "../../Graphics/MeshGeneration.h"
#include "../../Graphics/OpenGL/GLUtils.h"
#include "../../Graphics/OpenGL/Shader.h"
#include "../Actions.h"
#include "../EditConstants.h"
#include "../EditorLevel.h"
#include "../EditorState.h"
#include "../LevelTextures.h"

namespace
{
    constexpr float AREA_SIZE = 16.0f;
    constexpr float MIN_SELECT_DISTANCE = AREA_SIZE / 2.0f;

} // namespace

UpdatePolygonTool::UpdatePolygonTool(LevelObject object, PolygonPlatformObject& polygon, int floor,
                                     const LevelTextures& drawing_pad_texture_map)
    : object_(object)
    , polygon_{polygon}
    , floor_(floor)
    , state_floor_(floor)
{
    vertex_selector_mesh_ = generate_2d_quad_mesh(
        {0, 0}, {AREA_SIZE, AREA_SIZE},
        static_cast<float>(*drawing_pad_texture_map.get_texture("SelectCircle")));
    vertex_selector_mesh_.buffer();

    // Ensure the polygon outline can always be seen
    polygon_preview_2d_ = object_to_outline_2d(polygon_);
    polygon_preview_2d_.update();
}

bool UpdatePolygonTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                                 const LevelTextures& drawing_pad_texture_map,
                                 [[maybe_unused]] bool mouse_in_2d_view)
{
    state_floor_ = state.current_floor;
    state_world_position_hovered_ = state.world_position_hovered;
    state_node_hovered_ = state.node_hovered;
    if (state.current_floor != floor_)
    {
        return false;
    }

    auto& outer_points = polygon_.properties.geometry[0];

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            if (target_index_)
            {
                active_dragging_ = true;
            }

            // Try to add a new vertex to the polygon if a vertex did not get selected
            if (!active_dragging_ && line_.is_selected_)
            {
                // If a line is clicked, then add a new point to the polygon between the two
                // vertices
                outer_points.insert(outer_points.begin() + line_.index + 1, line_.node_point);
                update_polygon(state.current_floor, actions, PolygonUpdateAction::AddOrDeletePoint);

                // Allow dragging the newly added point immediately after adding it
                target_index_ = line_.index + 1;
                active_dragging_ = true;
            }

            // If one of the points was selected, then it must update the preview to prep for the
            // next frame
            if (active_dragging_)
            {
                target_new_position_ =
                    state_node_hovered_ - glm::ivec2{polygon_.parameters.position};
                update_previews(PolygonUpdateAction::MovePoint);
                return true;
            }
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        const auto& polygon_position = polygon_.parameters.position;

        if (!active_dragging_)
        {
            target_index_ = closest_point_index(outer_points, state_world_position_hovered_);

            // Try to find a point along the line where a new vertex could be added if the mouse
            // cursor is close to it
            line_.is_selected_ = false;
            for (size_t i = 0; i < outer_points.size(); i++)
            {
                auto p1 = outer_points[i] + polygon_position;
                auto p2 = outer_points[(i + 1) % outer_points.size()] + polygon_position;
                Line line{p1, p2};

                auto distance = distance_to_line(state_world_position_hovered_, line);
                if (distance < MIN_SELECT_DISTANCE)
                {
                    line_ = {
                        .world_point = closest_point_on_line(state_world_position_hovered_, line),
                        .node_point = glm::vec2{state_node_hovered_} - polygon_position,
                        .index = i,
                        .is_selected_ = true,
                    };
                    break;
                }
            }
        }

        if (active_dragging_)
        {
            target_new_position_ = state_node_hovered_ - glm::ivec2{polygon_.parameters.position};
            update_previews(PolygonUpdateAction::MovePoint);
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left &&
            active_dragging_)
        {
            delete_holes_outside_polygon();
            update_polygon(state.current_floor, actions, PolygonUpdateAction::MovePoint);
            active_dragging_ = false;
            update_previews(PolygonUpdateAction::AddOrDeletePoint);
        }
    }
    else if (auto key = event.getIf<sf::Event::KeyReleased>())
    {
        // Delete a vertex if the mouse is close and backspace is pressed
        if (key->code == sf::Keyboard::Key::Backspace && outer_points.size() > 3)
        {

            if (auto i = closest_point_index(outer_points, state_world_position_hovered_))
            {
                outer_points.erase(outer_points.begin() + *i);
                delete_holes_outside_polygon();
                update_polygon(state.current_floor, actions, PolygonUpdateAction::AddOrDeletePoint);
                active_dragging_ = false;
                update_previews(PolygonUpdateAction::AddOrDeletePoint);
                return true;
            }
        }
    }
    return false;
}

void UpdatePolygonTool::render_preview()
{
    if (polygon_preview_.has_buffered() && active_dragging_)
    {
        // Use polygon offset to prevent Z-Fighting the existing polygon
        gl::enable(gl::Capability::PolygonOffsetFill);
        glPolygonOffset(-1.0f, -1.0f);

        polygon_preview_.bind().draw_elements();

        gl::disable(gl::Capability::PolygonOffsetFill);
    }
}

void UpdatePolygonTool::render_preview_2d(gl::Shader& scene_shader_2d)
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

    if (state_floor_ == floor_)
    {
        const auto& outer_points = polygon_.properties.geometry[0];
        auto polygon_position = polygon_.parameters.position;

        // Render the outline of the polygon so it is clear
        scene_shader_2d.set_uniform("use_texture", false);
        scene_shader_2d.set_uniform("is_selected", false);
        scene_shader_2d.set_uniform("on_floor_below", false);
        polygon_preview_2d_.bind().draw_elements(gl::PrimitiveType::Lines);

        // Render the circles that show the grabbable vertices
        scene_shader_2d.set_uniform("on_floor_below", false);
        scene_shader_2d.set_uniform("use_texture", true);
        scene_shader_2d.set_uniform("use_world_texture", false);
        scene_shader_2d.set_uniform("use_texture_alpha_channel", true);

        for (size_t i = 0; i < outer_points.size(); i++)
        {
            // Draw a selection circle at each vertex, if one is hovered then it is drawn scaled up.
            float scale = 1.0f;
            if (target_index_ && *target_index_ == i)
            {
                scale = 1.5f;
            }
            draw_selection_point(outer_points[i] + polygon_position, scale);

            // Show where a new point could be added
            if (line_.is_selected_ && !active_dragging_ && !target_index_)
            {
                draw_selection_point(line_.world_point, scale);
            }
        }
    }
}

void UpdatePolygonTool::update_previews(PolygonUpdateAction action)
{
    // If not moving a vertex, this can cause OOB error if points have been removed, or cause
    // the outline geometry to be incorrect
    if (action == PolygonUpdateAction::MovePoint)
    {
        polygon_.properties.geometry[0][*target_index_] = target_new_position_;
    }
    polygon_preview_2d_ = object_to_outline_2d(polygon_);
    polygon_preview_2d_.update();

    polygon_preview_ = object_to_geometry(polygon_, state_floor_);
    polygon_preview_.update();
}

void UpdatePolygonTool::update_polygon(int current_floor, ActionManager& actions,
                                       PolygonUpdateAction action)
{
    auto new_object = object_;
    auto& new_polygon = std::get<PolygonPlatformObject>(new_object.object_type);

    new_polygon.properties.geometry = polygon_.properties.geometry;

    // If not moving a vertex, this can cause OOB error if points have been removed
    if (action == PolygonUpdateAction::MovePoint)
    {
        new_polygon.properties.geometry[0][*target_index_] = target_new_position_;
    }

    actions.push_action(std::make_unique<UpdateObjectAction>(object_, new_object, current_floor));

    object_ = new_object;
    polygon_ = new_polygon;

    active_dragging_ = false;
}

void UpdatePolygonTool::delete_holes_outside_polygon()
{
    // When deleting points, or moving points, holes outside the outer region causes buggy
    // geometry, so it is best to just remove them
    auto& geometry = polygon_.properties.geometry;

    auto holes = geometry | std::views::drop(1);

    auto [first, last] = std::ranges::remove_if(
        holes,
        [&](const auto& hole)
        {
            return std::ranges::any_of(hole, [&](const auto& vertex)
                                       { return !point_in_polygon(vertex, geometry[0]); });
        });
    geometry.erase(first, last);
}

std::optional<size_t> UpdatePolygonTool::closest_point_index(const std::vector<glm::vec2>& points,
                                                             const glm::vec2& world_position) const
{
    const auto& polygon_position = polygon_.parameters.position;

    for (size_t i = 0; i < points.size(); i++)
    {
        auto p = points[i] + polygon_position;
        if (glm::distance(world_position, p) < MIN_SELECT_DISTANCE)
        {
            return i;
        }
    }
    return std::nullopt;
}

ToolType UpdatePolygonTool::get_tool_type() const
{
    return ToolType::UpdatePolygon;
}