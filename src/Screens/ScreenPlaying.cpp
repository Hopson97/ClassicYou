#include "ScreenPlaying.h"

#include <imgui.h>

#include "../Graphics/OpenGL/GLUtils.h"
#include "../Util/ImGuiExtras.h"
#include "../Util/Keyboard.h"
#include "../Util/Util.h"

ScreenPlaying::ScreenPlaying(ScreenManager& screens)
    : Screen(screens)
    , perspective_camera_(CameraConfig{
          .type = CameraType::Perspective,
          .viewport_size = {window().getSize().x, window().getSize().y},
          .near = 0.1f,
          .far = 1000.0f,
          .fov = 90.0f,
      })
    , ortho_camera_(CameraConfig{
          .type = CameraType::OrthographicWorld,
          .viewport_size = {window().getSize().x, window().getSize().y},
          .near = 0.5f,
          .far = 1000.0f,
      })
{
    p_active_camera_ = &perspective_camera_;
}

bool ScreenPlaying::on_init()
{
    window().setMouseCursorVisible(false);

    // -----------------------
    // ==== Load textures ====
    // -----------------------
    if (!grass_material_.load_from_file("assets/textures/grass.png", 8, false, false))
    {
        return false;
    }

    // ---------------------------
    // ==== Buffer thr meshes ====
    // ---------------------------
    terrain_mesh_.buffer();

    // ----------------------------
    // ==== Load scene shaders ====
    // ----------------------------
    if (!scene_shader_.load_stage("assets/shaders/Scene/SceneVertex.glsl",
                                  gl::ShaderType::Vertex) ||
        !scene_shader_.load_stage("assets/shaders/Scene/SceneFragment.glsl",
                                  gl::ShaderType::Fragment) ||
        !scene_shader_.link_shaders())
    {
        return false;
    }
    scene_shader_.set_uniform("diffuse", 0);

    // -------------------------
    // ==== Set up the SSBO ====
    // -------------------------
    // Note this an SSBO just as an example - this works fine as a UBO as well.
    matrices_ssbo_.create_as_ssbo<glm::mat4>(0, 2);

    // -----------------------------------
    // ==== Entity Transform Creation ====
    // -----------------------------------
    perspective_camera_.transform = {.position = {3.0f, 4.0f, 16.0f},
                                     .rotation = {4.0f, 300.0f, 0.0f}};
    ortho_camera_.transform = {.position = {6, 20, 5}, .rotation = {333.4f, 45.0f, 0.0f}};

    return true;
}

void ScreenPlaying::on_event(const sf::Event& event)
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

            default:
                break;
        }
    }
}

void ScreenPlaying::on_update(const Keyboard& keyboard, sf::Time dt)
{
    //free_camera_controller(keyboard, *p_active_camera_, dt, camera_keybinds_, window(),
    //                       {.lock_rotation = false, .free_movement = false});
}

void ScreenPlaying::on_fixed_update([[maybe_unused]] sf::Time dt)
{
}

void ScreenPlaying::on_render(bool show_debug)
{
    if (game_paused_)
    {
        pause_menu();
    }

    if (show_debug)
    {
        p_active_camera_->gui("Camera");

        if (ImGui::Begin("Camera"))
        {
            if (ImGui::Button("Use Perspective"))
            {
                p_active_camera_ = &perspective_camera_;
            }
            if (ImGui::Button("Use Ortho"))
            {
                p_active_camera_ = &ortho_camera_;
            }
        }
        ImGui::End();
    }

    // Update the shader buffers
    matrices_ssbo_.buffer_sub_data(0, p_active_camera_->get_projection_matrix());
    matrices_ssbo_.buffer_sub_data(sizeof(glm::mat4), p_active_camera_->get_view_matrix());

    scene_shader_.bind();
    render_scene(scene_shader_);

    // Ensure GUI etc are rendered using fill
    gl::polygon_mode(gl::Face::FrontAndBack, gl::PolygonMode::Fill);
}

void ScreenPlaying::pause_menu()
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
        if (ImGuiExtras::CustomButton("Resume"))
        {
            game_paused_ = false;
        }
        if (ImGuiExtras::CustomButton("Exit"))
        {
            p_screen_manager_->pop_screen();
        }
        ImGui::End();
    }
}

void ScreenPlaying::render_scene(gl::Shader& shader)
{
    // Set up the capabilities/ render states
    gl::enable(gl::Capability::DepthTest);
    // gl::enable(gl::Capability::CullFace);
    gl::cull_face(gl::Face::Back);
    gl::polygon_mode(gl::Face::FrontAndBack,
                     settings_.wireframe ? gl::PolygonMode::Line : gl::PolygonMode::Fill);

    // Draw terrain
    grass_material_.bind(0);
    shader.set_uniform("model_matrix", create_model_matrix({.position = {-5, 0, -5}}));
    terrain_mesh_.bind();
    terrain_mesh_.draw_elements();
}
