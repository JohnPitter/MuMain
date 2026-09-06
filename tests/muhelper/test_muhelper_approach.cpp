#include "doctest.h"

#include "MUHelper/MuHelperApproach.h"

#include <algorithm>
#include <vector>

using namespace MUHelper::Approach;

namespace
{
// Synthetic map for the candidate generator. Everything is walkable unless a
// test blocks it; blocked cells carry TW_NOMOVE, exactly like a wall, a fence
// or a cage wall in the real terrain.
class GridTerrain final : public ITerrain
{
public:
    static constexpr int kSize = 64;

    GridTerrain()
        : m_attributes(static_cast<size_t>(kSize) * kSize, 0u)
    {
    }

    void Set(int x, int y, unsigned int attribute)
    {
        m_attributes[Index(x, y)] = attribute;
    }

    // A straight wall segment; the classic "hero on one side, mob on the
    // other" obstacle.
    void BlockColumn(int x, int y0, int y1)
    {
        for (int y = y0; y <= y1; ++y)
        {
            Set(x, y, kTwNoMove);
        }
    }

    unsigned int Attribute(int x, int y) const override
    {
        if (x < 0 || y < 0 || x >= kSize || y >= kSize)
        {
            return kTwNoMove;
        }
        return m_attributes[Index(x, y)];
    }

    // Sampled line of sight, same spirit as CheckWall (ZzzInterface.cpp): the
    // line is blocked by anything at or above TW_NOMOVE.
    bool HasLineOfSight(int ax, int ay, int bx, int by) const override
    {
        const int steps = std::max(std::abs(bx - ax), std::abs(by - ay));
        if (steps == 0)
        {
            return true;
        }

        for (int i = 0; i <= steps; ++i)
        {
            const double t = static_cast<double>(i) / steps;
            const int x = static_cast<int>(std::lround(ax + (bx - ax) * t));
            const int y = static_cast<int>(std::lround(ay + (by - ay) * t));
            if (Attribute(x, y) >= kTwNoMove)
            {
                return false;
            }
        }
        return true;
    }

private:
    static size_t Index(int x, int y)
    {
        return static_cast<size_t>(y) * kSize + static_cast<size_t>(x);
    }

    std::vector<unsigned int> m_attributes;
};

Request MeleeRequest(Cell hero, Cell target)
{
    Request req;
    req.hero = hero;
    req.target = target;
    req.range = 1.8f; // default basic-attack range in MuHelper.cpp
    return req;
}

bool Contains(const std::vector<Candidate>& list, Cell cell)
{
    return std::any_of(list.begin(), list.end(), [&](const Candidate& c)
    {
        return SameCell(c.cell, cell);
    });
}
}

TEST_CASE("walkability mirrors the client pathfinder")
{
    CHECK(IsWalkable(0u));
    // The pathfinder strips these three before comparing against TW_CHARACTER.
    CHECK(IsWalkable(kTwAction));
    CHECK(IsWalkable(kTwHeight));
    CHECK(IsWalkable(kTwCameraUp));

    CHECK_FALSE(IsWalkable(kTwNoMove));
    // Holes were accepted by the old helper and refused by the pathfinder,
    // which is one way it picked a destination it could never reach.
    CHECK_FALSE(IsWalkable(kTwNoGround));
    CHECK_FALSE(IsWalkable(kTwCharacter));
    CHECK_FALSE(IsWalkable(kTwNoMove | kTwAction));
    // Safe zones are walkable for the pathfinder but off limits for the bot:
    // stepping into one auto-stops the helper.
    CHECK_FALSE(IsWalkable(kTwSafeZone));
}

TEST_CASE("travel distance matches the helper's own metric")
{
    CHECK(TravelDistance({ 10, 10 }, { 10, 10 }) == 0);
    CHECK(TravelDistance({ 10, 10 }, { 13, 10 }) == 3);
    CHECK(TravelDistance({ 10, 10 }, { 13, 14 }) == 5);
    CHECK(TravelDistance({ 10, 10 }, { 11, 11 }) == 2); // ceil(1.41)
}

TEST_CASE("open ground yields an in-range cell on the hero's side first")
{
    const GridTerrain terrain;
    std::vector<Candidate> candidates;
    BuildCandidates(MeleeRequest({ 10, 20 }, { 20, 20 }), terrain, candidates);

    REQUIRE_FALSE(candidates.empty());
    const Candidate& best = candidates.front();
    CHECK(best.inRange);
    CHECK(best.hasLos);
    // Nearest attack cell to a hero standing due west of the mob.
    CHECK(best.cell.x == 19);
    CHECK(best.cell.y == 20);
}

TEST_CASE("the target's own cell and the hero's cell are never candidates")
{
    const GridTerrain terrain;
    std::vector<Candidate> candidates;
    // Hero already standing on an attack cell: it stalled there, so it must
    // not be offered again.
    BuildCandidates(MeleeRequest({ 19, 20 }, { 20, 20 }), terrain, candidates);

    CHECK_FALSE(Contains(candidates, { 20, 20 }));
    CHECK_FALSE(Contains(candidates, { 19, 20 }));
    CHECK_FALSE(candidates.empty());
}

TEST_CASE("blocked cells are never offered as approach cells")
{
    GridTerrain terrain;
    terrain.Set(19, 20, kTwNoMove);
    terrain.Set(19, 19, kTwNoGround);
    terrain.Set(19, 21, kTwCharacter);

    std::vector<Candidate> candidates;
    BuildCandidates(MeleeRequest({ 10, 20 }, { 20, 20 }), terrain, candidates);

    CHECK_FALSE(Contains(candidates, { 19, 20 }));
    CHECK_FALSE(Contains(candidates, { 19, 19 }));
    CHECK_FALSE(Contains(candidates, { 19, 21 }));
    CHECK_FALSE(candidates.empty());
}

TEST_CASE("a ranged class never stops where the fence blocks its shot")
{
    GridTerrain terrain;
    // A fence three tiles west of the mob. Bow range reaches past it, but the
    // client refuses an attack through a wall (CheckWall in SkillExecution.cpp),
    // so cells on the hero's side of the fence are not attack positions.
    terrain.BlockColumn(17, 14, 26);

    Request req = MeleeRequest({ 10, 20 }, { 20, 20 });
    req.range = 6.0f;

    std::vector<Candidate> candidates;
    BuildCandidates(req, terrain, candidates);

    REQUIRE_FALSE(candidates.empty());
    bool anyAttackCell = false;
    for (const Candidate& candidate : candidates)
    {
        CHECK(candidate.cell.x != 17); // the fence itself is not walkable
        if (!candidate.inRange)
        {
            continue;
        }
        anyAttackCell = true;
        CHECK(candidate.hasLos);
        CHECK(candidate.cell.x > 17);
    }
    CHECK(anyAttackCell);
}

TEST_CASE("a wall between hero and target still produces somewhere to walk")
{
    GridTerrain terrain;
    // Long fence with a gap far to the south: the mob is reachable, but only
    // by walking around. This is the reported bug -- the old code found no
    // straight-line cell and simply stood still.
    terrain.BlockColumn(15, 10, 28);

    std::vector<Candidate> candidates;
    BuildCandidates(MeleeRequest({ 10, 20 }, { 20, 20 }), terrain, candidates);

    REQUIRE_FALSE(candidates.empty());
    for (const Candidate& candidate : candidates)
    {
        CHECK(IsWalkable(terrain.Attribute(candidate.cell.x, candidate.cell.y)));
    }
}

TEST_CASE("staging cells outside attack range are offered as detours")
{
    const GridTerrain terrain;
    Request req = MeleeRequest({ 10, 20 }, { 20, 20 });
    req.attempt = 3; // late attempt: detours are cheap by then

    std::vector<Candidate> candidates;
    BuildCandidates(req, terrain, candidates);

    REQUIRE_FALSE(candidates.empty());
    const bool anyStaging = std::any_of(candidates.begin(), candidates.end(),
        [](const Candidate& c) { return !c.inRange; });
    CHECK(anyStaging);
}

TEST_CASE("consecutive attempts alternate sides around the target")
{
    const GridTerrain terrain;
    const Cell hero{ 10, 20 };
    const Cell target{ 20, 20 };

    CHECK(PreferredSide(0) == 0);
    CHECK(PreferredSide(1) == 1);
    CHECK(PreferredSide(2) == -1);
    CHECK(PreferredSide(3) == 1);

    Request left = MeleeRequest(hero, target);
    left.attempt = 1;
    Request right = MeleeRequest(hero, target);
    right.attempt = 2;

    std::vector<Candidate> leftCandidates;
    std::vector<Candidate> rightCandidates;
    BuildCandidates(left, terrain, leftCandidates);
    BuildCandidates(right, terrain, rightCandidates);

    REQUIRE_FALSE(leftCandidates.empty());
    REQUIRE_FALSE(rightCandidates.empty());

    const int leftSide = SideOfApproach(hero, target, leftCandidates.front().cell);
    const int rightSide = SideOfApproach(hero, target, rightCandidates.front().cell);
    CHECK(leftSide == 1);
    CHECK(rightSide == -1);
}

TEST_CASE("cells already tried are skipped along with their neighbours")
{
    const GridTerrain terrain;
    std::vector<Candidate> candidates;
    BuildCandidates(MeleeRequest({ 10, 20 }, { 20, 20 }), terrain, candidates);
    REQUIRE_FALSE(candidates.empty());
    const Cell tried = candidates.front().cell;

    Request retry = MeleeRequest({ 10, 20 }, { 20, 20 });
    retry.attempt = 1;
    retry.avoid = &tried;
    retry.avoidCount = 1;

    std::vector<Candidate> retryCandidates;
    BuildCandidates(retry, terrain, retryCandidates);

    REQUIRE_FALSE(retryCandidates.empty());
    for (const Candidate& candidate : retryCandidates)
    {
        CHECK_FALSE(IsNearAny(candidate.cell, &tried, 1, kAvoidRadius));
    }
}

TEST_CASE("the leash drops candidates the hero may not walk to")
{
    const GridTerrain terrain;
    Request req = MeleeRequest({ 10, 20 }, { 30, 20 });
    req.maxTravel = 8; // hero is 20 tiles away: nothing is inside the leash

    std::vector<Candidate> candidates;
    BuildCandidates(req, terrain, candidates);
    CHECK(candidates.empty());

    req.maxTravel = 25;
    BuildCandidates(req, terrain, candidates);
    REQUIRE_FALSE(candidates.empty());
    for (const Candidate& candidate : candidates)
    {
        CHECK(candidate.travel <= 25);
    }
}

TEST_CASE("a bow's range produces attack cells much further from the target")
{
    const GridTerrain terrain;
    Request req = MeleeRequest({ 10, 20 }, { 20, 20 });
    req.range = 6.0f; // BASIC_RANGE_BOW in MuHelper.cpp

    std::vector<Candidate> candidates;
    BuildCandidates(req, terrain, candidates);

    REQUIRE_FALSE(candidates.empty());
    const Candidate& best = candidates.front();
    CHECK(best.inRange);
    CHECK(best.hasLos);
    // A ranged class stops as soon as it is inside range instead of walking
    // into melee distance.
    CHECK(TravelDistance(best.cell, { 20, 20 }) > 3);
}

TEST_CASE("the candidate list is deterministic and bounded")
{
    const GridTerrain terrain;
    std::vector<Candidate> first;
    std::vector<Candidate> second;
    Request req = MeleeRequest({ 10, 20 }, { 20, 20 });
    req.range = 6.0f;

    BuildCandidates(req, terrain, first);
    BuildCandidates(req, terrain, second);

    REQUIRE(first.size() == second.size());
    CHECK(first.size() <= static_cast<size_t>(kMaxCandidates));
    for (size_t i = 0; i < first.size(); ++i)
    {
        CHECK(SameCell(first[i].cell, second[i].cell));
    }

    // Sorted by score, cheapest first.
    for (size_t i = 1; i < first.size(); ++i)
    {
        CHECK(first[i - 1].score <= first[i].score);
    }
}

TEST_CASE("the reposition budget is spent before the bot gives up")
{
    for (int attempt = 0; attempt < kMaxRepositionAttempts; ++attempt)
    {
        CHECK(DecideStallAction(attempt, true) == StallAction::Reposition);
    }
    CHECK(DecideStallAction(kMaxRepositionAttempts, true) == StallAction::BackOff);
    CHECK(DecideStallAction(kMaxRepositionAttempts + 1, true) == StallAction::BackOff);
}

TEST_CASE("with nowhere to walk the bot backs off immediately")
{
    CHECK(DecideStallAction(0, false) == StallAction::BackOff);
}

TEST_CASE("a target is given up on cycles or on total stall time")
{
    CHECK_FALSE(ShouldGiveUpTarget(0, 0));
    CHECK_FALSE(ShouldGiveUpTarget(kMaxStallCycles - 1, kUnreachableGiveUpMs - 1));
    CHECK(ShouldGiveUpTarget(kMaxStallCycles, 0));
    CHECK(ShouldGiveUpTarget(kMaxStallCycles + 1, 0));
    CHECK(ShouldGiveUpTarget(0, kUnreachableGiveUpMs));
    CHECK(ShouldGiveUpTarget(0, kUnreachableGiveUpMs + 5000));
}

TEST_CASE("moving is not the same as getting closer")
{
    // First observation of a lock: whatever the distance, it is the best so far.
    CHECK(ClassifyProgress(12, -1, false, false) == ProgressKind::Approach);
    // Genuinely closing in.
    CHECK(ClassifyProgress(8, 12, true, false) == ProgressKind::Approach);
    // The target died: the chase succeeded.
    CHECK(ClassifyProgress(12, 8, false, true) == ProgressKind::Approach);
    // Sidestepping around an obstacle without getting closer. This must not
    // reset the give-up budget, or an unreachable mob is chased forever.
    CHECK(ClassifyProgress(12, 8, true, false) == ProgressKind::Motion);
    CHECK(ClassifyProgress(8, 8, true, false) == ProgressKind::Motion);
    // Nothing at all happened.
    CHECK(ClassifyProgress(8, 8, false, false) == ProgressKind::None);
}

TEST_CASE("failed probes mask out their own neighbourhood")
{
    const Cell failed[] = { { 20, 20 }, { 25, 25 } };
    CHECK(IsNearAny({ 20, 20 }, failed, 2, kProbeSpread));
    CHECK(IsNearAny({ 21, 22 }, failed, 2, kProbeSpread));
    CHECK(IsNearAny({ 27, 25 }, failed, 2, kProbeSpread));
    CHECK_FALSE(IsNearAny({ 20, 30 }, failed, 2, kProbeSpread));
    CHECK_FALSE(IsNearAny({ 20, 20 }, nullptr, 0, kProbeSpread));
}
