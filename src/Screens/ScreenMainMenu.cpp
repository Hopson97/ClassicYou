#include "ScreenMainMenu.h"

#include <print>

#include <imgui.h>

#include "../Util/ImGuiExtras.h"
#include "ScreenEditGame.h"
#include "ScreenPlaying.h"
#include "../Editor/EditorGUI.h"

ScreenMainMenu::ScreenMainMenu(ScreenManager& screens)
    : Screen(screens)
{
}

bool ScreenMainMenu::on_init()
{
    return true;
}

void ScreenMainMenu::on_render(bool show_debug)
{

    ImVec2 window_size(window().getSize().x / 4.0f, window().getSize().y / 2.0f);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
    ImGui::SetNextWindowPos({window_size.x + window_size.x * 4.0f / 8.0f, window_size.y / 2.0f},
                            ImGuiCond_Always);
    if (ImGui ::Begin("M A I N   M E N U", nullptr,
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoCollapse))
    {
        ImGui::Text("ClassicYou");
        ImGui::Separator();

        switch (current_menu_)
        {
            case ScreenMainMenu::Menu::MainMenu:
                main_menu();
                break;

            case ScreenMainMenu::Menu::CreateMenu:
                create_game_menu();
                break;

            default:
                break;
        }
    }
    ImGui::End();
}

void ScreenMainMenu::main_menu()
{

    if (ImGui::CustomButton("Create Game"))
    {
        p_screen_manager_->push_screen(std::make_unique<ScreenEditGame>(*p_screen_manager_));
        // current_menu_ = Menu::CreateMenu;
    }
    if (ImGui::CustomButton("Load Edit Game"))
    {
        show_load_dialog_ = true;
        // p_screen_manager_->push_screen(std::make_unique<ScreenPlaying>(*p_screen_manager_));
    }
    if (ImGui::CustomButton("Exit"))
    {
        p_screen_manager_->pop_screen();
    }

    if (show_load_dialog_)
    {
        if (display_level_list(show_load_dialog_, load_level_name_))
        {
            show_load_dialog_ = false;
            p_screen_manager_->push_screen(
                std::make_unique<ScreenEditGame>(*p_screen_manager_, load_level_name_));
        }
    }
}

void ScreenMainMenu::create_game_menu()
{
    if (ImGui::CustomButton("Begin Creating"))
    {
        p_screen_manager_->push_screen(std::make_unique<ScreenEditGame>(*p_screen_manager_));
        current_menu_ = Menu::CreateMenu;
    }
    if (ImGui::CustomButton("Back"))
    {
        current_menu_ = Menu::MainMenu;
    }
}
