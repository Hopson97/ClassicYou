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

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            // Try to select one of the points within the polygon
            for (size_t i = 0; i < polygon_.properties.points[0].size(); i++)
            {
                auto point = polygon_.properties.points[0][i] + polygon_.parameters.position;

                if (glm::distance(state.world_position_hovered, point) < MIN_SELECT_DISTANCE)
                {
                    target_index_ = i;
                    active_dragging_ = true;
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
            auto new_object = object_;
            auto& new_polygon = std::get<PolygonPlatformObject>(new_object.object_type);

            new_polygon.properties.points[0] = polygon_.properties.points[0];
            new_polygon.properties.points[0][target_index_] = target_new_position_;

            actions.push_action(
                std::make_unique<UpdateObjectAction>(object_, new_object, state.current_floor));

            object_ = new_object;
            polygon_ = new_polygon;

            active_dragging_ = false;
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

        for (auto& point : polygon_.properties.points[0])
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
    polygon_.properties.points[0][target_index_] = target_new_position_;

    polygon_preview_2d_ = object_to_geometry_2d(polygon_, drawing_pad_texture_map).first;
    polygon_preview_2d_.update();

    polygon_preview_ = object_to_geometry(polygon_, state.current_floor);
    polygon_preview_.update();
}

ToolType UpdatePolygonTool::get_tool_type() const
{
    return ToolType::UpdatePolygon;
}