#pragma once

#include <SFML/System/Vector2.hpp>
#include <string>
#include <imgui.h>

// Adds helpers for ImGUI
namespace ImGui
{
   bool CustomButton(const char* text);
   bool SliderFloatStepped(const char* label, float& value, float min, float max, float interval,
                           const char* fmt = "%.1f");

   bool BeginCentredWindow(const char* name, const ImVec2& size);
}