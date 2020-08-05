#include "ServiceLocator.h"

using namespace Corium3DUtils;

namespace Corium3D {

	Logger ServiceLocator::logger = Logger();
	Timer ServiceLocator::timer = Timer();
	Randomizer ServiceLocator::randomizer = Randomizer(ServiceLocator::timer);

} // namespace Corium3D
