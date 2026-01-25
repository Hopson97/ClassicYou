#pragma once

#include <memory>
#include <vector>

#include <SFML/Window/Event.hpp>

#include "../../Graphics/Mesh.h"

class LevelTextures;
struct EditorState;
class ActionManager;
class EditorLevel;

class LevelObjectPropertyEditor
{
  public:
    virtual ~LevelObjectPropertyEditor() = default;

    virtual bool handle_event(const sf::Event& event, EditorState& state, ActionManager& actions,
                              const LevelTextures& drawing_pad_texture_map) = 0;
    virtual void render_preview_2d(gl::Shader& scene_shader_2d) = 0;
};

using LevelObjectPropertyEditors = std::vector<std::unique_ptr<LevelObjectPropertyEditor>>;
