#include "ObjectBaseHeightEditor.h"
#include "../../Graphics/MeshGeneration.h"

#include "../EditorState.h"
#include "../EditorUtils.h"

ObjectBaseHeightEditor::ObjectBaseHeightEditor(const LevelObject& object, float base_hight,
                                               int object_floor)
    : base_hight_(base_hight)
    , object_floor_{object_floor}
    , cached_object_(object)
{
    update_previews();
}

bool ObjectBaseHeightEditor::handle_event(const sf::Event& event, EditorState& state,
                                          ActionManager& actions,
                                          const LevelTextures& drawing_pad_texture_map,
                                          const Camera& camera_3d, bool mouse_in_2d_view)
{
    // Only enable the editor when control is pressed
    if (auto key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::LControl)
        {
            editing_enabled_ = true;
        }
    }
    else if (auto key = event.getIf<sf::Event::KeyReleased>())
    {
        if (key->code == sf::Keyboard::Key::LControl)
        {
            editing_enabled_ = false;
        }
    }



    if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        gizmo_preview_position_ =
            get_mouse_floor_snapped_intersect(camera_3d, mouse->position, state.current_floor) /
            TILE_SIZE_F;

    }

    return false;
}

void ObjectBaseHeightEditor::render_preview_3d(gl::Shader& scene_shader_3d, bool always_show_gizmos)
{
    // if (editing_enabled_)
    {
        std::println("Gizmo: {} {}", gizmo_preview_position_.x, gizmo_preview_position_.y);
        scene_shader_3d.set_uniform("model_matrix",
                                    create_model_matrix({.position = {gizmo_preview_position_.x, object_floor_ * FLOOR_HEIGHT,
                                                                      gizmo_preview_position_.y},
                                                 .rotation = {0, 0, 90}}));

        arrow_preview_3d_.bind().draw_elements();
    }
}

void ObjectBaseHeightEditor::render_to_picker_mouse_over(const MousePickingState& picker_state,
                                                         gl::Shader& picker_shader)
{

}

bool ObjectBaseHeightEditor::hide_normal_previews() const
{
    return true;
}

void ObjectBaseHeightEditor::update_previews()
{
    arrow_preview_3d_ = generate_pyramid_mesh({0.3, 0.5, 0.3}, Colour::MAGENTA, Colour::GREY);

    arrow_preview_3d_.buffer();
}