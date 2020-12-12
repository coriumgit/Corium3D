#include "ThePrimitives.h"

TheCube::TheCube(Corium3DEngine& corium3DEngine, Transform3D& initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 0, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheCube::receiveRay, this)) {
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheCube::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheSphere::TheSphere(Corium3DEngine& corium3DEngine, Transform3D& initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 3, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheSphere::receiveRay, this)) {
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheSphere::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheCapsule::TheCapsule(Corium3DEngine& corium3DEngine, Transform3D& initTransform, float initCollisionPrimitiveRot, glm::vec3& linVel, float angVelMag, glm::vec3& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 1, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheCapsule::receiveRay, this)) {
	//accessMobilityAPI()->setLinVel(linVel);
	//accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheCapsule::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}