#include "DxVisualizer.h"

using namespace System;
using namespace DirectX;
using namespace System::Windows::Media::Media3D;
using namespace System::Runtime::InteropServices;

namespace CoriumDirectX {

	inline XMFLOAT3 marshalVector3D(Vector3D^ src) {
		return XMFLOAT3((float)src->X, (float)src->Y, (float)src->Z);
	}

	inline XMFLOAT3 marshalPoint3D(Point3D^ src) {
		return XMFLOAT3((float)src->X, (float)src->Y, (float)src->Z);
	}

	/*
	DxVisualizer::Scene::SceneModelInstance::SceneModelInstance(DxRenderer::Scene* sceneRef, unsigned int modelID, DxRenderer::Transform const& transformInit, IScene::SelectionHandler^ selectionHandler) {					
		DxRenderer::Scene::SceneModelInstance::SelectionHandler selectionHandlerMarshaled;
		if (selectionHandler) {
			callbackHandle = GCHandle::Alloc(selectionHandler);
			selectionHandlerMarshaled = static_cast<DxRenderer::Scene::SceneModelInstance::SelectionHandler>(Marshal::GetFunctionPointerForDelegate(selectionHandler).ToPointer());
		}
		else
			selectionHandlerMarshaled = NULL;

		sceneRef->createModelInstance(modelID, transformInit, selectionHandlerMarshaled);
	}
	*/

	DxVisualizer::Scene::SceneModelInstance::SceneModelInstance(DxRenderer::Scene::SceneModelInstance* _sceneModelInstanceRef) :
		sceneModelInstanceRef(_sceneModelInstanceRef) {}

	void DxVisualizer::Scene::SceneModelInstance::translate(Vector3D^ translation) {
		sceneModelInstanceRef->translate(marshalVector3D(translation));
	}

	void DxVisualizer::Scene::SceneModelInstance::setTranslation(Media3D::Vector3D^ translation) {
		sceneModelInstanceRef->setTranslation(marshalVector3D(translation));
	}

	void DxVisualizer::Scene::SceneModelInstance::scale(Vector3D^ scale) {
		sceneModelInstanceRef->scale(marshalVector3D(scale));
	}

	void DxVisualizer::Scene::SceneModelInstance::setScale(Media3D::Vector3D^ scaleFactor) {
		sceneModelInstanceRef->setScale(marshalVector3D(scaleFactor));
	}

	void DxVisualizer::Scene::SceneModelInstance::rotate(Vector3D^ ax, float ang) {
		sceneModelInstanceRef->rotate(marshalVector3D(ax), ang);
	}
	void DxVisualizer::Scene::SceneModelInstance::setRotation(Media3D::Vector3D^ ax, float ang) {
		sceneModelInstanceRef->setRotation(marshalVector3D(ax), ang);
	}

	void DxVisualizer::Scene::SceneModelInstance::highlight() {
		sceneModelInstanceRef->highlight();
	}

	void DxVisualizer::Scene::SceneModelInstance::release() {
		sceneModelInstanceRef->release();
	}
		
	DxVisualizer::Scene::Scene(DxRenderer::Scene* _sceneRef) : sceneRef(_sceneRef) {}

	void DxVisualizer::Scene::activate() {
		sceneRef->activate();
	}

	DxVisualizer::IScene::ISceneModelInstance^ DxVisualizer::Scene::createModelInstance(unsigned int modelID, Vector3D^ translationInit, Vector3D^ scaleFactorInit, Vector3D^ rotAxInit, float rotAngInit, IScene::SelectionHandler^ selectionHandler) {
		DxRenderer::Transform transform = { marshalVector3D(translationInit), marshalVector3D(scaleFactorInit), marshalVector3D(rotAxInit), rotAngInit };
		DxRenderer::Scene::SceneModelInstance::SelectionHandler selectionHandlerMarshaled =
			selectionHandler ? static_cast<DxRenderer::Scene::SceneModelInstance::SelectionHandler>(Marshal::GetFunctionPointerForDelegate(selectionHandler).ToPointer()) : NULL;
		return gcnew SceneModelInstance(sceneRef->createModelInstance(modelID, transform, selectionHandlerMarshaled));
	}

	void DxVisualizer::Scene::panCamera(float x, float y) {
		sceneRef->panCamera(x, y);
	}

	void DxVisualizer::Scene::rotateCamera(float x, float y) {
		sceneRef->rotateCamera(x, y);
	}

	void DxVisualizer::Scene::zoomCamera(float amount) {
		sceneRef->zoomCamera(amount);
	}

	Media3D::Vector3D^ DxVisualizer::Scene::getCameraPos() {
		XMFLOAT3 cameraPos = sceneRef->getCameraPos();

		return gcnew Vector3D(cameraPos.x, cameraPos.y, cameraPos.z);
	}

	void DxVisualizer::Scene::cursorSelect(float x, float y) {
		sceneRef->cursorSelect(x, y);
	}

	Vector3D^ DxVisualizer::Scene::cursorPosToRayDirection(float x, float y) {		
		XMFLOAT3 rayDirection = sceneRef->cursorPosToRayDirection(x, y);

		return gcnew Vector3D(rayDirection.x, rayDirection.y, rayDirection.z);
	}

	void DxVisualizer::Scene::release() {
		sceneRef->release();
	}	

	DxVisualizer::DxVisualizer(float fov, float nearZ, float farZ) {
		renderer = new DxRenderer(fov, nearZ, farZ);
	}

	DxVisualizer::~DxVisualizer() {
		delete renderer;
	}

	void DxVisualizer::addModel(array<Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, PrimitiveTopology primitiveTopology, [System::Runtime::InteropServices::Out] UINT% modelIDOut) {
		std::vector<DxRenderer::VertexData> verticesData(modelVertices->Length);		
		std::vector<WORD> vertexIndicesMarshaled(modelVertexIndices->Length); 
		D3D_PRIMITIVE_TOPOLOGY primitiveTopologyMarshaled;
		marshalModelData(modelVertices, verticesData, modelVertexIndices, vertexIndicesMarshaled, primitiveTopology, primitiveTopologyMarshaled);

		UINT modelID;
		renderer->addModel(verticesData, vertexIndicesMarshaled, primitiveTopologyMarshaled, &modelID);
		modelIDOut = modelID;
	}

	void DxVisualizer::updateModelData(unsigned int modelID, array<Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, PrimitiveTopology primitiveTopology) {
		std::vector<DxRenderer::VertexData> verticesData(modelVertices->Length);
		std::vector<WORD> vertexIndicesMarshaled(modelVertexIndices->Length);
		D3D_PRIMITIVE_TOPOLOGY primitiveTopologyMarshaled;
		marshalModelData(modelVertices, verticesData, modelVertexIndices, vertexIndicesMarshaled, primitiveTopology, primitiveTopologyMarshaled);

		renderer->updateModelData(modelID, verticesData, vertexIndicesMarshaled, primitiveTopologyMarshaled);
	}

	void DxVisualizer::removeModel(unsigned int modelID) {
		renderer->removeModel(modelID);
	}

	DxVisualizer::IScene^ DxVisualizer::createScene() {
		return gcnew Scene(renderer->createScene());
	}

	void DxVisualizer::initRenderer(System::IntPtr surface) {
		renderer->initDirectXLmnts((void*)surface);
	}

	void DxVisualizer::render()
	{
		renderer->render();
	}

	void DxVisualizer::marshalModelData(array<Point3D>^ modelVertices, std::vector<DxRenderer::VertexData>& verticesDataMarshaled,
										array<unsigned short>^ modelVertexIndices, std::vector<WORD>& vertexIndicesMarshaled, 
										PrimitiveTopology primitiveTopology, D3D_PRIMITIVE_TOPOLOGY& primitiveTopologyMarshaled) {
		
		for (unsigned int vertexIdx = 0; vertexIdx < modelVertices->Length; vertexIdx++) {
			verticesDataMarshaled[vertexIdx].pos = marshalPoint3D(modelVertices[vertexIdx]);
			verticesDataMarshaled[vertexIdx].color = { 0.5f * ((float)modelVertices[vertexIdx].X + 1), 0.5f * ((float)modelVertices[vertexIdx].Y + 1), 0.5f * ((float)modelVertices[vertexIdx].Z + 1), 1.0f };
			//verticesDataMarshaled[vertexIdx].color = { 1.0f, 1.0f, 1.0f, 1.0f };
		}
		
		{
			pin_ptr<unsigned short> modelVertexIndicesPinned(&modelVertexIndices[0]);
			std::copy(static_cast<WORD*>(modelVertexIndicesPinned),
				static_cast<WORD*>(modelVertexIndicesPinned + modelVertexIndices->Length),
				vertexIndicesMarshaled.begin());
		}
		
		switch (primitiveTopology) {
			case PrimitiveTopology::LINELIST:
				primitiveTopologyMarshaled = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
				break;

			case PrimitiveTopology::TRIANGLELIST:
				primitiveTopologyMarshaled = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				break;
		}
	}
}