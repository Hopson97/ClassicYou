#pragma once

#include <SFML/System/Time.hpp>
#include <SFML/Window/Window.hpp>

struct Camera;
class Keyboard;

struct CameraKeybinds
{
    sf::Keyboard::Key forward = sf::Keyboard::Key::W;
    sf::Keyboard::Key left = sf::Keyboard::Key::A;
    sf::Keyboard::Key right = sf::Keyboard::Key::D;
    sf::Keyboard::Key back = sf::Keyboard::Key::S;
};

struct CameraControllerOptions
{
    bool lock_rotation = true;
    bool free_movement = false;
};

void free_camera_controller(const Keyboard& keyboard, Camera& camera, sf::Time dt,
                            const CameraKeybinds& keybinds, const sf::Window& window,
                            CameraControllerOptions options);

void free_camera_controller_2d(const Keyboard& keyboard, Camera& camera, sf::Time dt,
                               const CameraKeybinds& keybinds);
