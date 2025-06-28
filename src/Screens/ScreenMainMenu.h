#pragma once

#include "Screen.h"

class ScreenMainMenu final : public Screen
{
    enum class Menu
    {
        MainMenu,
        CreateMenu,
    };

  public:
    ScreenMainMenu(ScreenManager& screens);

    bool on_init() override;
    void on_render(bool show_debug) override;

  private:
    void main_menu();
    void create_game_menu();

    Menu current_menu_ = Menu::MainMenu;
};