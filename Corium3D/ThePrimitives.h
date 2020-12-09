//
// Created by omer on 26/01/02018.
//

#pragma once

#include "Corium3D.h"

using namespace Corium3D;

class TheCube : public Corium3DEngine::GameLmnt {
public:
	TheCube(Corium3DEngine& corium3D, Transform3D& initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods);
	bool receiveRay();
};

/*
class TheSphere : public Corium3DEngine::GameLmnt {
public:
	TheSphere(Corium3DEngine& corium3DEngine, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods);
	bool receiveRay();
};

class TheCapsule : public  Corium3DEngine::GameLmnt {
public:
	TheCapsule(Corium3DEngine& corium3D, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods);
	bool receiveRay();
};

*/