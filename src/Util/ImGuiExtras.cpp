#include "ImGuiExtras.h"

#include <imgui.h>

namespace ImGui
{
    bool CustomButton(const char* text)
    {
        ImGui::SetCursorPos({ImGui::GetCursorPosX() + 150, ImGui::GetCursorPosY() + 20});
        return ImGui::Button(text, {100, 50});
    }
} // namespace ImGui