#pragma once

// Draws the VIP badge to the right of a name, at the same on-screen size and
// vertical-centering rule as CNewUIGensRanking::RanderMark's left-side gens
// mark (see NewUIGensRanking.cpp, MARK_BOOLEAN case), mirrored to the other
// side. Used from Chat.cpp's nameplate rendering.
namespace UI::Vip::Badge
{
    // rightEdgeX: left edge of the badge, i.e. where the name text ends plus margin.
    // topY: top of the nameplate block (same anchor RanderMark receives as "y").
    // bottomY: bottom of the nameplate block after all its lines were rendered
    //          (same value RanderMark receives as its yOffset/RenderPos.y).
    void Render(float rightEdgeX, float topY, float bottomY);
}
