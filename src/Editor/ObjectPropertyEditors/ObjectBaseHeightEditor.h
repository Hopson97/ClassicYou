#pragma once

#include "LevelObjectPropertyEditor.h"

#include <glm/glm.hpp>

#include "../../Util/Maths.h"
#include "../EditConstants.h"
#include "../EditorGUI.h"
#include "../LevelObjects/LevelObject.h"

/// Class for updating the size of certain objects via the world views
class ObjectBaseHeightEditor : public LevelObjectPropertyEditor
{
  public:
    ObjectBaseHeightEditor(const LevelObject& object, float base_hight, int object_floor);

    bool handle_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                      const LevelTextures& drawing_pad_texture_map, const Camera& camera_3d,
                      bool mouse_in_2d_view) override;

    void render_preview_2d(gl::Shader& scene_shader_2d) override {};
    void render_preview_3d(gl::Shader& scene_shader_3d, bool always_show_gizmos) override;

    void render_to_picker_mouse_over(const MousePickingState& picker_state,
                                     gl::Shader& picker_shader) override;

    bool hide_normal_previews() const override;

  private:
    void update_previews();

    /// The object before any state changes to ensure history can be restored to the cached object
    /// state for "undo".
    LevelObject cached_object_;

    // Fields modified by the editor
    glm::vec2 base_hight_;

    // The floor of the object to prevent updating when on a different floor to it
    int object_floor_ = 0;
    int state_floor_ = 0;

    /// True if the size is being modified in the 3D view
    bool active_dragging_3d_ = false;

    /// Preview gizmos for the up/down arrows
    Mesh3D arrow_preview_3d_;
    Mesh3D down_arrow_preview_3d_;

    bool editing_enabled_ = false;

    glm::vec2 gizmo_preview_position_{0.0f};
};