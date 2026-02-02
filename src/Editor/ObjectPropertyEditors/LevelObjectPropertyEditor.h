#pragma once

#include <memory>
#include <vector>

#include <SFML/Window/Event.hpp>

#include "../../Graphics/Mesh.h"

class LevelTextures;
class ActionManager;
class EditorLevel;
class Camera;

struct EditorState;
struct MousePickingState;

/// Base class for propety editors enabling using gizos to edit objects in the 2D and 3D views
class LevelObjectPropertyEditor
{
  public:
    virtual ~LevelObjectPropertyEditor() = default;

    virtual bool handle_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map,
                              const Camera& camera_3d) = 0;
    virtual void render_preview_2d(gl::Shader& scene_shader_2d) = 0;
    virtual void render_preview_3d(gl::Shader& scene_shader_3d, bool always_show_gizmos) = 0;

    virtual void render_to_picker(const MousePickingState& picker_state, gl::Shader& picker_shader)
    {
    }

    virtual void render_to_picker_mouse_over(const MousePickingState& picker_state,
                                             gl::Shader& picker_shader)
    {
    }

    virtual bool hide_normal_previews() const
    {
        return false;
    }
};

using LevelObjectPropertyEditors = std::vector<std::unique_ptr<LevelObjectPropertyEditor>>;
