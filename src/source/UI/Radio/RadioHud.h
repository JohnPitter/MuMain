#pragma once

// Left-edge HUD: the radio button (same aço-e-ouro empty-button plate as the
// voice mic/sound dock, mirrored to the LEFT screen edge) plus the live
// "tocando agora" label above it. Input and draw happen together, exactly
// like MiniMapCorner's voice buttons.

namespace UI::Radio
{
    // Loads Interface\Radio_icon.OZT (BITMAP_LUXUI_RADIO). Shares the main
    // HUD's texture lifetime: called from CNewUIMainFrameWindow's
    // LoadImages/UnloadImages, next to UI::Voice::LoadIcons.
    void LoadIcon();
    void UnloadIcon();

    // Per-frame input handling + draw of the button and the now-playing
    // label. Called from CNewUINameWindow::Render, next to the voice dock.
    void RenderHud();
}
