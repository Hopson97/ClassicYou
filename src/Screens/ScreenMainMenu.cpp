#include "ScreenMainMenu.h"

#include <print>

#include <imgui.h>

#include "../Util/ImGuiExtras.h"
#include "ScreenPlaying.h"

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

    ImVec2 window_size(window().getSize().x / 4,
                       window().getSize().y / 2);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
    ImGui::SetNextWindowPos({window_size.x + window_size.x * 4 / 8.0f, window_size.y / 2},
                            ImGuiCond_Always);
    if (ImGui ::Begin("M A I N   M E N U", nullptr,
                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("ClassicYou");
        ImGui::Separator();

        if (ImGui::CustomButton("New Game"))
        {
            p_screen_manager_->push_screen(std::make_unique<ScreenPlaying>(*p_screen_manager_));
        }
        if (ImGui::CustomButton("Load Game"))
        {
            p_screen_manager_->push_screen(std::make_unique<ScreenPlaying>(*p_screen_manager_));
        }
        if (ImGui::CustomButton("Exit Game"))
        {
            p_screen_manager_->pop_screen();
        }
        ImGui::End();
    }
}
