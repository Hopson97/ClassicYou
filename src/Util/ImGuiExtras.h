#pragma once

#include <string>

#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <imgui.h>
#include <magic_enum/magic_enum_all.hpp>

#include "Util.h"

// Adds helpers for ImGUI
namespace ImGuiExtras
{
    bool CustomButton(const char* text);
    bool SliderFloatStepped(const char* label, float& value, float min, float max, float interval,
                            const char* fmt = "%.1f");

    bool BeginCentredWindow(const char* name, const ImVec2& size);

    /// GUI for a enum property such a styles etc
    template <typename EnumType>
    bool EnumSelect(const char* label, EnumType& current, const char* tooltip = nullptr)
    {
        ImGui::Text("%s", label);
        if (tooltip)
        {
            ImGui::SameLine();
            ImGui::Text("[?]");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", tooltip);
            }
        }

        size_t n = 0;
        bool option_selected = false;
        magic_enum::enum_for_each<EnumType>(
            [&](EnumType value)
            {
                int v = static_cast<int>(current);

                if (n != 0)
                {
                    if (++n % 3 != 0)
                    {
                        ImGui::SameLine();
                    }
                }
                n++;

                if (ImGui::RadioButton(magic_enum::enum_name(value).data(), &v,
                                       static_cast<int>(value)))
                {
                    current = value;
                    option_selected = true;
                }
            });
        return option_selected;
    }
} // namespace ImGuiExtras

// Displays the messages along the bottom of the window
class MessagesManager
{
    struct Message
    {
        std::string message;
        epoch_t timestamp;
    };

  public:
    void add_message(const std::string& message);

    void render();

  private:
    CircularQueue<Message, 7> messages_;
    sf::Clock clock_;
    sf::Time last_message_time_;
};
