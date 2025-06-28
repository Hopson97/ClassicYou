#pragma once

#include "../Editor/DrawingPad.h"
#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/BufferObject.h"
#include "../Graphics/OpenGL/Texture.h"
#include "../Settings.h"
#include "Screen.h"

class ScreenEditGame final : public Screen
{
  public:
    ScreenEditGame(ScreenManager& screens);

    bool on_init() override;
    void on_event(const sf::Event& e) override;
    void on_update(const Keyboard& keyboard, sf::Time dt) override;
    void on_fixed_update(sf::Time dt) override;
    void on_render(bool show_debug) override;

  private:
    void pause_menu();
    void debug_gui();

    void render_scene(gl::Shader& shader);

    Camera camera_;

    Mesh3D grid_mesh_ = generate_grid_mesh(64, 64);

    gl::BufferObject matrices_ssbo_;
    gl::Shader scene_shader_;

    Mesh3D terrain_mesh_ = generate_terrain_mesh(64, 64);
    gl::Texture2D grass_material_;

    DrawingPad drawing_pad_;

    bool rotation_locked_ = false;
    bool game_paused_ = false;
    Settings settings_;

    CameraKeybinds camera_keybinds_;
};