#include "ScreenEditGame.h"

#include <ranges>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "../Editor/EditConstants.h"
#include "../Editor/EditorGUI.h"
#include "../Graphics/OpenGL/GLUtils.h"
#include "../Util/ImGuiExtras.h"
#include "../Util/Keyboard.h"
#include "../Util/Util.h"

namespace
{
    constexpr std::array<const char*, 26> TEXTURE_NAMES = {
        "Red Bricks", "Grey Bricks", "Stone Bricks", "Stone Bricks Mossy",
        "Bars",       "Chain Fence", "Grass",        "Dirt",
        "Glass",      "Sand",        "Bark",         "Leaf",
        "Planks",     "Rock",        "Stucco",       "Ancient",
        "Blank",      "Happy",       "SciFi",        "Tiles",
        "Book Case",  "Parquet",     "Tarmac",       "Large Stone Bricks",
        "Slate",      "Board",
    };

    glm::ivec2 map_pixel_to_tile(glm::vec2 point, const Camera& camera)
    {
        auto scale = HALF_TILE_SIZE;
        auto& transform = camera.transform.position;
        auto cam_scale = camera.get_orthographic_scale();
        return {
            std::round((point.x * cam_scale + transform.x) / scale) * scale,
            std::round((point.y * cam_scale + transform.y) / scale) * scale,
        };
    }

    std::filesystem::path make_level_path(const std::string& level_name)
    {
        return "levels/" + level_name + ".cly";
    }
} // namespace

ScreenEditGame::ScreenEditGame(ScreenManager& screens)
    : Screen(screens)
    , camera_(CameraConfig{
          .type = CameraType::Perspective,
          .viewport_size = {window().getSize().x / 2, window().getSize().y},
          .near = 0.1f,
          .far = 1000.0f,
          .fov = 90.0f,
      })
    , drawing_pad_({window().getSize().x / 2, window().getSize().y}, editor_state_.node_hovered)
    , action_manager_(editor_state_, level_)
    , object_move_handler_(level_, action_manager_)
    , copy_paste_handler_(level_, action_manager_)
    , picker_fbo_(window().getSize().x, window().getSize().y)
{
}

ScreenEditGame::ScreenEditGame(ScreenManager& screens, std::string level_name)
    : Screen(screens)
    , camera_(CameraConfig{
          .type = CameraType::Perspective,
          .viewport_size = {window().getSize().x / 2, window().getSize().y},
          .near = 0.1f,
          .far = 1000.0f,
          .fov = 90.0f,
      })
    , drawing_pad_({window().getSize().x / 2, window().getSize().y}, editor_state_.node_hovered)
    , action_manager_(editor_state_, level_)
    , object_move_handler_(level_, action_manager_)
    , copy_paste_handler_(level_, action_manager_)
    , level_name_(level_name)
    , level_name_actual_(level_name)
    , picker_fbo_(window().getSize().x, window().getSize().y)
{
}

bool ScreenEditGame::on_init()
{

    tool_ = std::make_unique<CreateWallTool>();
    if (!drawing_pad_.init())
    {
        return false;
    }

    // -----------------------
    // ==== Load textures ====
    // -----------------------
    texture_.create(16, 32, gl::TEXTURE_PARAMS_NEAREST);
    for (auto& texture : TEXTURE_NAMES)
    {
        std::string name = texture;
        name.erase(std::remove_if(name.begin(), name.end(), [](char c) { return c == ' '; }),
                   name.end());

        if (!level_textures_.register_texture(texture, "assets/textures/World/" + name + ".png",
                                              texture_))
        {
            return false;
        }
    }

    // ---------------------------
    // ==== Buffer thr meshes ====
    // ---------------------------
    selection_mesh_.buffer();

    // ----------------------------
    // ==== Load scene shaders ====
    // ----------------------------
    // Load the shader for the basic parts of a scene
    scene_shader_.add_replace_word({"TEX_COORD_LENGTH", "vec2"});
    scene_shader_.add_replace_word({"SAMPLER_TYPE", "sampler2D"});
    if (!scene_shader_.load_stage("assets/shaders/Scene/SceneVertex.glsl",
                                  gl::ShaderType::Vertex) ||
        !scene_shader_.load_stage("assets/shaders/Scene/SceneFragment.glsl",
                                  gl::ShaderType::Fragment) ||
        !scene_shader_.link_shaders())
    {
        return false;
    }
    scene_shader_.set_uniform("diffuse", 0);

    // Load the shader for world geometry. This is a separate shader
    // as it needs to use 3D texture coords to work with GL_TEXTURE_2D_ARRAY
    world_geometry_shader_.add_replace_word({"TEX_COORD_LENGTH", "vec3"});
    world_geometry_shader_.add_replace_word({"SAMPLER_TYPE", "sampler2DArray"});
    if (!world_geometry_shader_.load_stage("assets/shaders/Scene/SceneVertex.glsl",
                                           gl::ShaderType::Vertex) ||
        !world_geometry_shader_.load_stage("assets/shaders/Scene/SceneFragment.glsl",
                                           gl::ShaderType::Fragment) ||
        !world_geometry_shader_.link_shaders())
    {
        return false;
    }
    world_geometry_shader_.set_uniform("diffuse", 0);

    // -------------------------------------------
    // ==== Set up the picker FBO and shader  ====
    // -------------------------------------------
    picker_fbo_.attach_colour(gl::TextureFormat::R32I).attach_renderbuffer();
    if (!picker_fbo_.is_complete())
    {
        return false;
    }

    if (!picker_shader_.load_stage("assets/shaders/Scene/PickerVertex.glsl",
                                   gl::ShaderType::Vertex) ||
        !picker_shader_.load_stage("assets/shaders/Scene/PickerFragment.glsl",
                                   gl::ShaderType::Fragment) ||
        !picker_shader_.link_shaders())
    {
        return false;
    }

    // -------------------------
    // ==== Set up the SSBO ====
    // -------------------------
    // Note this an SSBO just as an example - this works fine as a UBO as well.
    matrices_ssbo_.create_as_ssbo<glm::mat4>(0, 2);

    // -----------------------------------
    // ==== Entity Transform Creation ====
    // -----------------------------------
    camera_.transform = {.position = {WORLD_SIZE / 2, 12, WORLD_SIZE + 3},
                         .rotation = {-40, 270.0f, 0.0f}};

    // Load the level if the name has been set already
    if (!level_name_.empty())
    {
        level_.load(make_level_path(level_name_));
    }

    // Misc
    if (!grid_.init())
    {
        return false;
    }

    return true;
}

void ScreenEditGame::on_event(const sf::Event& event)
{
    if (showing_dialog())
    {
        return;
    }

    // CTRL C and CTRL V handling
    copy_paste_handler_.handle_events(event, editor_state_.selection, editor_state_.current_floor);

    // True when objects are moved - prevents placing objects where objects are moved to
    auto move_finished = object_move_handler_.handle_move_events(
        event, editor_state_, tool_ ? tool_->get_tool_type() : ToolType::CreateWall);

    // Certain events cause issues if the current tool is UpdateWall (such as rendering the 2D
    // preview of deleting walls) so this prevents that.
    bool try_set_tool_to_wall = false;

    if (auto key = event.getIf<sf::Event::KeyReleased>())
    {
        switch (key->code)
        {
            case sf::Keyboard::Key::L:
                rotation_locked_ = !rotation_locked_;
                window().setMouseCursorVisible(rotation_locked_);
                break;

            case sf::Keyboard::Key::Delete:
                if (editor_state_.selection.has_selection())
                {
                    auto [objects, floors] =
                        level_.copy_objects_and_floors(editor_state_.selection.objects);

                    action_manager_.push_action(
                        std::make_unique<DeleteObjectAction>(objects, floors));
                    try_set_tool_to_wall = true;
                }
                break;

            // Undo functionality with CTRL+Z
            case sf::Keyboard::Key::Z:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
                {
                    action_manager_.undo_action();
                    try_set_tool_to_wall = true;
                }
                break;

            // Redo functionality with CTRL+Y
            case sf::Keyboard::Key::Y:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
                {
                    action_manager_.redo_action();
                    try_set_tool_to_wall = true;
                }
                break;

            case sf::Keyboard::Key::Escape:
                if (level_.changes_made_since_last_save())
                {
                    std::println("TODO: Implement saving before loading menu");
                }
                exit_editor();
                break;

            // Rotate
            case sf::Keyboard::Key::R:

                if (editor_state_.selection.has_selection())
                {
                    auto [objects, _] =
                        level_.copy_objects_and_floors(editor_state_.selection.objects);
                    auto cached = objects;
                    for (auto& object : objects)
                    {
                        object.rotate(editor_state_.node_hovered);
                    }
                    action_manager_.push_action(
                        std::make_unique<BulkUpdateObjectAction>(cached, objects));
                }

                break;

            default:
                break;
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        editor_state_.node_hovered =
            map_pixel_to_tile({mouse->position.x, mouse->position.y}, drawing_pad_.get_camera());
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (ImGui::GetIO().WantCaptureMouse)
        {
            return;
        }
        auto window_size = sf::Vector2i{window().getSize()};
        auto clicked_2d = mouse->position.x < window_size.x / 2 && editor_settings_.show_2d_view;
        auto clicked_3d = !clicked_2d || !editor_settings_.show_2d_view;

        // Try to select an object (2D view)
        if (mouse->button == sf::Mouse::Button::Right && clicked_2d &&
            editor_settings_.show_2d_view)
        {
            auto selection = level_.try_select(
                map_pixel_to_tile({mouse->position.x, mouse->position.y},
                                  drawing_pad_.get_camera()),
                editor_state_.selection.p_active_object, editor_state_.current_floor);

            if (selection)
            {
                select_object(selection);
            }
            else
            {
                editor_state_.selection.clear_selection();
                // Nothing was selected, default back to CreateWallTool if currently selecting a
                // wall
                try_set_tool_to_create_wall();
            }
        }
        else if (mouse->button == sf::Mouse::Button::Right && clicked_3d)
        {
            int x = sf::Mouse::getPosition(window()).x;

            // When the 2D view is showing, the 3D view is half the screen
            if (editor_settings_.show_2d_view)
            {
                x -= window_size.x / 2;
            }

            // The Y view must be inverted as the mouse click origin is th window top-left, but the
            // OpenGL texture origin is the bottom left
            int y = window_size.y - sf::Mouse::getPosition(window()).y - 1;

            int max_x = editor_settings_.show_2d_view ? window_size.x / 2 : window_size.x;
            if (x >= 0 && x < max_x && y >= 0 && y < window_size.y)
            {
                mouse_picker_point_ = {x, y};
                try_pick_3d_ = true;
            }
        }
    }

    if (!object_move_handler_.is_moving_objects() && !move_finished)
    {
        tool_->on_event(event, editor_state_.node_hovered, editor_state_, action_manager_);
    }

    if (move_finished)
    {
        if (tool_->get_tool_type() == ToolType::UpdateWall)
        {
            // When updating a wall, this ensures the the start/end render points are drawn in the
            // correct location
            assert(editor_state_.selection.p_active_object);
            auto object = editor_state_.selection.p_active_object;
            if (auto wall = std::get_if<WallObject>(&object->object_type))
            {
                auto floor = level_.get_object_floor(object->object_id);
                tool_ = std::make_unique<UpdateWallTool>(*object, *wall, *floor);
            }
        }
        else if (tool_->get_tool_type() == ToolType::AreaSelectTool)
        {
            // After the selection has been moved, the old selection area is invalidated
            // TOOD: Find a way to move the selection rather than just set to wall
            tool_ = std::make_unique<CreateWallTool>();
        }
    }

    if (try_set_tool_to_wall)
    {
        try_set_tool_to_create_wall();
    }
}

void ScreenEditGame::on_update(const Keyboard& keyboard, sf::Time dt)
{
    if (showing_dialog())
    {
        return;
    }
    free_camera_controller(keyboard, camera_, dt, camera_keybinds_, window(), rotation_locked_);
    drawing_pad_.update(keyboard, dt);

    if (editor_settings_.always_center_2d_to_3d_view)
    {
        set_2d_to_3d_view();
    }

    // For 3D mouse picking multiple objects
    is_shift_down_ = keyboard.is_key_down(sf::Keyboard::Key::LShift);
}

void ScreenEditGame::on_fixed_update([[maybe_unused]] sf::Time dt)
{
}

void ScreenEditGame::on_render(bool show_debug)
{

    //=============================================
    //          Render the 2D View
    //=============================================
    if (editor_settings_.show_2d_view)
    {
        glViewport(0, 0, window().getSize().x / 2, window().getSize().y);

        if (tool_->get_tool_type() == ToolType::UpdateWall || !ImGui::GetIO().WantCaptureMouse)
        {
            if (!object_move_handler_.is_moving_objects())
            {
                tool_->render_preview_2d(drawing_pad_, editor_state_);
            }
        }

        level_.render_2d(drawing_pad_, editor_state_.selection.objects, editor_state_.current_floor,
                         object_move_handler_.get_move_offset());

        // Finalise 2d rendering
        drawing_pad_.display(camera_.transform);

        // When showing the 2D view, the 3D view is half width
        glViewport(window().getSize().x / 2, 0, window().getSize().x / 2, window().getSize().y);
    }
    else
    {
        // Render the 3D view as full width
        glViewport(0, 0, window().getSize().x, window().getSize().y);
    }

    //=============================================
    //      Render the 3D View
    // ============================================
    // Update the shader buffers
    matrices_ssbo_.buffer_sub_data(0, camera_.get_projection_matrix());
    matrices_ssbo_.buffer_sub_data(sizeof(glm::mat4), camera_.get_view_matrix());

    scene_shader_.bind();

    // Set up the capabilities/ render states
    gl::enable(gl::Capability::DepthTest);
    gl::enable(gl::Capability::CullFace);
    gl::cull_face(gl::Face::Back);
    gl::polygon_mode(gl::Face::FrontAndBack,
                     settings_.wireframe ? gl::PolygonMode::Line : gl::PolygonMode::Fill);

    // Draw grid
    if (editor_settings_.show_grid)
    {
        grid_.render(camera_.transform.position, editor_state_.current_floor);
    }
    scene_shader_.bind();
    //=============================================
    //      Render the actual level and previews
    // ============================================
    // Draw the selection node
    if (tool_->get_tool_type() == ToolType::CreateWall)
    {
        scene_shader_.set_uniform(
            "model_matrix",
            create_model_matrix({.position = {editor_state_.node_hovered.x / TILE_SIZE,
                                              editor_state_.current_floor * FLOOR_HEIGHT,
                                              editor_state_.node_hovered.y / TILE_SIZE}}));
        scene_shader_.set_uniform("use_texture", false);
        selection_mesh_.bind().draw_elements();
    }

    // Draw the current tool preview
    world_geometry_shader_.bind();
    texture_.bind(0);
    world_geometry_shader_.set_uniform("use_texture", true);
    world_geometry_shader_.set_uniform("model_matrix", create_model_matrix({}));

    // Render the level itself
    // All objects have their positions baked and are rendered where they are created. Selected
    // objects, however, have their positions moved by the given offset for when they are being
    // moved around
    auto offset = object_move_handler_.get_move_offset();
    level_.render(world_geometry_shader_, editor_state_.selection.objects,
                  editor_state_.current_floor, {offset.x, 0, offset.y});

    if (!object_move_handler_.is_moving_objects())
    {
        tool_->render_preview();
    }

    // Ensure GUI etc are rendered using fill
    gl::polygon_mode(gl::Face::FrontAndBack, gl::PolygonMode::Fill);

    //======================================
    //      3D Mouse Picking Objects
    // =====================================
    if (try_pick_3d_)
    {
        picker_fbo_.bind(gl::FramebufferTarget::Framebuffer, false);
        picker_shader_.bind();

        GLint clear_value = -1;
        glClear(GL_DEPTH_BUFFER_BIT);
        glClearNamedFramebufferiv(picker_fbo_.id, GL_COLOR, 0, &clear_value);

        auto window_size = window().getSize();
        glViewport(0, 0, window_size.x / (editor_settings_.show_2d_view ? 2 : 1), window_size.y);

        // Render the scene to texture's single channel texture containing object IDs
        level_.render_to_picker(picker_shader_, editor_state_.current_floor);

        // The pixel's value on the image maps to a LevelObject's id value
        GLint picked_object_id = 0;
        glReadPixels(mouse_picker_point_.x, mouse_picker_point_.y, 1, 1, GL_RED_INTEGER, GL_INT,
                     &picked_object_id);

        // If the pixel was non-empty, then try to select the corrsponding object
        if (picked_object_id > -1)
        {
            if (auto object = level_.get_object(picked_object_id))
            {
                select_object(object);
            }
        }
        else
        {
            editor_state_.selection.clear_selection();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    try_pick_3d_ = false;

    //=============================
    //     Render the ImGUI
    // ============================
    show_menu_bar();
    render_editor_ui();
    if (show_debug)
    {
        debug_gui();
    }

    if (editor_settings_.show_history)
    {
        action_manager_.display_action_history();
    }

    // Dialogs for saving and loading levels from the disk
    if (show_save_dialog_)
    {
        show_save_dialog();
    }

    if (show_load_dialog_)
    {
        if (display_level_list(show_load_dialog_, level_name_))
        {
            show_load_dialog_ = false;

            // Reset the state
            action_manager_.clear();
            editor_state_.selection.clear_selection();

            level_.load(make_level_path(level_name_));
            level_name_actual_ = level_name_;
        }
    }

    if (tool_)
    {
        tool_->show_gui(editor_state_);
    }
}

void ScreenEditGame::select_object(LevelObject* object)
{
    if (is_shift_down_)
    {
        editor_state_.selection.add_to_selection(object);
        try_set_tool_to_create_wall();
    }
    else
    {
        editor_state_.selection.set_selection(object);

        if (editor_settings_.jump_to_selection_floor)
        {
            if (auto floor = level_.get_object_floor(object->object_id))
            {
                editor_state_.current_floor = *floor;
            }
        }

        // Editing a wall requires a special tool to enable resizing, so after a object
        // it switches between tools
        if (auto wall = std::get_if<WallObject>(&object->object_type))
        {
            auto floor = level_.get_object_floor(object->object_id);
            tool_ = std::make_unique<UpdateWallTool>(*object, *wall, *floor);
        }
        else
        {
            tool_ = std::make_unique<CreateWallTool>();
        }
    }
}

void ScreenEditGame::render_editor_ui()
{
    // Draw the editor gui itself, such as property editors, changing floors, etc
    if (ImGui::Begin("Editor"))
    {
        // Display the list of objects that can be placed
        ImGui::Text("Tools");
        ImGui::Separator();
        if (ImGui::Button("Wall"))
        {
            tool_ = std::make_unique<CreateWallTool>();
            editor_state_.selection.clear_selection();
        }
        ImGui::SameLine();
        if (ImGui::Button("Platform"))
        {
            tool_ = std::make_unique<CreateObjectTool>(ObjectTypeName::Platform);
            editor_state_.selection.clear_selection();
        }
        ImGui::SameLine();
        if (ImGui::Button("Polygon"))
        {
            tool_ = std::make_unique<CreateObjectTool>(ObjectTypeName::PolygonPlatform);
            editor_state_.selection.clear_selection();
        }

        if (ImGui::Button("Pillar"))
        {
            tool_ = std::make_unique<CreateObjectTool>(ObjectTypeName::Pillar);
            editor_state_.selection.clear_selection();
        }
        ImGui::SameLine();
        if (ImGui::Button("Ramp"))
        {
            tool_ = std::make_unique<CreateObjectTool>(ObjectTypeName::Ramp);
            editor_state_.selection.clear_selection();
        }

        ImGui::Separator();
        if (ImGui::Button("Area Selection"))
        {
            tool_ = std::make_unique<AreaSelectTool>(level_);
            editor_state_.selection.clear_selection();
        }

        ImGui::Separator();

        // Display the floor options, so going up or down a floor
        ImGui::Text("Floors");
        if (ImGui::Button("Floor Down"))
        {
            level_.ensure_floor_exists(--editor_state_.current_floor);
            camera_.transform.position.y -= FLOOR_HEIGHT;
        }
        ImGui::SameLine();
        if (ImGui::Button("Floor Up"))
        {
            level_.ensure_floor_exists(++editor_state_.current_floor);
            camera_.transform.position.y += FLOOR_HEIGHT;
        }
        ImGui::Text("Lowest: %d - Current: %d - Highest: %d", level_.get_min_floor(),
                    editor_state_.current_floor, level_.get_max_floor());

        ImGui::Separator();
        if (ImGui::Button("Center 2D View To 3D Camera"))
        {
            set_2d_to_3d_view();
        }
        ImGui::Separator();
        drawing_pad_.camera_gui();
        ImGui::Separator();

        ImGuiExtras::EnumSelect(
            "Editor Mode", editor_state_.edit_mode,
            "-Legacy: Options based on the original ChallengeYou editor.\n-Extended: "
            "Adds  options such as custom wall start/end heights, and texturing both "
            "sides of  a platform differently.\n-Advanced: Adds advanced options "
            "such as extending heights of walls and pillars beyond a single floor.");
    }
    ImGui::End();

    // When an object is selected, its properties is rendered
    if (editor_state_.selection.single_object_is_selected())
    {
        if (ImGui::Begin("Object Properties"))
        {
            editor_state_.selection.p_active_object->property_gui(editor_state_, level_textures_,
                                                                  action_manager_);
        }
        ImGui::End();
    }
}

void ScreenEditGame::exit_editor()
{
    level_.save(make_level_path("backup"));
    p_screen_manager_->pop_screen();
    window().setMouseCursorVisible(true);
}

void ScreenEditGame::save_level()
{
    // Ensure a level name is actually set before saving
    if (level_name_.empty())
    {
        show_save_dialog_ = true;
    }
    else
    {
        if (level_.save(make_level_path(level_name_)))
        {
            level_name_actual_ = level_name_;
        }
        show_save_dialog_ = false;
    }
}

void ScreenEditGame::show_save_dialog()
{
    if (ImGuiExtras::BeginCentredWindow("Save As...", {300, 200}))
    {
        ImGui::InputText("Level Name", &level_name_);
        bool can_save = !level_name_.empty();

        // Only enable the save button if a name is actually set
        // clang-format off
        if (!can_save)                  { ImGui::BeginDisabled();    } 
        if (ImGui::Button("Save Game")) { save_level();              }
        if (!can_save)                  { ImGui::EndDisabled();      }
        if (ImGui::Button("Cancel"))    { show_save_dialog_ = false; }
        // clang-format on
    }
    ImGui::End();
}

void ScreenEditGame::show_menu_bar()
{
    // clang-format off
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            // if (ImGui::MenuItem("New")) {  }
            if (ImGui::MenuItem("Open...")) 
            { 
                if (level_.changes_made_since_last_save())
                {
                    std::println("TODO: Implement saving before loading menu");
                }
                show_load_dialog_ = true;
            }
            if (ImGui::MenuItem("Save"))        { save_level();             }
            if (ImGui::MenuItem("Save As..."))  { show_save_dialog_ = true; }
            if (ImGui::MenuItem("Exit"))        { exit_editor();            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo (CTRL + Z)")) { action_manager_.undo_action(); }
            if (ImGui::MenuItem("Redo (CTRL + Y)")) { action_manager_.redo_action(); }

            if (ImGui::MenuItem("Copy (CTRL + C)")) 
            { 
                copy_paste_handler_.copy_selection(editor_state_.selection, editor_state_.current_floor); 
            }
            if (ImGui::MenuItem("Paste (CTRL + V)")) 
            { 
                copy_paste_handler_.paste_selection(editor_state_.current_floor); 
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::Checkbox("Lock 2D To 3D View?", &editor_settings_.always_center_2d_to_3d_view);
            ImGui::Checkbox("Show History?", &editor_settings_.show_history);
            ImGui::Checkbox("Show Grid?", &editor_settings_.show_grid);
            if (ImGui::Checkbox("Show 2D View? (Full Screen 3D)", &editor_settings_.show_2d_view))
            {
                auto factor = editor_settings_.show_2d_view ? 2 : 1;
                camera_.set_viewport_size({window().getSize().x / factor, window().getSize().y});
            }
            ImGui::Checkbox("Auto-Jump to selection floor?", &editor_settings_.jump_to_selection_floor);

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // clang-format on
}

void ScreenEditGame::debug_gui()
{
    camera_.gui("Camera");

    // clang-format off
    if (ImGui::Begin("Camera Kind"))
    {
        if (ImGui::Button("Perspective"))   { camera_.set_type(CameraType::Perspective);        }
        if (ImGui::Button("Orthographic"))  { camera_.set_type(CameraType::OrthographicWorld);  }
    }

    // clang-format on
    ImGui::End();
}

bool ScreenEditGame::showing_dialog() const
{
    return show_save_dialog_ || show_load_dialog_;
}

void ScreenEditGame::set_2d_to_3d_view()
{
    drawing_pad_.set_camera_position({
        camera_.transform.position.x * TILE_SIZE,
        camera_.transform.position.z * TILE_SIZE,
    });
}

void ScreenEditGame::try_set_tool_to_create_wall()
{
    // When the current tool is updating walls, it can cause be a bit jarring when doing things such
    // as selecting multiple objects or moving between floors.
    // For example, selecting a wall and then moving up a floor, you should not be able to then
    // resize that wall from the "wrong floor"
    // So this explictly prevents that from happening
    if (tool_ && tool_->get_tool_type() == ToolType::UpdateWall)
    {
        tool_ = std::make_unique<CreateWallTool>();
    }
}
