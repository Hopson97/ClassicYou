#include "ScreenEditGame.h"

#include <fstream>
#include <ranges>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "../Editor/EditConstants.h"
#include "../Editor/EditorGUI.h"
#include "../Editor/LevelFileIO.h"
#include "../Graphics/OpenGL/GLUtils.h"
#include "../Util/ImGuiExtras.h"
#include "../Util/Keyboard.h"
#include "../Util/Util.h"

namespace
{
    constexpr std::array TEXTURE_NAMES = {
        "Red Bricks", "Grey Bricks", "Stone Bricks", "Stone Bricks Mossy",
        "Bars",       "Chain Fence", "Grass",        "Dirt",
        "Glass",      "Sand",        "Bark",         "Leaf",
        "Planks",     "Rock",        "Stucco",       "Ancient",
        "Blank",      "Happy",       "SciFi",        "Tiles",
        "Book Case",  "Parquet",     "Tarmac",       "Large Stone Bricks",
        "Slate",      "Board",
    };

    constexpr std::array TEXTURE_NAMES_2D = {
        "Arrow", "Selection", "SelectCircle", "Platform", "Ramp", "Pillar",
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

    constexpr float CAMERA_BASE_Y = 10.0f;

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
    , camera_2d_(CameraConfig{
          .type = CameraType::OrthographicScreen,
          .viewport_size = {window().getSize().x / 2, window().getSize().y},
          .near = 0.5f,
          .far = 1000.0f,
      })
      , keybinds_2d_{
          .forward = sf::Keyboard::Key::Down,
          .left = sf::Keyboard::Key::Left,
          .right = sf::Keyboard::Key::Right,
          .back = sf::Keyboard::Key::Up,
      }
    , level_(drawing_pad_texture_map_)
    , action_manager_(editor_state_, level_)
    , object_move_handler_(level_, action_manager_)
    , copy_paste_handler_(level_, action_manager_, messages_manager_)
    , picker_fbo_(window().getSize().x, window().getSize().y)
{
}

ScreenEditGame::ScreenEditGame(ScreenManager& screens, std::string level_name)
    : ScreenEditGame(screens)
{
    level_name_ = level_name;
    level_name_actual_ = level_name;
}

ScreenEditGame::~ScreenEditGame()
{
    editor_settings_.save();
}

bool ScreenEditGame::on_init()
{
    // Start with loading settings
    editor_settings_.load();

    // -----------------------
    // ==== Load textures ====
    // -----------------------
    world_textures_.create(16, static_cast<GLint>(TEXTURE_NAMES.size()),
                           gl::TEXTURE_PARAMS_NEAREST);
    for (auto& texture : TEXTURE_NAMES)
    {
        std::string name = texture;
        name.erase(std::remove_if(name.begin(), name.end(), [](char c) { return c == ' '; }),
                   name.end());

        if (!level_texture_map_.register_texture(texture, "assets/textures/World/" + name + ".png",
                                                 world_textures_))
        {
            return false;
        }
    }

    drawing_pad_textures_.create(16, static_cast<GLint>(TEXTURE_NAMES_2D.size()),
                                 gl::TEXTURE_PARAMS_NEAREST);
    for (auto& texture : TEXTURE_NAMES_2D)
    {
        if (!drawing_pad_texture_map_.register_texture(
                texture, "assets/textures/DrawingPad/" + std::string{texture} + ".png",
                drawing_pad_textures_))
        {
            return false;
        }
    }

    // ---------------------------
    // ==== Buffer the meshes ====
    // ---------------------------
    arrow_mesh_ = generate_2d_quad_mesh({0, 0}, {16, 16},
                                        (float)*drawing_pad_texture_map_.get_texture("Arrow"));
    arrow_mesh_.buffer();
    selection_mesh_.buffer();
    sun_mesh_.buffer();

    // ----------------------------
    // ==== Load scene shaders ====
    // ----------------------------
    // clang-format off
    // Load the shader for the basic parts of a scene
    scene_shader_.add_replace_word({"TEX_COORD_LENGTH", "vec2"});
    scene_shader_.add_replace_word({"SAMPLER_TYPE", "sampler2D"});
    if (!scene_shader_.load_stage("assets/shaders/Scene/SceneVertex.glsl",      gl::ShaderType::Vertex) ||
        !scene_shader_.load_stage("assets/shaders/Scene/SceneFragment.glsl",    gl::ShaderType::Fragment) ||
        !scene_shader_.link_shaders())
    {
        return false;
    }
    scene_shader_.set_uniform("diffuse", 0);

    // Load the shader for world geometry. This is a separate shader
    // as it needs to use 3D texture coords to work with GL_TEXTURE_2D_ARRAY
    world_geometry_shader_.add_replace_word({"TEX_COORD_LENGTH", "vec3"});
    world_geometry_shader_.add_replace_word({"SAMPLER_TYPE", "sampler2DArray"});
    if (!world_geometry_shader_.load_stage("assets/shaders/Scene/SceneVertex.glsl",     gl::ShaderType::Vertex) ||
        !world_geometry_shader_.load_stage("assets/shaders/Scene/SceneFragment.glsl",   gl::ShaderType::Fragment) ||
        !world_geometry_shader_.link_shaders())
    {
        return false;
    }
    world_geometry_shader_.set_uniform("diffuse", 0);

    // Load the shader for the 2D view
    if (!drawing_pad_shader_.load_stage("assets/shaders/Scene/Scene2DVertex.glsl",   gl::ShaderType::Vertex) ||
        !drawing_pad_shader_.load_stage("assets/shaders/Scene/Scene2DFragment.glsl", gl::ShaderType::Fragment) ||
        !drawing_pad_shader_.link_shaders())
    {
        return false;
    }
    drawing_pad_shader_.set_uniform("base_texture", 0);
    drawing_pad_shader_.set_uniform("world_texture", 1);

    // Load the shader for showing vertex normals
    if (!world_normal_shader_.load_stage("assets/shaders/Normals/NormalVisualiserVertex.glsl",      gl::ShaderType::Vertex) ||
        !world_normal_shader_.load_stage("assets/shaders/Normals/NormalVisualiserGeometry.glsl",    gl::ShaderType::Geometry) ||
        !world_normal_shader_.load_stage("assets/shaders/Normals/NormalVisualiserFragment.glsl",    gl::ShaderType::Fragment) ||
        !world_normal_shader_.link_shaders())
    {
        return false;
    }
    // clang-format on

    // ----------------------------------------------------------------
    // ==== Set up the picker FBO and shader for 3D mouse picking  ====
    // ----------------------------------------------------------------
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
    camera_.transform = {.position = {WORLD_SIZE / 2, CAMERA_BASE_Y, WORLD_SIZE + 3},
                         .rotation = {-40, 270.0f, 0.0f}}; // Slightly pointing down

    // -----------------------------
    // ==== Optional load level ====
    // -----------------------------
    if (!level_name_.empty())
    {
        if (!load_level())
        {
            return false;
        }
    }

    // --------------
    // ==== Misc ====
    // --------------
    if (!grid_.init())
    {
        return false;
    }

    if (!grid_2d_.init())
    {
        return false;
    }

    // Set up the default tool
    tool_ = std::make_unique<CreateWallTool>(drawing_pad_texture_map_);

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
            case sf::Keyboard::Key::T:
                camera_controller_options_.lock_rotation =
                    !camera_controller_options_.lock_rotation;
                window().setMouseCursorVisible(camera_controller_options_.lock_rotation);
                break;

            case sf::Keyboard::Key::L:
                camera_controller_options_.free_movement =
                    !camera_controller_options_.free_movement;

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

            // Up/Down floor
            case sf::Keyboard::Key::PageUp:
                increase_floor();
                break;

            case sf::Keyboard::Key::PageDown:
                decrease_floor();
                break;

            default:
                break;
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        editor_state_.node_hovered =
            map_pixel_to_tile({mouse->position.x, mouse->position.y}, camera_2d_);
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
                map_pixel_to_tile({mouse->position.x, mouse->position.y}, camera_2d_),
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
        tool_->on_event(event, editor_state_.node_hovered, editor_state_, action_manager_,
                        drawing_pad_texture_map_);
    }

    if (move_finished)
    {
        if (tool_->get_tool_type() == ToolType::UpdateWall)
        {
            // When updating a wall, this ensures the the start/end render points are drawn in the
            // correct location
            try_reset_update_wall_tool();
        }
        else if (tool_->get_tool_type() == ToolType::AreaSelectTool)
        {
            // After the selection has been moved, the old selection area is invalidated
            // TODO: Find a way to move the selection rather than just set to wall
            tool_ = std::make_unique<CreateWallTool>(drawing_pad_texture_map_);
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
    free_camera_controller(keyboard, camera_, dt, camera_keybinds_, window(),
                           camera_controller_options_);
    free_camera_controller_2d(keyboard, camera_2d_, dt, keybinds_2d_);

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
        gl::enable(gl::Capability::Blend);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, window().getSize().x / 2, window().getSize().y);

        gl::disable(gl::Capability::DepthTest);
        gl::disable(gl::Capability::CullFace);

        // Render grid underneath
        grid_2d_.render(camera_2d_);

        // Update the shaders
        drawing_pad_shader_.bind();
        drawing_pad_shader_.set_uniform("projection_matrix", camera_2d_.get_projection_matrix());
        drawing_pad_shader_.set_uniform("view_matrix", camera_2d_.get_view_matrix());
        drawing_pad_shader_.set_uniform("model_matrix", create_model_matrix({}));
        drawing_pad_shader_.set_uniform("use_world_texture",
                                        editor_settings_.show_textures_in_2d_view);
        drawing_pad_shader_.set_uniform("texture_mix", editor_settings_.texture_mix);

        drawing_pad_textures_.bind(0);
        world_textures_.bind(1);

        // Render level
        level_.render_2d(drawing_pad_shader_, editor_state_.selection.objects,
                         editor_state_.current_floor, object_move_handler_.get_move_offset());

        // Render the tool preview
        if (tool_->get_tool_type() == ToolType::UpdateWall || !ImGui::GetIO().WantCaptureMouse)
        {
            if (!object_move_handler_.is_moving_objects())
            {
                tool_->render_preview_2d(drawing_pad_shader_);
            }
        }

        // Render the arrow showing where the 3D camera is in the 2D view
        drawing_pad_shader_.set_uniform("use_texture", true);
        drawing_pad_shader_.set_uniform("is_selected", false);
        drawing_pad_shader_.set_uniform("on_floor_below", false);
        drawing_pad_shader_.set_uniform("use_texture_alpha_channel", true);
        drawing_pad_shader_.set_uniform("use_world_texture", false);

        drawing_pad_shader_.set_uniform(
            "model_matrix",
            create_model_matrix_orbit(
                {.position = {camera_.transform.position.x * TILE_SIZE - TILE_SIZE / 4,
                              camera_.transform.position.z * TILE_SIZE, 0},
                 .rotation = {0, 0, camera_.transform.rotation.y + 90.0f}},
                {8, 8, 0}));

        arrow_mesh_.bind().draw_elements();

        // When showing the 2D view, the 3D view is half width
        glViewport(window().getSize().x / 2, 0, window().getSize().x / 2, window().getSize().y);
        gl::disable(gl::Capability::Blend);
    }
    else
    {
        // Render the 3D view as full width
        glViewport(0, 0, window().getSize().x, window().getSize().y);
    }

    //=============================================
    //      Render the 3D View
    // ============================================
    auto& main_light = level_.get_light_settings();
    scene_shader_.bind();
    scene_shader_.set_uniform("main_light_position", main_light.position);
    scene_shader_.set_uniform("main_light_colour", main_light.colour);
    scene_shader_.set_uniform("main_light_brightness", main_light.brightness);

    // Update the shader buffers
    matrices_ssbo_.buffer_sub_data(0, camera_.get_projection_matrix());
    matrices_ssbo_.buffer_sub_data(sizeof(glm::mat4), camera_.get_view_matrix());

    // Set up the capabilities/ render states
    gl::enable(gl::Capability::DepthTest);
    gl::enable(gl::Capability::CullFace);
    gl::cull_face(gl::Face::Back);
    gl::polygon_mode(gl::Face::FrontAndBack, editor_settings_.render_as_wireframe
                                                 ? gl::PolygonMode::Line
                                                 : gl::PolygonMode::Fill);

    scene_shader_.set_uniform("use_texture", false);
    if (tool_->get_tool_type() == ToolType::CreateWall)
    {
        scene_shader_.set_uniform(
            "model_matrix",
            create_model_matrix({.position = {editor_state_.node_hovered.x / HALF_TILE_SIZE_F,
                                              editor_state_.current_floor * FLOOR_HEIGHT,
                                              editor_state_.node_hovered.y / HALF_TILE_SIZE_F}}));
        selection_mesh_.bind().draw_elements();
    }

    if (editor_settings_.render_main_light)
    {
        // Draw main light source position - the cube is 3x3x3 so offset by 1.5
        scene_shader_.set_uniform(
            "model_matrix",
            create_model_matrix({.position = main_light.position - glm::vec3{1.5}}));
        sun_mesh_.bind().draw_elements();
    }

    // Draw grid
    if (editor_settings_.show_grid)
    {
        grid_.render(camera_.transform.position, editor_state_.current_floor);
    }

    //=============================================
    //      Render the level and previews
    // ============================================
    world_geometry_shader_.bind();
    world_textures_.bind(0);
    world_geometry_shader_.set_uniform("use_texture", true);
    world_geometry_shader_.set_uniform("model_matrix", create_model_matrix({}));
    world_geometry_shader_.set_uniform("main_light_position", main_light.position);
    world_geometry_shader_.set_uniform("main_light_colour", main_light.colour);
    world_geometry_shader_.set_uniform("main_light_brightness", main_light.brightness);

    // Render the level itself
    // All objects have their positions baked and are rendered where they are created. Selected
    // objects, however, have their positions moved by the given offset for when they are being
    // moved around
    auto offset = object_move_handler_.get_move_offset();
    level_.render(world_geometry_shader_, editor_state_.selection.objects,
                  editor_state_.current_floor, {offset.x, 0, offset.y});

    // Draw the current tool preview
    if (!object_move_handler_.is_moving_objects())
    {
        tool_->render_preview();
    }

    // Ensure GUI etc are rendered using fill
    gl::polygon_mode(gl::Face::FrontAndBack, gl::PolygonMode::Fill);

    //=======================
    //      Debug Rendering
    // ======================
    if (editor_settings_.render_vertex_normals)
    {
        world_normal_shader_.bind();
        world_normal_shader_.set_uniform("model_matrix", create_model_matrix({}));
        level_.render(world_normal_shader_, editor_state_.selection.objects,
                      editor_state_.current_floor, {offset.x, 0, offset.y});
    }

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
    display_menu_bar_gui();
    display_editor_gui();
    if (show_debug)
    {
        display_debug_gui();
    }

    if (editor_settings_.show_history)
    {
        action_manager_.display_action_history();
    }

    // Dialogs for saving and loading levels from the disk
    if (show_save_dialog_)
    {
        display_save_as_gui();
    }

    if (auto level = level_file_selector_.display_level_select_gui())
    {
        level_name_ = *level;
        // Reset the state
        action_manager_.clear();
        editor_state_.selection.clear_selection();

        load_level();
    }

    if (tool_)
    {
        tool_->show_gui(editor_state_);
    }

    if (editor_settings_.show_messages_log)
    {
        messages_manager_.render();
    }
}

void ScreenEditGame::select_object(LevelObject* object)
{
    // Multi-select when shift is pressed
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
            int old_floor = editor_state_.current_floor;
            if (auto floor = level_.get_object_floor(object->object_id))
            {
                editor_state_.current_floor = *floor;
            }
            offset_camera_to_floor(old_floor);
        }

        // Editing a wall requires a special tool to enable resizing, so after a object
        // it switches between tools
        if (auto wall = std::get_if<WallObject>(&object->object_type))
        {
            auto floor = level_.get_object_floor(object->object_id);
            tool_ =
                std::make_unique<UpdateWallTool>(*object, *wall, *floor, drawing_pad_texture_map_);
        }
        else
        {
            tool_ = std::make_unique<CreateWallTool>(drawing_pad_texture_map_);
        }
    }
}

void ScreenEditGame::exit_editor()
{
    LevelFileIO level_file_io;

    if (level_.serialise(level_file_io))
    {
        save_level("backup");
        level_name_actual_ = level_name_;
    }

    p_screen_manager_->pop_screen();
    window().setMouseCursorVisible(true);
}

void ScreenEditGame::offset_camera_to_floor(int old_floor)
{
    camera_.transform.position.y += FLOOR_HEIGHT * (editor_state_.current_floor - old_floor);
}

void ScreenEditGame::increase_floor()
{
    int old_floor = editor_state_.current_floor;
    level_.ensure_floor_exists(++editor_state_.current_floor);
    offset_camera_to_floor(old_floor);
}

void ScreenEditGame::decrease_floor()
{
    int old_floor = editor_state_.current_floor;
    level_.ensure_floor_exists(--editor_state_.current_floor);
    offset_camera_to_floor(old_floor);
}

bool ScreenEditGame::showing_dialog() const
{
    return show_save_dialog_ || level_file_selector_.is_showing();
}

void ScreenEditGame::set_2d_to_3d_view()
{
    const auto& viewport = camera_2d_.get_config().viewport_size;
    camera_2d_.transform.position = {camera_.transform.position.x * TILE_SIZE - viewport.x / 2,
                                     camera_.transform.position.z * TILE_SIZE - viewport.y / 2, 1};
}

void ScreenEditGame::try_set_tool_to_create_wall()
{
    // When the current tool is updating walls, it can cause be a bit jarring when doing things such
    // as selecting multiple objects or moving between floors.
    // For example, selecting a wall and then moving up a floor, you should not be able to then
    // resize that wall from the "wrong floor"
    // So this explicitly prevents that from happening
    if (tool_ && tool_->get_tool_type() == ToolType::UpdateWall)
    {
        tool_ = std::make_unique<CreateWallTool>(drawing_pad_texture_map_);
    }
}

void ScreenEditGame::try_reset_update_wall_tool()
{
    if (tool_->get_tool_type() == ToolType::UpdateWall)
    {
        // When updating a wall, this ensures the the start/end render points are drawn in
        // the correct location and that the wall is the correct if props get updated
        assert(editor_state_.selection.p_active_object);
        auto object = editor_state_.selection.p_active_object;
        if (auto wall = std::get_if<WallObject>(&object->object_type))
        {
            auto floor = level_.get_object_floor(object->object_id);
            tool_ =
                std::make_unique<UpdateWallTool>(*object, *wall, *floor, drawing_pad_texture_map_);
        }
    }
}

bool ScreenEditGame::load_level()
{
    LevelFileIO level_file_io;
    if (!level_file_io.open(level_name_, false))
    {
        return false;
    }

    if (!level_.deserialise(level_file_io))
    {
        return false;
    }

    auto& main_light = level_.get_light_settings();
    glClearColor(main_light.sky_colour.r, main_light.sky_colour.g, main_light.sky_colour.b, 1.0f);

    messages_manager_.add_message(std::format("Successfully loaded {}.", level_name_));
    return true;
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
        save_level(level_name_);
        level_name_actual_ = level_name_;

        show_save_dialog_ = false;
    }
}

void ScreenEditGame::save_level(const std::string& name)
{
    LevelFileIO level_file_io;

    if (level_.serialise(level_file_io))
    {
        if (level_file_io.save(name, true))
        {
            messages_manager_.add_message(std::format("Successfully saved to {}.", name));
        }
    }
}

// ---------------------------
// ==== GUI Functions ====
// ---------------------------
void ScreenEditGame::display_editor_gui()
{
    // Draw the editor gui itself, such as property editors, changing floors, etc
    if (ImGui::Begin("Editor"))
    {
        // Display the list of objects that can be placed
        ImGui::Text("Tools");
        ImGui::Separator();

        if (ImGui::Button("Wall"))
        {
            tool_ = std::make_unique<CreateWallTool>(drawing_pad_texture_map_);
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
        if (ImGui::Button("Floor Down (PgDown)"))
        {
            decrease_floor();
        }
        ImGui::SameLine();
        if (ImGui::Button("Floor Up (PgUp)"))
        {
            increase_floor();
        }
        ImGui::Text("Lowest: %d - Current: %d - Highest: %d", level_.get_min_floor(),
                    editor_state_.current_floor, level_.get_max_floor());

        if (editor_settings_.show_2d_view)
        {
            ImGui::Separator();
            auto scale = camera_2d_.get_orthographic_scale();
            if (ImGui::SliderFloat("2D Camera Zoom", &scale, 0.5, 2.5))
            {
                camera_2d_.set_orthographic_scale(scale);
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Center 2D View To 3D Camera"))
        {
            set_2d_to_3d_view();
        }
        ImGui::Separator();
        // drawing_pad_.camera_gui();
        ImGui::Separator();

        ImGuiExtras::EnumSelect(
            "Editor Mode", editor_state_.edit_mode,
            "-Legacy: Options based on the original ChallengeYou editor.\n-Extended: "
            "Adds  options such as custom wall start/end heights, and texturing both "
            "sides of  a platform differently.\n-Advanced: Adds advanced options "
            "such as extending heights of walls and pillars beyond a single floor.");
        ImGui::Separator();

        ImGui::Checkbox("Show Level Settings", &editor_settings_.show_level_settings);

        ImGui::Separator();
        if (ImGui::Button("Reset Settings"))
        {
            editor_settings_.set_to_default();
            messages_manager_.add_message("Settings reset to default.");
        }
        if (ImGui::Button("Reset Level Light/Colour Settings"))
        {
            level_.reset_light_settings();
            messages_manager_.add_message("Level light colours and settings reset to default.");
        }
    }
    ImGui::End();

    if (editor_settings_.show_level_settings)
    {
        level_.display_settings_gui();
    }

    // When an object is selected, its properties is rendered
    if (editor_state_.selection.single_object_is_selected())
    {
        if (ImGui::Begin("Object Properties"))
        {
            auto object_updated = editor_state_.selection.p_active_object->property_gui(
                editor_state_, level_texture_map_, action_manager_);

            if (object_updated)
            {
                // The UpdateWallTool caches the wall when it is right-clicked. This means that
                // updating a wall and then moving its start/end resets the walls props to how it
                // was before.
                // This ensures that the wall props stay correct
                try_reset_update_wall_tool();
            }
        }
        ImGui::End();
    }
}

void ScreenEditGame::display_menu_bar_gui()
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
                level_file_selector_.show();
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
            ImGui::Checkbox("Show History?", &editor_settings_.show_history);
            ImGui::Checkbox("Show Messages?", &editor_settings_.show_messages_log);
            ImGui::Checkbox("Lock 2D To 3D View?", &editor_settings_.always_center_2d_to_3d_view);

            ImGui::Checkbox("Show Textures in 2D View?", &editor_settings_.show_textures_in_2d_view);
            if (!editor_settings_.show_textures_in_2d_view) ImGui::BeginDisabled();
            ImGui::SliderFloat("Base/World Texture Mix (2D)", &editor_settings_.texture_mix, 0.0f, 1.0f);
            if (!editor_settings_.show_textures_in_2d_view) ImGui::EndDisabled();

            ImGui::Checkbox("Show Grid?", &editor_settings_.show_grid);
            if (ImGui::Checkbox("Show 2D View? (Full Screen 3D)", &editor_settings_.show_2d_view))
            {
                auto factor = editor_settings_.show_2d_view ? 2 : 1;
                camera_.set_viewport_size({window().getSize().x / factor, window().getSize().y});
            }
            ImGui::Checkbox("Render As Wireframe", &editor_settings_.render_as_wireframe);
            ImGui::Checkbox("Display Normals", &editor_settings_.render_vertex_normals);
            ImGui::Checkbox("Display Main Light", &editor_settings_.render_main_light);
            ImGui::Checkbox("Show Level Settings", &editor_settings_.show_level_settings);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::Checkbox("Auto-Jump to selection floor?", &editor_settings_.jump_to_selection_floor);
            ImGui::Checkbox("Lock Mouse?", &camera_controller_options_.lock_rotation);
            ImGui::Checkbox("Free camera movement?", &camera_controller_options_.free_movement);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
    // clang-format on
}

void ScreenEditGame::display_save_as_gui()
{
    // clang-format off
    if (ImGuiExtras::BeginCentredWindow("Save As...", {300, 200}))
    {
        bool bad_file_name = level_name_.starts_with(INTERNAL_FILE_ID);
        bool can_save = !level_name_.empty() && !bad_file_name;

        if (bad_file_name)
        {
            ImGui::Text("Level names cannot start with '%s'", INTERNAL_FILE_ID.c_str());
        }
        ImGui::InputText("Level Name", &level_name_);
        // Only enable the save button if a name is actually set
        if (!can_save)                  { ImGui::BeginDisabled();    } 
        if (ImGui::Button("Save Game")) { save_level();              }
        if (!can_save)                  { ImGui::EndDisabled();      }
        if (ImGui::Button("Cancel"))    { show_save_dialog_ = false; }
    }
    ImGui::End();
    // clang-format on
}

void ScreenEditGame::display_debug_gui()
{
    // clang-format off
    camera_.gui("Camera");
    if (ImGui::Begin("Camera Kind"))
    {
        if (ImGui::Button("Perspective"))   { camera_.set_type(CameraType::Perspective);        }
        if (ImGui::Button("Orthographic"))  { camera_.set_type(CameraType::OrthographicWorld);  }
    }
    ImGui::End();
        
    camera_2d_.gui("Camera2D");

    // clang-format on
}

// ----------------------------------------
// ==== Editor Settings saving/loading ====
// ----------------------------------------
void ScreenEditGame::EditorSettings::save() const
{
    nlohmann::json output = {
        {"show_grid", show_grid},
        {"show_2d_view", show_2d_view},
        {"show_grid", show_grid},
        {"always_center_2d_to_3d_view", always_center_2d_to_3d_view},
        {"show_history", show_history},
        {"show_textures_in_2d_view", show_textures_in_2d_view},
        {"texture_mix", texture_mix},
        {"jump_to_selection_floor", jump_to_selection_floor},
        {"show_messages_log", show_messages_log},
        {"render_as_wireframe", render_as_wireframe},
        {"render_vertex_normals", render_vertex_normals},
        {"render_main_light", render_main_light},
        {"show_level_settings", show_level_settings},
    };

    std::ofstream settings_file("settings.json");
    settings_file << output;
}

void ScreenEditGame::EditorSettings::load()
{
    std::ifstream settings_file("settings.json");
    if (settings_file)
    {
        nlohmann::json input;
        settings_file >> input;

        // clang-format off
        show_grid                       = input.value("show_grid", show_grid);
        show_2d_view                    = input.value("show_2d_view", show_2d_view);
        always_center_2d_to_3d_view     = input.value("always_center_2d_to_3d_view", always_center_2d_to_3d_view);
        show_history                    = input.value("show_history", show_history);
        show_textures_in_2d_view        = input.value("show_textures_in_2d_view", show_textures_in_2d_view);
        texture_mix                     = input.value("texture_mix", texture_mix);
        jump_to_selection_floor         = input.value("jump_to_selection_floor", jump_to_selection_floor);
        show_messages_log               = input.value("show_messages_log", show_messages_log);
        render_as_wireframe             = input.value("render_as_wireframe", render_as_wireframe);
        render_vertex_normals           = input.value("render_vertex_normals", render_vertex_normals);
        render_main_light               = input.value("render_main_light", render_main_light);
        show_level_settings             = input.value("show_level_settings", show_level_settings);
        // clang-format on
    }
}

void ScreenEditGame::EditorSettings::set_to_default()
{
    *this = EditorSettings{};
}
