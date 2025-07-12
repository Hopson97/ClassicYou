#pragma once

#include <SFML/Window/Event.hpp>

#include "../Graphics/Mesh.h"
#include "LevelObject.h"
#include "../Util/Maths.h"


class DrawingPad;
class LevelTextures;
struct EditorState;
class ActionManager;

class ITool
{
  public:
    virtual ~ITool() = default;

    virtual void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                          ActionManager& actions) = 0;
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
    LevelObjectsMesh3D wall_preview_;
    Line wall_line_;
    bool active_dragging_ = false;
};

class UpdateWallTool : public ITool
{
  public:
    UpdateWallTool(LevelObject object, WallObject& wall);
    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad) override;

  private:
    LevelObjectsMesh3D wall_preview_;
    LevelObject object_;
    WallObject wall_;

    Line wall_line_;

    bool active_dragging_ = false;

    enum class DragTarget
    {
        Start,
        End
    } target_;
};

class CreatePlatformTool : public ITool
{
  public:
    CreatePlatformTool(const PlatformProps& platform_default);

    void on_event(sf::Event event, glm::vec2 node, EditorState& state,
                  ActionManager& actions) override;
    void render_preview() override;
    void render_preview_2d(DrawingPad& drawing_pad) override;

  private:
    const PlatformProps* p_platform_default_;
    LevelObjectsMesh3D platform_preview_;
    glm::vec2 tile_{0.0f};
};