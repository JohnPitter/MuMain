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

    // GDI RenderText uses ConvertPosX AND the font-DIB pitch from g_fScreenRate.
    // Inside FixedToolbarScope those are the bar scale (~1.25), so glyphs land
    // in window pixels while the DIB pitch / box size assume toolbar space —
    // tiny black text, tooltip background misses. Pause the bar transform,
    // rewrite (x,y) from toolbar-640 into window-640, draw, then restore.
    // MouseX/Y stay in toolbar space so hit-tests keep working. No-op when
    // the bar scope is not armed.
    class FixedToolbarTextScope
    {
    public:
        FixedToolbarTextScope(int& x, int& y);
        ~FixedToolbarTextScope();

        FixedToolbarTextScope(const FixedToolbarTextScope&) = delete;
        FixedToolbarTextScope& operator=(const FixedToolbarTextScope&) = delete;

    private:
        bool m_armed = false;
    };
}
