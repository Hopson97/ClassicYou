#pragma once

#include <SFML/System/Time.hpp>
#include <SFML/Window/Window.hpp>
#include <glm/glm.hpp>

struct Camera;
class Keyboard;

/// Keybinds for controlling the camera.
struct CameraKeybinds
{
    sf::Keyboard::Key forward = sf::Keyboard::Key::W;
    sf::Keyboard::Key left = sf::Keyboard::Key::A;
    sf::Keyboard::Key right = sf::Keyboard::Key::D;
    sf::Keyboard::Key back = sf::Keyboard::Key::S;

    sf::Keyboard::Key up = sf::Keyboard::Key::Space;
    sf::Keyboard::Key down = sf::Keyboard::Key::Q;
};

/// Options for controlling the 3D camera.
struct CameraControllerOptions3D
{
    float look_sensitivity = 0.35f;
    bool lock_rotation = true;
    bool free_movement = false;
};

/// Camera controller for the 3D view.
class CameraController3D
{
  public:
    CameraController3D(Camera& camera, const CameraKeybinds& keybinds,
                       const CameraControllerOptions3D& options);

    void handle_event(const sf::Event& event);
    void handle_inputs(const Keyboard& keyboard, sf::Time dt, sf::Window& window);

  private:
    glm::vec3 get_keyboard_input(const Keyboard& keyboard);
    void handle_look(const sf::Window& window);

    Camera* p_camera_ = nullptr;
    const CameraKeybinds* p_keybinds_ = nullptr;
    const CameraControllerOptions3D* p_camera_options_ = nullptr;

    /// Last recorded mouse position. Stored to prevent sudden jumps when unlocking the mouse.
    sf::Vector2i last_mouse_position_{};

    /// Whether the middle mouse button is currently held down, temporarily unlocking camera
    /// rotation.
    bool middle_mouse_down_ = false;
};

/// Camera controller for the 2D view.
class CameraController2D
{
  public:
    CameraController2D(Camera& camera, const CameraKeybinds& keybinds);

    void handle_inputs(const Keyboard& keyboard, sf::Time dt);

  private:
    Camera* p_camera_ = nullptr;
    const CameraKeybinds* p_keybinds_ = nullptr;
};
