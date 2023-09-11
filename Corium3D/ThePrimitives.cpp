#include "ThePrimitives.h"

TheCube::TheCube(Corium3DEngine& corium3DEngine, Transform3D const& initTransform, float initCollisionPrimitiveRot, glm::vec3 const& linVel, float angVelMag, glm::vec3 const& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 0, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheCube::receiveRay, this))
{
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheCube::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheSphere::TheSphere(Corium3DEngine& corium3DEngine, Transform3D const& initTransform, float initCollisionPrimitiveRot, glm::vec3 const& linVel, float angVelMag, glm::vec3 const& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 1, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheSphere::receiveRay, this)) 
{
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheSphere::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheCapsule::TheCapsule(Corium3DEngine& corium3DEngine, Transform3D const& initTransform, float initCollisionPrimitiveRot, glm::vec3 const& linVel, float angVelMag, glm::vec3 const& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 2, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheCapsule::receiveRay, this))
{
	accessMobilityAPI()->setLinVel(linVel);
	accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheCapsule::receiveRay() {
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheCone::TheCone(Corium3DEngine& corium3DEngine, Transform3D const& initTransform, float initCollisionPrimitiveRot, glm::vec3 const& linVel, float angVelMag, glm::vec3 const& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 3, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheCone::receiveRay, this)) 
{
	//accessMobilityAPI()->setLinVel(linVel);
	//accessMobilityAPI()->setAngVel(angVelMag, angVelAx);
}

bool TheCone::receiveRay()
{
	accessGraphicsAPI()->changeVerticesColors(0, 1);
	return true;
}

TheFrog::TheFrog(Corium3DEngine& corium3DEngine, Transform3D const& initTransform, float initCollisionPrimitiveRot, glm::vec3 const& linVel, float angVelMag, glm::vec3 const& angVelAx, ProximityHandlingMethods* proximityHandlingMethods) :
	GameLmnt(corium3DEngine, Component::Graphics, NULL, NULL, 0, &initTransform, initCollisionPrimitiveRot, proximityHandlingMethods, std::bind(&TheFrog::receiveRay, this))
{
}

bool TheFrog::receiveRay()
{
	return false;
}
