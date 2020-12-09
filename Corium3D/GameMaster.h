#pragma once

#include "Corium3D.h"
#include "ThePrimitives.h"
#include "ObjPool.h"

#define TEST_LMNTS_NR 3
	
using namespace Corium3D;

class GameMaster {
public:
	GameMaster(Corium3DEngine& corium3Dengine);
	~GameMaster();

private:
	Corium3DEngine& corium3DEngine;

	Corium3DUtils::ObjPool<TheCube> cubesPool;
	//Corium3DUtils::ObjPoolIteratable<TheSphere> spheresPool;
	//Corium3DUtils::ObjPoolIteratable<TheCapsule> capsulesPool;
	TheCube* cubes[TEST_LMNTS_NR];
	//TheSphere* spheres[TEST_LMNTS_NR];
	//TheCapsule* capsules[TEST_LMNTS_NR];
	TheCube* player;
	Corium3DEngine::GameLmnt::MobilityAPI* playerMobilityAPI;
	TheCube* other;

	enum CameraTransformMode { PANNING, ROTATION, NONE };
	CameraTransformMode cameraTransformMode = CameraTransformMode::NONE;

	glm::vec2 prevInputCursorPos;

	void primitiveColoringCollisionCallback(Corium3DEngine::GameLmnt* primitive1, Corium3DEngine::GameLmnt* primitive2);
	void primitiveColoringDetachmentCallback(Corium3DEngine::GameLmnt* primitive1, Corium3DEngine::GameLmnt* primitive2);
	void removingCollisionCallbackCubeCube(Corium3DEngine::GameLmnt* cube1, Corium3DEngine::GameLmnt* cube2);

	void playerCollisionCallback(Corium3DEngine::GameLmnt* cube1, Corium3DEngine::GameLmnt* cube2);
	void playerDetachmentCallback(Corium3DEngine::GameLmnt* cube, Corium3DEngine::GameLmnt* sphere);
	void movePlayerLeft(double unused);
	void movePlayerRight(double unused);
	void movePlayerUp(double unused);
	void movePlayerDown(double unused);
	void movePlayerFar(double unused);
	void movePlayerClose(double unused);
	void stopPlayerLeft(double unused);
	void stopPlayerRight(double unused);
	void stopPlayerUp(double unused);
	void stopPlayerDown(double unused);
	void stopPlayerFar(double unused);
	void stopPlayerClose(double unused);
	void activatePanning(double timeStamp, glm::vec2 const& cursorPos);
	void deactivatePanning(double timeStamp, glm::vec2 const& cursorPos);
	void activateRotation(double timeStamp, glm::vec2 const& cursorPos);
	void deactivateRotation(double timeStamp, glm::vec2 const& cursorPos);
	void walkIn(double timeStamp, glm::vec2 const& cursorPos);
	void walkOut(double timeStamp, glm::vec2 const& cursorPos);
	void transformCamera(double timeStamp, glm::vec2 const& cursorPos);
	void shootRay(double timeStamp, glm::vec2 const& cursorPos);
	void doNothing(double unused);
};
