#pragma once

#include "DxRenderer.h"
#include "Model.h"
#include "Camera.h"

#include <d3d11.h>
#include <list>

namespace Media3D = System::Windows::Media::Media3D;
namespace Collections = System::Collections::Generic;

namespace CoriumDirectX {

	public enum class PrimitiveTopology { LINELIST, TRIANGLELIST };

	public ref class DxVisualizer {
	public:	
		interface class IScene {
			interface class ISceneModelInstance {				
				void translate(Media3D::Vector3D^ translation);
				void setTranslation(Media3D::Vector3D^  translation);
				void scale(Media3D::Vector3D^ scale);
				void setScale(Media3D::Vector3D^ scaleFactor);
				void rotate(Media3D::Vector3D^ rotAx, float ang);
				void setRotation(Media3D::Vector3D^ ax, float ang);
				void highlight();
				void release();				
			};

			delegate void SelectionHandler();

			void activate();
			ISceneModelInstance^ createModelInstance(unsigned int modelID, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleFactorInit, Media3D::Vector3D^ rotAxInit, float rotAngInit, SelectionHandler^ selectionHandler);
			void panCamera(float x, float y);
			void rotateCamera(float x, float y);
			void zoomCamera(float amount);
			Media3D::Vector3D^ getCameraPos();
			bool cursorSelect(float x, float y);
			Media3D::Vector3D^ cursorPosToRayDirection(float x, float y);
			void dimHighlightedInstance();
			void release();
		};

		DxVisualizer(float fov, float nearZ, float farZ);
		~DxVisualizer();
		void addModel(array<Media3D::Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, PrimitiveTopology primitiveTopology, [System::Runtime::InteropServices::Out] UINT% modelIDOut);
		void updateModelData(unsigned int modelID, array<Media3D::Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, PrimitiveTopology primitiveTopology);
		void removeModel(unsigned int modelID);
		IScene^ createScene();				
		void initRenderer(System::IntPtr surface);
		void render();

	private:		
		ref class Scene : public IScene {
		public:
			ref class SceneModelInstance : public IScene::ISceneModelInstance {
			public:
				SceneModelInstance(DxRenderer::Scene::SceneModelInstance* sceneModelInstanceRef);
				void translate(Media3D::Vector3D^ translation) override;
				void setTranslation(Media3D::Vector3D^ translation) override;
				void scale(Media3D::Vector3D^ scale) override;
				void setScale(Media3D::Vector3D^ scaleFactor) override;
				void rotate(Media3D::Vector3D^ ax, float ang) override;
				void setRotation(Media3D::Vector3D^ ax, float ang) override;
				void highlight() override;				
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
			Media3D::Vector3D^ getCameraPos() override;
			bool cursorSelect(float x, float y) override;
			Media3D::Vector3D^ cursorPosToRayDirection(float x, float y) override;
			void dimHighlightedInstance() override;
			void release() override;

		private:						
			DxRenderer::Scene* sceneRef;
		};		

		static void marshalModelData(array<Media3D::Point3D>^ modelVertices, std::vector<DxRenderer::VertexData>& verticesDataMarshaled, 
									 array<unsigned short>^ modelVertexIndices, std::vector<WORD>& vertexIndicesMarshaled,
									 PrimitiveTopology primitiveTopology, D3D_PRIMITIVE_TOPOLOGY& primitiveTopologyMarshaled);

		DxRenderer* renderer;
	};
}

