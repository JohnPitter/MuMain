#pragma once

// Procedural HUD button art in MU's steel-and-gold palette: a neutral button
// plate plus one glyph per action, so buttons that used to share a pre-baked
// texture can each show what they actually do. Same technique as
// UI/Voice/VoiceIcons.h — filled quads with a dark "sticker" outline.
namespace UI::HUD::Icons
{
    // Glyphs are authored on a 24-unit grid, so `scale` is roughly
    // "pixels per icon unit x 24 = glyph height in pixels".
    constexpr float kGlyphGrid = 24.f;

    // MU Helper strip buttons are 18x13: an 18-unit tall glyph keeps a 1px
    // margin inside the plate.
    constexpr float kStripGlyphScale = 0.42f;

    // Main toolbar buttons are 30x41; the glyph fills the width and leaves
    // room above and below.
    constexpr float kToolbarGlyphScale = 1.f;

    // A pressed plate sinks its glyph by one pixel, like the original art.
    constexpr float kPressedGlyphNudge = 1.f;

    enum class PlateState
    {
        Normal,
        Hover,
        Pressed,
        Active,		// the window this button opens is currently on screen
        Alert,		// blinking for attention (unread mail, pending quest)
    };

    PlateState PlateStateFor(bool hovered, bool pressed, bool active, bool alert);

    // Dark steel plate with an ink edge and a beveled face, sized to a button
    // rect. Replaces the pre-baked button image so a glyph reads on top of it.
    void DrawButtonPlate(float x, float y, float w, float h, PlateState state);

    // MU Helper strip (see kStripGlyphScale).
    void DrawSettingsGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawPlayGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawStopGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawAutoBattleGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawMarketplaceGlyph(float centerX, float centerY, float scale, bool enabled);

    // Main toolbar (see kToolbarGlyphScale).
    void DrawCashShopGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawCharacterGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawInventoryGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawFriendsGlyph(float centerX, float centerY, float scale, bool enabled);
    void DrawMenuGlyph(float centerX, float centerY, float scale, bool enabled);

    // Every glyph entry point has this shape, so call sites can keep a table
    // of (button, glyph) pairs instead of a switch.
    using GlyphFn = void (*)(float centerX, float centerY, float scale, bool enabled);
}
