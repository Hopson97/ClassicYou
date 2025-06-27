#pragma once

#include <SFML/Window/Window.hpp>

#include "Settings.h"
#include "Graphics/OpenGL/Texture.h"
#include "Graphics/OpenGL/Shader.h"
#include "Graphics/OpenGL/VertexArrayObject.h"


namespace GUI
{
    [[nodiscard]] bool init(sf::Window* window);

    void begin_frame();

    void shutdown();
    void render();

    void event(const sf::Window& window, sf::Event& e);

    void debug_window(const glm::vec3& camera_position, const glm::vec3& camera_rotation,
                      Settings& settings);
} // namespace GUI

struct SpriteRenderer
{
  public:
    SpriteRenderer(glm::uvec2 window_size);
    void render(const gl::Texture2D& texture, glm::vec2 position, glm::vec2 size);

  private:
    gl::VertexArrayObject quad_;
    gl::Shader shader_;
    gl::Texture2D texture_;
};