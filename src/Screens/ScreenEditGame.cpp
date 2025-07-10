#include "ScreenEditGame.h"

#include <imgui.h>

#include "../Editor/EditConstants.h"
#include "../Graphics/OpenGL/GLUtils.h"
#include "../Util/ImGuiExtras.h"
#include "../Util/Keyboard.h"
#include "../Util/Util.h"

namespace
{
    constexpr std::array<std::pair<const char*, const char*>, 4> TEXTURES = {
        std::make_pair("Red Bricks", "assets/textures/RedBricks.png"),
        std::make_pair("Grey Bricks", "assets/textures/GreyBricks.png"),
        std::make_pair("Bars", "assets/textures/Bars.png"),
        std::make_pair("ChainFence", "assets/textures/ChainFence.png"),
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

bool ScreenEditGame::on_init()
{
    if (!drawing_pad_.init())
    {
        return false;
    }

    // -----------------------
    // ==== Load textures ====
    // -----------------------
    // Load textures
    texture_.create(16, 32, gl::TEXTURE_PARAMS_NEAREST);

    for (auto& texture : TEXTURES)
    {
        if (!level_textures_.register_texture(texture.first, texture.second, texture_))
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
    camera_.transform = {.position = {WORLD_SIZE / 2, 7, WORLD_SIZE + 1},
                         .rotation = {-40, 270.0f, 0.0f}};

    return true;
}

void ScreenEditGame::on_event(const sf::Event& event)
{
    if (auto key = event.getIf<sf::Event::KeyReleased>())
    {
        switch (key->code)
        {
            case sf::Keyboard::Key::L:
                if (!game_paused_)
                {
                    rotation_locked_ = !rotation_locked_;
                    window().setMouseCursorVisible(rotation_locked_);
                }
                break;

            case sf::Keyboard::Key::Tab:
                game_paused_ = !game_paused_;
                rotation_locked_ = game_paused_;
                window().setMouseCursorVisible(game_paused_);
                break;

            case sf::Keyboard::Key::Delete:
                if (editor_state_.p_active_object_)
                {
                    action_manager_.push_action(std::make_unique<DeleteObjectAction>(
                        *dynamic_cast<Wall*>(editor_state_.p_active_object_)));
                }
                break;

            case sf::Keyboard::Key::Z:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
                {
                    action_manager_.undo_action();
                }
                break;

            case sf::Keyboard::Key::Y:
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl))
                {
                    action_manager_.redo_action();
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
        if (mouse->button == sf::Mouse::Button::Right)
        {
            editor_state_.p_active_object_ =
                level_.try_select(map_pixel_to_tile({mouse->position.x, mouse->position.y},
                                                    drawing_pad_.get_camera()),
                                  editor_state_.p_active_object_);
        }
    }

    tool_.on_event(event, editor_state_.node_hovered, editor_state_, action_manager_);
}

void ScreenEditGame::on_update(const Keyboard& keyboard, sf::Time dt)
{
    free_camera_controller(keyboard, camera_, dt, camera_keybinds_, window(), rotation_locked_);
    drawing_pad_.update(keyboard, dt);
}

void ScreenEditGame::on_fixed_update(sf::Time dt)
{
}

void ScreenEditGame::on_render(bool show_debug)
{
    if (game_paused_)
    {
        pause_menu();
    }
    if (show_debug)
    {
        debug_gui();
    }

    // Render the drawing pad to the left side
    glViewport(0, 0, window().getSize().x / 2, window().getSize().y);

    tool_.render_preview_2d(drawing_pad_);
    level_.render_2d(drawing_pad_, editor_state_.p_active_object_);

    // Finalise 2d rendering
    drawing_pad_.display();

    // Render the actual scene
    // Update the shader buffers
    matrices_ssbo_.buffer_sub_data(0, camera_.get_projection_matrix());
    matrices_ssbo_.buffer_sub_data(sizeof(glm::mat4), camera_.get_view_matrix());

    glViewport(window().getSize().x / 2, 0, window().getSize().x / 2, window().getSize().y);
    scene_shader_.bind();
    render_scene(scene_shader_);

    // Draw preview wall
    world_geometry_shader_.bind();
    texture_.bind(0);
    world_geometry_shader_.set_uniform("use_texture", true);
    world_geometry_shader_.set_uniform("model_matrix", create_model_matrix({}));
    tool_.render_preview();

    level_.render();

    // Ensure GUI etc are rendered using fill
    gl::polygon_mode(gl::Face::FrontAndBack, gl::PolygonMode::Fill);

    if (editor_state_.p_active_object_)
    {
        editor_state_.p_active_object_->property_gui(editor_state_, level_textures_,
                                                     action_manager_);
    }

    // clang-format off
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New")) {  }
            if (ImGui::MenuItem("Open...")) {  }
            if (ImGui::MenuItem("Save")) {  }
            if (ImGui::MenuItem("Exit")) { p_screen_manager_->pop_screen(); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo (CTRL + Z)")) { action_manager_.undo_action(); }
            if (ImGui::MenuItem("Redo (CTRL + Y)")) { action_manager_.redo_action(); }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    // clang-format on

    action_manager_.display_action_history();
}

void ScreenEditGame::render_scene(gl::Shader& shader)
{
    // Set up the capabilities/ render states
    shader.bind();
    gl::enable(gl::Capability::DepthTest);
    gl::enable(gl::Capability::CullFace);
    gl::cull_face(gl::Face::Back);
    gl::polygon_mode(gl::Face::FrontAndBack,
                     settings_.wireframe ? gl::PolygonMode::Line : gl::PolygonMode::Fill);

    // Draw grid
    glLineWidth(2);
    shader.set_uniform("model_matrix", create_model_matrix({}));
    shader.set_uniform("use_texture", false);
    grid_mesh_.bind().draw_elements(GL_LINES);

    // Draw level

    // Draw the selection node
    shader.set_uniform(
        "model_matrix",
        create_model_matrix({.position = {editor_state_.node_hovered.x / TILE_SIZE, 0,
                                          editor_state_.node_hovered.y / TILE_SIZE},
                             .rotation = {-90, 0, 0}}));
    shader.set_uniform("use_texture", false);
    selection_mesh_.bind().draw_elements();
}

void ScreenEditGame::pause_menu()
{
    ImVec2 window_size(p_screen_manager_->get_window().getSize().x / 4.0f,
                       p_screen_manager_->get_window().getSize().y / 2.0f);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
    ImGui::SetNextWindowPos({window_size.x + window_size.x * 4 / 8.0f, window_size.y / 2},
                            ImGuiCond_Always);
    if (ImGui ::Begin("Paused", nullptr,
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::CustomButton("Resume"))
        {
            game_paused_ = false;
        }
        if (ImGui::CustomButton("Exit"))
        {
            p_screen_manager_->pop_screen();
        }
        ImGui::End();
    }
}

void ScreenEditGame::debug_gui()
{
    camera_.gui("Camera");

    if (ImGui::Begin("Camera Kind"))
    {
        if (ImGui::Button("Perspective"))
        {
            camera_.set_type(CameraType::Perspective);
        }
        if (ImGui::Button("Orthographic"))
        {
            camera_.set_type(CameraType::OrthographicWorld);
        }
    }
    ImGui::End();
}
