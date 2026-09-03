#pragma once

namespace UI::HUD
{
    bool IsFixedToolbar();
    float FixedToolbarScale();
    // 3D scene height in 640x480 space. Full window when the bar is an overlay.
    int MainSceneWorldRefHeight();

    // While alive, the 640x480 HUD toolbar is drawn at FixedToolbarScale
    // (default 1.25), centered at the bottom. MouseX/Y are remapped so
    // hit-tests stay aligned. Re-entrant: nested scopes share one transform.
    class FixedToolbarScope
    {
    public:
        FixedToolbarScope();
        ~FixedToolbarScope();

        FixedToolbarScope(const FixedToolbarScope&) = delete;
        FixedToolbarScope& operator=(const FixedToolbarScope&) = delete;

    private:
        bool m_armed = false;
    };
}
