#include "DxRenderer.h"
#include "DxUtils.h"

#include <exception>
#include <string>
#include <limits>

#pragma warning( disable : 3146081)

using namespace DirectX;

inline float gaussDistrib(float x, float y, float rho) {
    return 1.0f / sqrtf(2.0f * XM_PI * rho * rho) * expf(-(x*x + y*y) / (2 * rho * rho));    
}

namespace CoriumDirectX {    
    const float DxRenderer::BLUR_STD = 5.0f;
    const float DxRenderer::BLUR_FACTOR = 1.75f;
    const FLOAT DxRenderer::CLEAR_COLOR[4] = { 224.0f / 255, 224.0f / 255, 224.0f / 255, 1.0f };
    const FLOAT DxRenderer::SELECTION_TEXES_CLEAR_COLOR[4] = { MODELS_NR_MAX, 0, 0, 0}; //TODO: update the selection texes to hold INT instead of UINT
    const FLOAT DxRenderer::BLUR_TEXES_CLEAR_COLOR[4] = { CLEAR_COLOR[0], CLEAR_COLOR[1], CLEAR_COLOR[2], 0.0f };

    XMMATRIX DxRenderer::Transform::genTransformat() const {
        XMMATRIX transformat = XMMatrixScaling(scaleFactor.x, scaleFactor.y, scaleFactor.z);
        transformat = XMMatrixMultiply(XMMatrixRotationAxis(XMLoadFloat3(&rotAx), rotAng), transformat);
        transformat = XMMatrixMultiply(XMMatrixTranslationFromVector(XMLoadFloat3(&translation)), transformat);

        return transformat;
    }

    void DxRenderer::Scene::SceneModelInstance::translate(XMFLOAT3 const& _translation) {
        XMVECTOR translation = XMLoadFloat3(&_translation);
        scene.bsh.translateNodeBS(bshDataNode, translation);
        pos += translation;
        modelTransformat = XMMatrixMultiply(XMMatrixTranslationFromVector(translation), modelTransformat);
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::setTranslation(XMFLOAT3 const& _translation) {
        XMVECTOR translation = XMLoadFloat3(&_translation);
        scene.bsh.setTranslationForNodeBS(bshDataNode, translation);
        pos = translation;        
        recompTransformat();
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::scale(XMFLOAT3 const& _scaleFactorQ) {
        scene.bsh.scaleNodeBS(bshDataNode, max(max(_scaleFactorQ.x, _scaleFactorQ.y), _scaleFactorQ.z));
        XMVECTOR scaleFactorQ = XMLoadFloat3(&_scaleFactorQ);
        scaleFactor *= scaleFactorQ;
        modelTransformat = XMMatrixMultiply(XMMatrixScaling(XMVectorGetX(scaleFactor), XMVectorGetY(scaleFactor), XMVectorGetZ(scaleFactor)), modelTransformat);
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::setScale(XMFLOAT3 const& _scaleFactor) {
        scene.bsh.setRadiusForNodeBS(bshDataNode, max(max(_scaleFactor.x, _scaleFactor.y), _scaleFactor.z));
        scaleFactor = XMLoadFloat3(&_scaleFactor);
        recompTransformat();
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::rotate(XMFLOAT3 const& ax, float ang) {
        XMVECTOR rotQuat = XMQuaternionRotationAxis(XMVectorSet(ax.x, ax.y, ax.z, 0.0f), ang);
        rot *= rotQuat;
        modelTransformat = XMMatrixMultiply(XMMatrixRotationQuaternion(rotQuat), modelTransformat);
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::setRotation(XMFLOAT3 const& ax, float ang) {
        rot = XMQuaternionRotationAxis(XMVectorSet(ax.x, ax.y, ax.z, 0.0f), ang);
        recompTransformat();
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::highlight() {    
        if (isHighlighted)
            return;

        if (transformatsBufferOffset < (std::numeric_limits<UINT>::max)()) {
            unloadInstanceTransformatFromBuffer();
            isHighlighted = true;
            loadInstanceTransformatToBuffer();
        }
        else
            isHighlighted = true;                                                                 
    }

    void DxRenderer::Scene::SceneModelInstance::dim() {
        if (!isHighlighted)
            return;

        if (transformatsBufferOffset < (std::numeric_limits<UINT>::max)()) {
            unloadInstanceTransformatFromBuffer();
            isHighlighted = false;
            loadInstanceTransformatToBuffer();
        }
        else
            isHighlighted = false;
    }

    void DxRenderer::Scene::SceneModelInstance::show() {
        if (isShown)
            return;

        isShown = true;
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::hide() {
        if (!isShown)
            return;

        isShown = false;
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::release() {
        scene.bsh.destroy(bshDataNode);

        if (transformatsBufferOffset < (std::numeric_limits<UINT>::max)())
            unloadInstanceTransformatFromBuffer();
        
        for (std::list<SceneModelData*>::iterator sceneModelsIt = scene.sceneModelsData.begin(); sceneModelsIt != scene.sceneModelsData.end(); sceneModelsIt++) {
            SceneModelData* sceneModelData = *sceneModelsIt;
            if (sceneModelData->modelID == modelID) {
                sceneModelData->instanceIdxsPool.releaseIdx(instanceIdx);
                if (sceneModelData->instanceIdxsPool.acquiredIdxsNr() == 0)
                    scene.sceneModelsData.erase(sceneModelsIt); // TODO: delete the scene models instances

                delete this;
                return;
            }
        }

        throw std::exception("release was called on a SceneModelInstance of a removed model.");
    }

    DxRenderer::Scene::SceneModelInstance::SceneModelInstance(Scene& _scene, UINT _modelID, Transform const& transform, SceneModelInstance::SelectionHandler _selectionHandler) :
            scene(_scene), modelID(_modelID),
            selectionHandler(_selectionHandler),
            pos(XMLoadFloat3(&transform.translation)), 
            scaleFactor(XMLoadFloat3(&transform.scaleFactor)),
            rot(XMQuaternionRotationAxis(XMLoadFloat3(&transform.rotAx), transform.rotAng)),
            modelTransformat(transform.genTransformat()),
            bshDataNode(scene.bsh.insert(BoundingSphere::calcTransformedBoundingSphere(scene.renderer.modelsRenderData[modelID].boundingSphere, pos, max(max(transform.scaleFactor.x, transform.scaleFactor.y), transform.scaleFactor.z)), *this)) {

        SceneModelData* sceneModelData = NULL;
        for (std::list<SceneModelData*>::iterator it = scene.sceneModelsData.begin(); it != scene.sceneModelsData.end(); it++) {
            if ((*it)->modelID == modelID) {
                sceneModelData = *it;
                break;
            }
        }

        if (sceneModelData == NULL) {
            // modelID was not found
            sceneModelData = new SceneModelData{ modelID };
            scene.sceneModelsData.push_back(sceneModelData);
        }

        instanceIdx = sceneModelData->instanceIdxsPool.acquireIdx();
#if _DEBUG
        bshDataNode->setName(std::string("(") + std::to_string(modelID) + std::string(",") + std::to_string(instanceIdx) + std::string(")"));
#endif
        // TODO: if acquired indices number == visibleInstancesBuffersCapacity, reallocate:
        // visibleHighlightedInstancesIdxsBuffer
        // visibleInstancesIdxsBuffer
        // visibleInstancesIdxsStaging
        // visibleHighlightedInstancesIdxsStaging

        sceneModelData->sceneModelInstances[instanceIdx] = this;                           
        updateBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::loadInstanceTransformatToBuffer() {             
        if (isHighlighted) 
            transformatsBufferOffset = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesNr++;        
        else 
            transformatsBufferOffset = scene.renderer.modelsRenderData[modelID].visibleInstancesNr++;
                
        updateInstanceTransformatInBuffer();
    }

    void DxRenderer::Scene::SceneModelInstance::unloadInstanceTransformatFromBuffer() {
        ID3D11Buffer* transformatsBuffer;        
        std::vector<UINT>* visibleInstancesIdxs;
        ID3D11Buffer* idxsBuffer;
        UINT visibleInstancesNr;        
        if (isHighlighted) {
            transformatsBuffer = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesTransformatsBuffer;
            idxsBuffer = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesIdxsBuffer;
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesIdxs;
            visibleInstancesNr = --scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesNr;
        }
        else {
            transformatsBuffer = scene.renderer.modelsRenderData[modelID].visibleInstancesTransformatsBuffer;
            idxsBuffer = scene.renderer.modelsRenderData[modelID].visibleInstancesIdxsBuffer;
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleInstancesIdxs;
            visibleInstancesNr = --scene.renderer.modelsRenderData[modelID].visibleInstancesNr;
        }
                
        if (transformatsBufferOffset < visibleInstancesNr) {
            for (std::list<SceneModelData*>::iterator it = scene.sceneModelsData.begin(); it != scene.sceneModelsData.end(); it++) {
                SceneModelData* sceneModelData = *it;
                if (sceneModelData->modelID == modelID) {
                    D3D11_BOX rangeBox = { visibleInstancesNr * sizeof(XMMATRIX), 0U, 0U, (visibleInstancesNr + 1) * sizeof(XMMATRIX), 1U, 1U };
                    scene.renderer.devcon->CopySubresourceRegion(transformatsBuffer, 0, transformatsBufferOffset * sizeof(XMMATRIX), 0, 0, transformatsBuffer, 0, &rangeBox);

                    rangeBox.left = visibleInstancesNr * sizeof(UINT);
                    rangeBox.right = (visibleInstancesNr + 1) * sizeof(UINT);
                    scene.renderer.devcon->CopySubresourceRegion(idxsBuffer, 0, transformatsBufferOffset * sizeof(UINT), 0, 0, idxsBuffer, 0, &rangeBox);

                    SceneModelInstance* instanceToSwapWith = sceneModelData->sceneModelInstances[(*visibleInstancesIdxs)[visibleInstancesNr]];
                    (*visibleInstancesIdxs)[transformatsBufferOffset] = instanceToSwapWith->instanceIdx;
                    instanceToSwapWith->transformatsBufferOffset = transformatsBufferOffset;
                }
            }
        }

        transformatsBufferOffset = (std::numeric_limits<UINT>::max)();
    }

    void DxRenderer::Scene::SceneModelInstance::updateInstanceTransformatInBuffer() {
        ID3D11Buffer* transformatsBuffer;
        std::vector<UINT>* visibleInstancesIdxs;
        ID3D11Buffer* idxsBuffer;
        if (isHighlighted) {
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesIdxs;
            transformatsBuffer = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesTransformatsBuffer;
            idxsBuffer = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesIdxsBuffer;
        }
        else {
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleInstancesIdxs;
            transformatsBuffer = scene.renderer.modelsRenderData[modelID].visibleInstancesTransformatsBuffer;
            idxsBuffer = scene.renderer.modelsRenderData[modelID].visibleInstancesIdxsBuffer;
        }

        D3D11_BOX rangeBox = { transformatsBufferOffset * sizeof(XMMATRIX), 0U, 0U, (transformatsBufferOffset + 1) * sizeof(XMMATRIX), 1U, 1U };
        scene.renderer.devcon->UpdateSubresource(transformatsBuffer, 0, &rangeBox, &modelTransformat, 0, 0);

        rangeBox.left = transformatsBufferOffset * sizeof(UINT);
        rangeBox.right = (transformatsBufferOffset + 1) * sizeof(UINT);
        scene.renderer.devcon->UpdateSubresource(idxsBuffer, 0, &rangeBox, &instanceIdx, 0, 0);

        (*visibleInstancesIdxs)[transformatsBufferOffset] = instanceIdx;
    }

    void DxRenderer::Scene::SceneModelInstance::updateBuffers() {        
        if (isShown && scene.camera.isBoundingSphereVisible(bshDataNode->getBoundingSphere())) {
            if (transformatsBufferOffset == (std::numeric_limits<UINT>::max)())
                loadInstanceTransformatToBuffer();
            else
                updateInstanceTransformatInBuffer();
        }
        else if (transformatsBufferOffset < (std::numeric_limits<UINT>::max)())
            unloadInstanceTransformatFromBuffer();
    }       

    void DxRenderer::Scene::SceneModelInstance::recompTransformat() {
        modelTransformat = XMMatrixTranslationFromVector(pos);        
        modelTransformat = XMMatrixMultiply(XMMatrixRotationQuaternion(rot) , modelTransformat);
        modelTransformat = XMMatrixMultiply(XMMatrixScaling(XMVectorGetX(scaleFactor), XMVectorGetY(scaleFactor), XMVectorGetZ(scaleFactor)), modelTransformat);
    }

    void DxRenderer::Scene::activate() {        
        loadViewMatToBuffer();
        loadProjMatToBuffer();
        loadVisibleInstancesDataToBuffers();
        renderer.activeScene = this;
    }

    DxRenderer::Scene::SceneModelInstance* DxRenderer::Scene::createModelInstance(unsigned int modelID, Transform const& transformInit, SceneModelInstance::SelectionHandler selectionHandler) {                        
        return new SceneModelInstance(*this, modelID, transformInit, selectionHandler);
    }

    void DxRenderer::Scene::panCamera(float x, float y) {
        camera.panViaViewportDrag(x, y);
        loadViewMatToBuffer();
        loadVisibleInstancesDataToBuffers();
    }

    void DxRenderer::Scene::rotateCamera(float x, float y) {
        camera.rotateViaViewportDrag(x, y);
        loadViewMatToBuffer();
        loadVisibleInstancesDataToBuffers();
    }

    void DxRenderer::Scene::zoomCamera(float amount) {
        camera.zoom(amount);
        loadViewMatToBuffer();
        loadVisibleInstancesDataToBuffers();
    }

    XMFLOAT3 DxRenderer::Scene::getCameraPos() {
        XMVECTOR cameraPos = camera.getPos();

        return XMFLOAT3(XMVectorGetX(cameraPos), XMVectorGetY(cameraPos), XMVectorGetZ(cameraPos));
    }
    
    bool DxRenderer::Scene::cursorSelect(float x, float y) {
        UINT selectionX = (UINT)floorf(x), selectionY = (UINT)floorf(y);
        D3D11_BOX regionBox = { selectionX , selectionY, 0, selectionX + 1, selectionY + 1, 1 };
        renderer.devcon->CopySubresourceRegion(renderer.stagingSelectionTex, 0, selectionX, selectionY, 0, renderer.selectionTexes[(renderer.updatedSelectionTexIdx + 1) % 3], 0, &regionBox);
        D3D11_MAPPED_SUBRESOURCE selectionTexMapped;
        HRESULT hr = renderer.devcon->Map(renderer.stagingSelectionTex, 0, D3D11_MAP_READ, 0, &selectionTexMapped);
        if FAILED(hr) {
            OutputDebugStringW(L"Could not map a selection texture.");
            return false;
        }        
        SceneModelInstanceIdxs sceneModelInstanceIdxs = ((SceneModelInstanceIdxs*)selectionTexMapped.pData)[selectionTexMapped.RowPitch/sizeof(SceneModelInstanceIdxs)*selectionY + selectionX];        
        renderer.devcon->Unmap(renderer.stagingSelectionTex, 0);

        if (sceneModelInstanceIdxs.modelID < MODELS_NR_MAX) {
            for (std::list<SceneModelData*>::iterator modelsIt = sceneModelsData.begin(); modelsIt != sceneModelsData.end(); modelsIt++) {
                SceneModelData* sceneModelData = *modelsIt;
                if (sceneModelData->modelID == sceneModelInstanceIdxs.modelID) {   
                    SceneModelInstance* instance = sceneModelData->sceneModelInstances[sceneModelInstanceIdxs.instanceIdx];
                    if (instance->selectionHandler)
                        instance->selectionHandler();
                    
                    return true;
                }
            }

            assert(false);
        }
        else
            return false;
    }

    XMFLOAT3 DxRenderer::Scene::cursorPosToRayDirection(float x, float y) {
        XMVECTOR rayDirection = camera.cursorPosToRayDirection(x, y);
        return XMFLOAT3(XMVectorGetX(rayDirection), XMVectorGetY(rayDirection), XMVectorGetZ(rayDirection));
    }

    void DxRenderer::Scene::release() {           
        for (std::list<SceneModelData*>::iterator it = sceneModelsData.begin(); it != sceneModelsData.end(); it++){
            (*it)->sceneModelInstances.clear(); // TODO: delete the scene models instances
        }
        sceneModelsData.clear();
        renderer.scenes.remove(this);
        if (renderer.activeScene == this)
            renderer.activeScene = NULL;

        delete this;
    }

    DxRenderer::Scene::Scene(DxRenderer& _renderer) :
        renderer(_renderer), camera(renderer.fov, renderer.viewportWidth, renderer.viewportHeight, renderer.nearZ, renderer.farZ) {}           

    class DxRenderer::Scene::VisibleInstancesIt {
    public:
        VisibleInstancesIt(BSH<SceneModelInstance> const& _bsh, Camera const& _camera) : bsh(_bsh), camera(_camera) {
            init();
        }

        void init() {
            BSH<SceneModelInstance>::Node* root = bsh.getNodesRoot();
            if (root != NULL && camera.isBoundingSphereVisible(root->getBoundingSphere())) {
                it = root;
                findNextVisibleLeaf();
            }
        }

        SceneModelInstance* getNext() {
            if (!it)
                return NULL;

            SceneModelInstance* ret = &static_cast<BSH<SceneModelInstance>::DataNode*>(it)->getData();
            it = it->getEscapeNode();
            findNextVisibleLeaf();

            return ret;
        }       

    private:
        BSH<SceneModelInstance> const& bsh;
        Camera const& camera;
        BSH<SceneModelInstance>::Node* it = NULL;

        void findNextVisibleLeaf() {
            while (it != NULL) {
                if (it->isLeaf()) {
                    if (static_cast<BSH<SceneModelInstance>::DataNode*>(it)->getData().isShown && camera.isBoundingSphereVisible(it->getBoundingSphere()))
                        break;
                    else
                        it = it->getEscapeNode();
                }
                else {
                    if (camera.isBoundingSphereVisible(it->getBoundingSphere()))
                        it = it->getChild(0);
                    else
                        it = it->getEscapeNode();
                }
            }
        }
    };    
    
    void DxRenderer::Scene::loadViewMatToBuffer() {
        renderer.devcon->UpdateSubresource(renderer.cbViewMat, 0, NULL, &XMMatrixTranspose(camera.getViewMat()), 0, 0);
    }

    void DxRenderer::Scene::loadProjMatToBuffer() {
        renderer.devcon->UpdateSubresource(renderer.cbProjMat, 0, NULL, &XMMatrixTranspose(camera.getProjMat()), 0, 0);
    }            

    void DxRenderer::Scene::loadVisibleInstancesDataToBuffers() {        
        for (std::list<SceneModelData*>::iterator it = sceneModelsData.begin(); it != sceneModelsData.end(); it++) {
            ModelRenderData& modelRenderData = renderer.modelsRenderData[(*it)->modelID];
            for (unsigned int instanceIdx = 0; instanceIdx < modelRenderData.visibleInstancesNr; instanceIdx++)
                (*it)->sceneModelInstances[modelRenderData.visibleInstancesIdxs[instanceIdx]]->transformatsBufferOffset = (std::numeric_limits<UINT>::max)();
            for (unsigned int instanceIdx = 0; instanceIdx < modelRenderData.visibleHighlightedInstancesNr; instanceIdx++)
                (*it)->sceneModelInstances[modelRenderData.visibleHighlightedInstancesIdxs[instanceIdx]]->transformatsBufferOffset = (std::numeric_limits<UINT>::max)();

            modelRenderData.visibleInstancesNr = modelRenderData.visibleHighlightedInstancesNr = 0;            
        }
            
        VisibleInstancesIt visibleNodesIt(bsh, camera);        
        while (SceneModelInstance* visibleInstance = visibleNodesIt.getNext()) {
            ModelRenderData& modelRenderData = renderer.modelsRenderData[visibleInstance->modelID];
            if (!visibleInstance->isHighlighted) {
                modelRenderData.visibleInstancesIdxs[modelRenderData.visibleInstancesNr] = visibleInstance->instanceIdx;
                visibleInstance->transformatsBufferOffset = modelRenderData.visibleInstancesNr++;
            }
            else {
                modelRenderData.visibleHighlightedInstancesIdxs[modelRenderData.visibleHighlightedInstancesNr] = visibleInstance->instanceIdx;
                visibleInstance->transformatsBufferOffset = modelRenderData.visibleHighlightedInstancesNr++;
            }
        }
                
        for (std::list<SceneModelData*>::iterator sceneModelsIt = sceneModelsData.begin(); sceneModelsIt != sceneModelsData.end(); sceneModelsIt++) {
            SceneModelData* sceneModelData = *sceneModelsIt;
            ModelRenderData& modelRenderData = renderer.modelsRenderData[sceneModelData->modelID];
            if (modelRenderData.visibleInstancesNr) {
                std::vector<XMMATRIX> visibleInstancesTransformatsStaging(modelRenderData.visibleInstancesBuffersCapacity);
                for (unsigned int transformatsBufferOffset = 0; transformatsBufferOffset < modelRenderData.visibleInstancesNr; transformatsBufferOffset++) {
                    UINT instanceIdx = modelRenderData.visibleInstancesIdxs[transformatsBufferOffset];
                    visibleInstancesTransformatsStaging[transformatsBufferOffset] = sceneModelData->sceneModelInstances[instanceIdx]->getModelTransformat();
                }
                D3D11_BOX rangeBox = { 0U, 0U, 0U, modelRenderData.visibleInstancesNr * sizeof(XMMATRIX), 1U, 1U };
				renderer.devcon->UpdateSubresource(modelRenderData.visibleInstancesTransformatsBuffer, 0, &rangeBox, &visibleInstancesTransformatsStaging[0], 0, 0);

                rangeBox.right = modelRenderData.visibleInstancesNr * sizeof(UINT);
                renderer.devcon->UpdateSubresource(modelRenderData.visibleInstancesIdxsBuffer, 0, &rangeBox, &modelRenderData.visibleInstancesIdxs[0], 0, 0);
            }
            if (modelRenderData.visibleHighlightedInstancesNr) {
                std::vector<XMMATRIX> visibleHighlightedInstancesTransformatsStaging(modelRenderData.visibleInstancesBuffersCapacity);
                for (unsigned int transformatsBufferOffset = 0; transformatsBufferOffset < modelRenderData.visibleHighlightedInstancesNr; transformatsBufferOffset++) {
                    UINT instanceIdx = modelRenderData.visibleHighlightedInstancesIdxs[transformatsBufferOffset];
                    visibleHighlightedInstancesTransformatsStaging[transformatsBufferOffset] = sceneModelData->sceneModelInstances[instanceIdx]->getModelTransformat();
                }
                D3D11_BOX rangeBox = { 0U, 0U, 0U, modelRenderData.visibleHighlightedInstancesNr * sizeof(XMMATRIX), 1U, 1U };
                renderer.devcon->UpdateSubresource(modelRenderData.visibleHighlightedInstancesTransformatsBuffer, 0, &rangeBox, &visibleHighlightedInstancesTransformatsStaging[0], 0, 0);

                rangeBox.right = modelRenderData.visibleHighlightedInstancesNr * sizeof(UINT);
                renderer.devcon->UpdateSubresource(modelRenderData.visibleHighlightedInstancesIdxsBuffer, 0, &rangeBox, &modelRenderData.visibleHighlightedInstancesIdxs[0], 0, 0);
            }            
        }
    }

    DxRenderer::DxRenderer(float _fov, float _nearZ, float _farZ) : fov(_fov), nearZ(_nearZ), farZ(_farZ) {}    

    DxRenderer::~DxRenderer() {
        for (std::vector<ModelRenderData>::iterator it = modelsRenderData.begin(); it != modelsRenderData.end(); it++) {
            (*it).vertexBuffer->Release();
            (*it).indexBuffer->Release();
            (*it).visibleInstancesTransformatsBuffer->Release();
            (*it).visibleHighlightedInstancesTransformatsBuffer->Release();
        }

        modelsRenderData.clear();

        SAFE_RELEASE(dev);
        SAFE_RELEASE(devcon);
        SAFE_RELEASE(cbViewMat);
        SAFE_RELEASE(cbProjMat);
        SAFE_RELEASE(cbModelID);
        SAFE_RELEASE(cbBlur);
        SAFE_RELEASE(blendStateTransparency);

        SAFE_RELEASE(rtView);
        SAFE_RELEASE(selectionTexes[0]);
        SAFE_RELEASE(selectionTexes[1]);
        SAFE_RELEASE(selectionTexes[2]);
        SAFE_RELEASE(selectionRTViews[0]);
        SAFE_RELEASE(selectionRTViews[1]);        
        SAFE_RELEASE(selectionRTViews[2]);
        SAFE_RELEASE(dsStateJustDepth);
        SAFE_RELEASE(dsStateWriteStencil);
        SAFE_RELEASE(dsStateMaskStencil);
        SAFE_RELEASE(dsView);
        SAFE_RELEASE(dsTex);                        
        SAFE_RELEASE(blurSRView0);
        SAFE_RELEASE(blurRTView0);
        SAFE_RELEASE(blurTex0);
        SAFE_RELEASE(blurSRView1);
        SAFE_RELEASE(blurRTView1);
        SAFE_RELEASE(blurTex1);
        SAFE_RELEASE(vsScene);
        SAFE_RELEASE(psScene);
        SAFE_RELEASE(vlScene);
        SAFE_RELEASE(vsBlur);
        SAFE_RELEASE(psBlur);
        SAFE_RELEASE(vlBlur);  

        SAFE_RELEASE(pGraphicsAnalysis);
    }    

    HRESULT DxRenderer::initDirectXLmnts(void* resource) {
        HRESULT hr = S_OK;

        D3D_FEATURE_LEVEL featureLvls[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0
        };

        hr = D3D11CreateDevice(NULL,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
            featureLvls,
            ARRAYSIZE(featureLvls),
            D3D11_SDK_VERSION,
            &dev, NULL, &devcon);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not create a Direct3D 10 or 11 device.");
            goto CreateDeviceFailed;
        }                
        
        ID3D11Debug* d3dDebug;
        hr = dev->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Query of ID3D11Debug failed.");
            devcon->Release();
            dev->Release();
            goto CreateDeviceFailed;
        }
        
        ID3D11InfoQueue* d3dInfoQueue;
        hr = d3dDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue);
        if (FAILED(hr)) {
            d3dDebug->Release();
            devcon->Release();
            dev->Release();
            goto CreateDeviceFailed;
        }
        
        d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
        {
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
                D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET_DUE_TO_FLIP_PRESENT
            };

            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);            
        }
        d3dInfoQueue->Release();
        d3dDebug->Release();        

        D3D11_BUFFER_DESC bd;
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(XMMATRIX);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &cbViewMat);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not create the view matrix constant buffer.");
            goto CbViewMatFailed;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(XMMATRIX);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &cbProjMat);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not create the proj matrix constant buffer.");
            goto CbProjMatFailed;
        }
                
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = 4*sizeof(UINT);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &cbModelID);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not create the model ID constant buffer.");
            goto CbModelIDFailed;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(BlurVars);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &cbBlur);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not create the blur variables constant buffer.");
            goto CbBlurFailed;

            return hr;
        }       

        {
            ScreenQuadVertexData screenQuadVertexData[4] = {
                { {-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f} },
                { {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f} },
                { {-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f} },
                { {1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }
            };
            D3D11_SUBRESOURCE_DATA screenQuadSubresourceData;
            ZeroMemory(&screenQuadSubresourceData, sizeof(screenQuadSubresourceData));
            screenQuadSubresourceData.pSysMem = screenQuadVertexData;
        
            ZeroMemory(&bd, sizeof(bd));
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.ByteWidth = 4 * sizeof(ScreenQuadVertexData);
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = 0;
            hr = dev->CreateBuffer(&bd, &screenQuadSubresourceData, &vbScreenQuad);
            if (FAILED(hr)) {
                OutputDebugStringW(L"Could not create the screen quad vertex buffer.");
                goto VbScreenQuadFailed;
            }
        }

        D3D11_BLEND_DESC blendStateDesc;        
        ZeroMemory(&blendStateDesc, sizeof(blendStateDesc));  
        blendStateDesc.IndependentBlendEnable = TRUE;
        blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
        blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;        
        blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
        blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        blendStateDesc.RenderTarget[1].BlendEnable = FALSE;
        blendStateDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        blendStateDesc.RenderTarget[2].BlendEnable = TRUE;
        blendStateDesc.RenderTarget[2].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendStateDesc.RenderTarget[2].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendStateDesc.RenderTarget[2].BlendOp = D3D11_BLEND_OP_ADD;
        blendStateDesc.RenderTarget[2].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
        blendStateDesc.RenderTarget[2].DestBlendAlpha = D3D11_BLEND_ONE;
        blendStateDesc.RenderTarget[2].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendStateDesc.RenderTarget[2].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = dev->CreateBlendState(&blendStateDesc, &blendStateTransparency);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not load shaders.");
            goto BlendStateTransparencyFailed;

            return hr;
        }
        devcon->OMSetBlendState(blendStateTransparency, NULL, 0xffffffff);

        hr = loadShaders();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not load shaders.");
            goto LoadShadersFailed;

            return hr;
        }        

        hr = initShaderResources(resource);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to initialize the render target.");
            goto InitShaderResourcesFailed;
        }                       

        if (GRAPHICS_DBUG_RUN) {
            hr = DXGIGetDebugInterface1(0, __uuidof(pGraphicsAnalysis), reinterpret_cast<void**>(&pGraphicsAnalysis));
            if (FAILED(hr))
                goto PGraphicsAnalysisFailed;
        }

    Success:
        return hr; 
        
    PGraphicsAnalysisFailed:
    InitShaderResourcesFailed:
    LoadShadersFailed:
        blendStateTransparency->Release();

    BlendStateTransparencyFailed:
        vbScreenQuad->Release();

    VbScreenQuadFailed:
        cbBlur->Release(); 

    CbBlurFailed:
        cbModelID->Release();

    CbModelIDFailed:
        cbProjMat->Release();

    CbProjMatFailed:
        cbViewMat->Release();

    CbViewMatFailed:
        devcon->Release();
        dev->Release();

    CreateDeviceFailed:            
        return hr;
    }

    HRESULT DxRenderer::loadShaders() {
        HRESULT hr = S_OK;
        ID3D10Blob* d3d10Blob = NULL;

        // scene shader
        hr = CompileShaderFromFile(L"D3Dshader.fx", "PS", "ps_4_0", &d3d10Blob);
        if (FAILED(hr)) {
            OutputDebugStringW(L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");

            return hr;
        }

        hr = dev->CreatePixelShader(d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), NULL, &psScene);
        d3d10Blob->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create the scene pixel shader.");

            return hr;
        }

        hr = CompileShaderFromFile(L"D3Dshader.fx", "VS", "vs_4_0", &d3d10Blob);
        if (FAILED(hr)) {
            OutputDebugStringW(L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
            psScene->Release();

            return hr;
        }

        hr = dev->CreateVertexShader(d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), NULL, &vsScene);        
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create the scene vertex shader.");
            psScene->Release();            
            d3d10Blob->Release();

            return hr;
        }

        D3D11_INPUT_ELEMENT_DESC vlSceneLmnts[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"INSTANCE_TRANSFORMAT", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"INSTANCE_TRANSFORMAT", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"INSTANCE_TRANSFORMAT", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"INSTANCE_TRANSFORMAT", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"INSTANCE_IDX", 0, DXGI_FORMAT_R32_UINT, 2, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1}
        };
        hr = dev->CreateInputLayout(vlSceneLmnts, ARRAYSIZE(vlSceneLmnts), d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), &vlScene);
        d3d10Blob->Release();
        if (FAILED(hr)) {
            psScene->Release();
            vsScene->Release();

            return hr;
        }

        // blur shader
        hr = CompileShaderFromFile(L"D3Dshader.fx", "PSBlur", "ps_4_0", &d3d10Blob);
        if (FAILED(hr)) {
            OutputDebugStringW(L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();

            return hr;
        }

        hr = dev->CreatePixelShader(d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), NULL, &psBlur);
        d3d10Blob->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create the blur pixel shader.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();

            return hr;
        }

        hr = CompileShaderFromFile(L"D3Dshader.fx", "VSBlur", "vs_4_0", &d3d10Blob);
        if (FAILED(hr)) {
            OutputDebugStringW(L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();
            psBlur->Release();

            return hr;
        }

        hr = dev->CreateVertexShader(d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), NULL, &vsBlur);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create the blur vertex shader.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();
            psBlur->Release();
            d3d10Blob->Release();

            return hr;
        }

        D3D11_INPUT_ELEMENT_DESC vlBlurLmnts[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };
        hr = dev->CreateInputLayout(vlBlurLmnts, ARRAYSIZE(vlBlurLmnts), d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), &vlBlur);
        d3d10Blob->Release();
        if (FAILED(hr)) {
            psScene->Release();
            vsScene->Release();
            vlScene->Release();
            psBlur->Release();
            vsBlur->Release();

            return hr;
        }

        hr = CompileShaderFromFile(L"D3Dshader.fx", "PSOutline", "ps_4_0", &d3d10Blob);
        if (FAILED(hr)) {
            OutputDebugStringW(L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();
            psBlur->Release();
            vsBlur->Release();

            return hr;
        }

        hr = dev->CreatePixelShader(d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), NULL, &psOutline); 
        d3d10Blob->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create the scene pixel shader.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();
            psBlur->Release();
            vsBlur->Release();

            return hr;
        }

        hr = CompileShaderFromFile(L"D3Dshader.fx", "VSOutline", "vs_4_0", &d3d10Blob);
        if (FAILED(hr)) {
            OutputDebugStringW(L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();
            psBlur->Release();
            vsBlur->Release();
            psOutline->Release();

            return hr;
        }

        hr = dev->CreateVertexShader(d3d10Blob->GetBufferPointer(), d3d10Blob->GetBufferSize(), NULL, &vsOutline);
        d3d10Blob->Release();
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create the scene vertex shader.");
            psScene->Release();
            vsScene->Release();
            vlScene->Release();
            psBlur->Release();
            vsBlur->Release();
            psOutline->Release();

            return hr;
        }

        return hr;
    }

    HRESULT DxRenderer::initShaderResources(void* resource) {
        HRESULT hr = S_OK;
        IUnknown* pUnk = (IUnknown*)resource;

        IDXGIResource* pDXGIResource;
        hr = pUnk->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
        if (FAILED(hr))
            return hr;

        HANDLE sharedHandle;
        hr = pDXGIResource->GetSharedHandle(&sharedHandle);
        pDXGIResource->Release();
        if (FAILED(hr))
            return hr;

        IUnknown* tempResource11;
        hr = dev->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)(&tempResource11));
        if (FAILED(hr))
            return hr;

        ID3D11Texture2D* pOutputResource;
        hr = tempResource11->QueryInterface(__uuidof(ID3D11Texture2D), (void**)(&pOutputResource));
        tempResource11->Release();
        if (FAILED(hr))
            return hr;                

        D3D11_RENDER_TARGET_VIEW_DESC rtDesc;
        ZeroMemory(&rtDesc, sizeof(rtDesc));
        rtDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtDesc.Texture2D.MipSlice = 0;        
        hr = dev->CreateRenderTargetView(pOutputResource, &rtDesc, &rtView);
        if (FAILED(hr))
            return hr;

        D3D11_TEXTURE2D_DESC outputResourceDesc;
        ZeroMemory(&outputResourceDesc, sizeof(outputResourceDesc));
        pOutputResource->GetDesc(&outputResourceDesc);
        if (viewportWidth != outputResourceDesc.Width || viewportHeight != outputResourceDesc.Height) {
            // set up the viewport
            D3D11_VIEWPORT viewport;
            viewport.Width = viewportWidth = outputResourceDesc.Width;
            viewport.Height = viewportHeight = outputResourceDesc.Height;
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            devcon->RSSetViewports(1, &viewport);
            for (std::list<Scene*>::iterator it = scenes.begin(); it != scenes.end(); it++)
                (*it)->camera.updateScreenSz(viewportWidth, viewportHeight);
        }
        pOutputResource->Release();                
        
        D3D11_TEXTURE2D_DESC selectionTexDesc;
        ZeroMemory(&selectionTexDesc, sizeof(selectionTexDesc));
        selectionTexDesc.Width = outputResourceDesc.Width;
        selectionTexDesc.Height = outputResourceDesc.Height;
        selectionTexDesc.MipLevels = 1;
        selectionTexDesc.ArraySize = 1; // TODO: ???
        selectionTexDesc.Format = DXGI_FORMAT_R16G16_UINT;
        selectionTexDesc.SampleDesc.Count = 1; // TODO: ???
        selectionTexDesc.Usage = D3D11_USAGE_DEFAULT;
        selectionTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET;        
        hr = dev->CreateTexture2D(&selectionTexDesc, NULL, &selectionTexes[0]);
        if (FAILED(hr))
            goto SelectionTex0Fail;

        hr = dev->CreateTexture2D(&selectionTexDesc, NULL, &selectionTexes[1]);
        if (FAILED(hr))
            goto SelectionTex1Fail;

        hr = dev->CreateTexture2D(&selectionTexDesc, NULL, &selectionTexes[2]);
        if (FAILED(hr))
            goto SelectionTex2Fail;

        selectionTexDesc.Usage = D3D11_USAGE_STAGING;
        selectionTexDesc.BindFlags = 0;
        selectionTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;        
        hr = dev->CreateTexture2D(&selectionTexDesc, NULL, &stagingSelectionTex);
        if (FAILED(hr))
            goto StagingSelectionTexFail;

        D3D11_RENDER_TARGET_VIEW_DESC selectionRTViewDesc;
        ZeroMemory(&selectionRTViewDesc, sizeof(selectionRTViewDesc));
        selectionRTViewDesc.Format = selectionTexDesc.Format;
        selectionRTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        selectionRTViewDesc.Texture2D.MipSlice = 0;
        hr = dev->CreateRenderTargetView(selectionTexes[0], &selectionRTViewDesc, &selectionRTViews[0]);
        if (FAILED(hr))
            goto SelectionRTView0Fail;

        hr = dev->CreateRenderTargetView(selectionTexes[1], &selectionRTViewDesc, &selectionRTViews[1]);
        if (FAILED(hr))
            goto SelectionRTView1Fail;

        hr = dev->CreateRenderTargetView(selectionTexes[2], &selectionRTViewDesc, &selectionRTViews[2]);
        if (FAILED(hr))
            goto SelectionRTView2Fail;

        D3D11_TEXTURE2D_DESC blurTexDesc;
        ZeroMemory(&blurTexDesc, sizeof(blurTexDesc));
        blurTexDesc.Width = outputResourceDesc.Width;
        blurTexDesc.Height = outputResourceDesc.Height;
        blurTexDesc.MipLevels = 1; // TODO: ???
        blurTexDesc.ArraySize = 1; // TODO: ???
        blurTexDesc.Format = outputResourceDesc.Format;
        blurTexDesc.SampleDesc.Count = 1; // TODO: ???
        blurTexDesc.Usage = D3D11_USAGE_DEFAULT;
        blurTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;        
        hr = dev->CreateTexture2D(&blurTexDesc, NULL, &blurTex0);
        if (FAILED(hr))
            goto BlurTex0Fail;

        hr = dev->CreateTexture2D(&blurTexDesc, NULL, &blurTex1);
        if (FAILED(hr))
            goto BlurTex1Fail;        

        D3D11_SAMPLER_DESC sampDesc;
        ZeroMemory(&sampDesc, sizeof(sampDesc));
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = -FLT_MAX;
        sampDesc.MaxLOD = FLT_MAX;
        hr = dev->CreateSamplerState(&sampDesc, &blurTexSampState);
        if (FAILED(hr))
            goto BlurTexSampStateFail;

        D3D11_RENDER_TARGET_VIEW_DESC blurRTViewDesc;
        ZeroMemory(&blurRTViewDesc, sizeof(blurRTViewDesc));
        blurRTViewDesc.Format = blurTexDesc.Format;
        blurRTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        blurRTViewDesc.Texture2D.MipSlice = 0;        
        hr = dev->CreateRenderTargetView(blurTex0, &blurRTViewDesc, &blurRTView0);
        if (FAILED(hr))
            goto BlurRTView0Fail;

        hr = dev->CreateRenderTargetView(blurTex1, &blurRTViewDesc, &blurRTView1);
        if (FAILED(hr))
            goto BlurRTView1Fail;
        
        D3D11_SHADER_RESOURCE_VIEW_DESC blurSRViewDesc;
        ZeroMemory(&blurSRViewDesc, sizeof(blurSRViewDesc));
        blurSRViewDesc.Format = blurTexDesc.Format;
        blurSRViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        blurSRViewDesc.Texture2D.MostDetailedMip = 0;
        blurSRViewDesc.Texture2D.MipLevels = 1;
        hr = dev->CreateShaderResourceView(blurTex0, &blurSRViewDesc, &blurSRView0);
        if (FAILED(hr))
            goto BlurSRView0Fail;

        hr = dev->CreateShaderResourceView(blurTex1, &blurSRViewDesc, &blurSRView1);
        if (FAILED(hr))
            goto BlurSRView1Fail;

        {   // section 6.7 of the C++ standard:
            // A program that jumps from a point where a local variable with automatic storage duration is not 
            // in scope to a point where it is in scope is ill-formed
            float tu = 1.0f / outputResourceDesc.Width;
            float tv = 1.0f / outputResourceDesc.Height;
            int blurSpanMid = BLUR_SPAN / 2;
            // Fill one side
            for (int sampleIdx = 0; sampleIdx < blurSpanMid + 1; sampleIdx++) {
                blurVarsHorizon.offsets[blurSpanMid - sampleIdx] = XMFLOAT4(-sampleIdx * tu, 0.0f, 0.0f, 0.0f);
                blurVarsVert.offsets[blurSpanMid - sampleIdx] = XMFLOAT4(0.0f, -sampleIdx * tv, 0.0f, 0.0f);
                float weight = BLUR_FACTOR * gaussDistrib((float)sampleIdx, 0, BLUR_STD);
                blurVarsHorizon.weights[blurSpanMid - sampleIdx] = blurVarsVert.weights[blurSpanMid - sampleIdx] = XMFLOAT4(weight, weight, weight, weight);
            }
            // Copy to the other side
            for (int sampleIdx = blurSpanMid + 1; sampleIdx < BLUR_SPAN; sampleIdx++) {
                blurVarsHorizon.offsets[sampleIdx] = XMFLOAT4(-blurVarsHorizon.offsets[BLUR_SPAN - 1 - sampleIdx].x, 0.0f, 0.0f, 0.0f);
                blurVarsVert.offsets[sampleIdx] = XMFLOAT4(0.0f, -blurVarsVert.offsets[BLUR_SPAN - 1 - sampleIdx].y, 0.0f, 0.0f);
                blurVarsHorizon.weights[sampleIdx] = blurVarsVert.weights[sampleIdx] = blurVarsHorizon.weights[BLUR_SPAN - 1 - sampleIdx];
            }
        }

        D3D11_TEXTURE2D_DESC dsTexDesc;
        ZeroMemory(&dsTexDesc, sizeof(dsTexDesc));
        dsTexDesc.Width = outputResourceDesc.Width;
        dsTexDesc.Height = outputResourceDesc.Height;
        dsTexDesc.MipLevels = 1;
        dsTexDesc.ArraySize = 1;
        dsTexDesc.SampleDesc.Count = 1;
        dsTexDesc.SampleDesc.Quality = 0;
        dsTexDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsTexDesc.Usage = D3D11_USAGE_DEFAULT;
        dsTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        dsTexDesc.CPUAccessFlags = 0;
        dsTexDesc.MiscFlags = 0;
        hr = dev->CreateTexture2D(&dsTexDesc, NULL, &dsTex);
        if (FAILED(hr) != S_OK)
            goto DsTexFail;

        D3D11_DEPTH_STENCIL_VIEW_DESC dsViewDesc;
        ZeroMemory(&dsViewDesc, sizeof(dsViewDesc));
        dsViewDesc.Format = dsTexDesc.Format;
        dsViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        dsViewDesc.Texture2D.MipSlice = 0;
        hr = dev->CreateDepthStencilView(dsTex, &dsViewDesc, &dsView);
        if (FAILED(hr))
            goto DsViewFail;

        D3D11_DEPTH_STENCIL_DESC dsStateDesc;        
        dsStateDesc.DepthEnable = true;
        dsStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        dsStateDesc.StencilEnable = false;

        hr = dev->CreateDepthStencilState(&dsStateDesc, &dsStateJustDepth);
        if (FAILED(hr))
            goto DsStateJustDepthFail;

        dsStateDesc.DepthEnable = true;
        dsStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        dsStateDesc.StencilEnable = true;
        dsStateDesc.StencilReadMask = 0xFF;
        dsStateDesc.StencilWriteMask = 0xFF;

        dsStateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
        dsStateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        dsStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

        hr = dev->CreateDepthStencilState(&dsStateDesc, &dsStateWriteStencil);
        if (FAILED(hr))
            goto DsStateWriteStencilFail;
            
        dsStateDesc.DepthEnable = false;
        dsStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsStateDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        dsStateDesc.StencilEnable = true;
        dsStateDesc.StencilReadMask = 0xFF;
        dsStateDesc.StencilWriteMask = 0xFF;

        dsStateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

        dsStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dsStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

        hr = dev->CreateDepthStencilState(&dsStateDesc, &dsStateMaskStencil);
        if (FAILED(hr))
            goto DsStateMaskStencilFail;        

    SUCCESS:
        return hr;

    DsStateMaskStencilFail:
        dsStateWriteStencil->Release();

    DsStateWriteStencilFail:
        dsStateJustDepth->Release();

    DsStateJustDepthFail:
        dsView->Release();

    DsViewFail:
        dsTex->Release();

    DsTexFail:
        blurSRView1->Release();

    BlurSRView1Fail:
        blurSRView0->Release();

    BlurSRView0Fail:
        blurRTView1->Release();

    BlurRTView1Fail:
        blurRTView0->Release();

    BlurRTView0Fail:
        blurTexSampState->Release();

    BlurTexSampStateFail:
        blurTex1->Release();

    BlurTex1Fail:
        blurTex0->Release();

    BlurTex0Fail:
        selectionRTViews[2]->Release();

    SelectionRTView2Fail:
        selectionRTViews[1]->Release();

    SelectionRTView1Fail:
        selectionRTViews[0]->Release();

    SelectionRTView0Fail:
        stagingSelectionTex->Release();

    StagingSelectionTexFail:
        selectionTexes[2]->Release();

    SelectionTex2Fail:
        selectionTexes[1]->Release();

    SelectionTex1Fail:
        selectionTexes[0]->Release();

    SelectionTex0Fail:
        rtView->Release();
      
        return hr;
    }
    
    void DxRenderer::startFrameCapture() {        
        if (framesNrToCapture)
            pGraphicsAnalysis->BeginCapture();
    }

    void DxRenderer::endFrameCapture() {
        if (framesNrToCapture) {
            pGraphicsAnalysis->EndCapture();
            framesNrToCapture--;
        }
    }

    HRESULT DxRenderer::addModel(std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, XMFLOAT3 const& boundingSphereCenter, float boundingSphereRadius, D3D_PRIMITIVE_TOPOLOGY primitiveTopology, UINT* modelIDOut) {
        HRESULT hr = S_OK;
        D3D11_BUFFER_DESC bd;
        D3D11_SUBRESOURCE_DATA initData;
        ModelRenderData modelRenderData;

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = modelVertices.size() * sizeof(VertexData);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        ZeroMemory(&initData, sizeof(initData));
        initData.pSysMem = &modelVertices[0];
        hr = dev->CreateBuffer(&bd, &initData, &modelRenderData.vertexBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create a vertex buffer.");

            return hr;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = modelVertexIndices.size() * sizeof(WORD);
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;

        ZeroMemory(&initData, sizeof(initData));
        initData.pSysMem = &modelVertexIndices[0];        
        hr = dev->CreateBuffer(&bd, &initData, &modelRenderData.indexBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an index buffer.");
            modelRenderData.vertexBuffer->Release();

            return hr;
        }
                
        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = INSTANCES_BUFFERS_CAPACITY_INIT * sizeof(XMMATRIX);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &modelRenderData.visibleInstancesTransformatsBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an instances buffer..");
            modelRenderData.vertexBuffer->Release();
            modelRenderData.indexBuffer->Release();

            return hr;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = INSTANCES_BUFFERS_CAPACITY_INIT * sizeof(UINT);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &modelRenderData.visibleInstancesIdxsBuffer);        
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an instances buffer..");
            modelRenderData.vertexBuffer->Release();
            modelRenderData.indexBuffer->Release();
            modelRenderData.visibleInstancesTransformatsBuffer->Release();

            return hr;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = INSTANCES_BUFFERS_CAPACITY_INIT * sizeof(XMMATRIX);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &modelRenderData.visibleHighlightedInstancesTransformatsBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an instances buffer..");
            modelRenderData.vertexBuffer->Release();
            modelRenderData.indexBuffer->Release();
            modelRenderData.visibleInstancesTransformatsBuffer->Release();
            modelRenderData.visibleInstancesIdxsBuffer->Release();

            return hr;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = INSTANCES_BUFFERS_CAPACITY_INIT * sizeof(UINT);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &modelRenderData.visibleHighlightedInstancesIdxsBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an instances buffer..");
            modelRenderData.vertexBuffer->Release();
            modelRenderData.indexBuffer->Release();
            modelRenderData.visibleInstancesTransformatsBuffer->Release();
            modelRenderData.visibleInstancesIdxsBuffer->Release();
            modelRenderData.visibleHighlightedInstancesTransformatsBuffer->Release();

            return hr;
        }

        modelRenderData.boundingSphere = BoundingSphere(XMLoadFloat3(&boundingSphereCenter), boundingSphereRadius);
        modelRenderData.primitiveTopology = primitiveTopology;

        unsigned int modelID = modelsIDsPool.acquireIdx();
        *modelIDOut = modelID;
        modelsRenderData[modelID] = modelRenderData;

        return hr;
    }

    HRESULT DxRenderer::updateModelData(UINT modelID, std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, D3D_PRIMITIVE_TOPOLOGY primitiveTopology) {
        // TODO: Implement this
        return S_OK;
    }

    HRESULT DxRenderer::removeModel(UINT modelID) {
        for (std::list<Scene*>::iterator scenesIt = scenes.begin(); scenesIt != scenes.end(); scenesIt++) {
            std::list<Scene::SceneModelData*>& sceneModelsData = (*scenesIt)->sceneModelsData;
            for (std::list<Scene::SceneModelData*>::iterator sceneModelsDataIt = sceneModelsData.begin(); sceneModelsDataIt != sceneModelsData.end(); sceneModelsDataIt++) {
                if ((*sceneModelsDataIt)->modelID == modelID) {
                    (*sceneModelsDataIt)->sceneModelInstances.clear(); // TODO: delete the scene models instances
                    sceneModelsData.erase(sceneModelsDataIt);
                    break;
                }
            }
        }

        modelsRenderData[modelID].vertexBuffer->Release();
        modelsRenderData[modelID].indexBuffer->Release();
        modelsRenderData[modelID].visibleInstancesTransformatsBuffer->Release();
        modelsRenderData[modelID].visibleHighlightedInstancesTransformatsBuffer->Release();
        modelsIDsPool.releaseIdx(modelID);

        return S_OK;
    }

    DxRenderer::Scene* DxRenderer::createScene() {
        Scene* scene = new Scene(*this);
        scenes.push_back(scene);

        return scene;
    }  
        
    HRESULT DxRenderer::render() {
        HRESULT hr = S_OK;
        
        devcon->ClearRenderTargetView(rtView, CLEAR_COLOR);  
        UINT selectionTexIdxToUpdate = (updatedSelectionTexIdx + 1) % 3;
        devcon->ClearRenderTargetView(selectionRTViews[selectionTexIdxToUpdate], SELECTION_TEXES_CLEAR_COLOR);
        devcon->ClearDepthStencilView(dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);        
        if (activeScene != NULL) {  
            int objIdx = 0;
            for (std::list<Scene::SceneModelData*>::iterator it = activeScene->sceneModelsData.begin(); it != activeScene->sceneModelsData.end(); it++) {                                
                unsigned int modelID = (*it)->modelID;
                ModelRenderData modelRenderData = modelsRenderData[modelID];                
                if (modelRenderData.visibleInstancesNr || modelRenderData.visibleHighlightedInstancesNr) {
                    devcon->PSSetShader(psScene, NULL, 0);
                    devcon->VSSetShader(vsScene, NULL, 0);
                    devcon->IASetInputLayout(vlScene);                    

                    devcon->UpdateSubresource(cbModelID, 0, NULL, &modelID, 0, 0);

                    ID3D11Buffer* constBuffers[3] = { cbViewMat, cbProjMat, cbModelID };
                    devcon->VSSetConstantBuffers(0, 3, constBuffers);

                    ID3D11Buffer* vertexBuffers[3] = { modelRenderData.vertexBuffer, modelRenderData.visibleInstancesTransformatsBuffer, modelRenderData.visibleInstancesIdxsBuffer };
                    UINT strides[3] = { sizeof(VertexData), sizeof(XMMATRIX), sizeof(UINT) };
                    UINT offsets[3] = { 0, 0, 0 };
                    devcon->IASetVertexBuffers(0, 3, vertexBuffers, strides, offsets);

                    ID3D11Buffer* indexBuffer = modelRenderData.indexBuffer;
                    devcon->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
                    D3D11_BUFFER_DESC bd;
                    indexBuffer->GetDesc(&bd);

                    devcon->IASetPrimitiveTopology(modelRenderData.primitiveTopology);

                    devcon->OMSetDepthStencilState(dsStateJustDepth, 0);
                    
                    if (modelID == 1)
                        startFrameCapture();

                    if (modelRenderData.visibleInstancesNr) {
                        ID3D11RenderTargetView* sceneRenderTargets[2] = { rtView , selectionRTViews[selectionTexIdxToUpdate] };
                        devcon->OMSetRenderTargets(2, sceneRenderTargets, dsView);                        
                        devcon->DrawIndexedInstanced(bd.ByteWidth / sizeof(WORD), modelRenderData.visibleInstancesNr, 0, 0, 0);
                        updatedSelectionTexIdx = selectionTexIdxToUpdate;                        
                    }

                    if (modelRenderData.visibleHighlightedInstancesNr) {
                        devcon->ClearRenderTargetView(blurRTView0, BLUR_TEXES_CLEAR_COLOR);
                        devcon->ClearRenderTargetView(blurRTView1, BLUR_TEXES_CLEAR_COLOR);

                        // render the highlighted object targeting blur texture0 with stencil write
                        vertexBuffers[1] = modelRenderData.visibleHighlightedInstancesTransformatsBuffer;
                        vertexBuffers[2] = modelRenderData.visibleHighlightedInstancesIdxsBuffer;
                        devcon->IASetVertexBuffers(0, 3, vertexBuffers, strides, offsets);
                        
                        devcon->OMSetDepthStencilState(dsStateWriteStencil, 0);
                        ID3D11RenderTargetView* sceneRenderTargets[3] = { rtView, selectionRTViews[selectionTexIdxToUpdate], blurRTView0 };
                        devcon->OMSetRenderTargets(3, sceneRenderTargets, dsView);                        
                        devcon->DrawIndexedInstanced(bd.ByteWidth / sizeof(WORD), modelRenderData.visibleHighlightedInstancesNr, 0, 0, 0);
                        updatedSelectionTexIdx = selectionTexIdxToUpdate;
                        devcon->OMSetRenderTargets(1, &rtView, NULL); // temporarily rebind the back buffer

                        // render the texture with the blur algo twice, and the result to the back buffer: blurRTView0 -> blurRTView1 -> blurRTView0 -> back buffer
                        devcon->PSSetShader(psBlur, NULL, 0);
                        devcon->VSSetShader(vsBlur, NULL, 0);
                        devcon->IASetInputLayout(vlBlur);

                        UINT stride = sizeof(ScreenQuadVertexData);
                        UINT offset = 0;
                        devcon->IASetVertexBuffers(0, 1, &vbScreenQuad, &stride, &offset);

                        devcon->PSSetConstantBuffers(3, 1, &cbBlur);

                        devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                        devcon->PSSetSamplers(0, 1, &blurTexSampState);

                        // blur pass 1                                        
                        devcon->UpdateSubresource(cbBlur, 0, NULL, &blurVarsHorizon, 0, 0);
                        devcon->PSSetShaderResources(0, 1, &blurSRView0);
                        devcon->OMSetRenderTargets(1, &blurRTView1, NULL);
                        devcon->Draw(4, 0);
                        devcon->OMSetRenderTargets(1, &rtView, NULL); // temporarily rebind the back buffer

                        // blur pass 2                    
                        devcon->UpdateSubresource(cbBlur, 0, NULL, &blurVarsVert, 0, 0);
                        devcon->PSSetShaderResources(0, 1, &blurSRView1);
                        devcon->OMSetDepthStencilState(dsStateMaskStencil, 0);
                        devcon->OMSetRenderTargets(1, &rtView, dsView);
                        devcon->Draw(4, 0);
                        //devcon->OMSetRenderTargets(1, &rtView, NULL); // temporarily rebind the back buffer

                        /*
                        // render the result to the back buffer
                        devcon->PSSetShader(psOutline, NULL, 0);
                        devcon->VSSetShader(vsOutline, NULL, 0);
                        devcon->PSSetShaderResources(0, 1, &blurSRView0);
                        devcon->OMSetDepthStencilState(dsStateMaskStencil, 0);
                        devcon->OMSetRenderTargets(1, &rtView, dsView);
                        devcon->Draw(4, 0);
                        devcon->PSSetShaderResources(0, 1, &blurSRView1); // temporarily rebind the back buffer
                        */
                    }
                    
                    if (modelID == 1)
                        endFrameCapture();
                }                
            }
            devcon->OMSetRenderTargets(1, &rtView, dsView);

            devcon->Flush();
        }        

        return hr;
    }

    void DxRenderer::captureFrame() {
        framesNrToCapture++;
    }
}