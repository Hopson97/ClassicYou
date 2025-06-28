#include "ImGuiExtras.h"

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
} // namespace ImGui