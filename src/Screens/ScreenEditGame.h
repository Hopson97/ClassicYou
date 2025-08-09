#pragma once

#include "../Editor/Actions.h"
#include "../Editor/DrawingPad.h"
#include "../Editor/EditConstants.h"
#include "../Editor/EditorEventHandlers.h"
#include "../Editor/EditorLevel.h"
#include "../Editor/EditorState.h"
#include "../Editor/InfiniteGrid.h"
#include "../Editor/LevelTextures.h"
#include "../Editor/Tool.h"
#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/BufferObject.h"
#include "../Graphics/OpenGL/Framebuffer.h"
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

        bool jump_to_selection_floor = false;
    } editor_settings_;

  private:
    /// Sets or adds the given object to the selection (Such as when right clicking an object).
    /// Selecting walls sets the tool type to be "UpdateWallTool" such that it can be resized
    void select_object(LevelObject* object);

    void render_editor_ui();

    void exit_editor();

    // Dialog GUIs for loading/editing etc
    void show_menu_bar();
    void save_level();
    void show_save_dialog();

    void debug_gui();
    bool showing_dialog() const;

    void set_2d_to_3d_view();

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

    ObjectMoveHandler object_move_handler_;
    CopyPasteHandler copy_paste_handler_;

    bool show_save_dialog_ = false;
    bool show_load_dialog_ = false;

    /// Level name is used in the save dialog such that the actual name is not overriden if the save
    /// operation is cancelled
    std::string level_name_;
    std::string level_name_actual_;

    // When doing mouse picking in the 3D view, the scene is rendered to this
    gl::Framebuffer picker_fbo_;
    gl::Shader picker_shader_;

    // Must be set to false each frame - set to true when right-clicking the 3D view
    bool try_pick_3d_ = false;

    // The location where mouse picking was attempted in the 3d view
    sf::Vector2i mouse_picker_point_;

    // Used for checking if shift is pressed during rendering
    bool is_shift_down_ = false;
};
