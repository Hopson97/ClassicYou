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
    constexpr float AREA_SIZE = 32.0f;
    constexpr float MIN_SELECT_DISTANCE = AREA_SIZE / 2.0f;

} // namespace

UpdatePolygonTool::UpdatePolygonTool(LevelObject object, PolygonPlatformObject& polygon, int floor,
                                     const LevelTextures& drawing_pad_texture_map)
    : object_(object)
    , polygon_{polygon}
    , floor_(floor)
{
    vertex_selector_mesh_ = generate_2d_quad_mesh(
        {0, 0}, {AREA_SIZE, AREA_SIZE},
        static_cast<float>(*drawing_pad_texture_map.get_texture("SelectCircle")));
    vertex_selector_mesh_.buffer();
}

void UpdatePolygonTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                                 const LevelTextures& drawing_pad_texture_map)
{
    state_floor_ = state.current_floor;
    if (state.current_floor != floor_)
    {
        return;
    }

    auto& outer_points = polygon_.properties.geometry[0];
    const auto& hover_position = state.world_position_hovered;

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {

            const auto& polygon_position = polygon_.parameters.position;

            if (auto i = closest_point_index(outer_points, hover_position))
            {
                target_index_ = *i;
                active_dragging_ = true;
            }

            // Try to add a new vertex to the polygon if a vertex did not get selected
            if (!active_dragging_)
            {
                for (size_t i = 0; i < outer_points.size(); i++)
                {
                    auto p1 = outer_points[i] + polygon_position;
                    auto p2 = outer_points[(i + 1) % outer_points.size()] + polygon_position;

                    // If a line is clicked, then add a new point to the polygon
                    auto distance = distance_to_line(hover_position, {p1, p2});
                    if (distance < MIN_SELECT_DISTANCE)
                    {
                        auto new_point = glm::vec2{state.node_hovered} - polygon_position;
                        outer_points.insert(outer_points.begin() + i + 1, new_point);
                        update_polygon(state.current_floor, actions,
                                       PolygonUpdateAction::AddOrDeletePoint);

                        // Allow dragging the newly added point immediately after adding it
                        target_index_ = i + 1;
                        active_dragging_ = true;
                        break;
                    }
                }
            }

            // If one of the points was selected, then it must update the preview to prep for the
            // next frame
            if (active_dragging_)
            {
                target_new_position_ =
                    state.node_hovered - glm::ivec2{polygon_.parameters.position};
                update_previews(state, drawing_pad_texture_map);
            }
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (active_dragging_)
        {
            target_new_position_ = state.node_hovered - glm::ivec2{polygon_.parameters.position};
            update_previews(state, drawing_pad_texture_map);
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left &&
            active_dragging_)
        {
            update_polygon(state.current_floor, actions, PolygonUpdateAction::MovePoint);
            active_dragging_ = false;
        }
    }
    else if (auto key = event.getIf<sf::Event::KeyReleased>())
    {
        // Delete a vertex if the mouse is close and backspace is pressed
        if (key->code == sf::Keyboard::Key::Backspace && outer_points.size() > 3)
        {
            if (auto i = closest_point_index(outer_points, hover_position))
            {
                outer_points.erase(outer_points.begin() + *i);
                update_polygon(state.current_floor, actions, PolygonUpdateAction::AddOrDeletePoint);
                active_dragging_ = false;
            }
        }
    }
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
    if (state_floor_ == floor_)
    {

        if (active_dragging_)
        {
            scene_shader_2d.set_uniform("use_texture", false);
            scene_shader_2d.set_uniform("is_selected", true);
            scene_shader_2d.set_uniform("on_floor_below", false);
            polygon_preview_2d_.bind().draw_elements(gl::PrimitiveType::Lines);
        }

        scene_shader_2d.set_uniform("on_floor_below", false);
        scene_shader_2d.set_uniform("use_texture", true);
        scene_shader_2d.set_uniform("use_world_texture", false);
        scene_shader_2d.set_uniform("use_texture_alpha_channel", true);
        scene_shader_2d.set_uniform("is_selected", false);

        for (auto& point : polygon_.properties.geometry[0])
        {
            glm::vec3 p{point + polygon_.parameters.position - glm::vec2{MIN_SELECT_DISTANCE}, 0};
            scene_shader_2d.set_uniform("model_matrix", create_model_matrix({.position = p}));
            vertex_selector_mesh_.bind().draw_elements();
        }
    }
}

void UpdatePolygonTool::update_previews(const EditorState& state,
                                        const LevelTextures& drawing_pad_texture_map)
{
    polygon_.properties.geometry[0][target_index_] = target_new_position_;

    polygon_preview_2d_ = object_to_geometry_2d(polygon_, drawing_pad_texture_map).first;
    polygon_preview_2d_.update();

    polygon_preview_ = object_to_geometry(polygon_, state.current_floor);
    polygon_preview_.update();
}

void UpdatePolygonTool::update_polygon(int current_floor, ActionManager& actions,
                                       PolygonUpdateAction action)
{
    auto new_object = object_;
    auto& new_polygon = std::get<PolygonPlatformObject>(new_object.object_type);

    new_polygon.properties.geometry[0] = polygon_.properties.geometry[0];

    // If not moving a vertex, this can cause OOB error if points have been removed
    if (action == PolygonUpdateAction::MovePoint)
    {
        new_polygon.properties.geometry[0][target_index_] = target_new_position_;
    }

    actions.push_action(std::make_unique<UpdateObjectAction>(object_, new_object, current_floor));

    object_ = new_object;
    polygon_ = new_polygon;

    active_dragging_ = false;
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