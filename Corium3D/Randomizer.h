#pragma once

#include "Timer.h"

namespace Corium3DUtils {

	class Randomizer {
	public:
		Randomizer(Timer timer);
		int randI(unsigned int rangeMax);
		int randI(unsigned int rangeMin, unsigned int rangeMax);
		float randF(float rangeMax);
		float randF(float rangeMin, float rangeMax);
	};

} // namespace Corium3DUtils
