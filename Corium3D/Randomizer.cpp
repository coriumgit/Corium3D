#include "Randomizer.h"
#include <cstdlib>

namespace Corium3DUtils {

	Randomizer::Randomizer(Timer timer) {
		srand(timer.getCurrentTime());
	}

	int Randomizer::randI(unsigned int rangeMax) {
		return rand() % rangeMax;
	}

	int Randomizer::randI(unsigned int rangeMin, unsigned int rangeMax) {
		return (rand() % rangeMax) + rangeMin;
	}

} // namespace Corium3DUtils
