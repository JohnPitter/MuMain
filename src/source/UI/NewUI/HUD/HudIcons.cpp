#include "stdafx.h"
#include "UI/NewUI/HUD/HudIcons.h"

#include <cstddef>

#include "Render/Textures/ZzzOpenglUtil.h"

// HUD glyphs are drawn from filled quads (RenderColor) in MU's steel-and-gold
// interface palette, the same technique as UI/Voice/VoiceIcons.cpp: a whole-
// glyph "sticker" outline (the shape re-drawn dark at eight neighbor offsets)
// followed by the colored fills. Coordinates are in icon units on a 24-unit
// grid, origin at the icon center, +y downward (screen space).
//
// Two size classes share these tables:
//   - MU Helper strip, 18x13 buttons: glyphs stay inside +-11 units and avoid
//     features thinner than ~2.5 units, which vanish at kStripGlyphScale.
//   - Main toolbar, 30x41 buttons: same +-11 budget, finer detail is legible.
namespace
{
    struct Rgb { float r, g, b; };
    struct Quad { float x, y, w, h; Rgb color; };

    constexpr Rgb kInk{ 0.07f, 0.08f, 0.10f };
    constexpr Rgb kSteelD{ 0.36f, 0.38f, 0.42f };
    constexpr Rgb kSteelM{ 0.55f, 0.57f, 0.61f };
    constexpr Rgb kSteelL{ 0.76f, 0.78f, 0.82f };
    constexpr Rgb kSteelH{ 0.93f, 0.95f, 0.97f };
    constexpr Rgb kGold{ 0.91f, 0.71f, 0.31f };
    constexpr Rgb kGoldL{ 0.96f, 0.86f, 0.58f };

    constexpr float kOutlineWidth = 1.f;   // icon units
    constexpr float kDisabledTone = 0.5f;

    // Button plate: 1px ink rim, 1px steel bevel, then the face.
    constexpr float kPlateRim = 1.f;
    constexpr float kPlateBevel = 1.f;
    constexpr float kPlateHighlight = 1.f;
    constexpr Rgb kFaceNormal{ 0.15f, 0.16f, 0.19f };
    constexpr Rgb kFaceHover{ 0.27f, 0.29f, 0.34f };
    constexpr Rgb kFacePressed{ 0.10f, 0.11f, 0.13f };
    constexpr Rgb kFaceActive{ 0.24f, 0.20f, 0.12f };
    constexpr Rgb kFaceAlert{ 0.34f, 0.27f, 0.13f };

    // -- MU Helper strip -----------------------------------------------------

    // Gear: four axial teeth, four diagonal teeth, an octagonal body built
    // from two crossed slabs, and a gold hub.
    constexpr Quad kSettings[] = {
        { -3.f, -11.f, 6.f, 4.5f, kSteelD }, { -3.f, 6.5f, 6.f, 4.5f, kSteelD },
        { -11.f, -3.f, 4.5f, 6.f, kSteelD }, { 6.5f, -3.f, 4.5f, 6.f, kSteelD },
        { -9.5f, -9.5f, 5.f, 5.f, kSteelD }, { 4.5f, -9.5f, 5.f, 5.f, kSteelD },
        { -9.5f, 4.5f, 5.f, 5.f, kSteelD }, { 4.5f, 4.5f, 5.f, 5.f, kSteelD },
        // body
        { -8.f, -6.f, 16.f, 12.f, kSteelM }, { -6.f, -8.f, 12.f, 16.f, kSteelM },
        // bevel: lit from the top-left
        { -6.f, -7.5f, 12.f, 3.f, kSteelL }, { -7.5f, -6.f, 3.f, 12.f, kSteelL },
        { 4.5f, -6.f, 3.f, 12.f, kSteelD }, { -6.f, 4.5f, 12.f, 3.f, kSteelD },
        // hub
        { -4.f, -4.f, 8.f, 8.f, kInk }, { -3.f, -3.f, 6.f, 6.f, kGold },
        { -3.f, -3.f, 6.f, 2.f, kGoldL },
    };

    // Play: a right-pointing triangle in nine rows, flat back edge at x = -7
    // carrying a gold spine.
    constexpr Quad kPlay[] = {
        { -7.f, -9.f, 1.8f, 2.f, kSteelH }, { -7.f, -7.f, 5.3f, 2.f, kSteelH },
        { -7.f, -5.f, 8.9f, 2.f, kSteelH }, { -7.f, -3.f, 12.4f, 2.f, kSteelL },
        { -7.f, -1.f, 16.f, 2.f, kSteelL }, { -7.f, 1.f, 12.4f, 2.f, kSteelL },
        { -7.f, 3.f, 8.9f, 2.f, kSteelM }, { -7.f, 5.f, 5.3f, 2.f, kSteelM },
        { -7.f, 7.f, 1.8f, 2.f, kSteelM },
        { -7.f, -7.6f, 1.6f, 15.2f, kGold },
    };

    // Stop: a beveled steel plate with clipped corners and a gold inlay.
    constexpr Quad kStop[] = {
        { -6.f, -9.f, 12.f, 1.f, kSteelL }, { -7.f, -8.f, 14.f, 1.f, kSteelL },
        { -8.f, -7.f, 16.f, 14.f, kSteelM },
        { -7.f, 7.f, 14.f, 1.f, kSteelD }, { -6.f, 8.f, 12.f, 1.f, kSteelD },
        // bevel
        { -8.f, -7.f, 16.f, 2.5f, kSteelL }, { -8.f, -7.f, 2.5f, 14.f, kSteelL },
        { 5.5f, -7.f, 2.5f, 14.f, kSteelD }, { -8.f, 4.5f, 16.f, 2.5f, kSteelD },
        // inlay
        { -4.f, -4.f, 8.f, 8.f, kInk },
        { -3.f, -3.f, 6.f, 6.f, kGold }, { -3.f, -3.f, 6.f, 2.f, kGoldL },
    };

    // Auto battle: two blades crossed in an X, drawn as diagonal staircases.
    // The rear blade goes down first so the front one reads as on top.
    constexpr Quad kAutoBattle[] = {
        // rear blade: tip top-left, hilt bottom-right
        { 5.4f, 5.4f, 3.2f, 3.2f, kSteelD }, { 3.4f, 3.4f, 3.2f, 3.2f, kSteelD },
        { 1.4f, 1.4f, 3.2f, 3.2f, kSteelD }, { -0.6f, -0.6f, 3.2f, 3.2f, kSteelD },
        { -2.6f, -2.6f, 3.2f, 3.2f, kSteelD }, { -4.6f, -4.6f, 3.2f, 3.2f, kSteelD },
        { -6.6f, -6.6f, 3.2f, 3.2f, kSteelD }, { -8.6f, -8.6f, 3.2f, 3.2f, kSteelM },
        // front blade: tip top-right, hilt bottom-left
        { -8.6f, 5.4f, 3.2f, 3.2f, kSteelM }, { -6.6f, 3.4f, 3.2f, 3.2f, kSteelL },
        { -4.6f, 1.4f, 3.2f, 3.2f, kSteelL }, { -2.6f, -0.6f, 3.2f, 3.2f, kSteelL },
        { -0.6f, -2.6f, 3.2f, 3.2f, kSteelL }, { 1.4f, -4.6f, 3.2f, 3.2f, kSteelL },
        { 3.4f, -6.6f, 3.2f, 3.2f, kSteelL }, { 5.4f, -8.6f, 3.2f, 3.2f, kSteelH },
        // grips
        { -11.f, 6.2f, 5.5f, 3.4f, kGold }, { -11.f, 6.2f, 5.5f, 1.2f, kGoldL },
        { 5.5f, 6.2f, 5.5f, 3.4f, kGold }, { 5.5f, 6.2f, 5.5f, 1.2f, kGoldL },
    };

    // Marketplace: a balance scale. The wide beam plus two pans gives a
    // silhouette nothing else in the strip shares, which is what carries it
    // at 13px.
    constexpr Quad kMarketplace[] = {
        { -2.f, -8.f, 4.f, 14.f, kSteelM }, { -2.f, -8.f, 1.5f, 14.f, kSteelL },
        { -5.5f, 6.f, 11.f, 3.f, kSteelD }, { -5.5f, 6.f, 11.f, 1.2f, kSteelM },
        // beam
        { -11.f, -8.f, 22.f, 3.f, kSteelL }, { -11.f, -8.f, 22.f, 1.2f, kSteelH },
        // finial
        { -3.f, -11.5f, 6.f, 3.5f, kGold }, { -3.f, -11.5f, 6.f, 1.4f, kGoldL },
        // hangers
        { -8.4f, -5.f, 1.8f, 2.5f, kSteelD }, { 6.6f, -5.f, 1.8f, 2.5f, kSteelD },
        // pans
        { -11.f, -2.5f, 7.f, 2.5f, kSteelM }, { -10.f, 0.f, 5.f, 2.5f, kSteelD },
        { 4.f, -2.5f, 7.f, 2.5f, kSteelM }, { 5.f, 0.f, 5.f, 2.5f, kSteelD },
    };

    // -- Main toolbar --------------------------------------------------------

    // Cash shop: a shopping bag with a gold coin stamped on it.
    constexpr Quad kCashShop[] = {
        // handles
        { -5.5f, -11.5f, 2.5f, 5.5f, kSteelL }, { 3.f, -11.5f, 2.5f, 5.5f, kSteelL },
        { -5.5f, -11.5f, 11.f, 2.5f, kSteelL },
        // body
        { -9.5f, -6.5f, 19.f, 17.5f, kSteelM },
        { -9.5f, -6.5f, 19.f, 3.f, kSteelD },		// rim, in shadow
        { -9.5f, -3.5f, 3.f, 14.5f, kSteelL },
        { 6.5f, -3.5f, 3.f, 14.5f, kSteelD },
        { -9.5f, 9.f, 19.f, 2.f, kSteelD },
        // coin
        { -4.f, -0.5f, 8.f, 8.f, kInk },
        { -3.f, 0.5f, 6.f, 6.f, kGold }, { -3.f, 0.5f, 6.f, 2.f, kGoldL },
        { -1.f, 2.f, 2.f, 3.f, kInk },
    };

    // Character: a knight helm with a gold crest.
    constexpr Quad kCharacter[] = {
        { -5.f, -12.f, 10.f, 2.f, kSteelL },
        { -7.f, -10.f, 14.f, 2.f, kSteelL },
        { -8.5f, -8.f, 17.f, 3.f, kSteelM },
        { -9.f, -5.f, 18.f, 12.f, kSteelM },
        { -9.f, -5.f, 3.f, 12.f, kSteelL },			// lit cheek
        { 6.f, -5.f, 3.f, 12.f, kSteelD },			// shaded cheek
        { -7.f, 7.f, 14.f, 2.f, kSteelD },
        { -5.f, 9.f, 10.f, 2.f, kSteelD },
        // visor slit and breaths
        { -7.f, -2.5f, 14.f, 3.f, kInk },
        { -5.f, 3.f, 2.5f, 3.f, kInk }, { -1.25f, 3.f, 2.5f, 3.f, kInk },
        { 2.5f, 3.f, 2.5f, 3.f, kInk },
        // crest
        { -1.5f, -13.5f, 3.f, 9.f, kGold }, { -1.5f, -13.5f, 1.2f, 9.f, kGoldL },
    };

    // Inventory: a backpack — grab loop, side straps, a rounded top flap over
    // the body, and a front pocket held by a gold buckle.
    constexpr Quad kInventory[] = {
        { -2.5f, -12.5f, 5.f, 3.5f, kSteelD },		// grab loop
        { -11.f, -5.f, 2.5f, 9.f, kSteelD }, { 8.5f, -5.f, 2.5f, 9.f, kSteelD },	// straps
        // body
        { -9.5f, -6.f, 19.f, 17.f, kSteelM },
        { -9.5f, -6.f, 3.f, 17.f, kSteelL }, { 6.5f, -6.f, 3.f, 17.f, kSteelD },
        { -9.5f, 9.f, 19.f, 2.f, kSteelD },
        // flap, rounded at the top
        { -6.5f, -10.5f, 13.f, 1.5f, kSteelL }, { -8.5f, -9.f, 17.f, 1.5f, kSteelL },
        { -9.5f, -7.5f, 19.f, 5.5f, kSteelL },
        { -6.5f, -10.5f, 13.f, 1.5f, kSteelH }, { -8.5f, -9.f, 17.f, 1.f, kSteelH },
        { -9.5f, -2.f, 19.f, 1.5f, kInk },			// flap shadow line
        // front pocket
        { -6.f, 2.5f, 12.f, 7.f, kSteelD }, { -6.f, 2.5f, 12.f, 1.5f, kSteelM },
        // buckle on the flap edge
        { -2.5f, -4.f, 5.f, 5.f, kGold }, { -2.5f, -4.f, 5.f, 1.5f, kGoldL },
    };

    // Friends: two busts, heads rounded row by row. The near bust needs an
    // explicit ink relief because the sticker outline only wraps the union of
    // the table, not the seam between two overlapping figures.
    constexpr Quad kFriends[] = {
        // far friend, one step back and to the right
        { 2.75f, -8.5f, 3.5f, 1.f, kSteelD }, { 1.5f, -7.5f, 6.f, 1.5f, kSteelD },
        { 0.5f, -6.f, 8.f, 3.5f, kSteelD }, { 1.5f, -2.5f, 6.f, 1.5f, kSteelD },
        { 0.5f, 0.f, 8.f, 2.f, kSteelD }, { -0.5f, 2.f, 10.f, 2.f, kSteelD },
        { -1.f, 4.f, 11.f, 6.f, kSteelD },
        // ink relief: the near bust re-drawn one unit larger
        { -7.5f, -11.5f, 6.f, 3.f, kInk }, { -9.f, -10.5f, 9.f, 3.5f, kInk },
        { -10.f, -9.f, 11.f, 6.f, kInk }, { -9.f, -5.f, 9.f, 3.5f, kInk },
        { -7.5f, -3.5f, 6.f, 3.f, kInk }, { -9.5f, -1.5f, 10.f, 4.f, kInk },
        { -11.5f, 0.5f, 14.f, 4.f, kInk }, { -12.5f, 2.5f, 16.f, 8.5f, kInk },
        // near friend, lit
        { -6.5f, -10.5f, 4.f, 1.f, kSteelL }, { -8.f, -9.5f, 7.f, 1.5f, kSteelL },
        { -9.f, -8.f, 9.f, 3.5f, kSteelL }, { -8.f, -4.f, 7.f, 1.5f, kSteelM },
        { -6.5f, -2.5f, 4.f, 1.f, kSteelM }, { -8.5f, -0.5f, 8.f, 2.f, kSteelM },
        { -10.5f, 1.5f, 12.f, 2.f, kSteelM }, { -11.5f, 3.5f, 14.f, 6.5f, kSteelM },
        { -11.5f, 3.5f, 3.f, 6.5f, kSteelL },
        // collar
        { -8.5f, 1.5f, 8.f, 2.f, kGold },
    };

    // Menu: three list rows inside a gold-rimmed panel.
    constexpr Quad kMenu[] = {
        { -11.f, -11.f, 22.f, 22.f, kGold },
        { -11.f, -11.f, 22.f, 2.f, kGoldL },
        { -9.5f, -9.5f, 19.f, 19.f, kInk },
        { -7.f, -7.f, 14.f, 3.5f, kSteelL }, { -7.f, -7.f, 14.f, 1.2f, kSteelH },
        { -7.f, -1.75f, 14.f, 3.5f, kSteelL }, { -7.f, -1.75f, 14.f, 1.2f, kSteelH },
        { -7.f, 3.5f, 14.f, 3.5f, kSteelL }, { -7.f, 3.5f, 14.f, 1.2f, kSteelH },
    };

    void Fill(float x, float y, float w, float h, Rgb color, float tone)
    {
        glColor4f(color.r * tone, color.g * tone, color.b * tone, 1.f);
        RenderColor(x, y, w, h);
    }

    // Sticker outline: every quad re-drawn dark at eight neighbor offsets, then
    // covered by the colored fills — leaves a clean 1px exterior edge only.
    void DrawOutline(const Quad* quads, int count, float ox, float oy, float scale)
    {
        const float o = kOutlineWidth * scale;
        const float offsets[8][2] = {
            { -o, 0.f }, { o, 0.f }, { 0.f, -o }, { 0.f, o },
            { -o, -o }, { o, -o }, { -o, o }, { o, o },
        };

        for (const auto& off : offsets)
        {
            for (int i = 0; i < count; ++i)
            {
                const Quad& q = quads[i];
                Fill(ox + (q.x * scale) + off[0], oy + (q.y * scale) + off[1],
                    q.w * scale, q.h * scale, kInk, 1.f);
            }
        }
    }

    void DrawTable(const Quad* quads, int count, float ox, float oy, float scale, float tone)
    {
        DrawOutline(quads, count, ox, oy, scale);
        for (int i = 0; i < count; ++i)
        {
            const Quad& q = quads[i];
            Fill(ox + (q.x * scale), oy + (q.y * scale), q.w * scale, q.h * scale, q.color, tone);
        }
    }

    template <std::size_t Count>
    void DrawGlyph(const Quad(&quads)[Count], float centerX, float centerY, float scale, bool enabled)
    {
        EnableAlphaTest();
        DrawTable(quads, static_cast<int>(Count), centerX, centerY, scale,
            enabled ? 1.f : kDisabledTone);
        EndRenderColor();
    }

    Rgb PlateFace(UI::HUD::Icons::PlateState state)
    {
        switch (state)
        {
        case UI::HUD::Icons::PlateState::Hover:   return kFaceHover;
        case UI::HUD::Icons::PlateState::Pressed: return kFacePressed;
        case UI::HUD::Icons::PlateState::Active:  return kFaceActive;
        case UI::HUD::Icons::PlateState::Alert:   return kFaceAlert;
        default:                                  return kFaceNormal;
        }
    }

    // Latched and blinking plates trade the steel bevel for a gold one, which
    // is what replaces the "button down" frame the baked textures used to have.
    Rgb PlateBevel(UI::HUD::Icons::PlateState state)
    {
        switch (state)
        {
        case UI::HUD::Icons::PlateState::Active: return kGold;
        case UI::HUD::Icons::PlateState::Alert:  return kGoldL;
        default:                                 return kSteelD;
        }
    }
}

namespace UI::HUD::Icons
{
    PlateState PlateStateFor(bool hovered, bool pressed, bool active, bool alert)
    {
        if (pressed)
            return PlateState::Pressed;
        if (alert)
            return PlateState::Alert;
        if (hovered)
            return PlateState::Hover;
        if (active)
            return PlateState::Active;
        return PlateState::Normal;
    }

    void DrawButtonPlate(float x, float y, float w, float h, PlateState state)
    {
        const float inset = kPlateRim + kPlateBevel;
        if (w <= inset * 2.f || h <= inset * 2.f)
            return;

        EnableAlphaTest();
        Fill(x, y, w, h, kInk, 1.f);
        Fill(x + kPlateRim, y + kPlateRim, w - (kPlateRim * 2.f), h - (kPlateRim * 2.f),
            PlateBevel(state), 1.f);

        const float faceX = x + inset;
        const float faceY = y + inset;
        const float faceW = w - (inset * 2.f);
        const float faceH = h - (inset * 2.f);
        Fill(faceX, faceY, faceW, faceH, PlateFace(state), 1.f);
        // Lit top edge / shaded bottom edge, so the face does not read flat.
        Fill(faceX, faceY, faceW, kPlateHighlight, kSteelD, 1.f);
        Fill(faceX, faceY + faceH - kPlateHighlight, faceW, kPlateHighlight, kInk, 1.f);
        EndRenderColor();
    }

    void DrawSettingsGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kSettings, centerX, centerY, scale, enabled);
    }

    void DrawPlayGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kPlay, centerX, centerY, scale, enabled);
    }

    void DrawStopGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kStop, centerX, centerY, scale, enabled);
    }

    void DrawAutoBattleGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kAutoBattle, centerX, centerY, scale, enabled);
    }

    void DrawMarketplaceGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kMarketplace, centerX, centerY, scale, enabled);
    }

    void DrawCashShopGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kCashShop, centerX, centerY, scale, enabled);
    }

    void DrawCharacterGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kCharacter, centerX, centerY, scale, enabled);
    }

    void DrawInventoryGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kInventory, centerX, centerY, scale, enabled);
    }

    void DrawFriendsGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kFriends, centerX, centerY, scale, enabled);
    }

    void DrawMenuGlyph(float centerX, float centerY, float scale, bool enabled)
    {
        DrawGlyph(kMenu, centerX, centerY, scale, enabled);
    }
}
