#include "stdafx.h"
#include "Core/Text/TextLineWrap.h"
#include "UI/NewUI/HUD/Notices.h"

#include "UI/Legacy/UIControls.h"        // g_pRenderText, RT3_WRITE_CENTER
#include "App/Platform/Windows/Winmain.h"    // g_hFontBold
#include "Render/Textures/ZzzOpenglUtil.h" // EnableAlphaTest
#include "UI/NewUI/NewUISystem.h"        // g_pNewUISystem
#include "Engine/AI/ZzzAI.h"             // FPS_ANIMATION_FACTOR, REFERENCE_FPS
#include "Engine/Object/ZzzInterface.h"  // CutText

namespace
{
    constexpr int MAX_NOTICE = 6;
    // 9 seconds of 25fps-time. Must be float: subtracting FPS_ANIMATION_FACTOR from an int
    // truncated to 1-per-frame, so at 60fps a 300-frame lifetime lasted ~5 seconds.
    constexpr float NOTICE_LIFETIME = static_cast<float>(9.0 * REFERENCE_FPS);
    constexpr int NOTICE_TEXT_MAX = 256;

    struct Notice
    {
        wchar_t Text[NOTICE_TEXT_MAX];
        float   LifeTime;
        BYTE    Color;
    };

    int    s_count = 0;
    Notice s_notices[MAX_NOTICE];

    bool SameNotice(const Notice& notice, const wchar_t* text, int color)
    {
        return notice.Color == color && wcscmp(notice.Text, text) == 0;
    }

    bool RefreshExisting(const wchar_t* text, int color)
    {
        for (int i = 0; i < s_count; i++)
        {
            if (SameNotice(s_notices[i], text, color))
            {
                s_notices[i].LifeTime = NOTICE_LIFETIME;
                return true;
            }
        }

        return false;
    }

    void Push(const wchar_t* text, int color)
    {
        if (s_count >= MAX_NOTICE)
        {
            for (int i = 1; i < MAX_NOTICE; i++)
            {
                s_notices[i - 1] = s_notices[i];
            }

            s_count = MAX_NOTICE - 1;
        }

        Notice& notice = s_notices[s_count++];
        notice.Color = static_cast<BYTE>(color);
        notice.LifeTime = NOTICE_LIFETIME;
        wcsncpy(notice.Text, text, NOTICE_TEXT_MAX - 1);
        notice.Text[NOTICE_TEXT_MAX - 1] = 0;
    }
}

namespace UI::Notices
{
    void Clear()
    {
        memset(s_notices, 0, sizeof(s_notices));
        s_count = 0;
    }

    void Create(const wchar_t* text, int color)
    {
        if (text == nullptr || text[0] == 0)
        {
            return;
        }

        SIZE size;
        g_pRenderText->SetFont(g_hFontBold);
        GetTextExtentPoint32(g_pRenderText->GetFontDC(), text, lstrlen(text), &size);

        if (size.cx < NOTICE_TEXT_MAX)
        {
            if (RefreshExisting(text, color))
            {
                return;
            }

            Push(text, color);
            return;
        }

        wchar_t topText[NOTICE_TEXT_MAX] = { 0 };
        wchar_t bottomText[NOTICE_TEXT_MAX] = { 0 };
        CutText(text, topText, bottomText, NOTICE_TEXT_MAX);
        if (!RefreshExisting(topText, color))
        {
            Push(topText, color);
        }

        if (bottomText[0] != 0 && !RefreshExisting(bottomText, color))
        {
            Push(bottomText, color);
        }
    }

    void Move()
    {
        int keep = 0;
        for (int i = 0; i < s_count; i++)
        {
            s_notices[i].LifeTime -= FPS_ANIMATION_FACTOR;
            if (s_notices[i].LifeTime > 0.f && s_notices[i].Text[0] != 0)
            {
                if (keep != i)
                {
                    s_notices[keep] = s_notices[i];
                }

                keep++;
            }
        }

        if (keep < MAX_NOTICE)
        {
            memset(&s_notices[keep], 0, sizeof(Notice) * (MAX_NOTICE - keep));
        }

        s_count = keep;
    }

    void Render()
    {
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
        if (g_pNewUISystem->IsVisible(SEASON3B::INTERFACE_INGAMESHOP) == true)
            return;
#endif // KJH_ADD_INGAMESHOP_UI_SYSTEM

        EnableAlphaTest();
        g_pRenderText->SetFont(g_hFontBold);
        glColor3f(1.f, 1.f, 1.f);

        for (int i = 0; i < s_count; i++)
        {
            Notice* n = &s_notices[i];
            if (n->Text[0] == 0)
            {
                continue;
            }

            if (n->Color == 0)
            {
                // Solid gold. The old 128/255 blink looked like a blurry duplicate/shadow.
                g_pRenderText->SetTextColor(255, 200, 80, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
            }
            else
            {
                g_pRenderText->SetTextColor(100, 255, 200, 255);
                g_pRenderText->SetBgColor(0, 0, 0, 0);
            }

            g_pRenderText->RenderText(320, 300 + i * 13, n->Text, 0, 0, RT3_WRITE_CENTER);
        }
    }
}
