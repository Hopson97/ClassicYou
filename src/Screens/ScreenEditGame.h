#pragma once

#include "../Editor/Actions.h"
#include "../Editor/DrawingPad.h"
#include "../Editor/EditConstants.h"
#include "../Editor/EditorLevel.h"
#include "../Editor/LevelTextures.h"
#include "../Editor/Tool.h"
#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/BufferObject.h"
#include "../Settings.h"
#include "Screen.h"

class ScreenEditGame final : public Screen
{
  public:
    ScreenEditGame(ScreenManager& screens);

    bool on_init() override;
    void on_event(const sf::Event& event) override;
    void on_update(const Keyboard& keyboard, sf::Time dt) override;
    void on_fixed_update(sf::Time dt) override;
    void on_render(bool show_debug) override;

  private:
    void pause_menu();
    void debug_gui();

    void render_scene(gl::Shader& shader);

    Camera camera_;

    Mesh3D grid_mesh_ = generate_grid_mesh(WORLD_SIZE, WORLD_SIZE);

    gl::BufferObject matrices_ssbo_;
    gl::Shader scene_shader_;
    gl::Shader world_geometry_shader_;
    CameraKeybinds camera_keybinds_;

    Mesh3D selection_mesh_ = generate_quad_mesh(0.25, 0.25);

    DrawingPad drawing_pad_;

    bool rotation_locked_ = true;
    bool game_paused_ = false;
    Settings settings_;

    EditorState editor_state_;

    LevelTextures level_texures_;
    gl::Texture2DArray texture_;

    EditorLevel level_;
    CreateWallTool tool_;
    ActionManager action_manager_;
};