#include "ScreenEditGame.h"

#include <imgui.h>

#include "../Editor/EditConstants.h"
#include "../Graphics/OpenGL/GLUtils.h"
#include "../Util/ImGuiExtras.h"
#include "../Util/Keyboard.h"
#include "../Util/Util.h"

namespace
{
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

    // ---------------------------
    // ==== Buffer thr meshes ====
    // ---------------------------
    grid_mesh_.buffer();
    selection_mesh_.buffer();

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
        editor_state_.node_hovered = map_pixel_to_tile({mouse->position.x, mouse->position.y});
    }
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
    drawing_pad_.display();

    // Render the actual scene
    glViewport(window().getSize().x / 2, 0, window().getSize().x / 2, window().getSize().y);

    // Update the shader buffers
    matrices_ssbo_.buffer_sub_data(0, camera_.get_projection_matrix());
    matrices_ssbo_.buffer_sub_data(sizeof(glm::mat4), camera_.get_view_matrix());

    glViewport(window().getSize().x / 2, 0, window().getSize().x / 2, window().getSize().y);
    scene_shader_.bind();
    render_scene(scene_shader_);

    // Ensure GUI etc are rendered using fill
    gl::polygon_mode(gl::Face::FrontAndBack, gl::PolygonMode::Fill);
}

void ScreenEditGame::render_scene(gl::Shader& shader)
{
    // Set up the capabilities/ render states
    shader.bind();
    gl::enable(gl::Capability::DepthTest);
    gl::disable(gl::Capability::CullFace);
    gl::cull_face(gl::Face::Back);
    gl::polygon_mode(gl::Face::FrontAndBack,
                     settings_.wireframe ? gl::PolygonMode::Line : gl::PolygonMode::Fill);

    // Draw grid
    shader.set_uniform("model_matrix", create_model_matrix({}));
    shader.set_uniform("use_texture", false);
    grid_mesh_.bind().draw_elements(GL_LINES);

    // Draw the selection node
    shader.set_uniform("model_matrix",
                       create_model_matrix({
                           .position = {editor_state_.node_hovered.x , 0,
                                        editor_state_.node_hovered.y },
                           .rotation = {-90, 0, 0},
                       }));
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
