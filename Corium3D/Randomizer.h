#pragma once

#include "Timer.h"

namespace Corium3DUtils {

	class Randomizer {
	public:
		Randomizer(Timer timer);
		int randI(unsigned int rangeMax);
		int randI(unsigned int rangeMin, unsigned int rangeMax);
	};

} // namespace Corium3DUtils
