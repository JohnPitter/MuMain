#pragma once

// Pure helpers for the Options-window slider input model (volume and effect
// limitation sliders).  Extracted from CNewUIOptionWindow so the drag-gate and
// the position→level mapping can be unit-tested without the UI engine.
//
// The drag gate exists because of the volume-decay bug (2026-09): the slider
// handlers used to rewrite the level whenever the left button was HELD
// (KEY_REPEAT) and the cursor crossed the track — regardless of where the
// press had started.  MU gameplay holds the left button to walk/attack, so a
// held cursor sweeping across an open Options window silently rewrote (and
// saved!) the volume; the leftmost ~⅓ of each track even maps to level 0
// (mute), which is how players ended up with SoundVolume=0 in config.ini.

namespace VolumeSliderInput
{
    // True when the frame's mouse state may move the slider: either the press
    // started on the track this very frame (isPress), or a drag that started
    // on the track is still in progress (wasDragging && isRepeat).
    // A press that began elsewhere and is merely held (isRepeat without
    // wasDragging) must NOT touch the level — that is the stolen-drag bug.
    inline bool ShouldTrackDrag(bool cursorOnTrack, bool isPress, bool isRepeat, bool wasDragging)
    {
        if (!cursorOnTrack)
            return false;
        if (isPress)
            return true;
        return wasDragging && isRepeat;
    }

    // Map a cursor offset (pixels from the track's left edge) to a slider
    // level on the 0..maxLevel scale, rounding to the nearest level — the
    // exact arithmetic the Options window has always used.
    inline int MapOffsetToLevel(int offset, int sliderWidth, int maxLevel)
    {
        if (offset < 0)
            return 0;
        return static_cast<int>((static_cast<float>(maxLevel) * offset) / static_cast<float>(sliderWidth) + 0.5f);
    }
}
