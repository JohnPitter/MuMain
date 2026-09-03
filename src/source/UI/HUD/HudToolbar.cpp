#include "stdafx.h"
#include "UI/HUD/HudToolbar.h"

#include "Data/GameConfig/GameConfig.h"
#include "Render/Textures/ZzzOpenglUtil.h"

extern int MouseX;
extern int MouseY;

namespace
{
    constexpr float kBarW = static_cast<float>(REFERENCE_WIDTH);

    int s_depth = 0;
    float s_oldRx = 1.f;
    float s_oldRy = 1.f;
    float s_oldOx = 0.f;
    float s_oldOy = 0.f;
    int s_oldMx = 0;
    int s_oldMy = 0;

    float ClampedScale()
    {
        float scale = GameConfig::GetInstance().GetFixedToolbarScale();
        if (scale < 0.5f)
            scale = 0.5f;
        const float maxScale = static_cast<float>(WindowWidth) / kBarW;
        if (scale > maxScale)
            scale = maxScale;
        if (scale < 0.5f)
            scale = 0.5f;
        return scale;
    }
}

namespace UI::HUD
{
    bool IsFixedToolbar()
    {
        return GameConfig::GetInstance().GetFixedToolbar();
    }

    float FixedToolbarScale()
    {
        return IsFixedToolbar() ? ClampedScale() : 1.f;
    }

    int MainSceneWorldRefHeight()
    {
        return IsFixedToolbar() ? REFERENCE_HEIGHT : (REFERENCE_HEIGHT - 48);
    }

    FixedToolbarScope::FixedToolbarScope()
    {
        if (!IsFixedToolbar())
            return;

        if (s_depth++ > 0)
            return;

        m_armed = true;
        s_oldRx = g_fScreenRate_x;
        s_oldRy = g_fScreenRate_y;
        s_oldOx = g_fScreenOff_x;
        s_oldOy = g_fScreenOff_y;
        s_oldMx = MouseX;
        s_oldMy = MouseY;

        const float rateX = (s_oldRx > 0.f) ? s_oldRx : 1.f;
        const float rateY = (s_oldRy > 0.f) ? s_oldRy : 1.f;
        const float pixelX = static_cast<float>(MouseX) * rateX + s_oldOx;
        const float pixelY = static_cast<float>(MouseY) * rateY + s_oldOy;

        const float scale = ClampedScale();
        g_fScreenRate_x = scale;
        g_fScreenRate_y = scale;
        g_fScreenOff_x = (static_cast<float>(WindowWidth) - kBarW * scale) * 0.5f;
        g_fScreenOff_y = static_cast<float>(WindowHeight) - static_cast<float>(REFERENCE_HEIGHT) * scale;

        MouseX = static_cast<int>((pixelX - g_fScreenOff_x) / g_fScreenRate_x);
        MouseY = static_cast<int>((pixelY - g_fScreenOff_y) / g_fScreenRate_y);
    }

    FixedToolbarScope::~FixedToolbarScope()
    {
        if (!m_armed)
        {
            if (IsFixedToolbar() && s_depth > 0)
                --s_depth;
            return;
        }

        g_fScreenRate_x = s_oldRx;
        g_fScreenRate_y = s_oldRy;
        g_fScreenOff_x = s_oldOx;
        g_fScreenOff_y = s_oldOy;
        MouseX = s_oldMx;
        MouseY = s_oldMy;
        --s_depth;
        m_armed = false;
    }
}
