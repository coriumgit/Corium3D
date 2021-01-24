#include "DxVisualizer.h"

using namespace System;
using namespace DirectX;
using namespace System::Windows::Media;
using namespace System::Windows::Media::Media3D;
using namespace System::Runtime::InteropServices;

namespace CoriumDirectX {

	inline XMFLOAT3 marshalVector3D(Vector3D^ src) {
		return XMFLOAT3((float)src->X, (float)src->Y, (float)src->Z);
	}

	inline XMFLOAT3 marshalPoint3D(Point3D^ src) {
		return XMFLOAT3((float)src->X, (float)src->Y, (float)src->Z);
	}

	inline XMFLOAT4 marshalColor(Color color) {
		return XMFLOAT4(color.ScR, color.ScG, color.ScB, color.ScA);
	}

	inline DxRenderer::TransformReferenceFrame marshalTransformReferenceFrame(DxVisualizer::IScene::TransformReferenceFrame referenceFrame) {
		switch (referenceFrame) {
			case DxVisualizer::IScene::TransformReferenceFrame::World:
				return DxRenderer::TransformReferenceFrame::World;
			case DxVisualizer::IScene::TransformReferenceFrame::Local:
				return DxRenderer::TransformReferenceFrame::Local;
		}
	}

	
	inline DxRenderer::Scene::TransformScaleConstraint marshalScaleConstraint(DxVisualizer::IScene::TransformScaleConstraint constraint) {
		switch (constraint) {			
			case DxVisualizer::IScene::TransformScaleConstraint::None:
				return DxRenderer::Scene::TransformScaleConstraint::None;
			case DxVisualizer::IScene::TransformScaleConstraint::Ignore:
				return DxRenderer::Scene::TransformScaleConstraint::Ignore;
			case DxVisualizer::IScene::TransformScaleConstraint::MaxDimGrp:
				return DxRenderer::Scene::TransformScaleConstraint::MaxDimGrp;
			case DxVisualizer::IScene::TransformScaleConstraint::FollowMaxDimGrp:
				return DxRenderer::Scene::TransformScaleConstraint::FollowMaxDimGrp;
		}
	}
	
	inline DxRenderer::Scene::TransformRotConstraint marshalRotConstraint(DxVisualizer::IScene::TransformRotConstraint constraint) {
		switch (constraint) {
			case DxVisualizer::IScene::TransformRotConstraint::None:
				return DxRenderer::Scene::TransformRotConstraint::None;
			case DxVisualizer::IScene::TransformRotConstraint::Ignore:
				return DxRenderer::Scene::TransformRotConstraint::Ignore;		
		}
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

	DxVisualizer::Scene::SceneModelInstance::~SceneModelInstance() {
		if (isDisposed)
			return;

		this->!SceneModelInstance();
		isDisposed = true;
	}

	DxVisualizer::Scene::SceneModelInstance::!SceneModelInstance() {
		sceneModelInstanceRef->release();
	}

	Media3D::Vector3D^ DxVisualizer::Scene::SceneModelInstance::getTranslation()
	{
		XMFLOAT3 ret = sceneModelInstanceRef->getWorldTranslation();
		
		return gcnew Vector3D(ret.x, ret.y, ret.z);
	}

	void DxVisualizer::Scene::SceneModelInstance::highlight() {
		sceneModelInstanceRef->highlight();
	}
	
	void DxVisualizer::Scene::SceneModelInstance::dim() {
		sceneModelInstanceRef->dim();
	}

	void DxVisualizer::Scene::SceneModelInstance::show() {
		sceneModelInstanceRef->show();
	}

	void DxVisualizer::Scene::SceneModelInstance::hide() {
		sceneModelInstanceRef->hide();
	}	

	void DxVisualizer::Scene::SceneModelInstance::addToTransformGrp()
	{
		sceneModelInstanceRef->addToTransformGrp();
	}

	void DxVisualizer::Scene::SceneModelInstance::removeFromTransformGrp()
	{
		sceneModelInstanceRef->removeFromTransformGrp();
	}

	void DxVisualizer::Scene::SceneModelInstance::assignParent(IScene::ISceneModelInstance^ parent, bool keepWorldTransform)
	{
		sceneModelInstanceRef->assignParent(((SceneModelInstance^)parent)->sceneModelInstanceRef, keepWorldTransform);
	}

	void DxVisualizer::Scene::SceneModelInstance::unparent() {
		sceneModelInstanceRef->unparent();
	}
		
	DxVisualizer::Scene::ConstrainedTransformInstance::ConstrainedTransformInstance(DxRenderer::Scene::ConstrainedTransformInstance* _constrainedTransformInstanceRef) :
		DxVisualizer::Scene::SceneModelInstance(_constrainedTransformInstanceRef) {}

	void DxVisualizer::Scene::ConstrainedTransformInstance::setScaleConstraints(
		IScene::TransformScaleConstraint xScaleConstraint,
		IScene::TransformScaleConstraint yScaleConstraint,
		IScene::TransformScaleConstraint zScaleConstraint)
	{
		static_cast<DxRenderer::Scene::ConstrainedTransformInstance*>(sceneModelInstanceRef)->setScaleConstraints(marshalScaleConstraint(xScaleConstraint), marshalScaleConstraint(yScaleConstraint), marshalScaleConstraint(zScaleConstraint));
	}

	void DxVisualizer::Scene::ConstrainedTransformInstance::setRotConstraints(IScene::TransformRotConstraint rotConstraint)
	{
		static_cast<DxRenderer::Scene::ConstrainedTransformInstance*>(sceneModelInstanceRef)->setRotConstraints(marshalRotConstraint(rotConstraint));
	}

	/*
	void DxVisualizer::Scene::ConstrainedTransformInstance::setRotConstraints(
		IScene::IConstrainedTransformInstance::TransformRotConstraint xRotConstraint,
		IScene::IConstrainedTransformInstance::TransformRotConstraint yRotConstraint,
		IScene::IConstrainedTransformInstance::TransformRotConstraint zRotConstraint)
	{
		constrainedTransformInstanceRef->setRotConstraints(marshalRotConstraint(xRotConstraint), marshalRotConstraint(yRotConstraint), marshalRotConstraint(zRotConstraint));
	}
	*/

	DxVisualizer::Scene::ConstrainedTransform2dInstance::ConstrainedTransform2dInstance(DxRenderer::Scene::ConstrainedTransform2dInstance* _constrainedTransform2dInstanceRef) :
		DxVisualizer::Scene::SceneModelInstance(_constrainedTransform2dInstanceRef) {}
	
	void DxVisualizer::Scene::ConstrainedTransform2dInstance::setScaleConstraints(
		IScene::TransformScaleConstraint xScaleConstraint,
		IScene::TransformScaleConstraint yScaleConstraint)
	{
		static_cast<DxRenderer::Scene::ConstrainedTransform2dInstance*>(sceneModelInstanceRef)->setScaleConstraints(marshalScaleConstraint(xScaleConstraint), marshalScaleConstraint(yScaleConstraint));
	}

	DxVisualizer::Scene::Scene(DxRenderer* dxRenderer, DxRenderer::Scene::TransformCallbackHandlers const& transformCallbackHandlers, [System::Runtime::InteropServices::Out] DxVisualizer::MouseCallbacks^% mouseCallbacksManaged) :
			sceneRef(dxRenderer->createScene(transformCallbackHandlers, *(mouseCallbacksNative = new DxRenderer::MouseCallbacks))) {
		mouseCallbacksManaged = gcnew DxVisualizer::MouseCallbacks();
		mouseCallbacksManaged->onMouseMoveCallback = gcnew DxVisualizer::OnMouseMoveCallback(this, &DxVisualizer::Scene::onMouseMove);
		mouseCallbacksManaged->onMouseUpCallback = gcnew DxVisualizer::OnMouseUpCallback(this, &DxVisualizer::Scene::onMouseUp);
	}

	DxVisualizer::Scene::~Scene() {
		if (isDisposed)
			return;

		this->!Scene();
		isDisposed = true;
	}

	DxVisualizer::Scene::!Scene() {
		delete mouseCallbacksNative;
		//sceneRef->release();
	}

	void DxVisualizer::Scene::activate() {
		sceneRef->activate();
	}
	
	DxVisualizer::IScene::ISceneModelInstance^ DxVisualizer::Scene::createModelInstance(unsigned int modelID, Media::Color instanceColorMask, Vector3D^ translationInit, Vector3D^ scaleFactorInit, Vector3D^ rotAxInit, float rotAngInit, IScene::ISceneModelInstance::SelectionHandler^ selectionHandler) 
	{
		DxRenderer::Transform transform = { marshalVector3D(translationInit), marshalVector3D(scaleFactorInit), marshalVector3D(rotAxInit), rotAngInit };
		DxRenderer::Scene::SceneModelInstance::SelectionHandler selectionHandlerMarshaled =
			selectionHandler ? static_cast<DxRenderer::Scene::SceneModelInstance::SelectionHandler>(Marshal::GetFunctionPointerForDelegate(selectionHandler).ToPointer()) : NULL;
		return gcnew SceneModelInstance(sceneRef->createModelInstance(modelID, marshalColor(instanceColorMask), transform, selectionHandlerMarshaled));
	}	

	DxVisualizer::IScene::IConstrainedTransformInstance^ DxVisualizer::Scene::createConstrainedTransformInstance(unsigned int modelID, Media::Color instanceColorMask, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleFactorInit, Media3D::Vector3D^ rotAxInit, float rotAngInit, DxVisualizer::IScene::ISceneModelInstance::SelectionHandler^ selectionHandler) 
	{
		DxRenderer::Transform transform = { marshalVector3D(translationInit), marshalVector3D(scaleFactorInit), marshalVector3D(rotAxInit), rotAngInit };
		DxRenderer::Scene::SceneModelInstance::SelectionHandler selectionHandlerMarshaled =
			selectionHandler ? static_cast<DxRenderer::Scene::SceneModelInstance::SelectionHandler>(Marshal::GetFunctionPointerForDelegate(selectionHandler).ToPointer()) : NULL;
		return gcnew ConstrainedTransformInstance(sceneRef->createConstrainedTransformInstance(modelID, marshalColor(instanceColorMask), transform, selectionHandlerMarshaled));
	}

	DxVisualizer::IScene::IConstrainedTransform2dInstance^ DxVisualizer::Scene::createConstrainedTransform2dInstance(unsigned int modelID, Media::Color instanceColorMask, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleFactorInit, Media3D::Vector3D^ rotAxInit, float rotAngInit, DxVisualizer::IScene::ISceneModelInstance::SelectionHandler^ selectionHandler)
	{
		DxRenderer::Transform transform = { marshalVector3D(translationInit), marshalVector3D(scaleFactorInit), marshalVector3D(rotAxInit), rotAngInit };
		DxRenderer::Scene::SceneModelInstance::SelectionHandler selectionHandlerMarshaled =
			selectionHandler ? static_cast<DxRenderer::Scene::SceneModelInstance::SelectionHandler>(Marshal::GetFunctionPointerForDelegate(selectionHandler).ToPointer()) : NULL;
		return gcnew ConstrainedTransform2dInstance(sceneRef->createConstrainedTransform2dInstance(modelID, marshalColor(instanceColorMask), transform, selectionHandlerMarshaled));
	}

	void DxVisualizer::Scene::transformGrpTranslate(Media3D::Vector3D^ translation, IScene::TransformReferenceFrame referenceFrame)
	{
		sceneRef->transformGrpTranslate(marshalVector3D(translation), marshalTransformReferenceFrame(referenceFrame));
	}

	void DxVisualizer::Scene::transformGrpSetTranslation(Media3D::Vector3D^ translation, IScene::TransformReferenceFrame referenceFrame)
	{
		sceneRef->transformGrpSetTranslation(marshalVector3D(translation), marshalTransformReferenceFrame(referenceFrame));
	}

	void DxVisualizer::Scene::transformGrpScale(Media3D::Vector3D^ scaleFactorQ, IScene::TransformReferenceFrame referenceFrame)
	{
		sceneRef->transformGrpScale(marshalVector3D(scaleFactorQ), marshalTransformReferenceFrame(referenceFrame));
	}

	void DxVisualizer::Scene::transformGrpSetScale(Media3D::Vector3D^ scaleFactor, IScene::TransformReferenceFrame referenceFrame)
	{
		sceneRef->transformGrpSetScale(marshalVector3D(scaleFactor), marshalTransformReferenceFrame(referenceFrame));
	}

	void DxVisualizer::Scene::transformGrpRotate(Media3D::Vector3D^ ax, float ang, IScene::TransformReferenceFrame referenceFrame)
	{
		sceneRef->transformGrpRotate(marshalVector3D(ax), ang, marshalTransformReferenceFrame(referenceFrame));
	}

	void DxVisualizer::Scene::transformGrpSetRotation(Media3D::Vector3D^ ax, float ang, IScene::TransformReferenceFrame referenceFrame)
	{
		sceneRef->transformGrpSetRotation(marshalVector3D(ax), ang, marshalTransformReferenceFrame(referenceFrame));
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

	Media3D::Point3D^ DxVisualizer::Scene::getCameraPos() {
		XMFLOAT3 cameraPos = sceneRef->getCameraPos();

		return gcnew Point3D(cameraPos.x, cameraPos.y, cameraPos.z);
	}

	float DxVisualizer::Scene::getCameraFOV() {
		return sceneRef->getCameraFOV();
	}

	bool DxVisualizer::Scene::cursorSelect(float x, float y) {
		return sceneRef->cursorSelect(x, y);
	}

	Media3D::Vector3D^ DxVisualizer::Scene::screenVecToWorldVec(float x, float y)
	{
		XMFLOAT3 worldVec = sceneRef->screenVecToWorldVec(x, y);

		return gcnew Vector3D(worldVec.x, worldVec.y, worldVec.z);
	}

	Vector3D^ DxVisualizer::Scene::cursorPosToRayDirection(float x, float y) {		
		XMFLOAT3 rayDirection = sceneRef->cursorPosToRayDirection(x, y);

		return gcnew Vector3D(rayDirection.x, rayDirection.y, rayDirection.z);
	}

	void DxVisualizer::Scene::onMouseMove(float cursorPosX, float cursorPosY)
	{
		mouseCallbacksNative->onMouseMoveCallback(cursorPosX, cursorPosY);
	}

	void DxVisualizer::Scene::onMouseUp()
	{
		mouseCallbacksNative->onMouseUpCallback();
	}

	DxVisualizer::DxVisualizer(float fov, float nearZ, float farZ) {	
		renderer = new DxRenderer(fov, nearZ, farZ);		
	}

	DxVisualizer::~DxVisualizer() {		
		delete renderer;
	}

	void DxVisualizer::addModel(array<Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, Color modelColor, Point3D^ boundingSphereCenter, float boundingSphereRadius, PrimitiveTopology primitiveTopology, [System::Runtime::InteropServices::Out] UINT% modelIDOut) {
		std::vector<DxRenderer::VertexData> verticesData(modelVertices->Length);		
		std::vector<WORD> vertexIndicesMarshaled(modelVertexIndices->Length); 
		XMFLOAT3 boundingSphereCenterMarshaled;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopologyMarshaled;
		marshalModelData(modelVertices, modelColor, verticesData, modelVertexIndices, vertexIndicesMarshaled, boundingSphereCenter, boundingSphereCenterMarshaled, primitiveTopology, primitiveTopologyMarshaled);

		UINT modelID;		
		renderer->addModel(verticesData, vertexIndicesMarshaled, boundingSphereCenterMarshaled, boundingSphereRadius, primitiveTopologyMarshaled, &modelID);
		modelIDOut = modelID;
	}

	void DxVisualizer::updateModelData(unsigned int modelID, array<Point3D>^ modelVertices, array<unsigned short>^ modelVertexIndices, Color modelColor, PrimitiveTopology primitiveTopology) {
		std::vector<DxRenderer::VertexData> verticesData(modelVertices->Length);
		std::vector<WORD> vertexIndicesMarshaled(modelVertexIndices->Length);
		XMFLOAT3 boundingSphereCenterMarshaled;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopologyMarshaled;
		marshalModelData(modelVertices, modelColor, verticesData, modelVertexIndices, vertexIndicesMarshaled, gcnew Point3D(), boundingSphereCenterMarshaled, primitiveTopology, primitiveTopologyMarshaled);

		renderer->updateModelData(modelID, verticesData, vertexIndicesMarshaled, primitiveTopologyMarshaled);
	}

	void DxVisualizer::removeModel(unsigned int modelID) {
		renderer->removeModel(modelID);
	}
	
	DxVisualizer::IScene^ DxVisualizer::createScene(IScene::TransformCallbackHandlers^ transformCallbackHandlers, [System::Runtime::InteropServices::Out] DxVisualizer::MouseCallbacks^% mouseCallbacks) {
		DxRenderer::Scene::TransformCallbackHandlers transformCallbackHandlersMarshaled {
			static_cast<void(*)(float, float, float)>(Marshal::GetFunctionPointerForDelegate(transformCallbackHandlers->translationHandler).ToPointer()),
			static_cast<void(*)(float, float, float)>(Marshal::GetFunctionPointerForDelegate(transformCallbackHandlers->scaleHandler).ToPointer()),
			static_cast<void(*)(float, float, float, float)>(Marshal::GetFunctionPointerForDelegate(transformCallbackHandlers->rotationHandler).ToPointer())
		};
		
		return gcnew Scene(renderer, transformCallbackHandlersMarshaled, mouseCallbacks);
	}

	void DxVisualizer::initRenderer(System::IntPtr surface) {
		renderer->initDirectXLmnts((void*)surface);
		RendererInitialized();			
	}	 		

	void DxVisualizer::render()
	{
		renderer->render();
	}

	void DxVisualizer::captureFrame()
	{
		renderer->captureFrame();
	}

	void DxVisualizer::marshalModelData(array<Point3D>^ modelVertices, Color modelColor, std::vector<DxRenderer::VertexData>& verticesDataMarshaled,
										array<unsigned short>^ modelVertexIndices, std::vector<WORD>& vertexIndicesMarshaled, 
										Point3D^ boundingSphereCenter, XMFLOAT3& boundingSphereCenterMarshaled,
										PrimitiveTopology primitiveTopology, D3D_PRIMITIVE_TOPOLOGY& primitiveTopologyMarshaled) {
		
		for (unsigned int vertexIdx = 0; vertexIdx < modelVertices->Length; vertexIdx++) {
			verticesDataMarshaled[vertexIdx].pos = marshalPoint3D(modelVertices[vertexIdx]);
			verticesDataMarshaled[vertexIdx].color = { modelColor.ScR, modelColor.ScG, modelColor.ScB, modelColor.ScA };
		}
		
		{
			pin_ptr<unsigned short> modelVertexIndicesPinned(&modelVertexIndices[0]);
			std::copy(static_cast<WORD*>(modelVertexIndicesPinned),
				static_cast<WORD*>(modelVertexIndicesPinned + modelVertexIndices->Length),
				vertexIndicesMarshaled.begin());
		}
		
		boundingSphereCenterMarshaled = marshalPoint3D(boundingSphereCenter);

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