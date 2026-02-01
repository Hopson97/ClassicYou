#pragma once

#include "../Editor/Actions.h"
#include "../Editor/EditConstants.h"
#include "../Editor/EditorEventHandlers.h"
#include "../Editor/EditorLevel.h"
#include "../Editor/EditorSettings.h"
#include "../Editor/EditorState.h"
#include "../Editor/Grids.h"
#include "../Editor/LevelFileIO.h"
#include "../Editor/LevelTextures.h"
#include "../Editor/ObjectPropertyEditors/LevelObjectPropertyEditor.h"
#include "../Editor/Tools/Tool.h"
#include "../Graphics/Camera.h"
#include "../Graphics/CameraController.h"
#include "../Graphics/Mesh.h"
#include "../Graphics/OpenGL/BufferObject.h"
#include "../Graphics/OpenGL/Framebuffer.h"
#include "../Util/ImGuiExtras.h"
#include "Screen.h"

class ScreenEditGame final : public Screen
{
  public:
    ScreenEditGame(ScreenManager& screens);
    ScreenEditGame(ScreenManager& screens, std::string level_name);

    ScreenEditGame(const ScreenEditGame&) = delete;
    ScreenEditGame& operator=(const ScreenEditGame&) = delete;
    ScreenEditGame(ScreenEditGame&&) = delete;
    ScreenEditGame& operator=(ScreenEditGame&&) = delete;

    ~ScreenEditGame();

    bool on_init() override;
    void on_event(const sf::Event& event) override;
    void on_update(const Keyboard& keyboard, sf::Time dt) override;
    void on_fixed_update(sf::Time dt) override;
    void on_render(bool show_debug) override;

  private:
    /// Sets or adds the given object to the selection (Such as when right clicking an object).
    /// Selecting walls sets the tool type to be "UpdateWallTool" such that it can be resized
    void select_object(LevelObject* object);

    /// Creates property editors for the given object that enable editing via the views
    void create_property_editors(LevelObject* object);

    /// Exit to the main menu, saving the current level to "backup/backup.cly"
    void exit_editor();

    // Returns true if ANY dialog is showing (saving, loading)
    bool showing_dialog() const;

    // Sets the tool to create wall if the current tool is "UpdateWallTool"
    void try_set_tool_to_create_wall();

    /// If the current tool is UpdateWallTool or UpdatePolygonTool or similar, then this resets it
    /// to be the current selected object. This is done as if the wall properties change while it is
    /// selected, and then the wall is moved, then the moved wall will have "old" props as it is
    /// cached when the tool is created.
    void try_update_object_tools();

    /// Set the position of the 2D camera to be the same position as the 3D camera
    void set_2d_to_3d_view();

    // When the floor is increased/decreased, this can be used to jump the camera to the new floor
    void offset_camera_to_floor(int old_floor);

    void increase_floor();
    void decrease_floor();

    /// Loads a level from disk (Loads "level_name_")
    bool load_level();

    /// Saves the current level to disk
    void save_level(const std::string& name);

    // Saves the level or opens the save dialog if a level name has not been seleted
    void save_level();

    // GUI Functions
    void display_editor_gui();
    void display_menu_bar_gui();
    void display_save_as_gui();
    void display_debug_gui();

    void undo();
    void redo();

    bool mouse_in_2d_view() const;

    void enable_mouse_picking(MousePickingState& state, MousePickingState::Action action,
                              sf::Mouse::Button button, sf::Vector2i mouse_point);

    /// Ensure the 3D view is set up to be half width or full width
    void setup_camera_3d();

  private:
    Camera camera_3d_;
    Camera camera_2d_;

    gl::BufferObject matrices_ssbo_;
    gl::Shader scene_shader_;
    gl::Shader drawing_pad_shader_;

    // Used for rendering the level geometry
    gl::Shader world_geometry_shader_;

    sf::Vector2i mouse_position_;

    /// Shader for viewing the normals for debugging light issues etc
    gl::Shader world_normal_shader_;

    // Camera
    CameraKeybinds camera_keybinds_3d_;
    CameraKeybinds camera_keybinds_2d_;
    CameraControllerOptions3D camera_controller_options_3d_;
    CameraController3D camera_controller_3d_;
    CameraController2D camera_controller_2d_;

    Mesh3D sun_mesh_ = generate_cube_mesh({3.0f, 3.0f, 3.0f});
    Mesh2DWorld arrow_mesh_;

    /// Grid Mesh
    InfiniteGrid grid_;
    Grid2D grid_2d_;

    /// State of the editor such as the currently selected object and defaults
    EditorState editor_state_;

    EditorSettings editor_settings_;

    /// Wrapper around the selectable texture list
    LevelTextures level_texture_map_;
    LevelTextures drawing_pad_texture_map_;

    /// 2D Texture arrays to texture the world geometry/ drawing pad
    gl::Texture2DArray world_textures_;
    gl::Texture2DArray drawing_pad_textures_;

    EditorLevel level_;
    std::unique_ptr<ITool> tool_;

    /// History of actions performed, has functions to undo/redo
    ActionManager action_manager_;

    ObjectMoveHandler object_move_handler_;
    CopyPasteHandler copy_paste_handler_;

    bool show_save_dialog_ = false;

    /// Level name is used in the save dialog such that the actual name is not overriden if the save
    /// operation is cancelled
    std::string level_name_;
    std::string level_name_actual_;

    // When doing mouse picking in the 3D view, the scene is rendered to this
    gl::Framebuffer picker_fbo_;
    gl::Shader picker_shader_;

    MousePickingState mouse_picking_click_state_;
    MousePickingState mouse_picking_move_state_;

    // Used for checking if shift is pressed when picking objects
    bool is_shift_down_ = false;

    LevelFileSelectGUI level_file_selector_;

    MessagesManager messages_manager_;

    LevelObjectPropertyEditors property_updater_;
};
