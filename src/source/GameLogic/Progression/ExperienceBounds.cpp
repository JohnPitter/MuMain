#include "stdafx.h"
#include "GameLogic/Progression/ExperienceBounds.h"

#include <algorithm>

namespace GameLogic::Progression
{
	std::uint64_t GetNormalLowerBound(const unsigned short level)
	{
		if (level <= 1)
			return 0;

		const std::uint64_t priorLevel = static_cast<std::uint64_t>(level - 1);
		std::uint64_t priorExperience =
			(9ull + priorLevel) * priorLevel * priorLevel * 10ull;

		if (priorLevel > 255ull)
		{
			const std::uint64_t levelOverN = priorLevel - 255ull;
			priorExperience +=
				(9ull + levelOverN) * levelOverN * levelOverN * 1000ull;
		}

		return priorExperience;
	}

	std::int64_t GetMasterLowerBound(const short masterLevel)
	{
		const std::int64_t safeMasterLevel = std::max<std::int64_t>(masterLevel, 0);
		const std::int64_t totalLevel = safeMasterLevel + 400;
		const std::int64_t overLevel = totalLevel - 255;
		const std::int64_t dataMaster =
			(9 + totalLevel) * totalLevel * totalLevel * 10
			+ (9 + overLevel) * overLevel * overLevel * 1000;
		return (dataMaster - 3892250000ll) / 2;
	}

	double NormalExpBarRatio(const unsigned short level,
		const std::uint64_t experience, const std::uint64_t nextExperience)
	{
		const std::uint64_t lowerBound = GetNormalLowerBound(level);
		const std::uint64_t upperBound = std::max(nextExperience, lowerBound);
		if (upperBound == lowerBound)
			return 0.0;

		const std::uint64_t clampedExperience =
			std::clamp(experience, lowerBound, upperBound);
		return static_cast<double>(clampedExperience - lowerBound)
			/ static_cast<double>(upperBound - lowerBound);
	}

	double MasterExpBarRatio(const short masterLevel,
		const std::int64_t experience, const std::int64_t nextExperience)
	{
		const std::int64_t lowerBound = GetMasterLowerBound(masterLevel);
		const std::int64_t upperBound = std::max(nextExperience, lowerBound);
		if (upperBound == lowerBound)
			return 0.0;

		const std::int64_t clampedExperience =
			std::clamp(experience, lowerBound, upperBound);
		return static_cast<double>(clampedExperience - lowerBound)
			/ static_cast<double>(upperBound - lowerBound);
	}
}
