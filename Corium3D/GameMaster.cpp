#include "GameMaster.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // for angleAxis(valType const &angle, detail::tvec3< valType > const &v)
#include <vector>

#define __DEBUG__ 0 

using namespace Corium3D;

#if __DEBUG__ == 0
/*
Transform3D transforms[TEST_LMNTS_NR] = { { glm::vec3(0.0f, 0.0f, -7.0),   glm::vec3(1.0f, 1.0f, 1.0f), glm::quat(cos(0 / 4) , sin(0 / 4), sin(0 / 4), sin(0 / 8)) }
											  //{ glm::vec3(-4.0f, 2.5f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
										  	  //{ glm::vec3(-3.0f, 4.0f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) }
											  //{ glm::vec3(-2.0f, 0.5f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) }
											  //{ glm::vec3(1.0f, 0.5f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(2.0f, -1.0f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) }
											  //{ glm::vec3(3.0f, -3.0f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(-2.0f, -2.5f, -7.0),  glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(5.0f, -3.5f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(6.0f, -1.0f, -7.0),   glm::vec3(0.4f, 0.4f, 0.4f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(-10.0f, -10.0f, -7.0),  glm::vec3(0.1f, 0.1f, 0.1f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(0.0f, -5.0f, -7.0),   glm::vec3(0.2f, 0.2f, 0.2f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(-3.0f, -4.0f, -7.0),   glm::vec3(0.3f, 0.3f, 0.3f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(6.0f, 6.0f, -7.0),   glm::vec3(0.8f, 0.8f, 0.8f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(6.0f, 2.0f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
											  //{ glm::vec3(5.0f, 3.0f, -7.0),   glm::vec3(0.6f, 0.6f, 0.6f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) }
};
*/
glm::vec3  linVels[TEST_LMNTS_NR_MAX] = { glm::vec3(0.0f, 0.0f, 0.0f)
									//glm::vec3(0.0f, 1.0f, 0.0f),
									//glm::vec3(0.0f, 0.0f, 0.0f) 
									//glm::vec3(0.0f, 0.0f, 0.0f)
									//glm::vec3(0.0f, 0.0f, 0.0f), 
									//glm::vec3(0.0f, 0.1f, 0.0f)
									//glm::vec3(0.0f, 0.4f, 0.0f),
									//glm::vec3(0.0f, 1.0f, 0.0f), //6
									//glm::vec3(-1.0f, 1.0f, 0.0f), //7
									//glm::vec3(-0.5f, 0.8f, 0.0f), 
									//glm::vec3(0.5f, 2.0f, 0.0f),
									//glm::vec3(0.0f, 1.5f, 0.0f),
									//glm::vec3(-0.5f, 0.8f, 0.0f),
									//glm::vec3(-1.5f, -0.2f, 0.0f),
									//glm::vec3(-1.0f, -0.1f, 0.0f),
									//glm::vec3(-1.0f, 0.1f, 0.0f)
};

float angVelMag[TEST_LMNTS_NR_MAX] = {  0.0f
								    //0.0f,
								    //0.0f
								    //0.0f,
								    //0.0f, 
							  	    //0.0f, 
								    //0.0f, 
								    //0.0f, 
								    //0.0f, 
								    //0.0f 
};

// REMINDER: angular velocity axis must not equal the zero vector
glm::vec3 angVelAx[TEST_LMNTS_NR_MAX] = { glm::vec3(1.0f, 0.0f, 1.0f)
									//glm::vec3(1.0f, 0.0f, 0.0f),
									//glm::vec3(1.0f, 0.0f, 0.0f) 
									//glm::vec3(0.0f, -1.5f, 0.0f)
									//glm::vec3(0.0f, 0.0f, 0.0f), 
									//glm::vec3(0.0f, 0.1f, 0.0f)
									//glm::vec3(0.0f, 0.4f, 0.0f),
									//glm::vec3(0.0f, 1.0f, 0.0f), //6
									//glm::vec3(-1.0f, 1.0f, 0.0f), //7
									//glm::vec3(-0.5f, 0.8f, 0.0f), 
									//glm::vec3(0.5f, 2.0f, 0.0f),
									//glm::vec3(0.0f, 1.5f, 0.0f),
									//glm::vec3(-0.5f, 0.8f, 0.0f),
									//glm::vec3(-1.5f, -0.2f, 0.0f),
									//glm::vec3(-1.0f, -0.1f, 0.0f),
									//glm::vec3(-1.0f, 0.1f, 0.0f)
									};

#define TEST_PRIMITIVES 0

#elif __DEBUG__ == 1

Corium::Transform const transforms[TEST_LMNTS_NR] = { { glm::vec3(1.0f, 3.5f, -7.0),   glm::vec3(0.5f, 0.5f, 0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
													  { glm::vec3(3.0f, -1.0f, -7.0),   glm::vec3(0.5f, 0.5f, 0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
													  { glm::vec3(2.0f, -4.0f, -7.0),   glm::vec3(0.5f, 0.5f, 0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
													  { glm::vec3(-5.0f, 1.0f, -7.0),   glm::vec3(0.5f, 0.5f, 0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) },
													  { glm::vec3(-3.0f, 3.5f, -7.0),   glm::vec3(0.5f, 0.5f, 0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) }, 
													  { glm::vec3(-4.0f, -3.0f, -7.0),   glm::vec3(0.5f, 0.5f, 0.5f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f) } };

glm::vec3  linVels[TEST_LMNTS_NR] = { glm::vec3(0.0f, 0.0f, 0.0f),
									  glm::vec3(0.0f, 0.0f, 0.0f), 
									  glm::vec3(0.0f, 0.0f, 0.0f), 
									  glm::vec3(1.0f, 0.0f, 0.0f),
									  glm::vec3(0.0f, 0.0f, 0.0f),
									  glm::vec3(0.0f, 0.0f, 0.0f) }; 
#endif

const float fl = 26.0f;
const glm::vec3 BOX_MIN(-fl, -fl, -fl);
const glm::vec3 BOX_MAX(fl, fl, fl);
const glm::vec3 VEL_MIN(-0.57f, -0.57f, -0.57f);
const glm::vec3 VEL_MAX( 0.57f,  0.57f,  0.57f);

inline glm::vec3 randVec(glm::vec3 const& minVec, glm::vec3 const& maxVec)
{
	Corium3DUtils::Randomizer& randomizer = Corium3D::ServiceLocator().getRandomizer();
	
	return glm::vec3
	(
		randomizer.randF(minVec.x, maxVec.x),
		randomizer.randF(minVec.y, maxVec.y),
		randomizer.randF(minVec.z, maxVec.z)
	);
}

GameMaster::GameMaster(Corium3DEngine& _corium3Dengine) : corium3DEngine(_corium3Dengine), cubesPool(TEST_LMNTS_NR_MAX + 1), spheresPool(TEST_LMNTS_NR_MAX + 1), capsulesPool(TEST_LMNTS_NR_MAX + 1), conesPool(TEST_LMNTS_NR_MAX + 1)
{								
	std::vector<std::vector<Transform3D>> transformsInit = corium3DEngine.loadScene(0);

	// THE PRIMITIVES INSTANTIATION
	Corium3DEngine::GameLmnt::ProximityHandlingMethods coloringCallbacks = {
		std::bind(&GameMaster::primitiveColoringCollisionCallback, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&GameMaster::primitiveColoringDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2)
	};
	Corium3DEngine::GameLmnt::ProximityHandlingMethods coloringCallbacksBuffer[] =
		{ coloringCallbacks, coloringCallbacks, coloringCallbacks, coloringCallbacks };

	ServiceLocator::getLogger().logd("GameMaster", "creating the primitives.");
	

	//Scene C
	//TheFrog* frogMan = new TheFrog(corium3DEngine, transformsInit[0][0], 0, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f), coloringCallbacksBuffer);

	//Scene B
	cubes[0] = cubesPool.acquire(corium3DEngine, transformsInit[0][0], 0, glm::vec3(0.0), 0.0f, glm::vec3(1.0), coloringCallbacksBuffer);
	spheres[0] = spheresPool.acquire(corium3DEngine, transformsInit[1][0], 0, glm::vec3(0.0), 0.0f, glm::vec3(1.0), coloringCallbacksBuffer);
	capsules[0] = capsulesPool.acquire(corium3DEngine, transformsInit[2][0], 0, glm::vec3(0.0), 0.0f, glm::vec3(1.0), coloringCallbacksBuffer);

	//Scene A
	/*	
	for (int lmntIdx = 0; lmntIdx < TEST_LMNTS_NR_MAX; lmntIdx++)
	{
		double xy = 2.5 * (lmntIdx % (TEST_LMNTS_NR_MAX / 2) - TEST_LMNTS_NR_MAX / 4);
		double z = lmntIdx < TEST_LMNTS_NR_MAX / 2 ? 0.0 : -4.0;
		Transform3D transform = { glm::vec3(xy, xy, z), glm::vec3(1.0f, 1.0f, 1.0f), glm::quat(cos(0 / 4), sin(0 / 4), sin(0 / 4), sin(0 / 8)) };		
		cubes[lmntIdx] = cubesPool.acquire(corium3DEngine, transform, 0, glm::vec3(0.0), 0.0f, glm::vec3(1.0), coloringCallbacksBuffer);
	}
	*/
	
	/*
	for (unsigned int lmntIdx = 0; lmntIdx < TEST_LMNTS_NR_MAX; lmntIdx++)
	{
		Transform3D transform = { randVec(BOX_MIN, BOX_MAX), glm::vec3(1.0f, 1.0f, 1.0f), glm::quat(cos(0 / 4), sin(0 / 4), sin(0 / 4), sin(0 / 8)) };
		cubes[lmntIdx] = cubesPool.acquire(corium3DEngine, transform, 0, randVec(VEL_MIN, VEL_MAX), 0.0f, glm::vec3(1.0), coloringCallbacksBuffer);
	}

	for (unsigned int lmntIdx = 0; lmntIdx < TEST_LMNTS_NR_MAX; lmntIdx++)
	{
		Transform3D transform = { randVec(BOX_MIN, BOX_MAX), glm::vec3(1.0f, 1.0f, 1.0f), glm::quat(cos(0 / 4), sin(0 / 4), sin(0 / 4), sin(0 / 8)) };
		spheres[lmntIdx] = spheresPool.acquire(corium3DEngine, transform, 0, randVec(VEL_MIN, VEL_MAX), 0.0f, glm::vec3(1.0), coloringCallbacksBuffer);
	}

	for (unsigned int lmntIdx = 0; lmntIdx < TEST_LMNTS_NR_MAX; lmntIdx++)
	{
		Transform3D transform = { randVec(BOX_MIN, BOX_MAX), glm::vec3(1.0f, 1.0f, 1.0f), glm::quat(cos(0 / 4), sin(0 / 4), sin(0 / 4), sin(0 / 8)) };
		capsules[lmntIdx] = capsulesPool.acquire(corium3DEngine, transform, 0, randVec(VEL_MIN, VEL_MAX), 0.0f, glm::vec3(1.0), coloringCallbacksBuffer);
	}			
	*/

	// PLAYER INSTANTIATION
	Transform3D playerTransform({ glm::vec3(0.0f, 0.0f, 4.0), glm::vec3(1.0f, 1.0f, 1.0f), glm::quat(cos(0 / 6), sin(0 / 4), sin(0 / 4), sin(0 / 6)) });		
	Corium3DEngine::GameLmnt::ProximityHandlingMethods playerProximityCallbacks {
		std::bind(&GameMaster::playerCollisionCallback, this, std::placeholders::_1, std::placeholders::_2),
		std::bind(&GameMaster::playerDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2)
	};
	Corium3DEngine::GameLmnt::ProximityHandlingMethods playerProximityHandlingMethods[] =
		{ playerProximityCallbacks, playerProximityCallbacks, playerProximityCallbacks, playerProximityCallbacks };
	
	player = new TheCapsule(corium3DEngine, playerTransform, 0, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f), playerProximityHandlingMethods);
	playerMobilityAPI = player->accessMobilityAPI();
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::LEFT_ARROW, std::bind(&GameMaster::movePlayerLeft, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::RIGHT_ARROW, std::bind(&GameMaster::movePlayerRight, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::UP_ARROW, std::bind(&GameMaster::movePlayerUp, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::DOWN_ARROW, std::bind(&GameMaster::movePlayerDown, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::Q, std::bind(&GameMaster::movePlayerFar, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::A, std::bind(&GameMaster::movePlayerClose, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::G, std::bind(&GameMaster::Gooo, this));
	corium3DEngine.registerKeyboardInputEndCallback(KeyboardInputID::LEFT_ARROW, std::bind(&GameMaster::stopPlayerLeft, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputEndCallback(KeyboardInputID::RIGHT_ARROW, std::bind(&GameMaster::stopPlayerRight, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputEndCallback(KeyboardInputID::UP_ARROW, std::bind(&GameMaster::stopPlayerUp, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputEndCallback(KeyboardInputID::DOWN_ARROW, std::bind(&GameMaster::stopPlayerDown, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputEndCallback(KeyboardInputID::Q, std::bind(&GameMaster::stopPlayerFar, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputEndCallback(KeyboardInputID::A, std::bind(&GameMaster::stopPlayerClose, this, std::placeholders::_1));
	
	corium3DEngine.registerCursorInputCallback(CursorInputID::LEFT_DOWN, std::bind(&GameMaster::activatePanning, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.registerCursorInputCallback(CursorInputID::LEFT_UP, std::bind(&GameMaster::deactivatePanning, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.registerCursorInputCallback(CursorInputID::RIGHT_DOWN, std::bind(&GameMaster::activateRotation, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.registerCursorInputCallback(CursorInputID::RIGHT_UP, std::bind(&GameMaster::deactivateRotation, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.registerCursorInputCallback(CursorInputID::WHEEL_ROTATED_BACKWARD, std::bind(&GameMaster::walkOut, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.registerCursorInputCallback(CursorInputID::WHEEL_ROTATED_FORWARD, std::bind(&GameMaster::walkIn, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.registerCursorInputCallback(CursorInputID::MOVE, std::bind(&GameMaster::transformCamera, this, std::placeholders::_1, std::placeholders::_2));
	//corium3DEngine.registerCursorInputCallback(CursorInputID::MIDDLE_DOWN, std::bind(&GameMaster::shootRay, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.accessGuiAPI(0).show();
	
	corium3DEngine.accessCameraAPI().translate(glm::vec3(-36, 36, 36.0f));
	corium3DEngine.accessCameraAPI().rotate(-45, glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)));
}

GameMaster::~GameMaster() {
	/*
	delete player;
	for (unsigned int testLmntIdx = 0; testLmntIdx < cubesPool.getAcquiredObjsNr(); testLmntIdx++) {
		delete cubes[testLmntIdx];
		delete spheres[testLmntIdx];
		delete capsules[testLmntIdx];
	}
	*/
}

void GameMaster::primitiveColoringCollisionCallback(Corium3DEngine::GameLmnt* primitive1, Corium3DEngine::GameLmnt* primitive2) {
	//primitive1->accessGraphicsAPI()->changeVerticesColors(0, 1);
}

void GameMaster::primitiveColoringDetachmentCallback(Corium3DEngine::GameLmnt* primitive1, Corium3DEngine::GameLmnt* primitive2) {
	//primitive1->accessGraphicsAPI()->changeVerticesColors(0, 0);
}

void GameMaster::removingCollisionCallbackCubeCube(Corium3DEngine::GameLmnt* cube1, Corium3DEngine::GameLmnt* cube2) {
	cubesPool.release((TheCube*)cube1);
}

void GameMaster::playerCollisionCallback(Corium3DEngine::GameLmnt* cube, Corium3DEngine::GameLmnt* sphere) {
	//objPool.release((TheCube*)cube2);
	//cube->accessGraphicsAPI()->changeVerticesColors(0, 1);
}

void GameMaster::playerDetachmentCallback(Corium3DEngine::GameLmnt* cube, Corium3DEngine::GameLmnt* sphere) {
	//objPool.release((TheCube*)cube2);
	//cube->accessGraphicsAPI()->changeVerticesColors(0, 0);
}

void GameMaster::movePlayerLeft(double unused) {
	playerMobilityAPI->setLinVelX(-1.5f);
}

void GameMaster::movePlayerRight(double unused) {
	playerMobilityAPI->setLinVelX(1.5f);
}

void GameMaster::movePlayerUp(double unused) {
	playerMobilityAPI->setLinVelY(1.5f);
}

void GameMaster::movePlayerDown(double unused) {
	playerMobilityAPI->setLinVelY(-1.5f);
}

void GameMaster::movePlayerFar(double unused) {
	playerMobilityAPI->setLinVelZ(-1.5);
}

void GameMaster::movePlayerClose(double unused) {
	playerMobilityAPI->setLinVelZ(1.5);
}

void GameMaster::stopPlayerLeft(double unused) {
	playerMobilityAPI->setLinVelX(0.0);
}

void GameMaster::stopPlayerRight(double unused) {
	playerMobilityAPI->setLinVelX(0.0);
}

void GameMaster::stopPlayerUp(double unused) {
	playerMobilityAPI->setLinVelY(0.0);
}

void GameMaster::stopPlayerDown(double unused) {
	playerMobilityAPI->setLinVelY(0.0);
}

void GameMaster::stopPlayerFar(double unused) {
	playerMobilityAPI->setLinVelZ(0.0f);
}

void GameMaster::stopPlayerClose(double unused) {
	playerMobilityAPI->setLinVelZ(0.0f);
}

void GameMaster::activatePanning(double timeStamp, glm::vec2 const& cursorPos) {	
	cameraTransformMode = CameraTransformMode::PANNING;
	prevInputCursorPos = cursorPos;
}

void GameMaster::deactivatePanning(double timeStamp, glm::vec2 const& cursorPos) {
	if (cameraTransformMode == CameraTransformMode::PANNING)
		cameraTransformMode = CameraTransformMode::NONE;
}

void GameMaster::activateRotation(double timeStamp, glm::vec2 const& cursorPos) {
	cameraTransformMode = CameraTransformMode::ROTATION;
	prevInputCursorPos = cursorPos;
}

void GameMaster::deactivateRotation(double timeStamp, glm::vec2 const& cursorPos) {
	if (cameraTransformMode == CameraTransformMode::ROTATION)
		cameraTransformMode = CameraTransformMode::NONE;	
}

void GameMaster::Gooo() {
	for (int cubeIdx = 0; cubeIdx < TEST_LMNTS_NR_MAX; ++cubeIdx)
	{		
		cubes[cubeIdx]->accessMobilityAPI()->setLinVel(-cubes[cubeIdx]->accessMobilityAPI()->getLinVel());
		spheres[cubeIdx]->accessMobilityAPI()->setLinVel(-spheres[cubeIdx]->accessMobilityAPI()->getLinVel());
		capsules[cubeIdx]->accessMobilityAPI()->setLinVel(-capsules[cubeIdx]->accessMobilityAPI()->getLinVel());
	}
}

constexpr float ZOOM_FACTOR = 1.1;

void GameMaster::walkIn(double timeStamp, glm::vec2 const& cursorPos) {
	corium3DEngine.accessCameraAPI().translateInViewDirection(ZOOM_FACTOR);
}

void GameMaster::walkOut(double timeStamp, glm::vec2 const& cursorPos) {
	corium3DEngine.accessCameraAPI().translateInViewDirection(-ZOOM_FACTOR);
}

constexpr float CAMERA_TRANSLATION_PER_DOUBLE_CURSOR_MOVE = 0.5f / 20.0f;
constexpr float CAMERA_ROT_PER_CURSOR_MOVE = 0.05f * M_PI / 180;

void GameMaster::transformCamera(double timeStamp, glm::vec2 const& cursorPos) {
	glm::vec2 cursorMoveVec = cursorPos - prevInputCursorPos;
	//glm::vec2 cursorMoveVec = { 0.0f, cursorPos.y - prevInputCursorPos.y };
	switch (cameraTransformMode) {
		case CameraTransformMode::PANNING:
			corium3DEngine.accessCameraAPI().pan(cursorMoveVec * CAMERA_TRANSLATION_PER_DOUBLE_CURSOR_MOVE);
			prevInputCursorPos = cursorPos;
			break;

		case CameraTransformMode::ROTATION:			
			corium3DEngine.accessCameraAPI().rotAroundViewportContainedAx(glm::length(cursorMoveVec) * CAMERA_ROT_PER_CURSOR_MOVE, glm::vec2(cursorMoveVec.y, -cursorMoveVec.x));
			prevInputCursorPos = cursorPos;
			break;		

		case CameraTransformMode::NONE: {}										
	}	
}

void GameMaster::shootRay(double timeStamp, glm::vec2 const& cursorPos) {
	for (unsigned int testLmntIdx = 0; testLmntIdx < cubesPool.getAcquiredObjsNr(); testLmntIdx++)
#if TEST_PRIMITIVES == 0
		cubes[testLmntIdx]->accessGraphicsAPI()->changeVerticesColors(0, 0);
#elif TEST_PRIMITIVES == 1
		spheres[testLmntIdx]->accessGraphicsAPI()->changeVerticesColors(0, 0);
#elif TEST_PRIMITIVES == 2
		capsules[testLmntIdx]->accessGraphicsAPI()->changeVerticesColors(0, 0);
#endif

//	player->accessGraphicsAPI()->changeVerticesColors(0, 0);
//	corium3DEngine.accessCameraAPI().shootRay(cursorPos);
}

void GameMaster::doNothing(double unused) {}