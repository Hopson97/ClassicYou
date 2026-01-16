#include "Tool.h"

#include <imgui.h>

#include "../../Graphics/OpenGL/GLUtils.h"
#include "../../Graphics/OpenGL/Shader.h"
#include "../Actions.h"
#include "../EditConstants.h"
#include "../EditorLevel.h"
#include "../EditorState.h"
#include "../LevelTextures.h"

UpdatePolygonTool::UpdatePolygonTool(LevelObject object, PolygonPlatformObject& polygon, int floor,
                                     const LevelTextures& drawing_pad_texture_map)
    : object_(object)
    , polygon_{polygon}
    , floor_(floor)
{
    vertex_selector_mesh_ = generate_2d_quad_mesh(
        {0, 0}, {16, 16}, static_cast<float>(*drawing_pad_texture_map.get_texture("SelectCircle")));
    vertex_selector_mesh_.buffer();
}

void UpdatePolygonTool::on_event(sf::Event event, glm::vec2 node, EditorState& state,
                                 ActionManager& actions,
                                 const LevelTextures& drawing_pad_texture_map)
{
    const float MIN_DISTANCE = 16.0f;

    if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            for (size_t i = 0; i < polygon_.properties.points[0].size(); i++)
            {
                auto& point = polygon_.properties.points[0][i];
                if (glm::distance(node, point + polygon_.parameters.position) < MIN_DISTANCE)
                {
                    target_index_ = i;
                    active_dragging_ = true;
                }
            }

            if (active_dragging_)
            {
                update_previews(state, drawing_pad_texture_map);
                target_new_position_ = node - polygon_.parameters.position;
            }
        }
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (active_dragging_)
        {
            target_new_position_ = node - polygon_.parameters.position;
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
}

void UpdatePolygonTool::render_preview_2d(gl::Shader& scene_shader_2d)
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

    for (size_t i = 0; i < polygon_.properties.points[0].size(); i++)
    {
        auto point = polygon_.properties.points[0][i];

        if (i == target_index_)
        {
            point = target_new_position_;
        }
        glm::vec3 p{point + polygon_.parameters.position - glm::vec2{8}, 0};

        scene_shader_2d.set_uniform("model_matrix", create_model_matrix({.position = p}));
        vertex_selector_mesh_.bind().draw_elements();
    }

    for (auto& point : polygon_.properties.points[0])
    {
        glm::vec3 p{point + polygon_.parameters.position - glm::vec2{8}, 0};
        scene_shader_2d.set_uniform("model_matrix", create_model_matrix({.position = p}));
        vertex_selector_mesh_.bind().draw_elements();
    }
}

ToolType UpdatePolygonTool::get_tool_type() const
{
    return ToolType::UpdatePolygonTool;
}

void UpdatePolygonTool::update_previews(const EditorState& state,
                                        const LevelTextures& drawing_pad_texture_map)
{
    PolygonPlatformObject polygon = polygon_;
    polygon.properties.points[0][target_index_] = target_new_position_;

    //    wall_preview_ = object_to_geometry(wall, state.current_floor);
    // wall_preview_.update();

    polygon_preview_2d_ = object_to_geometry_2d(polygon, drawing_pad_texture_map).first;
    polygon_preview_2d_.update();
}