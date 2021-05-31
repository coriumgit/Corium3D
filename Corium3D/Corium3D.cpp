//
// Created by omer on 30/12/02017.
//

#include "Corium3D.h"

#include "Gui.h"
#include "Timer.h"
#include "Renderer.h"
#include "Logger.h"
#include "OpenGlTxtGen.h"
#include "IdxPool.h"
#include "AssetsOps.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <vector>
#include <fstream>

using namespace Corium3DUtils;

namespace Corium3D {

	// TODO: Need to add mesh transforms buffer
	const char* vertexShader =
		"#version 430 core \n"
    
		"in uint aInstanceDataIdx; \n"
		"in vec4 aPos; \n"	
		"in ivec4 aBonesIDs; \n"
		"in vec4 aBonesWeights; \n"
		"uniform mat4 uMeshTransform; \n"

		//"layout (std430, binding = 3) buffer MmvpsBaseIdxsBuffer { \n"
		//"	uint uMmvpsBaseIdx[]; \n"
		//"}; \n"
		"layout (std430, binding = 4) buffer MvpsBuffer { \n"
		"	mat4 uMVPs[]; \n"
		"}; \n "
		"layout (std430, binding = 5) buffer BonesTransformsBaseIdxsBuffer { \n"
		"	uint uBonesTransformsBaseIdxs[]; \n"
		"}; \n"
		"layout (std430, binding = 6) buffer BonesTransformsBuffer { \n"
		"	mat4 uBonesTransforms[]; \n"
		"}; \n"	
		"layout (std430, binding = 7) buffer SelectedColorsIdxsBuffer { \n"
		"	uint uSelectedColorsIdxs[]; \n"
		"}; \n"
		"layout (std430, binding = 8) buffer ColorsBuffer { \n" 
		"	vec4 uColors[]; \n"
		"}; \n"		

		"out vec4 passColor; \n"	
	
		"void main(void) { \n"	
		"   passColor = vec4(aBonesWeights[0], aBonesWeights[1], aBonesWeights[2], 1.0f); \n" // vec4(0.5f, 0.5f, 0.5f, 1.0f); \n" 
		"	mat4 boneTransform = mat4(1.0f); \n"
		"	if (aBonesWeights[0] != 0 || aBonesWeights[1] != 0 || aBonesWeights[2] != 0 || aBonesWeights[3] != 0) { \n"
		"		uint bonesTransformsBaseIdx = uBonesTransformsBaseIdxs[aInstanceDataIdx]; \n"
		"		boneTransform = uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[0]]*aBonesWeights[0] + \n"
		"						uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[1]]*aBonesWeights[1] + \n"
		"						uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[2]]*aBonesWeights[2] + \n"
		"						uBonesTransforms[bonesTransformsBaseIdx + aBonesIDs[3]]*aBonesWeights[3]; \n"
		"	} \n"
	
		"   gl_Position = uMVPs[aInstanceDataIdx] * uMeshTransform * boneTransform * aPos; \n"	
		"}";

	const char* fragShader =
		"#version 430 core \n"
		"precision mediump float; \n"

		"in vec4 passColor; \n"
		"out vec4 fragColor; \n"

		"void main(void) { \n"
		"   fragColor = passColor; \n"
		"}";

	const float SECS_PER_UPDATE = 0.016666667f;
	const unsigned int INPUTS_BUFFER_SZ = 10;

	const char* bonelessVertexShader =
		"#version 430 core \n"

		"in uint aInstanceDataIdx; \n"
		"uniform mat4 uVpMat; \n"
		"in vec4 aPos; \n"
		"uniform uint uBaseVertex; \n" // TODO: simplify this shit

		"layout (std430, binding = 4) buffer MvpsBuffer { \n"
		"	mat4 uMVPs[]; \n"
		"}; \n "
		"layout (std430, binding = 7) buffer SelectedColorsIdxsBuffer { \n"
		"	uint uSelectedColorsIdxs[]; \n"
		"}; \n"
		"layout (std430, binding = 8) buffer ColorsBuffer { \n"
		"	vec4 uColors[]; \n"
		"}; \n"

		"out vec4 passColor; \n"

		"void main(void) { \n"
		"	passColor = uColors[uSelectedColorsIdxs[aInstanceDataIdx] + gl_VertexID - uBaseVertex]; \n" //"	passColor = uColors[gl_VertexID]; \n"
		//"   passColor = vec4(aInstanceDataIdx/2.0f, aInstanceDataIdx/2.0f, aInstanceDataIdx/2.0f, 1.0f); \n"
		"   gl_Position = uMVPs[aInstanceDataIdx]*aPos; \n" //*** uVpMat 
		"}";
	
	class Corium3DEngine::Corium3DEngineImpl {
	public:	
		friend class Corium3DEngine::GameLmnt::GameLmntImpl;	
		friend class Corium3DEngine::CameraAPI::CameraApiImpl;
    
		Corium3DEngineImpl(Corium3DEngineOnlineCallback& corium3DOnlineCallback, AssetsFilesFullPaths const& assetsFilesFullPaths);
		~Corium3DEngineImpl();
		void startLoop();
		void signalResume();
		void signalPause();
		void signalSurfaceCreated(Corium3DEngineNativeWindowType window);
		void signalSurfaceSzChanged(unsigned int width, unsigned int height);
		void signalSurfaceDestroyed();
		void signalWindowFocusChanged(bool hasFocus);
		void signalDetachedFromWindow();	
		std::vector<std::vector<Transform3D>> loadScene(Corium3DEngine& owningEngine, unsigned int sceneIdx);
		void registerKeyboardInputStartCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback);
		void registerKeyboardInputEndCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback);
		void registerCursorInputCallback(CursorInputID inputId, CursorInputCallback inputCallback);
		void systemKeyboardInputStartCallback(KeyboardInputID inputId);
		void systemKeyboardInputEndCallback(KeyboardInputID inputId);
		void systemCursorInputCallback(CursorInputID inputId, vec2 const& cursorPos);		
		Corium3DEngine::GuiAPI& accessGuiAPI(unsigned int guiIdx);	
		Corium3DEngine::CameraAPI& accessCameraAPI();		

	private:    		
		Corium3DEngineOnlineCallback& corium3DEngineOnlineCallback;
		BVH* bvh;	
		CollisionPrimitivesFactory* collisionPrimitivesFactory;
		PhysicsEngine* physicsEngine;
		GUI** guis;
		GuiAPI** guiAPIs;
		GuiAPI::GuiApiImpl** guiApiImpls;
		Renderer* renderer;	
		CameraAPI* cameraAPI;
		CameraAPI::CameraApiImpl* cameraApiImpl;

		Corium3DEngineNativeWindowType window;
		unsigned int surfaceWidth, surfaceHeight;

		std::thread loopThread;
		std::mutex loopMutex;
		std::mutex eglMutex;
		std::condition_variable waitCond;
	
		bool isGameOn = true;
		bool isPaused = false;
		bool hasFocus = false;
		bool hasSurface = false;
		bool isSurfaceSzKnown = false;
		bool needInit = true;
		bool isSurfaceSzChangedSig = false;
		bool isSceneLoaded = false;

		std::string modelsScenesFullPath;
		std::vector<unsigned int> modelSceneModelIdxsMap;
		std::string guisDescsPath;		

		unsigned int sceneModelsNr;
		unsigned int staticModelsNr;		
		unsigned int* modelsInstancesNrsMaxima;
		IdxPool** modelsInstancesIdxPools;		
		GameLmnt*** gameLmnts;
		GameLmnt::ProximityHandlingMethods*** proximityHandlingMethods;		
		GameLmnt::OnRayHit** onRayHitCallbacks;
		ObjPoolIteratable<GameLmnt::StateUpdater>* stateUpdatersPool;
		ObjPoolIteratable<GameLmnt::StateUpdater>::ObjPoolIt* stateUpdatersIt;

		BoundingSphere* modelsPrimalBoundingSpheres;
		AABB3DRotatable* modelsPrimalAABB3Ds;	
		CollisionVolume** modelsPrimalCollisionVolumesPtrs;		

		AABB2DRotatable* modelsPrimalAABB2Ds;
		CollisionPerimeter** modelsPrimalCollisionPerimetersPtrs;

		KeyboardInputCallback* keyboardInputStartCallbacks;
		KeyboardInputCallback* keyboardInputEndCallbacks;
		CursorInputCallback* cursorInputCallbacks;
	
		struct InputEvent {
			KeyboardInputCallback keyboardInputCallback;
			CursorInputCallback cursorInputCallback;
			double inputTimeStamp;
			vec2 cursorPos;
		};
		InputEvent inputEventsBuffer[INPUTS_BUFFER_SZ];
		unsigned int currUpdateInputsNr = 0;	

		bool loop();	
		bool canLoopContinue();
		void unloadScene();
		void processInput();
		void update();
		void resolveCollisions3D();
		void resolveCollisions2D();
		template <class V>
		void doResolveCollisions(BVH::CollisionsData<V> const& collisionsData);
		//bool loopThreadStarter();
		inline void updateInputsCallbackBuffer(KeyboardInputCallback inputCallback) {
			inputEventsBuffer[currUpdateInputsNr].keyboardInputCallback = inputCallback;				
			inputEventsBuffer[currUpdateInputsNr].cursorInputCallback = NULL;
			inputEventsBuffer[currUpdateInputsNr].inputTimeStamp = ServiceLocator::getTimer().getCurrentTime();
			currUpdateInputsNr = (currUpdateInputsNr + 1) % INPUTS_BUFFER_SZ;
		}
		inline void updateInputsCallbackBuffer(CursorInputCallback inputCallback, vec2 const& cursorPos) {
			inputEventsBuffer[currUpdateInputsNr].keyboardInputCallback = NULL;
			inputEventsBuffer[currUpdateInputsNr].cursorInputCallback = inputCallback;
			inputEventsBuffer[currUpdateInputsNr].inputTimeStamp = ServiceLocator::getTimer().getCurrentTime();
			inputEventsBuffer[currUpdateInputsNr].cursorPos = cursorPos;
			currUpdateInputsNr = (currUpdateInputsNr + 1) % INPUTS_BUFFER_SZ;
		}
	};

	class Corium3DEngine::GameLmnt::GameLmntImpl {
	public:			
		GameLmntImpl(GameLmnt& owningGameLmnt, Corium3DEngineImpl& corium3DEngineImpl, 
					 unsigned int components,
					 StateUpdater stateUpdater,
					 OnMovementMadeCallback onMovementMadeCallback,
					 unsigned int modelIdx,
					 Transform3D const* initTransform,
					 float initCollisionPerimeterRot,
					 ProximityHandlingMethods* proximityHandlingMethods,
					 OnRayHit onRayHitCallback);
		~GameLmntImpl();
		void changeVerticesColors(unsigned int meshIdx, unsigned int colorsArrIdx);
		void changeAnimation(unsigned int animationIdx);
		void translate(glm::vec3 const& translate) { mobilityInterface->translate(translate); }
		void scale(float scaleFactor) { mobilityInterface->scale(scaleFactor); }
		void rot(float rot, glm::vec3 const& rotAx) { mobilityInterface->rot(rot, rotAx); }
		void rot(glm::quat const& rot) { mobilityInterface->rot(rot); }
		void setLinVel(glm::vec3 const& linVel) { mobilityInterface->setLinVel(linVel); }
		void setAngVel(float angVelMag, glm::vec3 const& angVelAx) { mobilityInterface->setAngVel(angVelMag, angVelAx); }
		void setLinAccel(glm::vec3 const& linAccel) { mobilityInterface->setLinAccel(linAccel); }
		void setAngAccel(glm::vec3 const& angAccel) { mobilityInterface->setAngAccel(angAccel); }
		void setLinVelX(float x) { mobilityInterface->setLinVelX(x); }
		void setLinVelY(float y) { mobilityInterface->setLinVelY(y); }
		void setLinVelZ(float z) { mobilityInterface->setLinVelZ(z); }
		// void assignStateUpdater(StateUpdater stateUpdater);
		// void assignOnMovementMadeCallback(OnMovementMadeCallback onMovementMadeCallback);
		// void assignProximityHandlingMethods(ProximityHandlingMethods* proximityHandlingMethods);
		GraphicsAPI* accessGraphicsAPI() { return graphicsAPI; }
		MobilityAPI* accessMobilityAPI() { return mobilityAPI; }	

	private:			
		Corium3DEngineImpl& corium3DEngineImpl;	
		GameLmnt& owningGameLmnt;
		unsigned int modelIdx;
		unsigned int instanceIdx;
		StateUpdater* stateUpdater;
		unsigned int componentsFlag;
		union {
			BVH::DataNode3D* staticGameLmntBvhDataNode3D;
			BVH::MobileGameLmntDataNode3D* mobileGameLmntBvhDataNode3D;
		};
		union {
			BVH::DataNode2D* staticGameLmntBvhDataNode2D;
			BVH::MobileGameLmntDataNode2D* mobileGameLmntBvhDataNode2D;
		};
		CollisionVolume* collisionVolume = NULL;
		CollisionPerimeter* collisionPerimeter = NULL;
		PhysicsEngine::MobilityInterface* mobilityInterface = NULL;
		GraphicsAPI* graphicsAPI = NULL;
		Renderer::InstanceAnimationInterface* instanceAnimationInterface = NULL;
		MobilityAPI* mobilityAPI = NULL;

		void updateBvhNodeBVs(Transform3DUS const& transformDelta);
		void updateBvhNodeBPs(Transform2DUS const& transformDelta);		
	};
	
	class Corium3DEngine::GuiAPI::GuiApiImpl {
	public:
		GuiApiImpl(GUI& gui, unsigned int imgsControlsNr, unsigned int txtControlsNr);
		~GuiApiImpl();
		void show() { gui.show(); }
		void hide() { gui.hide(); }
		ControlAPI& accessControl(unsigned int controlIdx) { return *(controlsAPIs[controlIdx]); }
		TxtControlAPI& accessTxtControl(unsigned int controlIdx) { return *(txtControlsAPIs[controlIdx]); }

	private:
		GUI& gui;
		unsigned int imgsControlsNr; 		
		Corium3DEngine::GuiAPI::ControlAPI** controlsAPIs;
		Corium3DEngine::GuiAPI::ControlAPI::ControlApiImpl** controlsApisImpls;
		unsigned int txtControlsNr;
		Corium3DEngine::GuiAPI::TxtControlAPI** txtControlsAPIs;
		Corium3DEngine::GuiAPI::TxtControlAPI::TxtControlApiImpl** txtControlsApisImpls;
	};

	class Corium3DEngine::GuiAPI::ControlAPI::ControlApiImpl {
	public:
		ControlApiImpl(GUI::Control& _control) : control(_control) {}
		void assignSelectCallback(OnSelectCallback onSelectCallback) { control.assignSelectCallback(onSelectCallback); }
		void updateGraphics(unsigned int* quadsIdxs, unsigned int quadsNr, Rect* quads, Rect* uvs) { control.updateGraphics(quadsIdxs, quadsNr, quads, uvs); }
		//void updateQuads(unsigned int* quadsIdxs, Rect const* quad) { control.updateQuads(quadsIdxs, quad); }
		//void updateUvs(unsigned int* quadsIdxs, Rect const* uvs) { control.updateUvs(quadsIdxs, uvs); }

	private:
		GUI::Control& control;
	};

	class Corium3DEngine::GuiAPI::TxtControlAPI::TxtControlApiImpl : public Corium3DEngine::GuiAPI::ControlAPI::ControlApiImpl {
	public:
		TxtControlApiImpl(GUI::TxtControl& _txtControl) : ControlApiImpl(_txtControl), txtControl(_txtControl) {}
		void setTxt(unsigned int number) { txtControl.setTxt(number); }
		//void setTxt(char const* txt) { txtControl.setTxt(txt); }

	private:
		GUI::TxtControl& txtControl;
	};

	// TODO: Move this to the renderer (?)
	class Corium3DEngine::CameraAPI::CameraApiImpl {
	public:
		CameraApiImpl(Corium3DEngineImpl& _corium3DEngineImpl) : corium3DEngineImpl(_corium3DEngineImpl), renderer(*(corium3DEngineImpl.renderer)) {}
		void translate(glm::vec3 translation);
		void rotate(float rotAng, glm::vec3 rotAx);
		void pan(glm::vec2 const& panVec);
		void rotAroundViewportContainedAx(float rotAng, glm::vec2 const& rotAx);
		void translateInViewDirection(float translation);
		void zoom(float factor);
		bool shootRay(glm::vec2 const& cursorPos);
	
	private:
		Corium3DEngineImpl& corium3DEngineImpl;
		Renderer& renderer;		
	};

	Corium3DEngine::Corium3DEngine(CallbackPtrs& callbacksPtrs, AssetsFilesFullPaths const& assetsFilesFullPaths) { //const char* guisDescsPath
		corium3DEngineImpl = new Corium3DEngineImpl(callbacksPtrs.corium3DEngineOnlineCallback, assetsFilesFullPaths);		
		callbacksPtrs.systemKeyboardInputStartCallbackPtr = std::bind(&Corium3DEngine::systemKeyboardInputStartCallback, this, std::placeholders::_1);
		callbacksPtrs.systemKeyboardInputEndCallbackPtr = std::bind(&Corium3DEngine::systemKeyboardInputEndCallback, this, std::placeholders::_1);
		callbacksPtrs.systemCursorInputCallbackPtr = std::bind(&Corium3DEngine::systemCursorInputCallback, this, std::placeholders::_1, std::placeholders::_2);
	}

	Corium3DEngine::~Corium3DEngine() {
		delete corium3DEngineImpl;
	}

	void Corium3DEngine::startLoop() {
		corium3DEngineImpl->startLoop();
	}

	void Corium3DEngine::signalResume() {
		corium3DEngineImpl->signalResume();
	}

	void Corium3DEngine::signalPause() {
		corium3DEngineImpl->signalPause();
	}

	void Corium3DEngine::signalSurfaceCreated(Corium3DEngineNativeWindowType window) {
		corium3DEngineImpl->signalSurfaceCreated(window);
	}

	void Corium3DEngine::signalSurfaceSzChanged(unsigned int width, unsigned int height) {
		corium3DEngineImpl->signalSurfaceSzChanged(width, height);
	}

	void Corium3DEngine::signalSurfaceDestroyed() {
		corium3DEngineImpl->signalSurfaceDestroyed();
	}

	void Corium3DEngine::signalWindowFocusChanged(bool hasFocus) {
		corium3DEngineImpl->signalWindowFocusChanged(hasFocus);
	}

	void Corium3DEngine::signalDetachedFromWindow() {
		corium3DEngineImpl->signalDetachedFromWindow();
	}

	std::vector<std::vector<Transform3D>> Corium3DEngine::loadScene(unsigned int sceneIdx)
	{			
		return corium3DEngineImpl->loadScene(*this, sceneIdx);		
	}

	void Corium3DEngine::registerKeyboardInputStartCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback) {
		corium3DEngineImpl->registerKeyboardInputStartCallback(inputId, inputCallback);
	}

	void Corium3DEngine::registerKeyboardInputEndCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback) {
		corium3DEngineImpl->registerKeyboardInputEndCallback(inputId, inputCallback);
	}

	void Corium3DEngine::registerCursorInputCallback(CursorInputID inputId, CursorInputCallback inputCallback) {
		corium3DEngineImpl->registerCursorInputCallback(inputId, inputCallback);
	}

	void Corium3DEngine::systemKeyboardInputStartCallback(KeyboardInputID inputId) {
		corium3DEngineImpl->systemKeyboardInputStartCallback(inputId);
	}

	void Corium3DEngine::systemKeyboardInputEndCallback(KeyboardInputID inputId) {
		corium3DEngineImpl->systemKeyboardInputEndCallback(inputId);
	}

	void Corium3DEngine::systemCursorInputCallback(CursorInputID inputId, vec2 const& cursorPos) {
		corium3DEngineImpl->systemCursorInputCallback(inputId, cursorPos);
	}

	Corium3DEngine::GuiAPI& Corium3DEngine::accessGuiAPI(unsigned int guiIdx) {
		return corium3DEngineImpl->accessGuiAPI(guiIdx);
	}

	Corium3DEngine::CameraAPI& Corium3DEngine::accessCameraAPI() {
		return corium3DEngineImpl->accessCameraAPI();
	}	

	Corium3DEngine::Corium3DEngineImpl::Corium3DEngineImpl(Corium3DEngineOnlineCallback& _corium3DEngineOnlineCallback, AssetsFilesFullPaths const& assetsFilesFullPaths) :
			corium3DEngineOnlineCallback(_corium3DEngineOnlineCallback), modelsScenesFullPath(assetsFilesFullPaths.modelsScenesFullPath) { //guisDescsPath(_guisDescsPath), 		
		unsigned int glyphsWidths[96] = { 8, 6, 0, 0, 0, 0, 0, 0, 12, 12, 0, 0, 8, 0, 8, 0,
										30,16,27,25,27,26,27,25,25,27, 0, 0, 0, 0, 0, 0,
										0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0,
										0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0,
										0,24,25,24,25,26,16,25,23, 7,10,24, 6,36,23,26,
										26,25,14,22,16,23,26,39,24,27,22, 0, 0, 0, 0, 0 };
		unsigned int glyphsHeights[96];
		for (unsigned int glyphIdx = 0; glyphIdx < 96; glyphIdx++)
			glyphsHeights[glyphIdx] = 64;
		GlyphsAtlas glyphsAtlas(assetsFilesFullPaths.txtTexAtlasPath, 16, 6, glyphsWidths, glyphsHeights, 96, GlyphsAtlas::GlyphsIdxsMap({ 0, 16, 33, 65, 1, 12, 14, 8, 9 }));
		GUI::TxtControlParams fpsControlParams = { 0u, 3u, { 0.02f, 0.93f }, 0.0f, 0.05f, {0.0f, 0.0f, 0.0f, 1.0f} };
		GUI::TxtControlParams visiblesControlParams = { 0u, 50u, { 0.02f, 0.8f }, 0.0f, 0.05f, {0.0f, 0.0f, 0.0f, 1.0f} };
		GUI::TxtControlParams txtControlsParams[2] = { fpsControlParams, visiblesControlParams };

		guiAPIs = new GuiAPI*[1];
		guiApiImpls = new GuiAPI::GuiApiImpl*[1];
		guis = new GUI*[1];
		guis[0] = new GUI(NULL, 0, NULL, 0, &glyphsAtlas, 1, txtControlsParams, 2);
		guiApiImpls[0] = new GuiAPI::GuiApiImpl(*(guis[0]), 0, 1);
		guiAPIs[0] = new GuiAPI(*(guiApiImpls[0]));					

		cameraApiImpl = new CameraAPI::CameraApiImpl(*this);
		cameraAPI = new CameraAPI(*cameraApiImpl);

		keyboardInputStartCallbacks = new KeyboardInputCallback[KeyboardInputID::__KEYBOARD_INPUT_IDS_NR__];
		keyboardInputEndCallbacks = new KeyboardInputCallback[KeyboardInputID::__KEYBOARD_INPUT_IDS_NR__];
		for (unsigned int keyboardInputCallbackIdx = 0; keyboardInputCallbackIdx < KeyboardInputID::__KEYBOARD_INPUT_IDS_NR__; keyboardInputCallbackIdx++)
			keyboardInputStartCallbacks[keyboardInputCallbackIdx] = keyboardInputEndCallbacks[keyboardInputCallbackIdx] = NULL;
		cursorInputCallbacks = new CursorInputCallback[CursorInputID::__CURSOR_INPUT_IDS_NR__];

		renderer = new Renderer(assetsFilesFullPaths.vertexShadersFullPaths, assetsFilesFullPaths.fragShadersFullPaths, assetsFilesFullPaths.shadersNr, *(guis[0]), guis[0]->accessTxtControl(0), guis[0]->accessTxtControl(1));

		loopThread = std::thread(&Corium3DEngineImpl::loop, this);
	}

	Corium3DEngine::Corium3DEngineImpl::~Corium3DEngineImpl() {
		delete[] keyboardInputStartCallbacks;
		delete[] keyboardInputEndCallbacks;
		delete[] cursorInputCallbacks;

		delete cameraAPI;
		delete cameraApiImpl;

		delete guiAPIs[0];
		delete guiApiImpls[0];
		delete guis[0];
		delete[] guis;
		delete[] guiApiImpls;
		delete[] guiAPIs;		
	}

	void Corium3DEngine::Corium3DEngineImpl::startLoop() {
	
	}

	void Corium3DEngine::Corium3DEngineImpl::signalResume() {	
		loopMutex.lock();
		isPaused = false;	
		waitCond.notify_one();	
		loopMutex.unlock();
	}

	void Corium3DEngine::Corium3DEngineImpl::signalPause() {	
		loopMutex.lock();
		isPaused = true;	
		waitCond.notify_one();	
		loopMutex.unlock();
	}

	void Corium3DEngine::Corium3DEngineImpl::signalSurfaceCreated(Corium3DEngineNativeWindowType _window) {
		ServiceLocator::getLogger().logd("Corium3DEngineImpl", "signalSurfaceCreated called.");	
		loopMutex.lock();
		window = _window;
		hasSurface = true;	
		waitCond.notify_one();	
		loopMutex.unlock();
	}

	void Corium3DEngine::Corium3DEngineImpl::signalSurfaceSzChanged(unsigned int width, unsigned int height) {
		ServiceLocator::getLogger().logd("Corium3DEngine", "signalSurfaceSzChanged called.");	
		loopMutex.lock();
		surfaceWidth = width;
		surfaceHeight = height;
		isSurfaceSzChangedSig = true;
		if (!isSurfaceSzKnown) {
			isSurfaceSzKnown = true;		
			waitCond.notify_one();
		}	
		loopMutex.unlock();
	}

	void Corium3DEngine::Corium3DEngineImpl::signalSurfaceDestroyed() {
		ServiceLocator::getLogger().logd("Corium3DEngine", "signalSurfaceDestroyed called.");	
		loopMutex.lock();
		hasSurface = false;
		isSurfaceSzKnown = false;	
		waitCond.notify_one();	
		loopMutex.unlock();
	}

	void Corium3DEngine::Corium3DEngineImpl::signalWindowFocusChanged(bool _hasFocus) {
		ServiceLocator::getLogger().logd("Corium3DEngine", "onWindowFocusChanged called.");	
		loopMutex.lock();
		hasFocus = _hasFocus;
		if (hasFocus)		
			waitCond.notify_one();	
		loopMutex.unlock();
	}

	void Corium3DEngine::Corium3DEngineImpl::signalDetachedFromWindow() {
		ServiceLocator::getLogger().logd("Corium3DEngine", "onDitachedFromWindow called.");	
		isGameOn = false;	
		waitCond.notify_one();		
		loopThread.join();
	}	

	void Corium3DEngine::Corium3DEngineImpl::registerKeyboardInputStartCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback) {
		keyboardInputStartCallbacks[inputId] = inputCallback;
	}

	void Corium3DEngine::Corium3DEngineImpl::registerKeyboardInputEndCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback) {
		keyboardInputEndCallbacks[inputId] = inputCallback;
	}

	void Corium3DEngine::Corium3DEngineImpl::registerCursorInputCallback(CursorInputID inputId, CursorInputCallback inputCallback) {
		cursorInputCallbacks[inputId] = inputCallback;
	}

	void Corium3DEngine::Corium3DEngineImpl::systemKeyboardInputStartCallback(KeyboardInputID inputId) {
		updateInputsCallbackBuffer(keyboardInputStartCallbacks[inputId]);
	}

	void Corium3DEngine::Corium3DEngineImpl::systemKeyboardInputEndCallback(KeyboardInputID inputId) {
		updateInputsCallbackBuffer(keyboardInputEndCallbacks[inputId]);
	}

	void Corium3DEngine::Corium3DEngineImpl::systemCursorInputCallback(CursorInputID inputId, vec2 const& cursorPos) {
		updateInputsCallbackBuffer(cursorInputCallbacks[inputId], cursorPos);
	}	

	std::vector<std::vector<Transform3D>> Corium3DEngine::Corium3DEngineImpl::loadScene(Corium3DEngine& owningEngine, unsigned int sceneIdx) {
		unloadScene();
		
		SceneData sceneData;
		std::vector<ModelDesc> modelDescs;
		readSceneAssets(modelsScenesFullPath, sceneIdx, sceneData, modelDescs, modelSceneModelIdxsMap);
		staticModelsNr = sceneData.staticModelsNr;
		sceneModelsNr = sceneData.sceneModelsData.size();
		
		// create game elements buffers		
		gameLmnts = new GameLmnt**[sceneModelsNr];
		modelsInstancesNrsMaxima = new unsigned int[sceneModelsNr];
		proximityHandlingMethods = new GameLmnt::ProximityHandlingMethods**[sceneModelsNr];
		onRayHitCallbacks = new GameLmnt::OnRayHit*[sceneModelsNr];
		modelsInstancesIdxPools = new IdxPool*[sceneModelsNr];
		modelsPrimalBoundingSpheres = new BoundingSphere[sceneModelsNr];
		modelsPrimalAABB3Ds = new AABB3DRotatable[sceneModelsNr];
		modelsPrimalCollisionVolumesPtrs = new CollisionVolume*[sceneModelsNr];
		modelsPrimalAABB2Ds = new AABB2DRotatable[sceneModelsNr];
		modelsPrimalCollisionPerimetersPtrs = new CollisionPerimeter*[sceneModelsNr];
		std::vector<std::vector<Transform3D>> instancesTransformsInit(modelSceneModelIdxsMap.size());

		collisionPrimitivesFactory = new CollisionPrimitivesFactory(&sceneData.collisionPrimitives3DInstancesNrsMaxima[0], &sceneData.collisionPrimitives2DInstancesNrsMaxima[0]);
		unsigned int staticInstancesNrOverallMax = 0;
		unsigned int mobileInstancesNrOverallMax = 0;
		for (unsigned int sceneModelIdx = 0; sceneModelIdx < sceneModelsNr; ++sceneModelIdx)
		{
			SceneData::SceneModelData& sceneModelData = sceneData.sceneModelsData[sceneModelIdx];
			unsigned int modelIdxMapped = modelSceneModelIdxsMap[sceneModelData.modelIdx];
			unsigned int instancesNrMax = modelsInstancesNrsMaxima[modelIdxMapped] = sceneModelData.instancesNrMax;
			if (sceneModelData.isStatic)
				staticInstancesNrOverallMax += instancesNrMax;
			else
				mobileInstancesNrOverallMax += instancesNrMax;
			modelsInstancesIdxPools[modelIdxMapped] = new IdxPool(instancesNrMax);
			gameLmnts[modelIdxMapped] = new GameLmnt * [instancesNrMax];
			proximityHandlingMethods[modelIdxMapped] = new GameLmnt::ProximityHandlingMethods * [instancesNrMax];
			for (unsigned int collidedWithModelIdx = 0; collidedWithModelIdx < instancesNrMax; collidedWithModelIdx++)
				proximityHandlingMethods[modelIdxMapped][collidedWithModelIdx] = new GameLmnt::ProximityHandlingMethods[sceneModelsNr];
			onRayHitCallbacks[modelIdxMapped] = new GameLmnt::OnRayHit[instancesNrMax];
			
			modelsPrimalBoundingSpheres[modelIdxMapped] = BoundingSphere(modelDescs[modelIdxMapped].boundingSphereCenter, modelDescs[modelIdxMapped].boundingSphereRadius);

			ColliderData colliderData = modelDescs[modelIdxMapped].colliderData;			
			if (colliderData.collisionPrimitive3DType != CollisionPrimitive3DType::NO_3D_COLLIDER) {
				modelsPrimalAABB3Ds[modelIdxMapped] = AABB3DRotatable(colliderData.aabb3DMinVertex, colliderData.aabb3DMaxVertex);
				switch (colliderData.collisionPrimitive3DType) {
					case CollisionPrimitive3DType::BOX: {
						ColliderData::CollisionBoxData& collisionBoxData = colliderData.collisionPrimitive3dData.collisionBoxData;
						modelsPrimalCollisionVolumesPtrs[modelIdxMapped] = collisionPrimitivesFactory->genCollisionBox(collisionBoxData.center, collisionBoxData.scale);
						break;
					}
					case CollisionPrimitive3DType::SPHERE: {
						ColliderData::CollisionSphereData& collisionSphereData = colliderData.collisionPrimitive3dData.collisionSphereData;
						modelsPrimalCollisionVolumesPtrs[modelIdxMapped] = collisionPrimitivesFactory->genCollisionSphere(collisionSphereData.center, collisionSphereData.radius);
						break;
					}
					case CollisionPrimitive3DType::CAPSULE: {
						ColliderData::CollisionCapsuleData collisionCapsuleData = colliderData.collisionPrimitive3dData.collisionCapsuleData;
						modelsPrimalCollisionVolumesPtrs[modelIdxMapped] = collisionPrimitivesFactory->genCollisionCapsule(collisionCapsuleData.center1, collisionCapsuleData.axisVec, collisionCapsuleData.radius);
						break;
					}
				}
			}
			else {
				// assigning aabb such that aabb.min = max_float && aabb.max == min_float, thus no intersection will be detected with this aabb
				modelsPrimalAABB3Ds[modelIdxMapped] = AABB3DRotatable(glm::vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
																	  glm::vec3(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()));
				modelsPrimalCollisionVolumesPtrs[modelIdxMapped] = NULL;
			}

			if (colliderData.collisionPrimitive2DType != CollisionPrimitive2DType::NO_2D_COLLIDER) {
				modelsPrimalAABB2Ds[modelIdxMapped] = AABB2DRotatable(colliderData.aabb2DMinVertex, colliderData.aabb2DMaxVertex);
				switch (colliderData.collisionPrimitive3DType) {
					case CollisionPrimitive2DType::RECT: {
						ColliderData::CollisionRectData& collisionRectData = colliderData.collisionPrimitive2dData.collisionRectData;
						modelsPrimalCollisionPerimetersPtrs[modelIdxMapped] = collisionPrimitivesFactory->genCollisionRect(collisionRectData.center, collisionRectData.scale);
						break;
					}
					case CollisionPrimitive2DType::CIRCLE: {
						ColliderData::CollisionCircleData& collisionCircleData = colliderData.collisionPrimitive2dData.collisionCircleData;
						modelsPrimalCollisionPerimetersPtrs[modelIdxMapped] = collisionPrimitivesFactory->genCollisionCircle(collisionCircleData.center, collisionCircleData.radius);
						break;
					}
					case CollisionPrimitive2DType::STADIUM: {
						ColliderData::CollisionStadiumData& collisionStadiumData = colliderData.collisionPrimitive2dData.collisionStadiumData;
						modelsPrimalCollisionPerimetersPtrs[modelIdxMapped] = collisionPrimitivesFactory->genCollisionStadium(collisionStadiumData.center1, collisionStadiumData.axisVec, collisionStadiumData.radius);
						break;
					}
				}
			}
			else {
				// assigning aabb such that aabb.min = max_float && aabb.max == min_float, thus no intersection will be detected with this aabb
				modelsPrimalAABB2Ds[modelIdxMapped] = AABB2DRotatable(glm::vec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
																	  -glm::vec2(std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()));
				modelsPrimalCollisionPerimetersPtrs[modelIdxMapped] = NULL;
			}
			
			instancesTransformsInit[sceneModelData.modelIdx] = sceneModelData.instancesTransformsInit;
		}

		bvh = new BVH(staticInstancesNrOverallMax, mobileInstancesNrOverallMax, staticInstancesNrOverallMax, mobileInstancesNrOverallMax, 100, 100);
		physicsEngine = new PhysicsEngine(mobileInstancesNrOverallMax + staticInstancesNrOverallMax, SECS_PER_UPDATE);
		renderer->loadScene(std::move(modelDescs), staticModelsNr, sceneModelsNr - staticModelsNr, modelsInstancesNrsMaxima, *bvh);
		stateUpdatersPool = new ObjPoolIteratable<GameLmnt::StateUpdater>(mobileInstancesNrOverallMax + staticInstancesNrOverallMax);
		stateUpdatersIt = new ObjPoolIteratable<GameLmnt::StateUpdater>::ObjPoolIt(*stateUpdatersPool);

		cameraApiImpl = new CameraAPI::CameraApiImpl(*this);
		cameraAPI = new CameraAPI(*cameraApiImpl);

		keyboardInputStartCallbacks = new KeyboardInputCallback[KeyboardInputID::__KEYBOARD_INPUT_IDS_NR__];
		keyboardInputEndCallbacks = new KeyboardInputCallback[KeyboardInputID::__KEYBOARD_INPUT_IDS_NR__];
		for (unsigned int keyboardInputCallbackIdx = 0; keyboardInputCallbackIdx < KeyboardInputID::__KEYBOARD_INPUT_IDS_NR__; keyboardInputCallbackIdx++)
			keyboardInputStartCallbacks[keyboardInputCallbackIdx] = keyboardInputEndCallbacks[keyboardInputCallbackIdx] = NULL;
		cursorInputCallbacks = new CursorInputCallback[CursorInputID::__CURSOR_INPUT_IDS_NR__];		

		return instancesTransformsInit;
	}

	Corium3DEngine::GuiAPI& Corium3DEngine::Corium3DEngineImpl::accessGuiAPI(unsigned int guiIdx) {
		return *(guiAPIs[guiIdx]);
	}

	Corium3DEngine::CameraAPI& Corium3DEngine::Corium3DEngineImpl::accessCameraAPI() {
		return *cameraAPI;
	}

	bool Corium3DEngine::Corium3DEngineImpl::loop() {	
		eglMutex.lock();
		double previous = ServiceLocator::getTimer().getCurrentTime();
		double lag = 0.0;
		bool isSurfaceSzChanged;	 
		std::unique_lock<std::mutex> loopMutexLock(loopMutex, std::defer_lock);
		while (isGameOn) {		
			loopMutexLock.lock();
			if (isPaused) {
				renderer->destroy();
				needInit = true;
			}
		
			waitCond.wait(loopMutexLock, std::bind(&Corium3DEngineImpl::canLoopContinue, this));		

			if (!isGameOn) {
				break;
			}

			isSurfaceSzChanged = isSurfaceSzChangedSig;
			isSurfaceSzChangedSig = false;		
			loopMutexLock.unlock();

			if (needInit) {
				renderer->init(window);
				needInit = false;
			}

			if (isSurfaceSzChanged) {
				if (!renderer->surfaceSzChanged(surfaceWidth, surfaceHeight)) {
					ServiceLocator::getLogger().loge("Corium3DEngine", "surfaceSzChanged failed."); //TODO: handle failure in the game loop thread			
					eglMutex.unlock();
					return false;
				}
			
				corium3DEngineOnlineCallback();
			}
		
			processInput();
			double current = ServiceLocator::getTimer().getCurrentTime();
			double elapsed = current - previous;
			previous = current;
			lag += elapsed;
			while (lag >= SECS_PER_UPDATE) {
				update();
				lag -= SECS_PER_UPDATE;
			}
			bvh->refitBPsDueToUpdate();
			//resolveCollisions3D();
			resolveCollisions2D();

	#if !DEBUG
			renderer->render(lag);
	#else
			if (!renderer->render(lag)) {
				ServiceLocator::getLogger().loge("Corium3DEngine", "render failed."); //TODO: handle failure in the GameLoop thread			
				eglMutex.unlock();
				return false;
			}
	#endif
		}	
		eglMutex.unlock();
		return true;
	}

	bool Corium3DEngine::Corium3DEngineImpl::canLoopContinue() {
		return !((isPaused || !hasFocus || !hasSurface || !isSurfaceSzKnown) && isGameOn);
	}

	void Corium3DEngine::Corium3DEngineImpl::unloadScene() {
		if (!isSceneLoaded)
			return;
	
		delete[] cursorInputCallbacks;
		delete[] keyboardInputEndCallbacks;
		delete[] keyboardInputStartCallbacks;

		delete cameraAPI;
		delete cameraApiImpl;

		delete stateUpdatersIt;	
		delete stateUpdatersPool;
		delete physicsEngine;
		delete bvh;	

		delete collisionPrimitivesFactory;
		for (unsigned int modelIdx = 0; modelIdx < sceneModelsNr; modelIdx++) {
			delete modelsInstancesIdxPools[modelIdx];		
			delete[] gameLmnts[modelIdx];	
			for (unsigned int collidedWithModelIdx = 0; collidedWithModelIdx < modelsInstancesNrsMaxima[modelIdx]; collidedWithModelIdx++)
				delete[] proximityHandlingMethods[modelIdx][collidedWithModelIdx];
			delete[] proximityHandlingMethods[modelIdx];
			delete[] onRayHitCallbacks[modelIdx];
		}
		delete[] modelsInstancesIdxPools;
		delete[] onRayHitCallbacks;
		delete[] proximityHandlingMethods;
		delete[] modelsInstancesNrsMaxima;
		delete[] gameLmnts;
					
		delete[] modelsPrimalBoundingSpheres;
		delete[] modelsPrimalAABB3Ds;
		delete[] modelsPrimalCollisionVolumesPtrs;				
		delete[] modelsPrimalAABB2Ds;
		delete[] modelsPrimalCollisionPerimetersPtrs;

		renderer->unloadScene();
	}

	void Corium3DEngine::Corium3DEngineImpl::processInput() {
		for (unsigned int currUpdateInputIdx = 0; currUpdateInputIdx < currUpdateInputsNr; currUpdateInputIdx++) {
			InputEvent const& inputEvent = inputEventsBuffer[currUpdateInputIdx];
			if (inputEvent.keyboardInputCallback)
				inputEvent.keyboardInputCallback(inputEvent.inputTimeStamp);
			else if (!guis[0]->select(inputEvent.cursorPos.x, inputEvent.cursorPos.y) && inputEvent.cursorInputCallback)
				inputEvent.cursorInputCallback(inputEvent.inputTimeStamp, inputEvent.cursorPos);
		}
		currUpdateInputsNr = 0;
	}

	void Corium3DEngine::Corium3DEngineImpl::update() {
		physicsEngine->update();
	}

	void Corium3DEngine::Corium3DEngineImpl::resolveCollisions3D() {
		BVH::CollisionsData<glm::vec3> const& collisionsData3D = bvh->getCollisionsData3D();
		doResolveCollisions<glm::vec3>(collisionsData3D);
	}

	void Corium3DEngine::Corium3DEngineImpl::resolveCollisions2D() {
		BVH::CollisionsData<glm::vec2> const& collisionsData2D = bvh->getCollisionsData2D();
		doResolveCollisions<glm::vec2>(collisionsData2D);
	}

	template <class V>
	void Corium3DEngine::Corium3DEngineImpl::doResolveCollisions(BVH::CollisionsData<V> const& collisionsData) {	
		GameLmnt::ProximityHandlingMethod proximityHandlingMethod;
		for (unsigned int collisionDataIdx = 0; collisionDataIdx < collisionsData.collisionsNr; collisionDataIdx++) {
			BVH::CollisionData<V>& collisionData = collisionsData.collisionsDataBuffer[collisionDataIdx];
			if (proximityHandlingMethod = proximityHandlingMethods[collisionData.modelIdx1][collisionData.instanceIdx1][collisionData.modelIdx2].collisionCallback)
				proximityHandlingMethod(gameLmnts[collisionData.modelIdx1][collisionData.instanceIdx1], gameLmnts[collisionData.modelIdx2][collisionData.instanceIdx2]);
			if (proximityHandlingMethod = proximityHandlingMethods[collisionData.modelIdx2][collisionData.instanceIdx2][collisionData.modelIdx1].collisionCallback)
				proximityHandlingMethod(gameLmnts[collisionData.modelIdx2][collisionData.instanceIdx2], gameLmnts[collisionData.modelIdx1][collisionData.instanceIdx1]);
		}

		for (unsigned int detachmentDataIdx = 0; detachmentDataIdx < collisionsData.detachmentsNr; detachmentDataIdx++) {
			BVH::CollisionData<V>& detachmentData = collisionsData.detachmentsDataBuffer[detachmentDataIdx];
			if (proximityHandlingMethod = proximityHandlingMethods[detachmentData.modelIdx1][detachmentData.instanceIdx1][detachmentData.modelIdx2].detachmentCallback)
				proximityHandlingMethod(gameLmnts[detachmentData.modelIdx1][detachmentData.instanceIdx1], gameLmnts[detachmentData.modelIdx2][detachmentData.instanceIdx2]);
			if (proximityHandlingMethod = proximityHandlingMethods[detachmentData.modelIdx2][detachmentData.instanceIdx2][detachmentData.modelIdx1].detachmentCallback)
				proximityHandlingMethod(gameLmnts[detachmentData.modelIdx2][detachmentData.instanceIdx2], gameLmnts[detachmentData.modelIdx1][detachmentData.instanceIdx1]);
		}
	}

	/*
	void* Corium3DEngine::Corium3DEngineImpl::loopThreadStarter(void* corium3DEngineImpl) {
		return (void*)((Corium3DEngine::Corium3DEngineImpl*)corium3DEngineImpl)->loop();
	}	
	
	bool Corium3DEngine::Corium3DEngineImpl::loopThreadStarter() {
		return loop();
	}
	*/	

	Corium3DEngine::GameLmnt::GameLmnt(Corium3DEngine& corium3DEngine,
									   unsigned int components, 
									   StateUpdater stateUpdater,
									   OnMovementMadeCallback onMovementMadeCallback,
									   unsigned int modelIdx,
									   Transform3D const* initTransform,
									   float initCollisionPerimeterRot,
									   ProximityHandlingMethods* proximityHandlingMethods,
									   OnRayHit onRayHitCallback) :
		gameLmntImpl(new GameLmntImpl(*this, *corium3DEngine.corium3DEngineImpl, components, stateUpdater, onMovementMadeCallback, modelIdx, initTransform, initCollisionPerimeterRot, proximityHandlingMethods, onRayHitCallback)) {}

	Corium3DEngine::GameLmnt::~GameLmnt() 
	{	
		delete gameLmntImpl;
	}	

	Corium3DEngine::GameLmnt::GraphicsAPI* Corium3DEngine::GameLmnt::accessGraphicsAPI() {
		return gameLmntImpl->accessGraphicsAPI();
	}

	Corium3DEngine::GameLmnt::MobilityAPI* Corium3DEngine::GameLmnt::accessMobilityAPI() {
		return gameLmntImpl->accessMobilityAPI();
	}


	//TODO: Add input correctness test under an #if DEBUG 
	Corium3DEngine::GameLmnt::GameLmntImpl::GameLmntImpl(GameLmnt& _owningGameLmnt, Corium3DEngineImpl& _corium3DEngineImpl,
										 				 unsigned int components,
									 					 StateUpdater stateUpdater,
								 						 OnMovementMadeCallback onMovementMadeCallback,
														 unsigned int modelIdx,
														 Transform3D const* initTransform,
														 float initCollisionPerimeterRot,
														 ProximityHandlingMethods* proximityHandlingMethods,
														 OnRayHit onRayHitCallback) :
			owningGameLmnt(_owningGameLmnt), corium3DEngineImpl(_corium3DEngineImpl) {		
		modelIdx = corium3DEngineImpl.modelSceneModelIdxsMap[modelIdx];
		components |= corium3DEngineImpl.staticModelsNr <= modelIdx ? Component::Mobility : 0;
		
		std::complex<float> initCollisionPerimeterRotComplex;
		Transform3D initTransformNormed;
		if (initTransform) {
			initTransformNormed = { initTransform->translate, initTransform->scale, normalize(initTransform->rot) };			
			if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx])
				initCollisionPerimeterRotComplex = rotQuatToRotComplex(initTransformNormed.rot);
		}	
		
		//if (components & Component::State)
		//	stateUpdater = corium3DEngineImpl.stateUpdatersPool->acquire(stateUpdater);

		if (components & Component::Mobility) {
			if (onMovementMadeCallback == NULL) {
				PhysicsEngine::OnMovementMadeCallback3D listener3D[1] = { std::bind(&GameLmntImpl::updateBvhNodeBVs, this, std::placeholders::_1) };
				if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx]) {								
					PhysicsEngine::OnMovementMadeCallback2D listener2D[1] = { std::bind(&GameLmntImpl::updateBvhNodeBPs, this, std::placeholders::_1) };
					mobilityInterface = corium3DEngineImpl.physicsEngine->addMobileGameLmnt(initTransformNormed, listener3D, 1, initCollisionPerimeterRotComplex, listener2D, 1);
				}
				else 				
					mobilityInterface = corium3DEngineImpl.physicsEngine->addMobileGameLmnt(initTransformNormed, listener3D, 1);			
			}
			else {
				PhysicsEngine::OnMovementMadeCallback3D listeners3D[2] = { onMovementMadeCallback , std::bind(&GameLmntImpl::updateBvhNodeBVs, this, std::placeholders::_1) };
				if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx]) {				
					PhysicsEngine::OnMovementMadeCallback2D listener2D[1] = { std::bind(&GameLmntImpl::updateBvhNodeBPs, this, std::placeholders::_1) };
					mobilityInterface = corium3DEngineImpl.physicsEngine->addMobileGameLmnt(initTransformNormed, listeners3D, 2, initCollisionPerimeterRotComplex, listener2D, 1);
				}
				else
					mobilityInterface = corium3DEngineImpl.physicsEngine->addMobileGameLmnt(initTransformNormed, listeners3D, 2);				
			}
			mobilityAPI = new MobilityAPI(*this);
		}			

		if (components & Component::Graphics) {			
			instanceIdx = corium3DEngineImpl.modelsInstancesIdxPools[modelIdx]->acquire();

			collisionVolume = static_cast<CollisionVolume*>(corium3DEngineImpl.collisionPrimitivesFactory->genCollisionPrimitive<glm::vec3>(*(corium3DEngineImpl.modelsPrimalCollisionVolumesPtrs[modelIdx]), initTransformNormed));
			if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx])
				collisionPerimeter = static_cast<CollisionPerimeter*>(corium3DEngineImpl.collisionPrimitivesFactory->genCollisionPrimitive<glm::vec2>(*(corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx]), initTransformNormed));							

			if (components & Component::Mobility) {						
				mobileGameLmntBvhDataNode3D = corium3DEngineImpl.bvh->insert(AABB3DRotatable::calcTransformedAABB(corium3DEngineImpl.modelsPrimalAABB3Ds[modelIdx], initTransformNormed),				
					BoundingSphere::calcTransformedBoundingSphere(corium3DEngineImpl.modelsPrimalBoundingSpheres[modelIdx], initTransformNormed),
																  modelIdx, instanceIdx, *collisionVolume, *mobilityInterface);
				if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx])
					mobileGameLmntBvhDataNode2D = corium3DEngineImpl.bvh->insert(AABB2DRotatable::calcTransformedAABB(corium3DEngineImpl.modelsPrimalAABB2Ds[modelIdx], Transform2D({ initTransformNormed.translate, initTransformNormed.scale, initCollisionPerimeterRotComplex })), modelIdx, instanceIdx, *collisionPerimeter, *mobilityInterface);
			}
			else {						
				staticGameLmntBvhDataNode3D = corium3DEngineImpl.bvh->insert(AABB3DRotatable::calcTransformedAABB(corium3DEngineImpl.modelsPrimalAABB3Ds[modelIdx], initTransformNormed),
					BoundingSphere::calcTransformedBoundingSphere(corium3DEngineImpl.modelsPrimalBoundingSpheres[modelIdx], initTransformNormed),
					modelIdx, instanceIdx, *collisionVolume);
				if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx])
					staticGameLmntBvhDataNode2D = corium3DEngineImpl.bvh->insert(AABB2DRotatable::calcTransformedAABB(corium3DEngineImpl.modelsPrimalAABB2Ds[modelIdx], Transform2D({ initTransformNormed.translate, initTransformNormed.scale, initCollisionPerimeterRotComplex })), modelIdx, instanceIdx, *collisionPerimeter);
		
				corium3DEngineImpl.renderer->setStaticModelInstanceTransform(modelIdx, instanceIdx, glm::translate(initTransformNormed.translate) * glm::mat4_cast(initTransformNormed.rot) * glm::scale(initTransformNormed.scale));
			}		
			for (unsigned int otherModelIdx = 0; otherModelIdx < corium3DEngineImpl.sceneModelsNr; otherModelIdx++)
				corium3DEngineImpl.proximityHandlingMethods[modelIdx][instanceIdx][otherModelIdx] = proximityHandlingMethods[otherModelIdx];
			corium3DEngineImpl.onRayHitCallbacks[modelIdx][instanceIdx] = onRayHitCallback;
			corium3DEngineImpl.gameLmnts[modelIdx][instanceIdx] = &owningGameLmnt;
			//instanceAnimationInterface = corium3DEngineImpl.renderer->activateAnimation(modelIdx, instanceIdx);
			graphicsAPI = new GraphicsAPI(*this);
		}	
		
		componentsFlag = components;
	}

	Corium3DEngine::GameLmnt::GameLmntImpl::~GameLmntImpl() {		
		if (componentsFlag & Component::State)
			corium3DEngineImpl.stateUpdatersPool->release(stateUpdater);	

		if (componentsFlag & Component::Mobility) {
			delete mobilityAPI;
			corium3DEngineImpl.physicsEngine->removeMobileGameLmnt(mobilityInterface);
		}

		if (componentsFlag & Component::Graphics) {
			delete graphicsAPI;
			//corium3DEngineImpl.renderer->deactivateAnimation(instanceAnimationInterface);		
			if (componentsFlag & Component::Mobility) {
				corium3DEngineImpl.bvh->remove(mobileGameLmntBvhDataNode3D);
				if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx])
					corium3DEngineImpl.bvh->remove(mobileGameLmntBvhDataNode2D);
			}
			else {
				corium3DEngineImpl.bvh->remove(staticGameLmntBvhDataNode3D);
				if (corium3DEngineImpl.modelsPrimalCollisionPerimetersPtrs[modelIdx])
					corium3DEngineImpl.bvh->remove(staticGameLmntBvhDataNode2D);
			}		
			corium3DEngineImpl.collisionPrimitivesFactory->destroyCollisionPrimitive(collisionVolume);
			corium3DEngineImpl.collisionPrimitivesFactory->destroyCollisionPrimitive(collisionPerimeter);

			for (unsigned int otherModelIdx = 0; otherModelIdx < corium3DEngineImpl.sceneModelsNr; otherModelIdx++)
				corium3DEngineImpl.proximityHandlingMethods[modelIdx][instanceIdx][otherModelIdx] = { NULL, NULL };				
			corium3DEngineImpl.gameLmnts[modelIdx][instanceIdx] = NULL;		
			corium3DEngineImpl.modelsInstancesIdxPools[modelIdx]->release(instanceIdx);
		}	
	}

	void Corium3DEngine::GameLmnt::GameLmntImpl::updateBvhNodeBVs(Transform3DUS const& transformDelta) {
		corium3DEngineImpl.bvh->updateNodeBPs(mobileGameLmntBvhDataNode3D, transformDelta);	
	}

	void Corium3DEngine::GameLmnt::GameLmntImpl::updateBvhNodeBPs(Transform2DUS const& transformDelta) {
		corium3DEngineImpl.bvh->updateNodeBPs(mobileGameLmntBvhDataNode2D, transformDelta);
	}

	void Corium3DEngine::GameLmnt::GameLmntImpl::changeVerticesColors(unsigned int meshIdx, unsigned int colorsArrIdx) {
		corium3DEngineImpl.renderer->changeModelInstanceColorsArr(modelIdx, instanceIdx, meshIdx, colorsArrIdx);
	}

	void Corium3DEngine::GameLmnt::GameLmntImpl::changeAnimation(unsigned int animationIdx) {
		instanceAnimationInterface->start(animationIdx);
	}

	void Corium3DEngine::GameLmnt::GraphicsAPI::changeVerticesColors(unsigned int meshIdx, unsigned int colorsArrIdx) {
		gameLmntImpl.changeVerticesColors(meshIdx, colorsArrIdx);
	}

	void Corium3DEngine::GameLmnt::GraphicsAPI::changeAnimation(unsigned int animationIdx) {
		gameLmntImpl.changeAnimation(animationIdx);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::translate(glm::vec3 const& translate) {
		gameLmntImpl.translate(translate);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::scale(float scaleFactor) {
		gameLmntImpl.scale(scaleFactor);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::rot(float rot, glm::vec3 const& rotAx) {
		gameLmntImpl.rot(rot, rotAx);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::rot(glm::quat const& rot) {
		gameLmntImpl.rot(rot);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::setLinVel(glm::vec3 const& linVel) {
		gameLmntImpl.setLinVel(linVel);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::setAngVel(float angVelMag, glm::vec3 const& angVelAx) {
		gameLmntImpl.setAngVel(angVelMag, angVelAx);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::setLinAccel(glm::vec3 const& linAccel) {
		gameLmntImpl.setLinAccel(linAccel);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::setAngAccel(glm::vec3 const& angAccel) {
		gameLmntImpl.setAngAccel(angAccel);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::setLinVelX(float x) { 
		gameLmntImpl.setLinVelX(x);
	}
	void Corium3DEngine::GameLmnt::MobilityAPI::setLinVelY(float y) { 
		gameLmntImpl.setLinVelY(y);
	}

	void Corium3DEngine::GameLmnt::MobilityAPI::setLinVelZ(float z) {
		gameLmntImpl.setLinVelZ(z);
	}

	void Corium3DEngine::GuiAPI::show() {
		guiApiImpl.show();
	}

	void Corium3DEngine::GuiAPI::hide() {
		guiApiImpl.hide();
	}

	Corium3DEngine::GuiAPI::GuiAPI(GuiApiImpl& _guiApiImpl) : guiApiImpl(_guiApiImpl) {}

	Corium3DEngine::GuiAPI::ControlAPI::ControlAPI(ControlApiImpl& _controlApiImpl) : controlApiImpl(_controlApiImpl) {}

	Corium3DEngine::GuiAPI::TxtControlAPI::TxtControlAPI(TxtControlApiImpl& _txtControlApiImpl) : 
		ControlAPI(_txtControlApiImpl), txtControlApiImpl(_txtControlApiImpl) {}

	Corium3DEngine::GuiAPI::ControlAPI& Corium3DEngine::GuiAPI::accessControl(unsigned int controlIdx) {
		return guiApiImpl.accessControl(controlIdx);
	}

	Corium3DEngine::GuiAPI::TxtControlAPI& Corium3DEngine::GuiAPI::accessTxtControl(unsigned int controlIdx) {
		return guiApiImpl.accessTxtControl(controlIdx);
	}

	void Corium3DEngine::GuiAPI::ControlAPI::assignSelectCallback(OnSelectCallback onSelectCallback) {
		controlApiImpl.assignSelectCallback(onSelectCallback);
	}

	void Corium3DEngine::GuiAPI::ControlAPI::updateGraphics(unsigned int* quadsIdxs, unsigned int quadsNr, Rect* quads, Rect* uvs) {
		controlApiImpl.updateGraphics(quadsIdxs, quadsNr, quads, uvs);
	}

	/*
	void Corium3DEngine::GuiAPI::ControlAPI::updateQuads(unsigned int* quadsIdxs, Rect const* quad) {
		controlApiImpl.updateQuads(quadsIdxs, quad);
	}

	void Corium3DEngine::GuiAPI::ControlAPI::updateUvs(unsigned int* quadsIdxs, Rect const* uvs) {
		controlApiImpl.updateUvs(quadsIdxs, uvs);
	}
	*/

	void Corium3DEngine::GuiAPI::TxtControlAPI::setTxt(unsigned int number) {
		txtControlApiImpl.setTxt(number);
	}

	/*
	void Corium3DEngine::GuiAPI::TxtControlAPI::setTxt(char const* txt) {
		txtControlApiImpl.setTxt(txt);
	}
	*/

	Corium3DEngine::GuiAPI::GuiApiImpl::GuiApiImpl(GUI& _gui, unsigned int _imgsControlsNr, unsigned int _txtControlsNr) :
		gui(_gui), imgsControlsNr(_imgsControlsNr), controlsAPIs(new Corium3DEngine::GuiAPI::ControlAPI*[imgsControlsNr]),
		controlsApisImpls(new Corium3DEngine::GuiAPI::ControlAPI::ControlApiImpl*[imgsControlsNr]),
		txtControlsNr(_txtControlsNr), txtControlsAPIs(new Corium3DEngine::GuiAPI::TxtControlAPI*[txtControlsNr]),
		txtControlsApisImpls(new Corium3DEngine::GuiAPI::TxtControlAPI::TxtControlApiImpl*[txtControlsNr]) {
		for (unsigned int imgsControlApiIdx = 0; imgsControlApiIdx < imgsControlsNr; imgsControlApiIdx++) {
			controlsApisImpls[imgsControlApiIdx] =
				new Corium3DEngine::GuiAPI::TxtControlAPI::ControlApiImpl(gui.accessTxtControl(imgsControlApiIdx));
			controlsAPIs[imgsControlApiIdx] = new Corium3DEngine::GuiAPI::ControlAPI(*(controlsApisImpls[imgsControlApiIdx]));
		}

		for (unsigned int txtControlApiIdx = 0; txtControlApiIdx < txtControlsNr; txtControlApiIdx++) {
			txtControlsApisImpls[txtControlApiIdx] = new Corium3DEngine::GuiAPI::TxtControlAPI::TxtControlApiImpl(gui.accessTxtControl(txtControlApiIdx));
			txtControlsAPIs[txtControlApiIdx] = new Corium3DEngine::GuiAPI::TxtControlAPI(*(txtControlsApisImpls[txtControlApiIdx]));
		}
	}

	Corium3DEngine::GuiAPI::GuiApiImpl::~GuiApiImpl() {
		for (unsigned int controlApiIdx = 0; controlApiIdx < imgsControlsNr + txtControlsNr; controlApiIdx++) {
			delete controlsAPIs[controlApiIdx];
			delete controlsApisImpls[controlApiIdx];
		}
		delete[] controlsAPIs;
		delete[] controlsApisImpls;

		for (unsigned int txtControlApiIdx = 0; txtControlApiIdx < txtControlsNr; txtControlApiIdx++) {
			delete txtControlsAPIs[txtControlApiIdx];
			delete txtControlsApisImpls[txtControlApiIdx];
		}
		delete[] txtControlsAPIs;
		delete[] txtControlsApisImpls;
	}

	Corium3DEngine::CameraAPI::CameraAPI(CameraApiImpl& _cameraApiImpl) : cameraApiImpl(_cameraApiImpl) {}

	void Corium3DEngine::CameraAPI::translate(glm::vec3 const& translation) {
		cameraApiImpl.translate(translation);
	}

	void Corium3DEngine::CameraAPI::rotate(float rotAng, glm::vec3 const& rotAx) {
		cameraApiImpl.rotate(rotAng, rotAx);
	}

	void Corium3DEngine::CameraAPI::pan(glm::vec2 const& panVec) {
		cameraApiImpl.pan(panVec);
	}

	void Corium3DEngine::CameraAPI::rotAroundViewportContainedAx(float rotAng, glm::vec2 const& rotAx) {
		cameraApiImpl.rotAroundViewportContainedAx(rotAng, rotAx);
	}

	void Corium3DEngine::CameraAPI::translateInViewDirection(float translation) {
		cameraApiImpl.translateInViewDirection(translation);
	}

	void Corium3DEngine::CameraAPI::zoom(float factor) {
		cameraApiImpl.zoom(factor);
	}

	bool Corium3DEngine::CameraAPI::shootRay(glm::vec2 const& cursorPos) {
		return cameraApiImpl.shootRay(cursorPos);
	}

	void Corium3DEngine::CameraAPI::CameraApiImpl::translate(glm::vec3 translation) {
		renderer.translateCamera(translation);
	}

	void Corium3DEngine::CameraAPI::CameraApiImpl::rotate(float rotAng, glm::vec3 rotAx) {
		renderer.rotCamera(rotAng, rotAx);
	}

	void Corium3DEngine::CameraAPI::CameraApiImpl::pan(glm::vec2 const& panVec) {
		renderer.panCamera(panVec);
	}

	void Corium3DEngine::CameraAPI::CameraApiImpl::rotAroundViewportContainedAx(float rotAng, glm::vec2 const& rotAx) {
		renderer.rotCameraAroundViewportContainedAx(rotAng, rotAx);
	}

	void Corium3DEngine::CameraAPI::CameraApiImpl::translateInViewDirection(float translation) {
		renderer.translateCameraInViewDirection(translation);
	}

	void Corium3DEngine::CameraAPI::CameraApiImpl::zoom(float factor) {
		renderer.zoom(factor);
	}

	bool Corium3DEngine::CameraAPI::CameraApiImpl::shootRay(glm::vec2 const& cursorPos) {
		glm::vec3 cameraUp = renderer.getCameraUp();	
		glm::vec3 cameraLookDirection = renderer.getCameraLookDirection();
		glm::vec3 cameraRight = glm::cross(cameraLookDirection, cameraUp);	
		// TODO: Move renderer.getWinWidth(), renderer.getWinHeight(), renderer.getViewScreenWidth(), renderer.getViewScreenHeight() and renderer.getFrustumNear() to the constructor (After adding a camera class to Renderer)	
		glm::vec2 cursorPosVirtualScreenCoords = (cursorPos/glm::vec2(renderer.getWinWidth(), renderer.getWinHeight()) - 0.5f) * glm::vec2(renderer.getVirtualScreenWidth(), renderer.getVirtualScreenHeight());
		const BVH::RayCollisionData rayCollisionData = corium3DEngineImpl.bvh->getRayCollisionData(renderer.getCameraPos(), cursorPosVirtualScreenCoords.x*cameraRight + cursorPosVirtualScreenCoords.y*cameraUp + renderer.getFrustumNear()*cameraLookDirection);
		if (rayCollisionData.hasCollided) {
			corium3DEngineImpl.onRayHitCallbacks[rayCollisionData.modelIdx][rayCollisionData.instanceIdx]();
			ServiceLocator::getLogger().logd("shoot ray", "ray hit.");
			return true;
		}
		else {
			ServiceLocator::getLogger().logd("shoot ray", "ray missed.");
			return false;
		}
	}

} //namespace Corium3D 