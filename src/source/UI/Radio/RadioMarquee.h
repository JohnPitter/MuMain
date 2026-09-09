#pragma once

// Pure math for the "tocando agora" marquee strip that sits above the bottom
// functionality bar (potions/skills HUD bar). Split out of RadioHud so the
// loop semantics stay unit-tested (tests/radio/test_radio_marquee.cpp).
//
// Loop shape (owner's spec: rolagem infinita DA ESQUERDA PARA A DIREITA):
//   phase 0          -> text fully hidden just left of the strip
//   phase increasing -> text slides continuously to the right
//   phase -> loop    -> text fully hidden just right of the strip, wraps
// Every frame is computed from (nowMs, widths, speed) alone: no state, no
// allocation, no per-frame string work — the render side only supplies a
// timestamp and a cached text width.

#include <cstdint>

#include "UI/Radio/RadioStatusText.h"

namespace UI::Radio
{
    // Total distance (UI px) the text travels in one loop: the text width plus
    // the strip width (hidden-left -> hidden-right).
    float MarqueeLoopWidthPx(float textWidthPx, float trackWidthPx);

    // Strip-local left edge (UI px) of the text at time `nowMs`. Ranges over
    // [-textWidth, +trackWidth) and wraps forever.
    float MarqueeOffsetPx(std::uint32_t nowMs, float textWidthPx, float trackWidthPx,
        float speedPxPerSec);

    // Gate for the whole "tocando agora" strip: it exists ONLY while the radio
    // is enabled AND actually playing (owner request: the band used to show all
    // the time — desligada/offline a faixa some). Connecting/Reconnecting/
    // Off all hide it, so a dead station never advertises itself.
    bool MarqueeVisible(bool radioEnabled, RadioStatusKind statusKind);
}
