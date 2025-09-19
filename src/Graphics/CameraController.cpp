#include "CameraController.h"

#include <print>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "../Util/Keyboard.h"
#include "Camera.h"

namespace
{
    glm::vec3 keyboard_input(const Keyboard& keyboard, const Camera& camera,
                             const CameraKeybinds& keybinds, CameraControllerOptions options)
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

    void mouse_input(const sf::Window& window, Camera& camera)
    {
        static auto last_mouse = sf::Mouse::getPosition(window);
        auto change = sf::Mouse::getPosition(window) - last_mouse;
        auto& r = camera.transform.rotation;

        r.x -= static_cast<float>(change.y * 0.35);
        r.y += static_cast<float>(change.x * 0.35);

        sf::Mouse::setPosition({(int)window.getSize().x / 2, (int)window.getSize().y / 2}, window);
        last_mouse = sf::Mouse::getPosition(window);

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

void free_camera_controller(const Keyboard& keyboard, Camera& camera, sf::Time dt,
                            const CameraKeybinds& keybinds, sf::Window& window,
                            CameraControllerOptions options)
{
    if (!window.hasFocus())
    {
        return;
    }

    auto move = keyboard_input(keyboard, camera, keybinds, options);
    move.y = camera.get_type() == CameraType::Perspective ? move.y : 0;
    camera.transform.position += move * dt.asSeconds();
    camera.update();

    if (!options.lock_rotation || sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
    {
        mouse_input(window, camera);
    }

    if (options.lock_rotation)
    {
        window.setMouseCursorVisible(!sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle));
    }
}

void free_camera_controller_2d(const Keyboard& keyboard, Camera& camera, sf::Time dt,
                               const CameraKeybinds& keybinds)
{
    glm::vec3 move{0.0f};

    if (keyboard.is_key_down(keybinds.forward))
    {
        move += glm::vec3{0, 1, 0};
    }
    else if (keyboard.is_key_down(keybinds.back))
    {
        move += glm::vec3{0, -1, 0};
    }

    if (keyboard.is_key_down(keybinds.left))
    {
        move += glm::vec3{-1, 0, 0};
    }
    else if (keyboard.is_key_down(keybinds.right))
    {
        move += glm::vec3{1, 0, 0};
    }

    // Speed up when left shift is held
    if (keyboard.is_key_down(sf::Keyboard::Key::LShift))
    {
        move *= 20.0f;
    }

    camera.transform.position += move * 150.0f * dt.asSeconds();
    camera.update();
}
