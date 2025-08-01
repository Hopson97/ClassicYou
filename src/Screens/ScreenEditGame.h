#pragma once

#include "../Editor/Actions.h"
#include "../Editor/DrawingPad.h"
#include "../Editor/EditConstants.h"
#include "../Editor/EditorLevel.h"
#include "../Editor/EditorState.h"
#include "../Editor/InfiniteGrid.h"
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
    struct EditorSettings
    {
        bool show_grid = true;
        bool show_2d_view = true;
        bool always_center_2d_to_3d_view = true;

        bool show_history = false;
    } editor_settings_;

  private:
    void render_editor_ui();

    void exit_editor();

    // Dialog GUIs for loading/editing etc
    void show_menu_bar();
    void save_level();
    void show_save_dialog();

    void debug_gui();
    bool showing_dialog() const;

    void set_2d_to_3d_view();

    void copy_selection();
    void paste_selection();

    void try_set_tool_to_create_wall();

  private:
    Camera camera_;

    gl::BufferObject matrices_ssbo_;
    gl::Shader scene_shader_;
    gl::Shader world_geometry_shader_;
    CameraKeybinds camera_keybinds_;

    Mesh3D selection_mesh_ = generate_cube_mesh({0.1, 1.0f, 0.1});

    DrawingPad drawing_pad_;

    bool rotation_locked_ = true;
    Settings settings_;

    /// Grid Mesh
    InfiniteGrid grid_;

    /// State of the editor such as the currently selected object and defaults
    EditorState editor_state_;

    /// Wrapper around the selectable texture list
    LevelTextures level_textures_;

    /// 2D Texture array used for rendering the world geometry
    gl::Texture2DArray texture_;

    EditorLevel level_;
    std::unique_ptr<ITool> tool_;

    /// History of actions performed, has functions to undo/redo
    ActionManager action_manager_;

    bool show_save_dialog_ = false;
    bool show_load_dialog_ = false;

    /// If the currently seleceted object being dragged?
    bool moving_object_ = false;

    /// When moving an object, this is the position to offset from
    glm::ivec2 select_position_{0};

    /// Capture the state of the object being moved at the start such that the inital state can be
    /// returned to when CTRL+Z is done
    std::vector<LevelObject> moving_object_cache_;
    std::vector<LevelObject*> moving_objects_;
    // LevelObject moving_object_cache_{-1};

    // For copy paste, this contains the current "clipboard"
    std::vector<LevelObject> copied_objects_;
    std::vector<int> copied_objects_floors_;
    int copy_start_floor_ = -1;

    /// Level name is used in the save dialog such that the actual name is not overriden if the save
    /// operation is cancelled
    std::string level_name_;
    std::string level_name_actual_;
};
