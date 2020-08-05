//
// Created by omer on 27/02/02018.
//

#pragma once

#include "Logger.h"
#include "Timer.h"
#include "Randomizer.h"

namespace Corium3D {

	class ServiceLocator {
	public:
		static Corium3DUtils::Logger& getLogger() { return logger; }
		static Corium3DUtils::Timer& getTimer() { return timer; }
		static Corium3DUtils::Randomizer& getRandomizer() { return randomizer; }

	private:
		static Corium3DUtils::Logger logger;
		static Corium3DUtils::Timer timer;
		static Corium3DUtils::Randomizer randomizer;
	};

}
