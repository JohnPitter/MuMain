// Unit tests for the "tocando agora" marquee math (UI/Radio/RadioMarquee).
// The owner's spec: infinite left-to-right scroll inside the full width of
// the bottom functionality bar — the text enters at the strip's left edge,
// slides right until it fully exits at the right edge, then repeats. These
// cases pin the loop geometry so a refactor cannot turn it into a bounce or
// a left-exit.

#include "doctest.h"

#include "UI/Radio/RadioMarquee.h"

#include <cstdint>

namespace
{
    constexpr float kTrackWidth = 640.f;   // full bottom bar width (UI px)
    constexpr float kTextWidth = 300.f;    // representative status text
    constexpr float kSpeed = 64.f;         // UI px per second
    constexpr std::uint32_t kDayMs = 86400000u;

    float LoopMs(float textWidth, float trackWidth, float speed)
    {
        const float loopPx = UI::Radio::MarqueeLoopWidthPx(textWidth, trackWidth);
        return loopPx / speed * 1000.f;
    }
}

TEST_CASE("marquee loop width is text + track")
{
    CHECK(UI::Radio::MarqueeLoopWidthPx(300.f, 640.f) == doctest::Approx(940.f));
    CHECK(UI::Radio::MarqueeLoopWidthPx(0.f, 640.f) == doctest::Approx(640.f));
    CHECK(UI::Radio::MarqueeLoopWidthPx(120.f, 0.f) == doctest::Approx(120.f));
    // Negative widths are clamped, never propagate.
    CHECK(UI::Radio::MarqueeLoopWidthPx(-5.f, 640.f) == doctest::Approx(640.f));
    CHECK(UI::Radio::MarqueeLoopWidthPx(-5.f, -5.f) == doctest::Approx(0.f));
}

TEST_CASE("marquee enters from the left edge")
{
    // Phase 0: the text's right edge touches the strip's left edge — fully
    // hidden LEFT of the strip (enters by sliding right).
    const float x0 = UI::Radio::MarqueeOffsetPx(0u, kTextWidth, kTrackWidth, kSpeed);
    CHECK(x0 == doctest::Approx(-kTextWidth).epsilon(0.001));

    // Early in the loop the text is still partially left of the strip.
    const float early = UI::Radio::MarqueeOffsetPx(1000u, kTextWidth, kTrackWidth, kSpeed);
    CHECK(early == doctest::Approx(-kTextWidth + kSpeed * 1.f).epsilon(0.01));
    CHECK(early < 0.f);
}

TEST_CASE("marquee slides right monotonically within one loop")
{
    float previous = UI::Radio::MarqueeOffsetPx(0u, kTextWidth, kTrackWidth, kSpeed);
    const std::uint32_t stepMs = 100u;
    for (std::uint32_t t = stepMs; t <= 14000u; t += stepMs)
    {
        const float x = UI::Radio::MarqueeOffsetPx(t, kTextWidth, kTrackWidth, kSpeed);
        CHECK(x > previous);
        previous = x;
    }
}

TEST_CASE("marquee exits fully at the right edge before wrapping")
{
    const float loopMs = LoopMs(kTextWidth, kTrackWidth, kSpeed);
    // Just before the loop ends the text's left edge has reached (almost) the
    // strip's right edge — fully hidden RIGHT of the strip.
    const float nearEnd = UI::Radio::MarqueeOffsetPx(
        static_cast<std::uint32_t>(loopMs) - 50u, kTextWidth, kTrackWidth, kSpeed);
    CHECK(nearEnd > kTrackWidth - kSpeed * 0.05f - 1.f);
    CHECK(nearEnd <= kTrackWidth);
    CHECK(nearEnd > 0.f);
}

TEST_CASE("marquee wraps forever with a constant period")
{
    const float loopMs = LoopMs(kTextWidth, kTrackWidth, kSpeed);
    const std::uint32_t periodMs = static_cast<std::uint32_t>(loopMs);

    const float x = UI::Radio::MarqueeOffsetPx(12345u, kTextWidth, kTrackWidth, kSpeed);
    const float xNext = UI::Radio::MarqueeOffsetPx(12345u + periodMs, kTextWidth, kTrackWidth, kSpeed);
    CHECK(xNext == doctest::Approx(x).epsilon(0.01));

    // Many periods later — including across the day boundary used to bound
    // the timestamp — the phase still matches.
    const float xMuchLater = UI::Radio::MarqueeOffsetPx(
        12345u + 37u * periodMs + kDayMs, kTextWidth, kTrackWidth, kSpeed);
    CHECK(xMuchLater == doctest::Approx(x).epsilon(0.01));
}

TEST_CASE("marquee covers the full strip width (text narrower than the strip)")
{
    // Halfway through the visible pass the text is fully inside the strip.
    const float loopMs = LoopMs(kTextWidth, kTrackWidth, kSpeed);
    const float midVisibleMs = (kTextWidth + kTrackWidth * 0.5f) / kSpeed * 1000.f;
    CHECK(midVisibleMs < loopMs);
    const float mid = UI::Radio::MarqueeOffsetPx(
        static_cast<std::uint32_t>(midVisibleMs), kTextWidth, kTrackWidth, kSpeed);
    CHECK(mid == doctest::Approx(kTrackWidth * 0.5f).epsilon(0.01));
}

TEST_CASE("marquee handles text wider than the strip")
{
    const float wide = 900.f;
    const float loopMs = LoopMs(wide, kTrackWidth, kSpeed);
    CHECK(loopMs == doctest::Approx((wide + kTrackWidth) / kSpeed * 1000.f).epsilon(0.001));

    // It still enters and exits on schedule; there is just no fully-visible
    // moment — the whole strip is covered mid-loop.
    const float midLoopMs = (wide + kTrackWidth * 0.5f) / kSpeed * 1000.f;
    const float mid = UI::Radio::MarqueeOffsetPx(
        static_cast<std::uint32_t>(midLoopMs), wide, kTrackWidth, kSpeed);
    CHECK(mid == doctest::Approx(kTrackWidth * 0.5f).epsilon(0.01));
}

TEST_CASE("marquee degenerate inputs stay safe")
{
    // Zero text: the "phase" is a bare point sweeping [0, trackWidth).
    const float x = UI::Radio::MarqueeOffsetPx(0u, 0.f, kTrackWidth, kSpeed);
    CHECK(x == doctest::Approx(0.f));
    CHECK(UI::Radio::MarqueeOffsetPx(5000u, 0.f, kTrackWidth, kSpeed)
        == doctest::Approx(64.f * 5.f).epsilon(0.01));

    // Zero speed falls back to a non-zero pace instead of freezing the
    // division; zero loop collapses to 0.
    CHECK(UI::Radio::MarqueeOffsetPx(1000u, kTextWidth, 0.f, 0.f)
        == doctest::Approx(-kTextWidth + 1.f).epsilon(0.01));
    CHECK(UI::Radio::MarqueeOffsetPx(1000u, 0.f, 0.f, kSpeed) == doctest::Approx(0.f));
}

TEST_CASE("marquee speed scales linearly")
{
    const std::uint32_t t = 2000u;
    const float base = UI::Radio::MarqueeOffsetPx(t, kTextWidth, kTrackWidth, kSpeed);
    const float doubled = UI::Radio::MarqueeOffsetPx(t, kTextWidth, kTrackWidth, kSpeed * 2.f);
    // offset = -textWidth + speed * t; doubling the speed doubles the travelled
    // distance: 2 * (base + textWidth) relation within float noise.
    CHECK(doubled == doctest::Approx(2.f * (base + kTextWidth) - kTextWidth).epsilon(0.01));
}
