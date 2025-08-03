#pragma once

#include <SFML/System/Vector2.hpp>
#include <imgui.h>
#include <magic_enum/magic_enum_all.hpp>

#include <string>

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
            ImGui::Text("[?]");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", tooltip);
            }
            ImGui::SameLine();
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
} // namespace ImGui