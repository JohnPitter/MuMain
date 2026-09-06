#pragma once

// Pure approach/repositioning rules for the Mu Helper: which cells around a
// locked target are worth walking to, in which order they should be probed
// with the client pathfinder, and when a target that cannot be reached is
// finally given up.
//
// Everything here is a pure function of its inputs (terrain is injected through
// ITerrain) so it can be unit tested without the game
// (tests/muhelper/test_muhelper_approach.cpp). MuHelper.cpp owns the game
// state, the pathfinder and the packets; this header owns only the geometry
// and the budgets.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace MUHelper::Approach
{
struct Cell
{
    int x = 0;
    int y = 0;
};

inline bool SameCell(const Cell& a, const Cell& b)
{
    return a.x == b.x && a.y == b.y;
}

// Terrain attribute bits. Mirror of the TW_* macros in Core/Globals/_define.h;
// MuHelper.cpp static_asserts them against the real macros so the two can never
// drift apart.
inline constexpr unsigned int kTwSafeZone = 0x0001;
inline constexpr unsigned int kTwCharacter = 0x0002;
inline constexpr unsigned int kTwNoMove = 0x0004;
inline constexpr unsigned int kTwNoGround = 0x0008;
inline constexpr unsigned int kTwAction = 0x0020;
inline constexpr unsigned int kTwHeight = 0x0040;
inline constexpr unsigned int kTwCameraUp = 0x0080;

// Walkability exactly as PATH::FindPath (Engine/Pathing/ZzzPath.h) sees it when
// called through PathFinding2 with the default iWall == TW_CHARACTER: the cell
// attribute, minus the three cosmetic bits the pathfinder strips, has to stay
// below TW_CHARACTER. The old helper only rejected TW_NOMOVE, so it happily
// picked holes (TW_NOGROUND), water, no-attack zones and cells another
// character was standing on -- destinations the pathfinder then refused, which
// is one of the ways the bot ended up standing still in front of an obstacle.
// Safe zones are walkable for the pathfinder but excluded here on purpose: the
// helper auto-stops as soon as the hero steps into one.
inline bool IsWalkable(unsigned int attribute)
{
    unsigned int a = attribute;
    if ((a & kTwAction) == kTwAction)
        a -= kTwAction;
    if ((a & kTwHeight) == kTwHeight)
        a -= kTwHeight;
    if ((a & kTwCameraUp) == kTwCameraUp)
        a -= kTwCameraUp;

    if ((a & kTwSafeZone) == kTwSafeZone)
        return false;

    return a < kTwCharacter;
}

// Same metric CMuHelper::ComputeDistanceBetween uses, so scores here and
// distances elsewhere in the helper are directly comparable.
inline int TravelDistance(const Cell& a, const Cell& b)
{
    const int dx = a.x - b.x;
    const int dy = a.y - b.y;
    return static_cast<int>(std::ceil(std::sqrt(static_cast<double>(dx * dx + dy * dy))));
}

// Usable terrain range; matches the guards the helper already used.
inline constexpr int kMinCell = 1;
inline constexpr int kMaxCell = 254;

// A cell only counts as "inside attack range" with this much slack, so the
// first swing from it is not sitting exactly on the range boundary.
inline constexpr float kRangeMargin = 0.35f;

// How far outside attack range staging cells may be picked. These are the
// cells that let the hero walk *around* an obstacle: they are not attack
// positions, they are places from which a new approach can be planned.
inline constexpr int kMaxDetourRings = 3;

// Hard cap on the generated list; the generator runs 4x/s.
inline constexpr int kMaxCandidates = 32;

// Pathfinder probes per planning pass. Each probe is one PathFinding2 call
// (<= 500 node expansions), and probes only happen when a path is actually
// (re)planned, never on every tick of an ongoing walk.
inline constexpr int kMaxPathProbes = 6;

// Once a candidate fails to produce a real path, its immediate neighbours fail
// for the same obstacle: skip anything this close to an already failed cell so
// the probe budget spreads around the target instead of hammering one side.
inline constexpr int kProbeSpread = 2;

// Reposition attempts allowed per stall cycle before the helper stops moving
// and falls back to the exponential backoff / give-up path.
inline constexpr int kMaxRepositionAttempts = 4;

// Give-up rule for a locked target that stays unreachable, unchanged from
// 02a75577 and now expressed as a pure predicate so it can be tested.
inline constexpr int kMaxStallCycles = 3;
inline constexpr std::uint32_t kUnreachableGiveUpMs = 20000;

// Score weights. Lower score wins.
// - a staging cell is worth taking only when no attack cell is reachable, so it
//   starts penalised and gets cheaper on later attempts (the obstacle is real).
// - a cell without line of sight to the target cannot host a swing.
// - the side penalty makes consecutive attempts alternate around the obstacle
//   instead of retrying the same blocked direction.
inline constexpr int kDetourPenalty = 6;
inline constexpr int kDetourRingPenalty = 2;
inline constexpr int kNoLosPenalty = 12;
inline constexpr int kSidePenalty = 5;
inline constexpr int kAvoidRadius = 1;

struct Candidate
{
    Cell cell{};
    int ring = 0;          // chebyshev ring around the target cell
    int travel = 0;        // hero -> cell distance
    bool inRange = false;  // a swing from here reaches the target
    bool hasLos = false;   // clear line from here to the target
    int score = 0;
};

// Terrain access, injected so the generator stays pure. The live
// implementation in MuHelper.cpp reads TerrainWall and CheckWall.
class ITerrain
{
public:
    virtual ~ITerrain() = default;
    virtual unsigned int Attribute(int x, int y) const = 0;
    virtual bool HasLineOfSight(int ax, int ay, int bx, int by) const = 0;
};

struct Request
{
    Cell hero{};
    Cell target{};
    float range = 1.8f;
    // Reposition attempt index. 0 is the plain "get in range" plan; every
    // further attempt alternates the preferred side and makes staging cells
    // cheaper.
    int attempt = 0;
    // Leash: candidates further than this from the hero are dropped. 0 = off.
    int maxTravel = 0;
    // Attacks need a clear line; only staging cells may be picked blind.
    bool requireLos = true;
    // Cells already tried and rejected during this stall; they and their
    // immediate neighbours are skipped.
    const Cell* avoid = nullptr;
    int avoidCount = 0;
};

inline bool IsNearAny(const Cell& c, const Cell* list, int count, int radius)
{
    if (list == nullptr)
        return false;

    for (int i = 0; i < count; ++i)
    {
        if (std::abs(c.x - list[i].x) <= radius && std::abs(c.y - list[i].y) <= radius)
            return true;
    }
    return false;
}

// Which side of the hero->target axis a cell sits on: +1 left, -1 right, 0 on
// the axis.
inline int SideOfApproach(const Cell& hero, const Cell& target, const Cell& cell)
{
    const int dirX = target.x - hero.x;
    const int dirY = target.y - hero.y;
    const int vX = cell.x - target.x;
    const int vY = cell.y - target.y;
    const int cross = dirX * vY - dirY * vX;
    if (cross > 0)
        return 1;
    if (cross < 0)
        return -1;
    return 0;
}

// The side attempt N should try first. Attempt 0 has no bias (just take the
// closest valid cell); every retry alternates.
inline int PreferredSide(int attempt)
{
    if (attempt <= 0)
        return 0;
    return (attempt % 2 == 1) ? 1 : -1;
}

// Ordered list of cells worth walking to in order to attack `target`.
//
// Two families are produced:
//  * attack cells  -- inside real attack range, with line of sight to the
//                     target, i.e. the swing is legal the moment the hero
//                     arrives (this is what ranged classes need too: the
//                     client refuses an attack through a wall, see CheckWall
//                     in SkillExecution.cpp / ClassAttack.cpp).
//  * staging cells -- walkable cells one to kMaxDetourRings rings beyond
//                     range, used to get around an obstacle; no line of sight
//                     required, because the point is to leave the blocked
//                     corridor and re-plan from the other side.
//
// The order is fully deterministic: score, then distance, then y, then x.
inline void BuildCandidates(const Request& req, const ITerrain& terrain,
    std::vector<Candidate>& out)
{
    out.clear();

    const float fRange = req.range > 0.0f ? req.range : 1.0f;
    const int rangeCells = static_cast<int>(std::ceil(fRange));
    const int searchR = std::min(rangeCells + kMaxDetourRings, 9);
    const int preferredSide = PreferredSide(req.attempt);
    const int detourPenalty = std::max(0, kDetourPenalty - req.attempt * kDetourRingPenalty);

    out.reserve(static_cast<size_t>(kMaxCandidates));

    for (int dy = -searchR; dy <= searchR; ++dy)
    {
        for (int dx = -searchR; dx <= searchR; ++dx)
        {
            if (dx == 0 && dy == 0)
                continue; // the target's own cell is never walkable

            const Cell cell{ req.target.x + dx, req.target.y + dy };
            if (cell.x < kMinCell || cell.y < kMinCell || cell.x > kMaxCell || cell.y > kMaxCell)
                continue;

            if (SameCell(cell, req.hero))
                continue; // standing here already stalled

            if (!IsWalkable(terrain.Attribute(cell.x, cell.y)))
                continue;

            if (IsNearAny(cell, req.avoid, req.avoidCount, kAvoidRadius))
                continue;

            const int travel = TravelDistance(req.hero, cell);
            if (req.maxTravel > 0 && travel > req.maxTravel)
                continue;

            const float fdx = static_cast<float>(dx);
            const float fdy = static_cast<float>(dy);
            const float dist = std::sqrt(fdx * fdx + fdy * fdy);
            const bool inRange = dist <= fRange - kRangeMargin;

            const int ring = std::max(std::abs(dx), std::abs(dy));

            Candidate candidate;
            candidate.cell = cell;
            candidate.ring = ring;
            candidate.travel = travel;
            candidate.inRange = inRange;
            candidate.hasLos = terrain.HasLineOfSight(cell.x, cell.y, req.target.x, req.target.y);

            if (inRange && req.requireLos && !candidate.hasLos)
                continue; // a swing from here would be refused by the client

            if (!inRange && ring < rangeCells)
                continue; // inside the range disk but out of range: useless

            candidate.score = travel;
            if (!inRange)
            {
                candidate.score += detourPenalty + (ring - rangeCells) * kDetourRingPenalty;
                if (!candidate.hasLos)
                    candidate.score += kNoLosPenalty;
            }

            // On a retry the straight corridor between hero and target is the
            // one that already failed, so cells on the axis are penalised just
            // like cells on the side tried last time.
            if (preferredSide != 0
                && SideOfApproach(req.hero, req.target, cell) != preferredSide)
            {
                candidate.score += kSidePenalty;
            }

            out.push_back(candidate);
        }
    }

    std::sort(out.begin(), out.end(), [](const Candidate& a, const Candidate& b)
    {
        if (a.score != b.score)
            return a.score < b.score;
        if (a.travel != b.travel)
            return a.travel < b.travel;
        if (a.cell.y != b.cell.y)
            return a.cell.y < b.cell.y;
        return a.cell.x < b.cell.x;
    });

    if (out.size() > static_cast<size_t>(kMaxCandidates))
        out.resize(static_cast<size_t>(kMaxCandidates));
}

// What a stalled attack should do next.
enum class StallAction
{
    Reposition, // still allowed to try walking somewhere else
    BackOff,    // attempts spent (or nothing to walk to): pause / give up
};

inline StallAction DecideStallAction(int repositionAttempts, bool hasCandidate)
{
    if (!hasCandidate || repositionAttempts >= kMaxRepositionAttempts)
        return StallAction::BackOff;
    return StallAction::Reposition;
}

inline bool ShouldGiveUpTarget(int stallCycles, std::uint32_t stalledMs)
{
    return stallCycles >= kMaxStallCycles || stalledMs >= kUnreachableGiveUpMs;
}

// How much a tick of chasing achieved. Distinguishing Motion from Approach is
// what keeps the give-up rule honest: a reposition step always moves the hero,
// and if bare movement counted as progress the stall budget would reset on
// every sidestep and an unreachable mob would be chased forever.
enum class ProgressKind
{
    None = 0,
    Motion = 1,   // something moved, but the hero is no closer than before
    Approach = 2, // the hero got closer than ever, or the target reacted/died
};

inline ProgressKind ClassifyProgress(int distance, int bestDistance, bool heroMoved,
    bool targetEngaged)
{
    if (targetEngaged)
        return ProgressKind::Approach;
    if (bestDistance < 0 || distance < bestDistance)
        return ProgressKind::Approach;
    if (heroMoved)
        return ProgressKind::Motion;
    return ProgressKind::None;
}
}
