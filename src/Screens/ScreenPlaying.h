#pragma once

#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/BufferObject.h"
#include "../Graphics/OpenGL/Texture.h"
#include "../Settings.h"
#include "Screen.h"

class ScreenPlaying final : public Screen
{
    enum class Direction
    {
        Up = 0,
        Down = 1,
        Left = 2,
        Right = 3
    };

  public:
    ScreenPlaying(ScreenManager& screens);

    bool on_init() override;
    void on_event(const sf::Event& event) override;
    void on_update(const Keyboard& keyboard, sf::Time dt) override;
    void on_fixed_update(sf::Time dt) override;
    void on_render(bool show_debug) override;

  private:
    void pause_menu();

    bool game_paused_ = false;

    void render_scene(gl::Shader& shader);

    Camera perspective_camera_;
    Camera ortho_camera_;

    Camera* p_active_camera_ = nullptr;

    gl::Texture2D grass_material_;

    gl::BufferObject matrices_ssbo_;
    gl::Shader scene_shader_;

    Mesh3D terrain_mesh_ = generate_terrain_mesh(604, 640);

    sf::Clock game_time_;

    bool rotation_locked_ = false;
    Settings settings_;

    CameraKeybinds camera_keybinds_;
};