//
// Created by omer on 26/01/02018.
//

#pragma once

#include "Corium3D.h"

using namespace Corium3D;

class TheCube : public Corium3DEngine::GameLmnt {
public:
	struct InitStruct {
		Corium3DEngine& corium3D;
		Transform3D* initTransform;
		float initCollisionPrimitiveRot;
		glm::vec3& linVel;
		float angVelMag;
		glm::vec3& angVelAx;
		ProximityHandlingMethods* proximityHandlingMethods;
	};
	TheCube(Corium3DEngine& corium3D, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods);
	TheCube(InitStruct const& initStruct);
	bool receiveRay() override;
};

class TheSphere : public Corium3DEngine::GameLmnt {
public:
	struct InitStruct {
		Corium3DEngine& corium3DEngine;
		Corium3D::Transform3D* initTransform;
		float initCollisionPrimitiveRot;
		glm::vec3& linVel;
		float angVelMag;
		glm::vec3& angVelAx;
		ProximityHandlingMethods* proximityHandlingMethods;
	};
	TheSphere(Corium3DEngine& corium3DEngine, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods);
	TheSphere(InitStruct const& initStruct);
	bool receiveRay() override;
};

class TheCapsule : public  Corium3DEngine::GameLmnt {
public:
	struct InitStruct {
		Corium3DEngine& corium3DEngine;
		Corium3D::Transform3D* initTransform;
		float initCollisionPrimitiveRot;
		glm::vec3& linVel;
		float angVelMag;
		glm::vec3& angVelAx;
		ProximityHandlingMethods* proximityHandlingMethods;
	};
	TheCapsule(Corium3DEngine& corium3D, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods);
	TheCapsule(InitStruct const& initStruct);
	bool receiveRay() override;
};

