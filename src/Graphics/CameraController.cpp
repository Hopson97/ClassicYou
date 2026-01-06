#include "CameraController.h"

#include <print>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../Util/Keyboard.h"
#include "Camera.h"

namespace
{
    glm::vec3 keyboard_input(const Keyboard& keyboard, const Camera& camera,
                             const CameraKeybinds& keybinds, CameraControllerOptions3D options)
    {
        glm::vec3 move{0.0f};
        auto flat = !options.free_movement || camera.get_type() != CameraType::Perspective;
        if (keyboard.is_key_down(keybinds.forward))
        {
            move += flat ? forward_vector_flat(camera.transform.rotation)
                         : forward_vector(camera.transform.rotation);
        }
        else if (keyboard.is_key_down(keybinds.back))
        {
            move += flat ? backward_vector_flat(camera.transform.rotation)
                         : backward_vector(camera.transform.rotation);
        }

        if (keyboard.is_key_down(keybinds.left))
        {
            move += left_vector(camera.transform.rotation);
        }
        else if (keyboard.is_key_down(keybinds.right))
        {
            move += right_vector(camera.transform.rotation);
        }

        if (keyboard.is_key_down(keybinds.up))
        {
            move.y += 1.5f;
        }
        else if (keyboard.is_key_down(keybinds.down))
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

    void mouse_input(const sf::Window& window, Camera& camera, sf::Vector2i& last_mouse_position, float look_sensitivity)
    {
        auto change = sf::Mouse::getPosition(window) - last_mouse_position;
        auto& r = camera.transform.rotation;

        r.x -= static_cast<float>(change.y * 0.35);
        r.y += static_cast<float>(change.x * 0.35);

        sf::Mouse::setPosition({(int)window.getSize().x / 2, (int)window.getSize().y / 2}, window);
        last_mouse_position = sf::Mouse::getPosition(window);

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
} // namespace

CameraController3D::CameraController3D(Camera& camera, const CameraKeybinds& keybinds)
    : p_camera_{&camera}
    , p_keybinds_{&keybinds}
{
}

void CameraController3D::handle_events(const sf::Event& event)
{
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

void CameraController3D::handle_inputs(const Keyboard& keyboard, sf::Time dt, sf::Window& window,
                                       const CameraControllerOptions3D& options)
{
    if (!window.hasFocus())
    {
        return;
    }

    auto move = keyboard_input(keyboard, *p_camera_, *p_keybinds_, options);
    move.y = p_camera_->get_type() == CameraType::Perspective ? move.y : 0;
    p_camera_->transform.position += move * dt.asSeconds();
    p_camera_->update();

    if (!options.lock_rotation || middle_mouse_down_)
    {
        mouse_input(window, *p_camera_, last_mouse_position_, options.look_sensitivity);
    }

    if (options.lock_rotation)
    {
        window.setMouseCursorVisible(!middle_mouse_down_);
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
