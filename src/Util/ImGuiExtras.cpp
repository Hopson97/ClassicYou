#include "ImGuiExtras.h"

#include <cmath>

#include <imgui.h>

namespace
{
    constexpr auto NO_MANIP_FLAGS = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_AlwaysAutoResize;
}

namespace ImGuiExtras
{

    bool CustomButton(const char* text)
    {
        // Calculate the positions of the button such that it always centered in the X
        float window_width = ImGui::GetContentRegionAvail().x;
        ImVec2 button_size = {window_width / 2.0f, 64.0f};
        float x = (window_width - button_size.x) * 0.5f;

        ImGui::SetCursorPos({ImGui::GetCursorPos().x + x, ImGui::GetCursorPos().y + 10.0f});
        return ImGui::Button(text, button_size);
    }

    bool SliderFloatStepped(const char* label, float& value, float min, float max, float interval,
                            const char* fmt)
    {
        float step = 1.0f / interval;
        if (ImGui::SliderFloat(label, &value, min, max, fmt))
        {
            value = round(value * step) / step;

            return true;
        }
        return false;
    }
    bool BeginCentredWindow(const char* name, const ImVec2& size)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 window_size = io.DisplaySize;

        ImGui::SetNextWindowPos({(window_size.x - size.x) * 0.5f, (window_size.y - size.y) * 0.5f},
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(size, ImGuiCond_Always);

        return ImGui::Begin(name, nullptr, NO_MANIP_FLAGS);
    }
} // namespace ImGuiExtras

void MessagesManager::add_message(const std::string& message)
{
    last_message_time_ = clock_.getElapsedTime();
    messages_.push_back({message, get_epoch()});
}

void MessagesManager::render()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 window_size = io.DisplaySize;
    auto now = clock_.getElapsedTime();

    auto& messages = messages_.data;

    int i = 0;
    if (!messages.empty() && now - last_message_time_ < sf::seconds(5))
    {
        ImGui::SetNextWindowSize({512, 140}, ImGuiCond_Always);

        ImGui::SetNextWindowPos({0, (window_size.y - 140)}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.45f);
        if (ImGui::Begin("Messages", nullptr, NO_MANIP_FLAGS | ImGuiWindowFlags_NoTitleBar))
        {
            for (auto itr = messages.rbegin(); itr != messages.rend(); itr++)
            {
                if (i != 0)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 200));
                }
                ImGui::Text("%s", std::format("[{}]   {}", epoch_to_datetime_string(itr->timestamp),
                                              itr->message)
                                      .c_str());
                if (i++ != 0)
                {
                    ImGui::PopStyleColor();
                }
            }
        }
        ImGui::End();
    }
}