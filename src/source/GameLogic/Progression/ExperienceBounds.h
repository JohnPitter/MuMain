#pragma once

#include <cstdint>

// Cumulative experience bounds shared by every EXP progress surface
// (HUD gauge, character info window, Hunt Analyzer). The formulas replicate
// the server-side curve: level L needs (9+L)*L*L*10 cumulative EXP, with an
// extra (9+(L-255))*(L-255)*(L-255)*1000 term above level 255; master-level
// bounds follow the S6 master curve ((total+9)*total^3*10 + (over+9)*over^3
// *1000 - 3892250000) / 2 with total = masterLevel + 400.
namespace GameLogic::Progression
{
	// Cumulative EXP at the start of `level` (0 for level <= 1).
	std::uint64_t GetNormalLowerBound(unsigned short level);

	// Cumulative EXP bound of the master curve at the given master level.
	std::int64_t GetMasterLowerBound(short masterLevel);

	// Progress [0..1] inside the current normal level; div0-safe and clamped.
	double NormalExpBarRatio(unsigned short level, std::uint64_t experience,
		std::uint64_t nextExperience);

	// Progress [0..1] inside the current master level; div0-safe and clamped.
	double MasterExpBarRatio(short masterLevel, std::int64_t experience,
		std::int64_t nextExperience);
}
