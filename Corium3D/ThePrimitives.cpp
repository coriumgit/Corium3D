#include "ThePrimitives.h"

TheCube::TheCube(Corium3DEngine& corium3DEngine, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(InitData({ corium3DEngine, Component::Mobility | Component::Graphics, NULL, NULL, 0, initTransform, initCollisionPrimitiveRot, proximityHandlingMethods })) {
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

TheCube::TheCube(InitStruct const& initStruct) :
	TheCube::TheCube(initStruct.corium3D, initStruct.initTransform, initStruct.initCollisionPrimitiveRot, initStruct.linVel, initStruct.angVelMag, initStruct.angVelAx, initStruct.proximityHandlingMethods) {}

bool TheCube::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheSphere::TheSphere(Corium3DEngine& corium3DEngine, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(InitData({ corium3DEngine, Component::Mobility | Component::Graphics, NULL, NULL, 1, initTransform, initCollisionPrimitiveRot, proximityHandlingMethods })) {
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheSphere::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheSphere::TheSphere(InitStruct const& initStruct) :
	TheSphere::TheSphere(initStruct.corium3DEngine, initStruct.initTransform, initStruct.initCollisionPrimitiveRot, initStruct.linVel, initStruct.angVelMag, initStruct.angVelAx, initStruct.proximityHandlingMethods) {}

TheCapsule::TheCapsule(Corium3DEngine& corium3DEngine, Transform3D* initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(InitData({ corium3DEngine, Component::Mobility | Component::Graphics, NULL, NULL, 2, initTransform, initCollisionPrimitiveRot, proximityHandlingMethods })) {
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

TheCapsule::TheCapsule(InitStruct const& initStruct) :
	TheCapsule::TheCapsule(initStruct.corium3DEngine, initStruct.initTransform, initStruct.initCollisionPrimitiveRot, initStruct.linVel, initStruct.angVelMag, initStruct.angVelAx, initStruct.proximityHandlingMethods) {}

bool TheCapsule::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}