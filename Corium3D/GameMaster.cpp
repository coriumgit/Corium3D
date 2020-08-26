#include "GameMaster.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // for angleAxis(valType const &angle, detail::tvec3< valType > const &v)

#define DEBUG 0 

using namespace Corium3D;

#if DEBUG == 0
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

glm::vec3  linVels[TEST_LMNTS_NR] = { glm::vec3(0.0f, 0.0f, 0.0f)
									//glm::vec3(0.0f, 0.0f, 0.0f),
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

float angVelMag[TEST_LMNTS_NR] = {  0.0f
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
glm::vec3 angVelAx[TEST_LMNTS_NR] = { glm::vec3(1.0f, 0.0f, 1.0f)
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

#elif DEBUG == 1

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

GameMaster::GameMaster(Corium3DEngine& _corium3Dengine) : corium3DEngine(_corium3Dengine), cubesPool(TEST_LMNTS_NR), spheresPool(TEST_LMNTS_NR), capsulesPool(TEST_LMNTS_NR) {
	Corium3DEngine::GameLmnt::ProximityHandlingMethods coloringCallbacksBuffer[3];
	Corium3DEngine::GameLmnt::ProximityHandlingMethods removingCallabackBuffer[3];
		
	coloringCallbacksBuffer[0].collisionCallback = std::bind(&GameMaster::primitiveColoringCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	coloringCallbacksBuffer[0].detachmentCallback = std::bind(&GameMaster::primitiveColoringDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2);
	coloringCallbacksBuffer[1].collisionCallback = std::bind(&GameMaster::primitiveColoringCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	coloringCallbacksBuffer[1].detachmentCallback = std::bind(&GameMaster::primitiveColoringDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2);
	coloringCallbacksBuffer[2].collisionCallback = std::bind(&GameMaster::primitiveColoringCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	coloringCallbacksBuffer[2].detachmentCallback = std::bind(&GameMaster::primitiveColoringDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2);
	removingCallabackBuffer[0].collisionCallback = std::bind(&GameMaster::removingCollisionCallbackCubeCube, this, std::placeholders::_1, std::placeholders::_2);	
	removingCallabackBuffer[1].collisionCallback = std::bind(&GameMaster::removingCollisionCallbackCubeCube, this, std::placeholders::_1, std::placeholders::_2);		
	
	//for (unsigned int cubeIdx = 0; cubeIdx < TEST_LMNTS_NR / 2; cubeIdx ++)
	//	cubes[cubeIdx] = cubesPool.acquire<TheCube::InitStruct>(TheCube::InitStruct({ corium3D, transforms + cubeIdx, linVels[cubeIdx], angVelMag[cubeIdx], angVelAx[cubeIdx], coloringCallbacksBuffer }));
	//for (unsigned int cubeIdx = TEST_LMNTS_NR / 2; cubeIdx < TEST_LMNTS_NR; cubeIdx++)
	//	cubes[cubeIdx] = cubesPool.acquire<TheCube::InitStruct>(TheCube::InitStruct({ corium3D, transforms + cubeIdx, linVels[cubeIdx], angVelMag[cubeIdx], angVelAx[cubeIdx], coloringCallbacksBuffer }));
#if TEST_PRIMITIVES == 0
	for (unsigned int lmntIdx = 0; lmntIdx < TEST_LMNTS_NR / 2; lmntIdx++)
		cubes[lmntIdx] = cubesPool.acquire<TheCube::InitStruct>(TheCube::InitStruct({ corium3DEngine, transforms + lmntIdx, 2 * asinf(transforms[lmntIdx].rot.z), linVels[lmntIdx], angVelMag[lmntIdx], angVelAx[lmntIdx], coloringCallbacksBuffer }));
	for (unsigned int lmntIdx = TEST_LMNTS_NR / 2; lmntIdx < TEST_LMNTS_NR; lmntIdx++)
		cubes[lmntIdx] = cubesPool.acquire<TheCube::InitStruct>(TheCube::InitStruct({ corium3DEngine, transforms + lmntIdx, 2 * asinf(transforms[lmntIdx].rot.z), linVels[lmntIdx], angVelMag[lmntIdx], angVelAx[lmntIdx], coloringCallbacksBuffer }));
#elif TEST_PRIMITIVES == 1
	for (unsigned int lmntIdx = 0; lmntIdx < TEST_LMNTS_NR / 2; lmntIdx++)
		spheres[lmntIdx] = spheresPool.acquire<TheSphere::InitStruct>(TheSphere::InitStruct({ corium3D, transforms + lmntIdx, 2 * asinf(transforms[lmntIdx].rot.z), linVels[lmntIdx], angVelMag[lmntIdx], angVelAx[lmntIdx], coloringCallbacksBuffer }));
	for (unsigned int lmntIdx = TEST_LMNTS_NR / 2; lmntIdx < TEST_LMNTS_NR; lmntIdx++)
		spheres[lmntIdx] = spheresPool.acquire<TheSphere::InitStruct>(TheSphere::InitStruct({ corium3D, transforms + lmntIdx, 2 * asinf(transforms[lmntIdx].rot.z), linVels[lmntIdx], angVelMag[lmntIdx], angVelAx[lmntIdx], coloringCallbacksBuffer }));
#elif TEST_PRIMITIVES == 2
	for (unsigned int lmntIdx = 0; lmntIdx < TEST_LMNTS_NR / 2; lmntIdx++)
		capsules[lmntIdx] = capsulesPool.acquire<TheCapsule::InitStruct>(TheCapsule::InitStruct({ corium3D, transforms + lmntIdx, M_PI / 4.0f , linVels[lmntIdx], angVelMag[lmntIdx], angVelAx[lmntIdx], coloringCallbacksBuffer }));
	for (unsigned int lmntIdx = TEST_LMNTS_NR / 2; lmntIdx < TEST_LMNTS_NR; lmntIdx++)
		capsules[lmntIdx] = capsulesPool.acquire<TheCapsule::InitStruct>(TheCapsule::InitStruct({ corium3D, transforms + lmntIdx, 2 * asinf(transforms[lmntIdx].rot.z), linVels[lmntIdx], angVelMag[lmntIdx], angVelAx[lmntIdx], coloringCallbacksBuffer }));
#endif

	Transform3D playerTransform({ glm::vec3(4.5f, 0.0f, -7.0), glm::vec3(1.0f, 1.0f, 1.0f), glm::quat(cos(0 / 6), sin(0 / 4), sin(0 / 4), sin(0 / 6)) });
	float playerCollisionPrimitiveRot = 0 / 3; // 2 * asinf(playerTransform.rot.z);
	Corium3DEngine::GameLmnt::ProximityHandlingMethods playerProximityHandlingMethods[3];
	playerProximityHandlingMethods[0].collisionCallback = std::bind(&GameMaster::playerCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	playerProximityHandlingMethods[0].detachmentCallback = std::bind(&GameMaster::playerDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2);
	playerProximityHandlingMethods[1].collisionCallback = std::bind(&GameMaster::playerCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	playerProximityHandlingMethods[1].detachmentCallback = std::bind(&GameMaster::playerDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2);
	playerProximityHandlingMethods[2].collisionCallback = std::bind(&GameMaster::playerCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	playerProximityHandlingMethods[2].detachmentCallback = std::bind(&GameMaster::playerDetachmentCallback, this, std::placeholders::_1, std::placeholders::_2);
	player = new TheCube(TheCube::InitStruct({ corium3DEngine, &playerTransform, playerCollisionPrimitiveRot, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, glm::vec3(1.0f, 0.0f, 1.0f), playerProximityHandlingMethods }));
	playerMobilityAPI = player->accessMobilityAPI();
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::LEFT_ARROW, std::bind(&GameMaster::movePlayerLeft, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::RIGHT_ARROW, std::bind(&GameMaster::movePlayerRight, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::UP_ARROW, std::bind(&GameMaster::movePlayerUp, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::DOWN_ARROW, std::bind(&GameMaster::movePlayerDown, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::Q, std::bind(&GameMaster::movePlayerFar, this, std::placeholders::_1));
	corium3DEngine.registerKeyboardInputStartCallback(KeyboardInputID::A, std::bind(&GameMaster::movePlayerClose, this, std::placeholders::_1));
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
	corium3DEngine.registerCursorInputCallback(CursorInputID::MIDDLE_DOWN, std::bind(&GameMaster::shootRay, this, std::placeholders::_1, std::placeholders::_2));
	corium3DEngine.accessGuiAPI(0).show();
}

GameMaster::~GameMaster() {
	delete player;
	for (unsigned int testLmntIdx = 0; testLmntIdx < TEST_LMNTS_NR; testLmntIdx++)
#if TEST_PRIMITIVES == 0
		delete cubes[testLmntIdx];
#elif TEST_PRIMITIVES == 1
		delete spheres[testLmntIdx];
#elif TEST_PRIMITIVES == 2
		delete capsules[testLmntIdx];
#endif
}

/*
GameMaster::GameMaster(Corium3D& _corium3D) : corium3D(_corium3D), objPool(TEST_LMNTS_NR) {		
	Corium::Transform playerTransform({ glm::vec3(0.0f, -1.0f, -5.0f),   glm::vec3(1.1f, 1.1f, 1.1f), glm::quat(sqrt(2) / 2, sqrt(2) / 2, 0.0f, 0.0f) }); //glm::quat(cosf(M_PI/4), sinf(M_PI / 4), 0.0f, 0.0f)*glm::quat(cosf(M_PI/4), 0.0f, 0.0f, sinf(M_PI/4)); glm::quat(-sqrt(2)/2, sqrt(2)/2, 0.0f, 0.0f);
	Corium3D::GameLmnt::ProximityHandlingMethods playerProximityHandlingMethods[2];
	playerProximityHandlingMethods[0].collisionCallback = std::bind(&GameMaster::playerCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	playerProximityHandlingMethods[1].collisionCallback = std::bind(&GameMaster::playerCollisionCallback, this, std::placeholders::_1, std::placeholders::_2);
	
	player = new TheCube(TheCube::InitStruct({ corium3D, &playerTransform, glm::vec3(0.0f, 0.0f, 0.0f), playerProximityHandlingMethods }));
	playerMobilityAPI = player->accessMobilityAPI();	
	corium3D.registerKeyboardInputStartCallback(Corium::KeyboardInputID::LEFT_ARROW, std::bind(&GameMaster::movePlayerLeft, this, std::placeholders::_1));
	corium3D.registerKeyboardInputStartCallback(Corium::KeyboardInputID::RIGHT_ARROW, std::bind(&GameMaster::movePlayerRight, this, std::placeholders::_1));
	corium3D.registerKeyboardInputStartCallback(Corium::KeyboardInputID::UP_ARROW, std::bind(&GameMaster::movePlayerUp, this, std::placeholders::_1));
	corium3D.registerKeyboardInputStartCallback(Corium::KeyboardInputID::DOWN_ARROW, std::bind(&GameMaster::movePlayerDown, this, std::placeholders::_1));
	corium3D.registerKeyboardInputStartCallback(Corium::KeyboardInputID::Q, std::bind(&GameMaster::movePlayerFar, this, std::placeholders::_1));
	corium3D.registerKeyboardInputStartCallback(Corium::KeyboardInputID::A, std::bind(&GameMaster::movePlayerClose, this, std::placeholders::_1));	
	corium3D.registerKeyboardInputEndCallback(Corium::KeyboardInputID::LEFT_ARROW, std::bind(&GameMaster::stopPlayerLeft, this, std::placeholders::_1));
	corium3D.registerKeyboardInputEndCallback(Corium::KeyboardInputID::RIGHT_ARROW, std::bind(&GameMaster::stopPlayerRight, this, std::placeholders::_1));
	corium3D.registerKeyboardInputEndCallback(Corium::KeyboardInputID::UP_ARROW, std::bind(&GameMaster::stopPlayerUp, this, std::placeholders::_1));
	corium3D.registerKeyboardInputEndCallback(Corium::KeyboardInputID::DOWN_ARROW, std::bind(&GameMaster::stopPlayerDown, this, std::placeholders::_1));
	corium3D.registerKeyboardInputEndCallback(Corium::KeyboardInputID::Q, std::bind(&GameMaster::stopPlayerFar, this, std::placeholders::_1));
	corium3D.registerKeyboardInputEndCallback(Corium::KeyboardInputID::A, std::bind(&GameMaster::stopPlayerClose, this, std::placeholders::_1));
	corium3D.registerCursorInputCallback(Corium::CursorInputID::LEFT_DOWN, std::bind(&GameMaster::activatePanning, this, std::placeholders::_1, std::placeholders::_2));
	corium3D.registerCursorInputCallback(Corium::CursorInputID::LEFT_UP, std::bind(&GameMaster::deactivatePanning, this, std::placeholders::_1, std::placeholders::_2));
	corium3D.registerCursorInputCallback(Corium::CursorInputID::RIGHT_DOWN, std::bind(&GameMaster::activateRotation, this, std::placeholders::_1, std::placeholders::_2));
	corium3D.registerCursorInputCallback(Corium::CursorInputID::RIGHT_UP, std::bind(&GameMaster::deactivateRotation, this, std::placeholders::_1, std::placeholders::_2));	
	corium3D.registerCursorInputCallback(Corium::CursorInputID::WHEEL_UP, std::bind(&GameMaster::walkIn, this, std::placeholders::_1, std::placeholders::_2));
	corium3D.registerCursorInputCallback(Corium::CursorInputID::WHEEL_DOWN, std::bind(&GameMaster::walkOut, this, std::placeholders::_1, std::placeholders::_2));
	corium3D.registerCursorInputCallback(Corium::CursorInputID::MOVE, std::bind(&GameMaster::transformCamera, this, std::placeholders::_1, std::placeholders::_2));

	//playerTransform.translate = { 1.5f, 0.0f, -4 };
	//other = new TheCube(TheCube::InitStruct({ corium3D, &playerTransform, glm::vec3(0.0f, 0.0f, 0.0f), playerProximityHandlingMethods }));
		
	corium3D.accessGuiAPI(0).show();

	corium3D.startLoop();
}


GameMaster::~GameMaster() {
	delete other;
	delete player;
}
*/

void GameMaster::primitiveColoringCollisionCallback(Corium3DEngine::GameLmnt* primitive1, Corium3DEngine::GameLmnt* primitive2) {
	primitive1->accessGraphicsAPI()->changeVerticesColors(0, 1);
}

void GameMaster::primitiveColoringDetachmentCallback(Corium3DEngine::GameLmnt* primitive1, Corium3DEngine::GameLmnt* primitive2) {
	primitive1->accessGraphicsAPI()->changeVerticesColors(0, 0);
}

void GameMaster::removingCollisionCallbackCubeCube(Corium3DEngine::GameLmnt* cube1, Corium3DEngine::GameLmnt* cube2) {
	cubesPool.release((TheCube*)cube1);
}

void GameMaster::playerCollisionCallback(Corium3DEngine::GameLmnt* cube, Corium3DEngine::GameLmnt* sphere) {
	//objPool.release((TheCube*)cube2);
	cube->accessGraphicsAPI()->changeVerticesColors(0, 1);
}

void GameMaster::playerDetachmentCallback(Corium3DEngine::GameLmnt* cube, Corium3DEngine::GameLmnt* sphere) {
	//objPool.release((TheCube*)cube2);
	cube->accessGraphicsAPI()->changeVerticesColors(0, 0);
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

constexpr float ZOOM_FACTOR = 1.1;

void GameMaster::walkIn(double timeStamp, glm::vec2 const& cursorPos) {
	corium3DEngine.accessCameraAPI().translateInViewDirection(ZOOM_FACTOR);
}

void GameMaster::walkOut(double timeStamp, glm::vec2 const& cursorPos) {
	corium3DEngine.accessCameraAPI().translateInViewDirection(-ZOOM_FACTOR);
}

constexpr float CAMERA_TRANSLATION_PER_DOUBLE_CURSOR_MOVE = 0.5f / 20.0f;
constexpr float CAMERA_ROT_PER_DOUBLE_CURSOR_MOVE = 0.5f * (0.5f * M_PI / 1000.0f);
void GameMaster::transformCamera(double timeStamp, glm::vec2 const& cursorPos) {
	glm::vec2 cursorMoveVec = cursorPos - prevInputCursorPos;
	//glm::vec2 cursorMoveVec = { 0.0f, cursorPos.y - prevInputCursorPos.y };
	switch (cameraTransformMode) {
		case CameraTransformMode::PANNING:
			corium3DEngine.accessCameraAPI().pan(cursorMoveVec * CAMERA_TRANSLATION_PER_DOUBLE_CURSOR_MOVE);
			prevInputCursorPos = cursorPos;
			break;

		case CameraTransformMode::ROTATION:			
			corium3DEngine.accessCameraAPI().rotAroundViewportContainedAx(glm::length2(cursorMoveVec) * CAMERA_ROT_PER_DOUBLE_CURSOR_MOVE, glm::vec2(cursorMoveVec.y, -cursorMoveVec.x));
			prevInputCursorPos = cursorPos;
			break;		

		case CameraTransformMode::NONE: {}										
	}	
}

void GameMaster::shootRay(double timeStamp, glm::vec2 const& cursorPos) {
	for (unsigned int testLmntIdx = 0; testLmntIdx < TEST_LMNTS_NR; testLmntIdx++)
#if TEST_PRIMITIVES == 0
		cubes[testLmntIdx]->accessGraphicsAPI()->changeVerticesColors(0, 0);
#elif TEST_PRIMITIVES == 1
		spheres[testLmntIdx]->accessGraphicsAPI()->changeVerticesColors(0, 0);
#elif TEST_PRIMITIVES == 2
		capsules[testLmntIdx]->accessGraphicsAPI()->changeVerticesColors(0, 0);
#endif

	player->accessGraphicsAPI()->changeVerticesColors(0, 0);
	corium3DEngine.accessCameraAPI().shootRay(cursorPos);
}

void GameMaster::doNothing(double unused) {}