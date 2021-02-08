#include "PhysicsEngine.h"

using namespace Corium3DUtils;

namespace Corium3D {

	PhysicsEngine::PhysicsEngine(unsigned int mobilityInterfacesNrMax, float _secsPerUpdate) :
		mobilityInterfacesPool(mobilityInterfacesNrMax), mobilityInterfacesIt(mobilityInterfacesPool), secsPerUpdate(_secsPerUpdate) {}

	PhysicsEngine::MobilityInterface* PhysicsEngine::addMobileGameLmnt(
		Transform3D const& initTransform,
		OnMovementMadeCallback3D* listeners3D, unsigned int listeners3DNr,
		std::complex<float> initTransform2DRot,
		OnMovementMadeCallback2D* listeners2D, unsigned int listeners2DNr) 
	{
		MobilityInterface* newMobilityInterface = mobilityInterfacesPool.acquire(initTransform, listeners3D, listeners3DNr, initTransform2DRot, listeners2D, listeners2DNr, secsPerUpdate);
		return newMobilityInterface;
	}

	PhysicsEngine::MobilityInterface* PhysicsEngine::addMobileGameLmnt(Transform3D const& initTransform, OnMovementMadeCallback3D* listeners3D, unsigned int listeners3DNr) 
	{
		MobilityInterface* newMobilityInterface = mobilityInterfacesPool.acquire(initTransform, listeners3D, listeners3DNr, secsPerUpdate);
		return newMobilityInterface;
	}

	void PhysicsEngine::removeMobileGameLmnt(MobilityInterface* removedMobilityInterface) {
		mobilityInterfacesPool.release(removedMobilityInterface);
	}

	void PhysicsEngine::update(float time) {
		for (mobilityInterfacesIt.reset(); mobilityInterfacesIt.hasNext(); mobilityInterfacesIt.next().update(time));
	}

	void PhysicsEngine::update() {
		for (mobilityInterfacesIt.reset(); mobilityInterfacesIt.hasNext(); mobilityInterfacesIt.next().update());
	}

	//REMINDER: depending on having at least one listener
	PhysicsEngine::MobilityInterface::MobilityInterface(Transform3D const& initTransform,
		OnMovementMadeCallback3D* _listeners3D, unsigned int _listeners3DNr,
		std::complex<float> initTransform2DRot,
		OnMovementMadeCallback2D* _listeners2D, unsigned int _listeners2DNr,
		float _secsPerUpdate) :
		transform(initTransform), transform2DRot(initTransform2DRot), listeners3DNr(_listeners3DNr), listeners2DNr(_listeners2DNr), secsPerUpdate(_secsPerUpdate) {
		listeners3D = new OnMovementMadeCallback3D[listeners3DNr];
		for (unsigned int listener3DIdx = 0; listener3DIdx < listeners3DNr; listener3DIdx++)
			listeners3D[listener3DIdx] = _listeners3D[listener3DIdx];
		if (listeners2DNr > 0) {
			listeners2D = new OnMovementMadeCallback2D[listeners2DNr];
			for (unsigned int listener2DIdx = 0; listener2DIdx < listeners2DNr; listener2DIdx++)
				listeners2D[listener2DIdx] = _listeners2D[listener2DIdx];
		}
		else
			listeners2D = NULL;
	}

	PhysicsEngine::MobilityInterface::MobilityInterface(Transform3D const& initTransform, OnMovementMadeCallback3D* listeners3D, unsigned int listeners3DNr, float secsPerUpdate) :
		MobilityInterface(initTransform, listeners3D, listeners3DNr, 0.0f, NULL, 0, secsPerUpdate) {}

	PhysicsEngine::MobilityInterface::MobilityInterface(MobilityInterface const& mobilityInterface) :
		transform(mobilityInterface.transform), listeners3DNr(mobilityInterface.listeners3DNr), transform2DRot(mobilityInterface.transform2DRot), listeners2DNr(mobilityInterface.listeners2DNr), secsPerUpdate(mobilityInterface.secsPerUpdate) {
		listeners3D = new OnMovementMadeCallback3D[listeners3DNr];
		for (unsigned int listener3DIdx = 0; listener3DIdx < listeners3DNr; listener3DIdx++)
			listeners3D[listener3DIdx] = mobilityInterface.listeners3D[listener3DIdx];
		if (listeners2DNr > 0) {
			listeners2D = new OnMovementMadeCallback2D[listeners2DNr];
			for (unsigned int listener2DIdx = 0; listener2DIdx < listeners2DNr; listener2DIdx++)
				listeners2D[listener2DIdx] = mobilityInterface.listeners2D[listener2DIdx];
		}
		else
			listeners2D = NULL;
	}

	//REMINDER: depending on having at least one listener
	PhysicsEngine::MobilityInterface::~MobilityInterface() {
		delete[] listeners3D;
		if (listeners2DNr > 0)
			delete[] listeners2D;
	}

	void PhysicsEngine::MobilityInterface::update(float time) {
		Transform3DUS transformDelta;
		transformDelta.translate = 0.5f * (linVel + time * linAccel) * time + 0.5f * linAccel * time * time;
		linVel += time * linAccel;
		transformDelta.rot = glm::angleAxis(angVelMag * time, angVelAx);
		translate(transformDelta.translate);
		rot(transformDelta.rot);
		std::complex<float> rot2DDelta = std::polar(1.0f, angVelMag2D * time);
		rot2D(rot2DDelta);
		for (unsigned int listener3DIdx = 0; listener3DIdx < listeners3DNr; listener3DIdx++)
			listeners3D[listener3DIdx](transformDelta);
		for (unsigned int listener2DIdx = 0; listener2DIdx < listeners2DNr; listener2DIdx++) {
			listeners2D[listener2DIdx](Transform2DUS({ transformDelta.translate, transformDelta.scale, rot2DDelta }));
		}
	}

	void PhysicsEngine::MobilityInterface::update() {
		linVel += linAccelPerUpdate;
		translate(transformDeltaPerUpdate.translate);
		rot(transformDeltaPerUpdate.rot);
		rot2D(transform2DRotDeltaPerUpdate);
		for (unsigned int listener3DIdx = 0; listener3DIdx < listeners3DNr; listener3DIdx++)
			listeners3D[listener3DIdx](transformDeltaPerUpdate);
		for (unsigned int listener2DIdx = 0; listener2DIdx < listeners2DNr; listener2DIdx++)
			listeners2D[listener2DIdx](Transform2DUS({ transformDeltaPerUpdate.translate, transformDeltaPerUpdate.scale, transform2DRotDeltaPerUpdate }));
	}

} // namespace Corium3D