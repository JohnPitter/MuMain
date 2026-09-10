// Unit tests for Audio/Radio/TitleTimeline — the pure "which title owns the
// current PLAYBACK position" selection that keeps the now-playing label in
// sync with the audible audio (the ICY title arrives one pre-buffer ahead of
// the sound; binding titles to stream positions is what closes that gap).

#include "doctest.h"

#include "Audio/Radio/TitleTimeline.h"

#include <string>

namespace
{
    constexpr std::int64_t SecondUs = 1000000LL;

    // Standard three-song timeline, one title every 30s of stream time,
    // mimicking what the parser records during a real connection.
    Audio::Radio::TitleTimeline MakeThreeTitleTimeline()
    {
        Audio::Radio::TitleTimeline timeline;
        timeline.Add(0 * SecondUs, L"Musica A");
        timeline.Add(30 * SecondUs, L"Musica B");
        timeline.Add(60 * SecondUs, L"Musica C");
        return timeline;
    }
}

TEST_CASE("Empty timeline reports no title at any position")
{
    Audio::Radio::TitleTimeline timeline;
    CHECK(timeline.Empty());
    CHECK(timeline.TitleAt(-1) == L"");
    CHECK(timeline.TitleAt(0) == L"");
    CHECK(timeline.TitleAt(3600 * SecondUs) == L"");
}

TEST_CASE("A single title covers every playback position")
{
    Audio::Radio::TitleTimeline timeline;
    timeline.Add(4 * SecondUs, L"Legiao Urbana - Tempo Perdido");

    CHECK_FALSE(timeline.Empty());
    CHECK(timeline.Size() == 1);

    // Long before the marker (the pre-first-ICY audio clamps to the first
    // title), exactly at it, and far past it — one song owns the whole line.
    CHECK(timeline.TitleAt(0) == L"Legiao Urbana - Tempo Perdido");
    CHECK(timeline.TitleAt(3 * SecondUs) == L"Legiao Urbana - Tempo Perdido");
    CHECK(timeline.TitleAt(4 * SecondUs) == L"Legiao Urbana - Tempo Perdido");
    CHECK(timeline.TitleAt(400 * SecondUs) == L"Legiao Urbana - Tempo Perdido");
}

TEST_CASE("Three titles switch exactly at their positions, boundaries inclusive")
{
    const Audio::Radio::TitleTimeline timeline = MakeThreeTitleTimeline();
    CHECK(timeline.Size() == 3);

    CHECK(timeline.TitleAt(0) == L"Musica A");
    CHECK(timeline.TitleAt(29 * SecondUs) == L"Musica A");
    CHECK(timeline.TitleAt(30 * SecondUs - 1) == L"Musica A");
    CHECK(timeline.TitleAt(30 * SecondUs) == L"Musica B");
    CHECK(timeline.TitleAt(45 * SecondUs) == L"Musica B");
    CHECK(timeline.TitleAt(60 * SecondUs - 1) == L"Musica B");
    CHECK(timeline.TitleAt(60 * SecondUs) == L"Musica C");
    CHECK(timeline.TitleAt(10 * 60 * SecondUs) == L"Musica C");
}

TEST_CASE("A title change inside the pre-buffer window only shows when playback crosses it")
{
    // Real shape: the download edge decoded ~40s of audio (deep backlog), so
    // the titles at 35s and 38s are already known while the listener is still
    // around 4s. The label must follow the PLAYBACK position, not the
    // download edge.
    Audio::Radio::TitleTimeline timeline;
    timeline.Add(0 * SecondUs, L"Tocando agora");
    timeline.Add(35 * SecondUs, L"Proxima 1");
    timeline.Add(38 * SecondUs, L"Proxima 2");

    const std::int64_t producedUs = 40 * SecondUs;   // decoder edge
    const std::int64_t queuedUs = 36 * SecondUs;     // backlog: the listener is 36s behind

    // Playback position = produced - queued = 4s: all three titles are known
    // on the download edge, yet the label still shows the first song.
    CHECK(timeline.TitleAt(producedUs - queuedUs) == L"Tocando agora");
    CHECK(timeline.TitleAt(producedUs - (5 * SecondUs + 1)) == L"Tocando agora");

    // Time passes; playback advances (the queue drains). The label flips
    // exactly at the 35s boundary — when that song becomes audible — not
    // when its title was downloaded.
    CHECK(timeline.TitleAt(producedUs - 5 * SecondUs) == L"Proxima 1");
    CHECK(timeline.TitleAt(38 * SecondUs) == L"Proxima 2");
    CHECK(timeline.TitleAt(producedUs - 2 * SecondUs) == L"Proxima 2");
}

TEST_CASE("Reset models a reconnect: everything clears, then a fresh session works")
{
    Audio::Radio::TitleTimeline timeline = MakeThreeTitleTimeline();
    REQUIRE(timeline.Size() == 3);

    // Station switch / reconnect: positions from the dead session are poison.
    timeline.Reset();
    CHECK(timeline.Empty());
    CHECK(timeline.TitleAt(45 * SecondUs) == L"");

    // The new connection restarts its timeline at zero.
    timeline.Add(2 * SecondUs, L"Nova sessao");
    CHECK(timeline.Size() == 1);
    CHECK(timeline.TitleAt(0) == L"Nova sessao");
    CHECK(timeline.TitleAt(2 * SecondUs) == L"Nova sessao");
}

TEST_CASE("Add keeps the timeline sorted even if positions arrive out of order")
{
    Audio::Radio::TitleTimeline timeline;
    timeline.Add(30 * SecondUs, L"Terceiro");
    timeline.Add(0 * SecondUs, L"Primeiro");
    timeline.Add(15 * SecondUs, L"Segundo");

    CHECK(timeline.Size() == 3);
    CHECK(timeline.TitleAt(14 * SecondUs) == L"Primeiro");
    CHECK(timeline.TitleAt(15 * SecondUs) == L"Segundo");
    CHECK(timeline.TitleAt(29 * SecondUs) == L"Segundo");
    CHECK(timeline.TitleAt(30 * SecondUs) == L"Terceiro");
}

TEST_CASE("Two titles at the same position: the newest announcement wins")
{
    Audio::Radio::TitleTimeline timeline;
    timeline.Add(5 * SecondUs, L"Corrigido");
    timeline.Add(5 * SecondUs, L"Anuncio duplicado");

    CHECK(timeline.Size() == 2);
    CHECK(timeline.TitleAt(5 * SecondUs) == L"Anuncio duplicado");
}

TEST_CASE("The cap keeps the most recent markers, dropping the oldest")
{
    Audio::Radio::TitleTimeline timeline;
    for (int song = 0; song < 12; ++song)
    {
        timeline.Add(song * 30 * SecondUs, L"Musica " + std::to_wstring(song));
    }

    CHECK(timeline.Size() == Audio::Radio::TitleTimeline::kMaxMarkers);
    // Songs 0..3 were dropped; the earliest kept marker is song 4. Playback
    // inside the dropped range clamps to the oldest kept title (its audio
    // left the backlog long ago).
    CHECK(timeline.TitleAt(0) == L"Musica 4");
    CHECK(timeline.TitleAt(4 * 30 * SecondUs) == L"Musica 4");
    CHECK(timeline.TitleAt(11 * 30 * SecondUs) == L"Musica 11");
}

TEST_CASE("Empty titles are ignored and never become markers")
{
    Audio::Radio::TitleTimeline timeline;
    timeline.Add(0, L"");
    timeline.Add(5 * SecondUs, L"");
    CHECK(timeline.Empty());
    CHECK(timeline.TitleAt(100 * SecondUs) == L"");
}
