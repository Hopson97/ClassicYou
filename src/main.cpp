#include <print>

#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Window.hpp>
#include <glad/glad.h>

#include "Editor/FloorManager.h"
#include "Editor/LevelFileIO.h"
#include "GUI.h"
#include "Graphics/OpenGL/GLUtils.h"
#include "Screens/Screen.h"
#include "Screens/ScreenMainMenu.h"
#include "Util/Keyboard.h"
#include "Util/Profiler.h"
#include "Util/TimeStep.h"

namespace
{
    void handle_event(const sf::Event& event, bool& show_debug_info, bool& close_requested);

    void load_legacy_levels()
    {
        auto legacy_path = "./levels/" + INTERNAL_FILE_ID + "legacy/";
        if (!std::filesystem::exists(legacy_path))
        {
            std::println(
                std::cerr,
                "Unable to convert legacy levels as the legacy folder does not exist. ('{}')",
                legacy_path);
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(legacy_path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".cy" &&
                !level_file_exists(entry.path().filename()))
            {
                convert_legacy_level(entry.path());
            }
        }
    }
} // namespace

int main()
{
    load_legacy_levels();

    sf::ContextSettings context_settings;
    context_settings.depthBits = 24;
    context_settings.stencilBits = 8;
    context_settings.antiAliasingLevel = 4;
    context_settings.majorVersion = 4;
    context_settings.minorVersion = 6;
    context_settings.attributeFlags = sf::ContextSettings::Debug;

    sf::Window window(sf::VideoMode::getDesktopMode(), "ClassicYou", sf::Style::None,
                      sf::State::Fullscreen, context_settings);

    window.setVerticalSyncEnabled(true);
    if (!window.setActive(true))
    {
        std::println(std::cerr, "Failed to activate the window.");
        return EXIT_FAILURE;
    }

    if (!gladLoadGL())
    {
        std::println(std::cerr, "Failed to initialise OpenGL - Is OpenGL linked correctly?");
        return EXIT_FAILURE;
    }
    glClearColor(0, 35.0f / 255.0f, 70.0f / 255.0f, 255.0f);
    glViewport(0, 0, window.getSize().x, window.getSize().y);
    gl::enable_debugging();

    TimeStep updater{50};
    Profiler profiler;
    bool show_debug_info = false;

    ScreenManager screens{window};
    screens.push_screen(std::make_unique<ScreenMainMenu>(screens));
    if (!screens.update())
    {
        return -1;
    }

    if (!GUI::init(&window))
    {
        std::println(std::cerr, "Failed to initialise ImGui.");
        return EXIT_FAILURE;
    }

    Keyboard keyboard;

    // -------------------
    // ==== Main Loop ====
    // -------------------
    sf::Clock clock;
    while (window.isOpen() && !screens.empty())
    {
        GUI::begin_frame();
        Screen& screen = screens.get_current();
        bool close_requested = false;
        while (auto event = window.pollEvent())
        {
            GUI::event(window, *event);
            keyboard.update(*event);
            screen.on_event(*event);
            handle_event(*event, show_debug_info, close_requested);
        }
        auto dt = clock.restart();

        // Update
        {
            auto& update_profiler = profiler.begin_section("Update");
            screen.on_update(keyboard, dt);
            update_profiler.end_section();
        }

        // Fixed-rate update
        {
            auto& fixed_update_profiler = profiler.begin_section("Fixed Update");
            updater.update([&](sf::Time dt) { screen.on_fixed_update(dt); });
            fixed_update_profiler.end_section();
        }

        // Render
        {
            auto& render_profiler = profiler.begin_section("Render");
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            screen.on_render(show_debug_info);
            render_profiler.end_section();
        }

        // Show profiler
        profiler.end_frame();
        if (show_debug_info)
        {
            profiler.gui();
        }

        // --------------------------
        // ==== End Frame ====
        // --------------------------
        GUI::render();
        window.display();
        if (close_requested || !screens.update())
        {
            window.close();
        }
    }

    // --------------------------
    // ==== Graceful Cleanup ====
    // --------------------------
    GUI::shutdown();
}

namespace
{
    void handle_event(const sf::Event& event, bool& show_debug_info, bool& close_requested)
    {
        if (event.is<sf::Event::Closed>())
        {
            close_requested = true;
        }
        else if (auto* key = event.getIf<sf::Event::KeyPressed>())
        {

            switch (key->code)
            {

                case sf::Keyboard::Key::F1:
                    show_debug_info = !show_debug_info;
                    break;

                default:
                    break;
            }
        }
    }
} // namespace
