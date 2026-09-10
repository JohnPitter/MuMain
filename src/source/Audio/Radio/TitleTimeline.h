#pragma once

// Playback-position-to-title timeline for the radio "now playing" label.
//
// The ICY StreamTitle arrives on the DOWNLOAD side of the stream, but the
// audio it describes only becomes audible after the pre-buffer (4s window,
// up to the ~30s backlog cap) drains. Binding every title to the stream
// position where its audio begins — and asking "which title owns the
// CURRENT PLAYBACK position" — is what keeps the label in sync with what
// the player actually hears.
//
// Positions are MICROSECONDS of decoded audio since the connection start
// (the decoder's timeline). Pure data structure: no SDL/network/threading
// dependencies, so the doctest suite can exercise the selection logic
// directly.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Audio::Radio
{
    struct TitleMarker
    {
        std::int64_t positionUs = 0; // stream position where the title's audio begins
        std::wstring title;
    };

    class TitleTimeline
    {
    public:
        // Deep backlog safety valve: the engine stops reading from the socket
        // past ~12MB (~30s+), so at most ~30s of markers can sit between the
        // download edge and the playback edge — one song, two at most. 8
        // markers is orders of magnitude beyond that.
        static constexpr std::size_t kMaxMarkers = 8;

        // Drops every marker (station switch / reconnect): positions from an
        // old session must never answer queries of a new one.
        void Reset();

        // Registers a title that starts playing at `positionUs`. Positions
        // arrive monotonically from the parser; out-of-order input is still
        // kept sorted so the selection stays well-defined. Empty titles are
        // ignored. Beyond kMaxMarkers the OLDEST marker is dropped (its audio
        // is long gone from the backlog by then).
        void Add(std::int64_t positionUs, std::wstring title);

        // Pure selection: the title whose position window contains
        // `playbackUs` — the LAST marker with positionUs <= playbackUs.
        // Before the first marker (audio that started before the first ICY
        // block arrived) the FIRST marker wins: the server announced that
        // song within one metadata period, so showing it beats a blank
        // label. Empty timeline -> empty string.
        const std::wstring& TitleAt(std::int64_t playbackUs) const;

        bool Empty() const { return m_markers.empty(); }
        std::size_t Size() const { return m_markers.size(); }

    private:
        std::vector<TitleMarker> m_markers; // sorted by positionUs ascending
    };
}
