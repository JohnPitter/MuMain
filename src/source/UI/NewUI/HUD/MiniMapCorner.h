#pragma once

namespace UI::HUD::MiniMap
{
    // Always-available corner minimap (top-right), cropped and centered on
    // the local player. Independent of the full-screen TAB map.
    void Render();
    void RenderCommands();
    void ToggleVisible();
    bool IsVisible();
    // Top-left of the box in reference HUD coords (640x480 space).
    void GetBoxRect(float* outX, float* outY, float* outSize);
}
