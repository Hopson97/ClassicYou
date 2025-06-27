#pragma once

#include <SFML/System/Time.hpp>
#include <SFML/Window/Window.hpp>

struct Camera;
class Keyboard;

struct CameraKeybinds
{
    sf::Keyboard::Key foward = sf::Keyboard::Key::W;
    sf::Keyboard::Key left = sf::Keyboard::Key::A;
    sf::Keyboard::Key right = sf::Keyboard::Key::D;
    sf::Keyboard::Key back = sf::Keyboard::Key::S;
};

void free_camera_controller(const Keyboard& keyboard, Camera& camera, sf::Time dt,
                            const CameraKeybinds& keybinds, const sf::Window& window,
                            bool is_rotation_locked);

void free_camera_controller_2d(const Keyboard& keyboard, Camera& camera, sf::Time dt,
                               const CameraKeybinds& keybinds);
