#include "Screen.h"

ScreenManager::ScreenManager(sf::Window& window)
    : window_(window)
{
}

void ScreenManager::push_screen(std::unique_ptr<Screen> screen)
{
    m_actions.push_back({.kind = Action::Kind::Push, .screen = std::move(screen)});
}

void ScreenManager::pop_screen()
{
    m_actions.push_back({.kind = Action::Kind::Pop});
}

void ScreenManager::change_screen(std::unique_ptr<Screen> screen)
{
    m_actions.push_back({.kind = Action::Kind::Change, .screen = std::move(screen)});
}

bool ScreenManager::update()
{
    bool init_top = false;
    for (Action& action : m_actions)
    {
        switch (action.kind)
        {
            case Action::Kind::None:
                break;

            case Action::Kind::Push:
                init_top = true;
                screen_stack_.push(std::move(action.screen));
                break;

            case Action::Kind::Pop:
                screen_stack_.pop();
                restore();
                break;

            case Action::Kind::Change:
                if (!screen_stack_.empty())
                {
                    screen_stack_.pop();
                }
                init_top = true;
                screen_stack_.push(std::move(action.screen));
                break;
        }
    }
    m_actions.clear();

    if (init_top)
    {
        return get_current().on_init();
    }
    return true;
}

Screen& ScreenManager::get_current()
{
    return *screen_stack_.top();
}

bool ScreenManager::empty() const
{
    return screen_stack_.empty();
}

sf::Window& ScreenManager::get_window()
{
    return window_;
}

void ScreenManager::restore()
{
}

Screen::Screen(ScreenManager& screen_manager)
    : p_screen_manager_(&screen_manager)
{
}

sf::Window& Screen::window()
{
    return p_screen_manager_->get_window();
}
