#pragma once

#include <SFML/Window/Event.hpp>

#include "../Graphics/Mesh.h"

class DrawingPad;
class LevelTextures;
struct EditorState;
class ActionManager;

class ITool
{
  public:
    virtual void on_event(sf::Event event, glm::vec2 node, EditorState& state, ActionManager& actions) = 0;
    virtual void render_preview() = 0;
    virtual void render_preview_2d(DrawingPad& drawing_pad) = 0;
};

class CreateWallTool : public ITool
{
  public:
    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad) override;

  private:
    WorldGeometryMesh3D wall_preview_;
    glm::vec2 start_{0.0f};
    glm::vec2 end_{0.0f};
    bool active_dragging_ = false;
};