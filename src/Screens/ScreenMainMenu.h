#pragma once

#include "Screen.h"

class ScreenMainMenu final : public Screen
{
    enum class Menu
    {
        MainMenu,
        SettingsMenu,
    };

  public:
    ScreenMainMenu(ScreenManager& screens);

    bool on_init() override;
    void on_render(bool show_debug) override;

  private:
};