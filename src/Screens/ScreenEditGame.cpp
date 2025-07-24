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
    constexpr std::array<const char*, 22> TEXTURE_NAMES = {
        "Red Bricks", "Grey Bricks", "Stone Bricks", "Stone Bricks Mossy",
        "Bars",       "Chain Fence", "Grass",        "Dirt",
        "Glass",      "Sand",        "Bark",         "Leaf",
        "Planks",     "Rock",        "Stucco",       "Ancient",
        "Blank",      "Happy",       "SciFi",        "Tiles",
        "Book Case",  "Parquet"

    };

    glm::ivec2 map_pixel_to_tile(glm::vec2 point, const Camera& camera)
    {
        // TODO handle camera zooming
        auto scale = HALF_TILE_SIZE;

        auto& transform = camera.transform.position;
        return {
            std::round((point.x + transform.x) / scale) * scale,
            std::round((point.y + transform.y) / scale) * scale,
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
    , level_name_(level_name)
    , level_name_actual_(level_name)
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
    grid_mesh_.buffer();
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

    // Load the shader for world geometry. This is a seperate shader
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

    return true;
}

void ScreenEditGame::on_event(const sf::Event& event)
{
    auto p_active = editor_state_.p_active_object;

    if (showing_dialog())
    {
        return;
    }

    // When dragging an object and the final placement is decided, this is set to true. This is to
    // prevent calls to the tool event fuctions, which can have issues (such as walls appears in odd
    // place) if their events are handled post-move
    bool finish_move = false;

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
                if (p_active)
                {
                    action_manager_.push_action(std::make_unique<DeleteObjectAction>(
                        *p_active, editor_state_.current_floor));
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

            default:
                break;
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (!ImGui::GetIO().WantCaptureMouse && mouse->button == sf::Mouse::Button::Left)
        {
            // Start dragging the selected object
            if (p_active && p_active->try_select_2d(editor_state_.node_hovered) &&
                !std::get_if<WallObject>(&p_active->object_type))
            {
                select_position_ = editor_state_.node_hovered;
                moving_object_ = true;
                moving_object_cache_ = *p_active;
            }
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        editor_state_.node_hovered =
            map_pixel_to_tile({mouse->position.x, mouse->position.y}, drawing_pad_.get_camera());

        if (p_active && moving_object_)
        {
            auto new_object = *p_active;
            new_object.move(editor_state_.node_hovered - select_position_);
            select_position_ = editor_state_.node_hovered;

            action_manager_.push_action(std::make_unique<UpdateObjectAction>(
                                            *p_active, new_object, editor_state_.current_floor),
                                        false);
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        // Try to select an object (2D view)
        if (mouse->button == sf::Mouse::Button::Right)
        {
            editor_state_.p_active_object =
                level_.try_select(map_pixel_to_tile({mouse->position.x, mouse->position.y},
                                                    drawing_pad_.get_camera()),
                                  p_active, editor_state_.current_floor);

            // Editing a wall requires a special tool to enable resizing, so after a object it
            // switches between tools
            if (editor_state_.p_active_object)
            {
                if (auto wall =
                        std::get_if<WallObject>(&editor_state_.p_active_object->object_type))
                {
                    tool_ = std::make_unique<UpdateWallTool>(*editor_state_.p_active_object, *wall);
                }
                else
                {
                    tool_ = std::make_unique<CreateWallTool>();
                }
            }
            else if (tool_->get_tool_type() == ToolType::UpdateWall)
            {
                // Nothing was selected, default back to CreateWallTool if currently selecting a
                // wall
                tool_ = std::make_unique<CreateWallTool>();
            }
        }
        if (mouse->button == sf::Mouse::Button::Left)
        {
            if (moving_object_)
            {
                auto new_object = *p_active;
                new_object.move(editor_state_.node_hovered - select_position_);

                action_manager_.push_action(
                    std::make_unique<UpdateObjectAction>(moving_object_cache_, new_object,
                                                         editor_state_.current_floor),
                    true);
                finish_move = true;
            }

            moving_object_ = false;
        }
    }

    if (!moving_object_ && !finish_move)
    {
        tool_->on_event(event, editor_state_.node_hovered, editor_state_, action_manager_);
    }

    if (try_set_tool_to_wall)
    {
        if (tool_->get_tool_type() == ToolType::UpdateWall)
        {
            tool_ = std::make_unique<CreateWallTool>();
        }
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

        level_.render_2d(drawing_pad_, editor_state_.p_active_object, editor_state_.current_floor);

        if (tool_->get_tool_type() == ToolType::UpdateWall ||
            (!ImGui::GetIO().WantCaptureMouse && !moving_object_))
        {
            tool_->render_preview_2d(drawing_pad_, editor_state_);
        }

        // Finalise 2d rendering
        drawing_pad_.display();

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
        glLineWidth(2);
        scene_shader_.set_uniform(
            "model_matrix",
            create_model_matrix({.position = {0, editor_state_.current_floor * FLOOR_HEIGHT, 0}}));

        scene_shader_.set_uniform("use_texture", false);
        grid_mesh_.bind().draw_elements(GL_LINES);
    }

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

    if (!moving_object_)
    {
        tool_->render_preview();
    }

    // Render the level itself
    level_.render(world_geometry_shader_, editor_state_.p_active_object,
                  editor_state_.current_floor);

    // Ensure GUI etc are rendered using fill
    gl::polygon_mode(gl::Face::FrontAndBack, gl::PolygonMode::Fill);

    //=============================
    //     Render the ImGUI
    // ============================
    show_menu_bar();
    render_editor_ui();
    if (show_debug)
    {
        debug_gui();
    }
    action_manager_.display_action_history();

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
            level_.load(make_level_path(level_name_));
            level_name_actual_ = level_name_;
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
        if (ImGui::Button("Wall"))
        {
            tool_ = std::make_unique<CreateWallTool>();
            editor_state_.p_active_object = nullptr;
        }
        if (ImGui::Button("Platform"))
        {
            tool_ = std::make_unique<CreateObjectTool>(ObjectTypeName::Platform);
            editor_state_.p_active_object = nullptr;
        }
        if (ImGui::Button("Polygon Platform"))
        {
            tool_ = std::make_unique<CreateObjectTool>(ObjectTypeName::PolygonPlatform);
            editor_state_.p_active_object = nullptr;
        }
        if (ImGui::Button("Pillar"))
        {
            tool_ = std::make_unique<CreateObjectTool>(ObjectTypeName::Pillar);
            editor_state_.p_active_object = nullptr;
        }

        ImGui::Separator();

        // Display the floor options, so going up or down a floor
        ImGui::Text("Floors");
        if (ImGui::Button("Floor Down"))
        {
            level_.ensure_floor_exists(--editor_state_.current_floor);
            editor_state_.p_active_object = nullptr;
            camera_.transform.position.y -= FLOOR_HEIGHT;
        }
        ImGui::SameLine();
        if (ImGui::Button("Floor Up"))
        {
            level_.ensure_floor_exists(++editor_state_.current_floor);
            editor_state_.p_active_object = nullptr;
            camera_.transform.position.y += FLOOR_HEIGHT;
        }
        ImGui::Text("Lowest: %d - Current: %d - Highest: %d", level_.get_min_floor(),
                    editor_state_.current_floor, level_.get_max_floor());

        ImGui::Separator();
    }
    ImGui::End();

    // When an object is selected, its properties is rendered
    if (editor_state_.p_active_object)
    {
        if (ImGui::Begin("Object Properties"))
        {
            editor_state_.p_active_object->property_gui(editor_state_, level_textures_,
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
    if (ImGui::BeginCentredWindow("Save As...", {300, 200}))
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
            if (ImGui::MenuItem("New")) {  }
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
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::Checkbox("Show Grid?", &editor_settings_.show_grid);
            if (ImGui::Checkbox("Show 2D View? (Full Screen 3D)", &editor_settings_.show_2d_view))
            {
                auto factor = editor_settings_.show_2d_view ? 2 : 1;
                camera_.set_viewport_size({window().getSize().x / factor, window().getSize().y});
            }
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
