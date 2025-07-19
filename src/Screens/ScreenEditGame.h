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
    ScreenEditGame(ScreenManager& screens, std::string level_name);

    bool on_init() override;
    void on_event(const sf::Event& event) override;
    void on_update(const Keyboard& keyboard, sf::Time dt) override;
    void on_fixed_update(sf::Time dt) override;
    void on_render(bool show_debug) override;

  private:
    void render_editor_ui();

    void exit_editor();

    // Dialog GUIs for loading/editing etc
    void show_menu_bar();
    void save_level();
    void show_save_dialog();

    void debug_gui();
    bool showing_dialog() const;

    Camera camera_;

    Mesh3D grid_mesh_ = generate_grid_mesh(WORLD_SIZE, WORLD_SIZE);

    gl::BufferObject matrices_ssbo_;
    gl::Shader scene_shader_;
    gl::Shader world_geometry_shader_;
    CameraKeybinds camera_keybinds_;

    Mesh3D selection_mesh_ = generate_quad_mesh(0.25, 0.25);

    DrawingPad drawing_pad_;

    bool rotation_locked_ = true;
    Settings settings_;

    EditorState editor_state_;

    LevelTextures level_textures_;
    gl::Texture2DArray texture_;

    EditorLevel level_;
    std::unique_ptr<ITool> tool_;
    ActionManager action_manager_;

    bool show_save_dialog_ = false;
    bool show_load_dialog_ = false;

    // If the currently seleceted object being dragged?
    bool moving_object_ = false;

    // Capture the state of the object being moved at the start such that the inital state can be
    // returned to when CTRL+Z is done
    LevelObject moving_object_cache_{-1};

    // Level name is used in the save dialog such that the actual name is not overriden if the save
    // operation is cancelled
    std::string level_name_;
    std::string level_name_actual_;
};
