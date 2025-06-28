#pragma once

#include <memory>
#include <stack>
#include <vector>

#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Window.hpp>

class Screen;
class Keyboard;

class ScreenManager final
{
    struct Action
    {
        enum class Kind
        {
            None,
            Push,
            Pop,
            Change,
        };
        Kind kind = Kind::None;
        std::unique_ptr<Screen> screen;
    };

  public:
    ScreenManager(sf::Window& window);

    void push_screen(std::unique_ptr<Screen> screen);
    void pop_screen();
    void change_screen(std::unique_ptr<Screen> screen);

    bool update();

    Screen& get_current();

    bool empty() const;

    sf::Window& get_window();

  private:
    void restore();

    std::stack<std::unique_ptr<Screen>> screen_stack_;
    std::vector<Action> m_actions;
    sf::Window& window_;
};

class Screen
{
  public:
    Screen(ScreenManager& screen_manager);

    virtual bool on_init() = 0;
    virtual void on_event(const sf::Event& event) {};
    virtual void on_update(const Keyboard& keyboard, sf::Time dt) {};
    virtual void on_fixed_update(sf::Time dt) {};
    virtual void on_render(bool show_debug) = 0;

  protected:
    sf::Window& window();

  protected:
    ScreenManager* p_screen_manager_ = nullptr;

  private:
};