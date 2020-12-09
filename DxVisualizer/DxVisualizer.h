#pragma once

#include "DxRenderer.h"
#include "Camera.h"

#include <d3d11.h>
#include <list>

namespace Media = System::Windows::Media;
namespace Media3D = System::Windows::Media::Media3D;
namespace Collections = System::Collections::Generic;

namespace CoriumDirectX {

	public enum class PrimitiveTopology { LINELIST, TRIANGLELIST };

	public ref class DxVisualizer {
	public:	
		interface class IScene : System::IDisposable {
			interface class ISceneModelInstance : System::IDisposable {
				// SelectionHandler in:
				//	x -> cursor x coord on selection
				//	y -> cursor y coord on selection			
				delegate void SelectionHandler(float x, float y);

				Media3D::Vector3D^ getTranslation();
				void addToTransformGrp();
				void removeFromTransformGrp();
				void highlight();
				void dim();
				void show();
				void hide();		
			};								

			delegate void TranslationHandler(float x, float y, float z);
			delegate void ScaleHandler(float x, float y, float z);
			delegate void RotationHandler(float axX, float axY, float axZ, float ang);

			ref struct TransformCallbackHandlers {
				TranslationHandler^ translationHandler;
				ScaleHandler^ scaleHandler;
				RotationHandler^ rotationHandler;
			};

			void activate();
			ISceneModelInstance^ createModelInstance(unsigned int modelID, Media::Color instanceColorMask, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleFactorInit, Media3D::Vector3D^ rotAxInit, float rotAngInit, ISceneModelInstance::SelectionHandler^ selectionHandler);
			void transformGrpTranslate(Media3D::Vector3D^ translation);
			void transformGrpSetTranslation(Media3D::Vector3D^ translation);
			void transformGrpScale(Media3D::Vector3D^ scaleFactorQ);
			void transformGrpSetScale(Media3D::Vector3D^ scaleFactor);
			void transformGrpRotate(Media3D::Vector3D^ ax, float ang);
			void transformGrpSetRotation(Media3D::Vector3D^ ax, float ang);
			void panCamera(float x, float y);
			void rotateCamera(float x, float y);
			void zoomCamera(float amount);
			Media3D::Point3D^ getCameraPos();
			float getCameraFOV();
			bool cursorSelect(float x, float y);
			Media3D::Vector3D^ screenVecToWorldVec(float x, float y);
			Media3D::Vector3D^ cursorPosToRayDirection(float x, float y);
		};

		delegate void OnMouseMoveCallback(float cursorX, float cursorY);
		delegate void OnMouseUpCallback();

		ref struct MouseCallbacks {
			OnMouseMoveCallback^ onMouseMoveCallback;
			OnMouseUpCallback^ onMouseUpCallback;
		};

		DxVisualizer(float fov, float nearZ, float farZ);
		~DxVisualizer();
		void addModel(array<Media3D::Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, Media::Color modelColor, Media3D::Point3D^ boundingSphereCenter, float boundingSphereRadius, PrimitiveTopology primitiveTopology, [System::Runtime::InteropServices::Out] UINT% modelIDOut);
		void updateModelData(unsigned int modelID, array<Media3D::Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, Media::Color modelColor, PrimitiveTopology primitiveTopology);
		void removeModel(unsigned int modelID);
		IScene^ createScene(IScene::TransformCallbackHandlers^ transformCallbackHandlers, [System::Runtime::InteropServices::Out] DxVisualizer::MouseCallbacks^% mouseCallbacks);
		void initRenderer(System::IntPtr surface);
		void render();
		void captureFrame();

	private:		
		ref class Scene : public IScene {
		public:
			ref class SceneModelInstance : public IScene::ISceneModelInstance {
			public:
				SceneModelInstance(DxRenderer::Scene::SceneModelInstance* sceneModelInstanceRef);
				~SceneModelInstance();
				!SceneModelInstance();
				virtual Media3D::Vector3D^ getTranslation() = ISceneModelInstance::getTranslation;
				virtual void highlight() = ISceneModelInstance::highlight;
				virtual void dim() = ISceneModelInstance::dim;
				virtual void show() = ISceneModelInstance::show;
				virtual void hide() = ISceneModelInstance::hide;				
				virtual void addToTransformGrp() = ISceneModelInstance::addToTransformGrp;
				virtual void removeFromTransformGrp() = ISceneModelInstance::removeFromTransformGrp;				

			private:								
				DxRenderer::Scene::SceneModelInstance* sceneModelInstanceRef;	
				bool isDisposed = false;
			};
			
			Scene(DxRenderer* renderer, DxRenderer::Scene::TransformCallbackHandlers const& transformCallbackHandlers, [System::Runtime::InteropServices::Out] DxVisualizer::MouseCallbacks^% mouseCallbacks);
			~Scene();
			!Scene();
			virtual void activate() = IScene::activate;
			virtual IScene::ISceneModelInstance^ createModelInstance(unsigned int modelID, Media::Color instanceColorMask, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleFactorInit, Media3D::Vector3D^ rotAxInit, float rotAngInit, IScene::ISceneModelInstance::SelectionHandler^ selectionHandler) = IScene::createModelInstance;
			virtual void transformGrpTranslate(Media3D::Vector3D^ translation) = IScene::transformGrpTranslate;
			virtual void transformGrpSetTranslation(Media3D::Vector3D^ translation) = IScene::transformGrpSetTranslation;
			virtual void transformGrpScale(Media3D::Vector3D^ scaleFactorQ) = IScene::transformGrpScale;
			virtual void transformGrpSetScale(Media3D::Vector3D^ scaleFactor) = IScene::transformGrpSetScale;
			virtual void transformGrpRotate(Media3D::Vector3D^ ax, float ang) = IScene::transformGrpRotate;
			virtual void transformGrpSetRotation(Media3D::Vector3D^ ax, float ang) = IScene::transformGrpSetRotation;
			virtual void panCamera(float x, float y) = IScene::panCamera;
			virtual void rotateCamera(float x, float y) = IScene::rotateCamera;
			virtual void zoomCamera(float amount) = IScene::zoomCamera;
			virtual Media3D::Point3D^ getCameraPos() = IScene::getCameraPos;
			virtual float getCameraFOV() = IScene::getCameraFOV;
			virtual bool cursorSelect(float x, float y) = IScene::cursorSelect;
			virtual Media3D::Vector3D^ screenVecToWorldVec(float x, float y) = IScene::screenVecToWorldVec;
			virtual Media3D::Vector3D^ cursorPosToRayDirection(float x, float y) = IScene::cursorPosToRayDirection;

		private:						
			DxRenderer::Scene* sceneRef;
			DxRenderer::MouseCallbacks* mouseCallbacksNative;
			bool isDisposed = false;

			void onMouseMove(float cursorPosX, float cursorPosY);
			void onMouseUp();
		};		

		static void marshalModelData(array<Media3D::Point3D>^ modelVertices, Media::Color modelColor, std::vector<DxRenderer::VertexData>& verticesDataMarshaled,
									 array<unsigned short>^ modelVertexIndices, std::vector<WORD>& vertexIndicesMarshaled,
									 Media3D::Point3D^ boundingSphereCenter, DirectX::XMFLOAT3& boundingSphereCenterMarshaled,
									 PrimitiveTopology primitiveTopology, D3D_PRIMITIVE_TOPOLOGY& primitiveTopologyMarshaled);		

		DxRenderer* renderer;		
	};
}