#include "stdafx.h"
#include "UI/Radio/RadioMarquee.h"

#include <cmath>

namespace UI::Radio
{
    bool MarqueeVisible(bool radioEnabled, RadioStatusKind statusKind)
    {
        return radioEnabled && statusKind == RadioStatusKind::Playing;
    }

    float MarqueeLoopWidthPx(float textWidthPx, float trackWidthPx)
    {
        if (textWidthPx < 0.f)
        {
            textWidthPx = 0.f;
        }
        if (trackWidthPx < 0.f)
        {
            trackWidthPx = 0.f;
        }
        return textWidthPx + trackWidthPx;
    }

    float MarqueeOffsetPx(std::uint32_t nowMs, float textWidthPx, float trackWidthPx,
        float speedPxPerSec)
    {
        const float loop = MarqueeLoopWidthPx(textWidthPx, trackWidthPx);
        if (loop <= 0.f)
        {
            return 0.f;
        }

        float speed = speedPxPerSec;
        if (speed <= 0.f)
        {
            speed = 1.f;
        }

        // Day-bounded timestamp: keeps the fmod stable across the ~49.7-day
        // timeGetTime() DWORD wrap (a wrap inside the modulo would jump the
        // phase once; bounding to a whole day makes the residue continuous).
        const float seconds = static_cast<float>(nowMs % 86400000u) / 1000.f;
        float phase = std::fmod(seconds * speed, loop);
        if (phase < 0.f)
        {
            phase += loop;
        }
        return -textWidthPx + phase;
    }
}
