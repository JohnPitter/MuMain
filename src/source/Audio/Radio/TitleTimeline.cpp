#include "Audio/Radio/TitleTimeline.h"

#include <algorithm>

namespace Audio::Radio
{
    namespace
    {
        const std::wstring kEmptyTitle;
    }

    void TitleTimeline::Reset()
    {
        m_markers.clear();
    }

    void TitleTimeline::Add(std::int64_t positionUs, std::wstring title)
    {
        if (title.empty())
        {
            return;
        }

        // Insert after any marker at the same position so the newest
        // announcement wins at that exact boundary.
        const auto it = std::upper_bound(m_markers.begin(), m_markers.end(), positionUs,
            [](std::int64_t value, const TitleMarker& marker)
            {
                return value < marker.positionUs;
            });
        m_markers.insert(it, TitleMarker { positionUs, std::move(title) });

        while (m_markers.size() > kMaxMarkers)
        {
            m_markers.erase(m_markers.begin());
        }
    }

    const std::wstring& TitleTimeline::TitleAt(std::int64_t playbackUs) const
    {
        if (m_markers.empty())
        {
            return kEmptyTitle;
        }

        if (playbackUs < m_markers.front().positionUs)
        {
            return m_markers.front().title;
        }

        // Last marker with positionUs <= playbackUs.
        const auto it = std::upper_bound(m_markers.begin(), m_markers.end(), playbackUs,
            [](std::int64_t value, const TitleMarker& marker)
            {
                return value < marker.positionUs;
            });
        return (it - 1)->title;
    }
}
