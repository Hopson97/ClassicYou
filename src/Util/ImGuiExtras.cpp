#include "ImGuiExtras.h"

#include <cmath>

#include <imgui.h>

namespace ImGui
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

        return ImGui::Begin(name, nullptr,
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                ImGuiWindowFlags_AlwaysAutoResize);
    }
} // namespace ImGui