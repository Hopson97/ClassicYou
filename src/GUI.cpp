#include "GUI.h"

#include <print>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_sfml/imgui-SFML.h>
#include <imgui_sfml/imgui_impl_opengl3.h>

namespace
{
    void base_light_widgets(LightBase& light)
    {
        ImGui::Text("Intensity");
        ImGui::SliderFloat3("Colour", &light.colour[0], 0.0, 1.0);
        ImGui::SliderFloat("Ambient Intensity", &light.ambient_intensity, 0.0, 1.0);
        ImGui::SliderFloat("Diffuse Intensity", &light.diffuse_intensity, 0.0, 1.0);
        ImGui::SliderFloat("Specular Intensity", &light.specular_intensity, 0.0, 1.0);
    }

    void attenuation_widgets(Attenuation& attenuation)
    {
        ImGui::SliderFloat("Attenuation Constant", &attenuation.constant, 0.0, 1.0f);
        ImGui::SliderFloat("Attenuation Linear", &attenuation.linear, 0.14f, 0.0014f, "%.6f");
        ImGui::SliderFloat("Attenuation Quadratic", &attenuation.exponent, 0.000007f, 0.03f,
                           "%.6f");
    }
} // namespace

namespace GUI
{
    bool init(sf::Window* window)
    {
        return ImGui::SFML::Init(*window, sf::Vector2f{window->getSize()}) &&
               ImGui_ImplOpenGL3_Init();
    }

    void begin_frame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui::NewFrame();
    }

    void shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui::SFML::Shutdown();
    }

    void render()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void event(const sf::Window& window, sf::Event& e)
    {
        ImGui::SFML::ProcessEvent(window, e);
    }

    void debug_window(const glm::vec3& camera_position, const glm::vec3& camera_rotation,
                      Settings& settings)
    {
        auto r = camera_rotation;
        auto p = camera_position;

        // clang-format off
        if (ImGui::Begin("Debug Window"))
        {
            ImGui::Text("Position: (%f, %f, %f)", p.x, p.y, p.z);
            ImGui::Text("Rotation: (%f, %f, %f)", r.x, r.y, r.z);

            ImGui::Separator();
            ImGui::Checkbox("Grass ground texture?", &settings.use_grass_texture);
            ImGui::Checkbox("Enable 'flashlight'?", &settings.use_flashlight);
            ImGui::Checkbox("Wireframe rendering?", &settings.wireframe);

            ImGui::Separator();

            ImGui::PushID("DirLight");
            ImGui::Text("Directional light");
            if (ImGui::SliderFloat3("Direction", &settings.lights.directional.direction[0], -1.0, 1.0))
            {
                settings.lights.directional.direction = glm::normalize(settings.lights.directional.direction);
            }
            base_light_widgets(settings.lights.directional);
            ImGui::PopID();

            ImGui::Separator();

            ImGui::PushID("PointLight");
            ImGui::Text("Point light");
            base_light_widgets(settings.lights.pointlight);
            attenuation_widgets(settings.lights.pointlight.att);
            ImGui::PopID();

            ImGui::Separator();

            ImGui::PushID("SpotLight");
            ImGui::Text("Spot light");
            ImGui::SliderFloat("Cutoff", &settings.lights.spotlight.cutoff, 0.0, 90.0f);
            base_light_widgets(settings.lights.spotlight);
            attenuation_widgets(settings.lights.spotlight.att);
            ImGui::PopID();
        }
        // clang-format on

        ImGui::End();
    }

} // namespace GUI

SpriteRenderer::SpriteRenderer(glm::uvec2 window_size)
{
    glm::mat4 projection{1.0f};
    projection =
        glm::ortho(0.0f, (float)window_size.x, 0.0f, (float)window_size.y, -100.0f, 100.0f);

    if (!shader_.load_stage("assets/shaders/GUIVertex.glsl", gl::ShaderType::Vertex) ||
        !shader_.load_stage("assets/shaders/GUIFragment.glsl", gl::ShaderType::Fragment) ||
        !shader_.link_shaders())
    {
        std::println("Failed to create GUI shaders...");
    }

    shader_.bind();
    shader_.set_uniform("orthographic_matrix", projection);
    shader_.set_uniform("gui_texture", 0);
}

void SpriteRenderer::render(const gl::Texture2D& texture, glm::vec2 position, glm::vec2 size)
{
    // glEnable(GL_BLEND);
    texture.bind(0);
    shader_.bind();

    glm::mat4 model{1.0f};
    model = glm::translate(model, {position.x, position.y, 0});
    model = glm::scale(model, {size.x, size.y, 1.0});
    shader_.set_uniform("transform", model);

    auto view_matrix = glm::lookAt(glm::vec3{0, 0, 10}, {0, 0, 0}, {0, 1, 0});




    shader_.set_uniform("transform", model);
    shader_.set_uniform("view_matrix", view_matrix);

    quad_.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // glDisable(GL_BLEND);
}
