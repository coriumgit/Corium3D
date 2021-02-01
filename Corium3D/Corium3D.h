//
// Created by omer on 30/12/2017.
// REMINDER: will govern everything and evolve according to current demand

#pragma once

#include "SystemDefs.h"
#include "InputsIDs.h"
#include "TransformsStructs.h"
#include "ServiceLocator.h"

#include <functional>
#include <vector>
#include <string>

namespace Corium3D {

	class Corium3DEngine {
	public:
		class GameLmnt;
		class GuiAPI;
		class CameraAPI;

		typedef std::function<void(double timeStamp)> KeyboardInputCallback;
		typedef std::function<void(double timeStamp, glm::vec2 const& cursorPos)> CursorInputCallback;
		typedef std::function<void(void)> Corium3DEngineOnlineCallback;		

		struct CallbackPtrs {
			//TODO: separate this:
			Corium3DEngineOnlineCallback& corium3DEngineOnlineCallback;
			// from these:
			std::function<void(KeyboardInputID inputID)>& systemKeyboardInputStartCallbackPtr;
			std::function<void(KeyboardInputID inputID)>& systemKeyboardInputEndCallbackPtr;
			std::function<void(CursorInputID inputID, glm::vec2 const& cursorPos)>& systemCursorInputCallbackPtr;
		};

		struct AssetsFilesFullPaths {
			std::string modelsScenesFullPath;
			const char* txtTexAtlasPath; //const char* guisDescsPath			
			const char** vertexShadersFullPaths;
			const char** fragShadersFullPaths;
			unsigned int shadersNr;
		};				

		// REMINDER: system functions of CallbacksPtrs -> [out] parameter to be assigned functions for the system inputs callbacks to call	
		// TODO: separate corium3DEngineOnlineCallback from the [out] functions 
		Corium3DEngine(CallbackPtrs& callbacksPtrs, AssetsFilesFullPaths const& assetsFilesFullPaths);
		Corium3DEngine(Corium3DEngine const& corium3DEngine) = delete;
		~Corium3DEngine();
		void startLoop();
		void signalResume();
		void signalPause();
		void signalSurfaceCreated(Corium3DEngineNativeWindowType window);
		void signalSurfaceSzChanged(unsigned int width, unsigned int height);
		void signalSurfaceDestroyed();
		void signalWindowFocusChanged(bool hasFocus);
		void signalDetachedFromWindow();		
		void registerKeyboardInputStartCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback);
		void registerKeyboardInputEndCallback(KeyboardInputID inputId, KeyboardInputCallback inputCallback);
		void registerCursorInputCallback(CursorInputID inputId, CursorInputCallback inputCallback);		
		std::vector<std::vector<Transform3D>> loadScene(unsigned int sceneIdx);
		GuiAPI& accessGuiAPI(unsigned int guiIdx);
		CameraAPI& accessCameraAPI();

	private:
		class Corium3DEngineImpl;
		Corium3DEngineImpl* corium3DEngineImpl;		
		
		void systemKeyboardInputStartCallback(KeyboardInputID inputId);
		void systemKeyboardInputEndCallback(KeyboardInputID inputId);
		void systemCursorInputCallback(CursorInputID inputId, glm::vec2 const& cursorPos);
	};
	
	/*
	template<class THead, class ...T, class ArgsTuplesHead, class ...ArgsTuples>
	std::vector<std::vector<Corium3DEngine::GameLmnt*>> loadScene(unsigned int sceneIdx, std::vector<ArgsTuplesHead> argsHead, std::vector<ArgsTuples>... args) {
		loadSceneImpl(sceneIdx);

		Tuple<TArgs...> t = v[0];
		//
		unsigned int modelsNr = gameLmntsInitData.size();
		std::vector<std::vector<Corium3DEngine::GameLmnt*>> initGameLmnts(modelsNr);
		for (unsigned int modelIdx = 0; modelIdx < modelsNr; ++modelIdx) {
			unsigned int instancesNr = gameLmntsInitData[modelIdx].size();
			initGameLmnts[modelIdx].resize(instancesNr);
			for (unsigned int instanceIdx = 0; instanceIdx < instancesNr; ++instanceIdx)
				initGameLmnts[modelIdx][instanceIdx] = genGameLmnt(gameLmntsInitData[modelIdx][instanceIdx]);
		}

		return initGameLmnts;
		//
	}
	*/

	class Corium3DEngine::GameLmnt {
		friend class Corium3DEngine;

	public:
		class GraphicsAPI;
		class MobilityAPI;

		enum Component { State = 1, Graphics = 2, Mobility = 4 };

		typedef void (GameLmnt::* StateUpdater)();
		typedef std::function<void(Transform3D const&)> OnMovementMadeCallback;
		typedef std::function<void(GameLmnt*, GameLmnt*)> ProximityHandlingMethod;
		typedef std::function<bool()> OnRayHit;

		struct ProximityHandlingMethods {
			ProximityHandlingMethod collisionCallback = NULL;
			ProximityHandlingMethod detachmentCallback = NULL;
		};

		GraphicsAPI* accessGraphicsAPI();
		MobilityAPI* accessMobilityAPI();

	protected:
		GameLmnt(Corium3DEngine& corium3DEngine, 
				 unsigned int components, 
				 StateUpdater stateUpdater,
				 OnMovementMadeCallback onMovementMadeCallback,
				 unsigned int modelIdx,
				 Transform3D* initTransform,
				 float initCollisionPerimeterRot,
				 ProximityHandlingMethods* proximityHandlingMethods,
				 OnRayHit onRayHitCallback);
		~GameLmnt();

	private:
		class GameLmntImpl;

		GameLmntImpl* gameLmntImpl;		
	};

	class Corium3DEngine::GameLmnt::GraphicsAPI {
		friend GameLmnt;

	public:
		void changeVerticesColors(unsigned int meshIdx, unsigned int colorsArrIdx);
		void changeAnimation(unsigned int animationIdx);

	private:
		GameLmntImpl& gameLmntImpl;

		GraphicsAPI(GameLmntImpl& _gameLmntImpl) : gameLmntImpl(_gameLmntImpl) {}
		~GraphicsAPI() {}
	};

	class Corium3DEngine::GameLmnt::MobilityAPI {
		friend GameLmnt;

	public:
		void translate(glm::vec3 const& translate);
		void scale(glm::vec3 const& scale);
		void rot(float rot, glm::vec3 const& rotAx);
		void rot(glm::quat const& rot);
		void setLinVel(glm::vec3 const& _linVel);
		void setAngVel(float _angVelMag, glm::vec3 const& _angVelAx);
		void setLinAccel(glm::vec3 const& _linAccel);
		void setAngAccel(glm::vec3 const& angAccel);
		void setLinVelX(float x);
		void setLinVelY(float y);
		void setLinVelZ(float z);

	private:
		GameLmntImpl& gameLmntImpl;

		MobilityAPI(GameLmntImpl& _gameLmntImpl) : gameLmntImpl(_gameLmntImpl) {}
		~MobilityAPI() {}
	};

	class Corium3DEngine::GuiAPI {
		friend class Corium3DEngine::Corium3DEngineImpl;

	private:
		class GuiApiImpl;
		GuiApiImpl& guiApiImpl;

		GuiAPI(GuiApiImpl& guiApiImpl);

	public:
		class ControlAPI {
			friend class Corium3DEngine::GuiAPI::GuiApiImpl;

		public:
			typedef std::function<void(float x, float y)> OnSelectCallback;

			void assignSelectCallback(OnSelectCallback onSelectCallback);
			void updateGraphics(unsigned int* quadsIdxs, unsigned int quadsNr, Rect* quads, Rect* uvs);
			//void updateQuads(unsigned int* quadsIdxs, Rect const* quad);
			//void updateUvs(unsigned int* quadsIdxs, Rect const* uvs);

		protected:
			class ControlApiImpl;
			ControlApiImpl& controlApiImpl;

			ControlAPI(ControlApiImpl& _controlApiImpl);
		};

		class TxtControlAPI : public ControlAPI {
			friend class Corium3DEngine::GuiAPI::GuiApiImpl;

		public:
			void setTxt(unsigned int number);
			//void setTxt(char const* txt);

		private:
			class TxtControlApiImpl;
			TxtControlApiImpl& txtControlApiImpl;

			TxtControlAPI(TxtControlApiImpl& _txtControlImpl);
		};

		void show();
		void hide();
		ControlAPI& accessControl(unsigned int controlIdx);
		TxtControlAPI& accessTxtControl(unsigned int controlIdx);
	};
	
	class Corium3DEngine::CameraAPI {
		friend class Corium3DEngine::Corium3DEngineImpl;
	
	public:
		void translate(glm::vec3 const& translation);
		void rotate(float rotAng, glm::vec3 const& rotAx);
		void pan(glm::vec2 const& panVec);
		void rotAroundViewportContainedAx(float rotAng, glm::vec2& rotAx);
		void translateInViewDirection(float translation);
		void zoom(float factor);
		bool shootRay(glm::vec2 const& cursorPos);

	private:
		class CameraApiImpl;
		CameraApiImpl& cameraApiImpl;

		CameraAPI(CameraApiImpl& cameraApiImpl);
	};

} // Corium3D namespace