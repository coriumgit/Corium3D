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
		interface class IScene {
			interface class ISceneModelInstance {				
				void translate(Media3D::Vector3D^ translation);
				void setTranslation(Media3D::Vector3D^ translation);
				Media3D::Vector3D^ getTranslation();
				void scale(Media3D::Vector3D^ scale);
				void setScale(Media3D::Vector3D^ scaleFactor);			
				void rotate(Media3D::Vector3D^ rotAx, float ang);
				void setRotation(Media3D::Vector3D^ ax, float ang);
				void highlight();
				void dim();
				void show();
				void hide();
				void release();				
			};

			// in:
			//	x -> cursor x coord on selection
			//	y -> cursor y coord on selection			
			delegate void SelectionHandler(float x, float y);

			void activate();
			ISceneModelInstance^ createModelInstance(unsigned int modelID, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleFactorInit, Media3D::Vector3D^ rotAxInit, float rotAngInit, SelectionHandler^ selectionHandler);
			void panCamera(float x, float y);
			void rotateCamera(float x, float y);
			void zoomCamera(float amount);
			Media3D::Point3D^ getCameraPos();
			float getCameraFOV();
			bool cursorSelect(float x, float y);
			Media3D::Vector3D^ screenVecToWorldVec(float x, float y);
			Media3D::Vector3D^ cursorPosToRayDirection(float x, float y);
			void release();
		};

		DxVisualizer(float fov, float nearZ, float farZ);
		~DxVisualizer();
		void addModel(array<Media3D::Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, Media::Color modelColor, Media3D::Point3D^ boundingSphereCenter, float boundingSphereRadius, PrimitiveTopology primitiveTopology, bool doDepthTest, [System::Runtime::InteropServices::Out] UINT% modelIDOut);
		void updateModelData(unsigned int modelID, array<Media3D::Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, Media::Color modelColor, PrimitiveTopology primitiveTopology);
		void removeModel(unsigned int modelID);
		IScene^ createScene();				
		void initRenderer(System::IntPtr surface);
		void render();
		void captureFrame();

	private:		
		ref class Scene : public IScene {
		public:
			ref class SceneModelInstance : public IScene::ISceneModelInstance {
			public:
				SceneModelInstance(DxRenderer::Scene::SceneModelInstance* sceneModelInstanceRef);
				void translate(Media3D::Vector3D^ translation) override;
				void setTranslation(Media3D::Vector3D^ translation) override;
				Media3D::Vector3D^ getTranslation() override;
				void scale(Media3D::Vector3D^ scale) override;
				void setScale(Media3D::Vector3D^ scaleFactor) override;
				void rotate(Media3D::Vector3D^ ax, float ang) override;
				void setRotation(Media3D::Vector3D^ ax, float ang) override;
				void highlight() override;		
				void dim() override;
				void show() override;
				void hide() override;
				void release() override;

			private:				
				DxRenderer::Scene::SceneModelInstance* sceneModelInstanceRef;				
			};
			
			Scene(DxRenderer::Scene* sceneRef);
			void activate() override;
			IScene::ISceneModelInstance^ createModelInstance(unsigned int modelID, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleFactorInit, Media3D::Vector3D^ rotAxInit, float rotAngInit, IScene::SelectionHandler^ selectionHandler) override;
			void panCamera(float x, float y) override;
			void rotateCamera(float x, float y) override;
			void zoomCamera(float amount) override;
			Media3D::Point3D^ getCameraPos() override;
			float getCameraFOV() override;
			bool cursorSelect(float x, float y) override;
			Media3D::Vector3D^ screenVecToWorldVec(float x, float y) override;
			Media3D::Vector3D^ cursorPosToRayDirection(float x, float y) override;
			void release() override;

		private:						
			DxRenderer::Scene* sceneRef;
		};		

		static void marshalModelData(array<Media3D::Point3D>^ modelVertices, Media::Color modelColor, std::vector<DxRenderer::VertexData>& verticesDataMarshaled,
									 array<unsigned short>^ modelVertexIndices, std::vector<WORD>& vertexIndicesMarshaled,
									 Media3D::Point3D^ boundingSphereCenter, DirectX::XMFLOAT3& boundingSphereCenterMarshaled,
									 PrimitiveTopology primitiveTopology, D3D_PRIMITIVE_TOPOLOGY& primitiveTopologyMarshaled);

		DxRenderer* renderer;
	};
}

