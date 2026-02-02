#include "Tool.h"

#include <print>

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>

#include "../../Graphics/OpenGL/GLUtils.h"
#include "../../Graphics/OpenGL/Shader.h"
#include "../Actions.h"
#include "../EditConstants.h"
#include "../EditorLevel.h"
#include "../EditorState.h"
#include "../EditorUtils.h"
#include "../LevelTextures.h"

CreateObjectTool::CreateObjectTool(ObjectTypeName object_type)
    : object_type_(object_type)
    , object_(-1)
{
}

bool CreateObjectTool::on_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                                const LevelTextures& drawing_pad_texture_map,
                                [[maybe_unused]] const Camera& camera_3d, bool mouse_in_2d_view)
{
    if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            update_previews(state, drawing_pad_texture_map);
            actions.push_action(std::make_unique<AddObjectAction>(object_, state.current_floor));
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        tile_hovered_ = mouse_in_2d_view ? glm::vec2{state.node_hovered}
                                         : get_mouse_floor_snapped_intersect(
                                               camera_3d, mouse->position, state.current_floor);
        update_previews(state, drawing_pad_texture_map);
    }
    return false;
}

void CreateObjectTool::render_preview()
{
    if (object_preview_.has_buffered())
    {
        object_preview_.bind().draw_elements();
    }
}

void CreateObjectTool::render_preview_2d(gl::Shader& scene_shader_2d)
{
    if (object_preview_2d_.has_buffered())
    {
        scene_shader_2d.set_uniform("use_texture",
                                    preview_2d_primitive_ != gl::PrimitiveType::Lines);
        scene_shader_2d.set_uniform("is_selected", true);
        scene_shader_2d.set_uniform("on_floor_below", false);
        object_preview_2d_.bind().draw_elements(preview_2d_primitive_);
    }
}

void CreateObjectTool::update_previews(const EditorState& state,
                                       const LevelTextures& drawing_pad_texture_map)
{
    switch (object_type_)
    {
        case ObjectTypeName::Platform:
        {
            // Ensure half tiles are not offset by quater tile increments by "nudging" it 0.5 (Also
            // done for ramps)
            auto size = state.platform_default.size;
            size = {size.x + (std::abs(std::fmod(size.x, 1.0)) == 0.5 ? 0.5 : 0),
                    size.y + (std::abs(std::fmod(size.y, 1.0)) == 0.5 ? 0.5 : 0)};
            object_.object_type = PlatformObject{
                .properties = state.platform_default,
                .parameters = {.position = tile_hovered_ - size * HALF_TILE_SIZE_F},
            };
            break;
        }
        case ObjectTypeName::PolygonPlatform:
            object_.object_type = PolygonPlatformObject{
                .properties = state.polygon_platform_default,
                .parameters = {.position = tile_hovered_},
            };
            break;

        case ObjectTypeName::Pillar:
            object_.object_type = PillarObject{
                .properties = state.pillar_default,
                .parameters = {.position = tile_hovered_},
            };
            break;

        case ObjectTypeName::Ramp:
        {
            auto size = state.ramp_default.size;
            size = {size.x + (std::abs(std::fmod(size.x, 1.0)) == 0.5 ? 0.5 : 0),
                    size.y + (std::abs(std::fmod(size.y, 1.0)) == 0.5 ? 0.5 : 0)};
            object_.object_type = RampObject{
                .properties = state.ramp_default,
                .parameters = {.position = tile_hovered_ - size * HALF_TILE_SIZE_F},
            };
            break;
        }
        break;

        default:
            std::println("Missing implementation for CreateObjectTool for {}",
                         magic_enum::enum_name(object_type_));
            break;
    }

    object_preview_ = object_.to_geometry(state.current_floor);
    object_preview_.update();

    auto [preview, primitive] = object_.to_2d_geometry(drawing_pad_texture_map);
    object_preview_2d_ = std::move(preview);
    preview_2d_primitive_ = primitive;
    object_preview_2d_.update();
}

ToolType CreateObjectTool::get_tool_type() const
{
    return ToolType::CreateObject;
}
