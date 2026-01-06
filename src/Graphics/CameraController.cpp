#include "CameraController.h"

#include <print>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../Util/Keyboard.h"
#include "Camera.h"

CameraController3D::CameraController3D(Camera& camera, const CameraKeybinds& keybinds,
                                       const CameraControllerOptions3D& options)
    : p_camera_{&camera}
    , p_keybinds_{&keybinds}
    , p_camera_options_(&options)
{
}

void CameraController3D::handle_events(const sf::Event& event)
{
    // Ensures that the camera look does not jerk when the middle mouse button is pressed
    if (auto mouse = event.getIf<sf::Event::MouseMoved>())
    {
        if (!middle_mouse_down_)
        {
            last_mouse_position_ = mouse->position;
        }
    }
    else if (auto mouse = event.getIf<sf::Event::MouseButtonPressed>())
    {
        if (mouse->button == sf::Mouse::Button::Middle)
        {
            middle_mouse_down_ = true;
        }
    }

    else if (auto mouse = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mouse->button == sf::Mouse::Button::Middle)
        {
            middle_mouse_down_ = false;
        }
    }
}


void CameraController3D::handle_inputs(const Keyboard& keyboard, sf::Time dt, sf::Window& window)
{
    if (!window.hasFocus())
    {
        return;
    }

    auto move = get_keybord_input(keyboard);
    move.y = p_camera_->get_type() == CameraType::Perspective ? move.y : 0;
    p_camera_->transform.position += move * dt.asSeconds();
    p_camera_->update();

    if (!p_camera_options_->lock_rotation || middle_mouse_down_)
    {
        handle_look(window);
    }

    if (p_camera_options_->lock_rotation)
    {
        window.setMouseCursorVisible(!middle_mouse_down_);
    }
}

glm::vec3 CameraController3D::get_keybord_input(const Keyboard& keyboard)
{
    glm::vec3 move{0.0f};

    auto flat_movement = !p_camera_options_->free_movement || p_camera_->get_type() != CameraType::Perspective;

    if (keyboard.is_key_down(p_keybinds_->forward))
    {
        move += flat_movement ? forward_vector_flat(p_camera_->transform.rotation)
                     : forward_vector(p_camera_->transform.rotation);
    }
    else if (keyboard.is_key_down(p_keybinds_->back))
    {
        move += flat_movement ? backward_vector_flat(p_camera_->transform.rotation)
                     : backward_vector(p_camera_->transform.rotation);
    }

    if (keyboard.is_key_down(p_keybinds_->left))
    {
        move += left_vector(p_camera_->transform.rotation);
    }
    else if (keyboard.is_key_down(p_keybinds_->right))
    {
        move += right_vector(p_camera_->transform.rotation);
    }

    if (keyboard.is_key_down(p_keybinds_->up))
    {
        move.y += 1.5f;
    }
    else if (keyboard.is_key_down(p_keybinds_->down))
    {
        move.y -= 1.5f;
    }

    move *= 4.0f;

    // Speed up when left shift is held
    if (keyboard.is_key_down(sf::Keyboard::Key::LShift))
    {
        move *= 4.0f;
    }

    return move;
}

void CameraController3D::handle_look(const sf::Window& window)
{
    auto change = sf::Mouse::getPosition(window) - last_mouse_position_;
    auto& r = p_camera_->transform.rotation;

    r.x -= static_cast<float>(change.y) * p_camera_options_->look_sensitivity;
    r.y += static_cast<float>(change.x) * p_camera_options_->look_sensitivity;

    sf::Mouse::setPosition({(int)window.getSize().x / 2, (int)window.getSize().y / 2}, window);
    last_mouse_position_ = sf::Mouse::getPosition(window);

    r.x = glm::clamp(r.x, -89.9f, 89.9f);
    if (r.y >= 360.0f)
    {
        r.y = 0.0f;
    }
    else if (r.y < 0.0f)
    {
        r.y = 359.9f;
    }
}

CameraController2D::CameraController2D(Camera& camera, const CameraKeybinds& keybinds)
    : p_camera_{&camera}
    , p_keybinds_{&keybinds}
{
}

void CameraController2D::handle_inputs(const Keyboard& keyboard, sf::Time dt)
{
    glm::vec3 move{0.0f};

    if (keyboard.is_key_down(p_keybinds_->forward))
    {
        move += glm::vec3{0, 1, 0};
    }
    else if (keyboard.is_key_down(p_keybinds_->back))
    {
        move += glm::vec3{0, -1, 0};
    }

    if (keyboard.is_key_down(p_keybinds_->left))
    {
        move += glm::vec3{-1, 0, 0};
    }
    else if (keyboard.is_key_down(p_keybinds_->right))
    {
        move += glm::vec3{1, 0, 0};
    }

    // Speed up when left shift is held
    if (keyboard.is_key_down(sf::Keyboard::Key::LShift))
    {
        move *= 20.0f;
    }

    p_camera_->transform.position += move * 150.0f * dt.asSeconds();
    p_camera_->update();
}
