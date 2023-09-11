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
		return (rand() % (rangeMax - rangeMin)) + rangeMin;
	}

	float Randomizer::randF(float rangeMax)
	{
		return rangeMax * ((float)rand() / RAND_MAX);
	}

	float Randomizer::randF(float rangeMin, float rangeMax)
	{
		float r = (float)rand() / RAND_MAX;
		return (1 - r) * rangeMin + r * rangeMax;
	}
} // namespace Corium3DUtils
