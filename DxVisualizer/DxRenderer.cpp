#include "DxRenderer.h"
#include "DxUtils.h"

#include <exception>

using namespace DirectX;

inline float gaussDistrib(float x, float y, float rho) {
    return 1.0f / sqrtf(2.0f * XM_PI * rho * rho) * expf(-(x*x + y*y) / (2 * rho * rho));    
}

namespace CoriumDirectX {    
    const float DxRenderer::BLUR_STD = 5.0f;
    const float DxRenderer::BLUR_FACTOR = 1.75f;
    const FLOAT DxRenderer::CLEAR_COLOR[4] = { 224.0f / 255, 224.0f / 255, 224.0f / 255, 1.0f };
    const FLOAT DxRenderer::SELECTION_TEXES_CLEAR_COLOR[2] = { (FLOAT)MODELS_NR_MAX , (FLOAT)INSTANCES_NR_MAX };    
    const FLOAT DxRenderer::BLUR_TEXES_CLEAR_COLOR[4] = { CLEAR_COLOR[0], CLEAR_COLOR[1], CLEAR_COLOR[2], 0.0f };
    XMMATRIX DxRenderer::Transform::genTransformat() const {
        XMMATRIX transformat = XMMatrixScaling(scaleFactor.x, scaleFactor.y, scaleFactor.z);
        transformat = XMMatrixMultiply(XMMatrixRotationAxis(XMLoadFloat3(&rotAx), rotAng), transformat);
        transformat = XMMatrixMultiply(XMMatrixTranslationFromVector(XMLoadFloat3(&translation)), transformat);

        return transformat;
    }

    void DxRenderer::Scene::SceneModelInstance::translate(XMFLOAT3 const& _translation) {
        XMVECTOR translation = XMLoadFloat3(&_translation);
        pos += translation;
        modelTransformat = XMMatrixMultiply(XMMatrixTranslationFromVector(translation), modelTransformat);
        loadDataToBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::setTranslation(XMFLOAT3 const& translation) {
        pos = XMLoadFloat3(&translation);
        recompTransformat();
        loadDataToBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::scale(XMFLOAT3 const& _scaleFactorQ) {
        XMVECTOR scaleFactorQ = XMLoadFloat3(&_scaleFactorQ);
        scaleFactor *= scaleFactorQ;
        modelTransformat = XMMatrixMultiply(XMMatrixScaling(XMVectorGetX(scaleFactor), XMVectorGetY(scaleFactor), XMVectorGetZ(scaleFactor)), modelTransformat);
        loadDataToBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::setScale(XMFLOAT3 const& _scaleFactor) {
        scaleFactor = XMLoadFloat3(&_scaleFactor);
        recompTransformat();
        loadDataToBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::rotate(XMFLOAT3 const& ax, float ang) {
        XMVECTOR rotQuat = XMQuaternionRotationAxis(XMVectorSet(ax.x, ax.y, ax.z, 0.0f), ang);
        rot *= rotQuat;
        modelTransformat = XMMatrixMultiply(XMMatrixRotationQuaternion(rotQuat), modelTransformat);
        loadDataToBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::setRotation(XMFLOAT3 const& ax, float ang) {
        rot = XMQuaternionRotationAxis(XMVectorSet(ax.x, ax.y, ax.z, 0.0f), ang);
        recompTransformat();
        loadDataToBuffers();
    }

    void DxRenderer::Scene::SceneModelInstance::highlight() {
        for (std::list<SceneModelData>::iterator it = scene.sceneModelsData.begin(); it != scene.sceneModelsData.end(); it++) {
            SceneModelData& sceneModelData = (*it);
            if (sceneModelData.modelID == modelID) {
                if (instanceIdx > 0) {
                    std::list<SceneModelInstance*>::iterator instanceIt = sceneModelData.sceneModelInstances.begin();                
                    std::advance(instanceIt, instanceIdx);                 

                    SceneModelInstance* frontInstance = sceneModelData.sceneModelInstances.front();
                    sceneModelData.sceneModelInstances.insert(instanceIt, frontInstance);
                    frontInstance->instanceIdx = instanceIdx;
                    frontInstance->loadDataToBuffers();
                    sceneModelData.sceneModelInstances.pop_front();

                    sceneModelData.sceneModelInstances.push_front(this);
                    instanceIdx = 0;
                    loadDataToBuffers();
                    sceneModelData.sceneModelInstances.erase(instanceIt);                    
                }
                                
                scene.highlightedModelID = modelID;
            }            
        }
    }    

    void DxRenderer::Scene::SceneModelInstance::release() {
        for (std::list<SceneModelData>::iterator it = scene.sceneModelsData.begin(); it != scene.sceneModelsData.end(); it++) {
            SceneModelData& sceneModelData = (*it);
            if (sceneModelData.modelID == modelID) {     
                std::list<SceneModelInstance*>::iterator instanceIt = sceneModelData.sceneModelInstances.begin();
                std::advance(instanceIt, instanceIdx);
                if (instanceIdx < sceneModelData.sceneModelInstances.size() - 1) {
                    SceneModelInstance* backInstance = sceneModelData.sceneModelInstances.back();                    
                    sceneModelData.sceneModelInstances.insert(instanceIt, backInstance);
                    backInstance->instanceIdx = instanceIdx;
                    backInstance->loadDataToBuffers();
                    sceneModelData.sceneModelInstances.pop_back();
                }
                sceneModelData.sceneModelInstances.erase(instanceIt);

                if (sceneModelData.sceneModelInstances.size() == 0)
                    scene.sceneModelsData.erase(it);

                delete this;
                return;
            }
        }

        throw std::exception("release was called on a SceneModelInstance of a removed model.");
    }

    DxRenderer::Scene::SceneModelInstance::SceneModelInstance(Scene& _scene, UINT _modelID, UINT _instanceIdx, Transform const& transform, SceneModelInstance::SelectionHandler _selectionHandler) :
            scene(_scene), modelID(_modelID), instanceIdx(_instanceIdx), selectionHandler(_selectionHandler),
            pos(XMVectorSet(transform.translation.x, transform.translation.y, transform.translation.z, 0.0f)),
            scaleFactor(XMVectorSet(transform.scaleFactor.x, transform.scaleFactor.y, transform.scaleFactor.z, 0.0f)),
            rot(XMQuaternionRotationAxis(XMVectorSet(transform.rotAx.x, transform.rotAx.y, transform.rotAx.z, 0.0f), transform.rotAng)), 
            modelTransformat(transform.genTransformat()) {}

    void DxRenderer::Scene::SceneModelInstance::loadDataToBuffers() {
        UINT transformatsBufferOffset = instanceIdx * sizeof(XMMATRIX);
        D3D11_BOX rangeBox = { transformatsBufferOffset , 0U, 0U, transformatsBufferOffset + sizeof(XMMATRIX), 1U, 1U };
        scene.renderer.devcon->UpdateSubresource(scene.renderer.modelsRenderData[modelID].instancesTransformatsBuffer, 0, &rangeBox, &(modelTransformat), 0, 0);
    }

    void DxRenderer::Scene::SceneModelInstance::recompTransformat() {
        modelTransformat = XMMatrixTranslationFromVector(pos);        
        modelTransformat = XMMatrixMultiply(XMMatrixRotationQuaternion(rot) , modelTransformat);
        modelTransformat = XMMatrixMultiply(XMMatrixScaling(XMVectorGetX(scaleFactor), XMVectorGetY(scaleFactor), XMVectorGetZ(scaleFactor)), modelTransformat);
    }

    void DxRenderer::Scene::activate() {
        renderer.activeScene = this;
        highlightedModelID = MODELS_NR_MAX;
        loadDataToBuffers();
    }

    DxRenderer::Scene::SceneModelInstance* DxRenderer::Scene::createModelInstance(UINT modelID, Transform const& transformInit, SceneModelInstance::SelectionHandler selectionHandler) {
        for (std::list<SceneModelData>::iterator it = sceneModelsData.begin(); it != sceneModelsData.end(); it++) {
            SceneModelData& sceneModelData = (*it);
            if (sceneModelData.modelID == modelID) {
                SceneModelInstance* sceneModelInstance = new SceneModelInstance(*this, modelID, sceneModelData.sceneModelInstances.size(), transformInit, selectionHandler);
                sceneModelInstance->loadDataToBuffers();
                sceneModelData.sceneModelInstances.push_back(sceneModelInstance);                
                
                return sceneModelInstance;
            }
        }

        // modelID was not found
        SceneModelData sceneModelData;
        sceneModelData.modelID = modelID;
        SceneModelInstance* sceneModelInstance = new SceneModelInstance(*this, modelID, 0, transformInit, selectionHandler);
        sceneModelInstance->loadDataToBuffers();
        sceneModelData.sceneModelInstances.push_back(sceneModelInstance);
        sceneModelsData.push_back(sceneModelData);

        return sceneModelInstance;
    }

    void DxRenderer::Scene::panCamera(float x, float y) {
        camera.panViaViewportDrag(x, y);
        loadViewMatToBuffer();
    }

    void DxRenderer::Scene::rotateCamera(float x, float y) {
        camera.rotateViaViewportDrag(x, y);
        loadViewMatToBuffer();
    }

    void DxRenderer::Scene::zoomCamera(float amount) {
        camera.zoom(amount);
        loadViewMatToBuffer();
    }

    XMFLOAT3 DxRenderer::Scene::getCameraPos() {
        XMVECTOR cameraPos = camera.getPos();

        return XMFLOAT3(XMVectorGetX(cameraPos), XMVectorGetY(cameraPos), XMVectorGetZ(cameraPos));
    }
    
    void DxRenderer::Scene::cursorSelect(float x, float y) {
        UINT selectionX = (UINT)floorf(x), selectionY = (UINT)floorf(y);
        D3D11_BOX regionBox = { selectionX , selectionY, 0, selectionX + 1, selectionY + 1, 1 };
        renderer.devcon->CopySubresourceRegion(renderer.stagingSelectionTex, 0, selectionX, selectionY, 0, renderer.selectionTexes[(renderer.updatedSelectionTexIdx + 1) % 3], 0, &regionBox);
        D3D11_MAPPED_SUBRESOURCE selectionTexMapped;
        HRESULT hr = renderer.devcon->Map(renderer.stagingSelectionTex, 0, D3D11_MAP_READ, 0, &selectionTexMapped);
        if FAILED(hr) {
            OutputDebugStringW(L"Could not map a selection texture.");
            return;
        }
        
        SceneModelInstanceIdxs sceneModelInstanceIdxs = ((SceneModelInstanceIdxs*)selectionTexMapped.pData)[selectionTexMapped.RowPitch/sizeof(SceneModelInstanceIdxs)*selectionY + selectionX];
        if (sceneModelInstanceIdxs.modelID < MODELS_NR_MAX) {
            for (std::list<SceneModelData>::iterator modelsIt = sceneModelsData.begin(); modelsIt != sceneModelsData.end(); modelsIt++) {
                SceneModelData& sceneModelData = (*modelsIt);
                if (sceneModelData.modelID == sceneModelInstanceIdxs.modelID) {
                    std::list<SceneModelInstance*>::iterator instanceIt = sceneModelData.sceneModelInstances.begin();
                    std::advance(instanceIt, sceneModelInstanceIdxs.instanceIdx);
                    if ((*instanceIt)->selectionHandler)
                        (*instanceIt)->selectionHandler();
                    break;
                }
            }
        }
    }

    XMFLOAT3 DxRenderer::Scene::cursorPosToRayDirection(float x, float y)
    {
        XMVECTOR rayDirection = camera.cursorPosToRayDirection(x, y);
        return XMFLOAT3(XMVectorGetX(rayDirection), XMVectorGetY(rayDirection), XMVectorGetZ(rayDirection));
    }

    void DxRenderer::Scene::dimHighlightedInstance() {
        highlightedModelID = MODELS_NR_MAX;
    }

    void DxRenderer::Scene::release() {           
        for (std::list<SceneModelData>::iterator it = sceneModelsData.begin(); it != sceneModelsData.end(); it++ ){
            (*it).sceneModelInstances.clear();
        }
        sceneModelsData.clear();
        renderer.scenes.remove(this);
        if (renderer.activeScene == this)
            renderer.activeScene = NULL;

        delete this;
    }

    DxRenderer::Scene::Scene(DxRenderer& _renderer) :
        renderer(_renderer), camera(renderer.fov, renderer.viewportWidth, renderer.viewportHeight, renderer.nearZ, renderer.farZ) {}           

    void DxRenderer::Scene::loadDataToBuffers() {
        loadViewMatToBuffer();
        loadProjMatToBuffer();        

        for (std::list<SceneModelData>::iterator it = sceneModelsData.begin(); it != sceneModelsData.end(); it++) {
            for (std::list<SceneModelInstance*>::iterator instanceIt = (*it).sceneModelInstances.begin(); instanceIt != (*it).sceneModelInstances.end(); instanceIt++)
                (*instanceIt)->loadDataToBuffers();
        }                
    }
    
    void DxRenderer::Scene::loadViewMatToBuffer() {
        renderer.devcon->UpdateSubresource(renderer.cbViewMat, 0, NULL, &XMMatrixTranspose(camera.getViewMat()), 0, 0);
    }

    void DxRenderer::Scene::loadProjMatToBuffer() {
        renderer.devcon->UpdateSubresource(renderer.cbProjMat, 0, NULL, &XMMatrixTranspose(camera.getProjMat()), 0, 0);
    }
        
    DxRenderer::DxRenderer(float _fov, float _nearZ, float _farZ) : fov(_fov), nearZ(_nearZ), farZ(_farZ) {}    

    DxRenderer::~DxRenderer() {
        for (std::vector<ModelRenderData>::iterator it = modelsRenderData.begin(); it != modelsRenderData.end(); it++) {
            (*it).vertexBuffer->Release();
            (*it).indexBuffer->Release();
            (*it).instancesTransformatsBuffer->Release();
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
        blendStateDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendStateDesc.RenderTarget[1].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendStateDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
        blendStateDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
        blendStateDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ONE;
        blendStateDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendStateDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
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

        if (FRAME_CAPTURES_NR_MAX) {
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
            {"INSTANCE_TRANSFORMAT", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1}
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

        dsStateDesc.DepthEnable = false;
        dsStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
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

    int delay = 50;
    void DxRenderer::startFrameCapture() {        
        if (framesCapturesNr < FRAME_CAPTURES_NR_MAX && delay-- > 0)
            pGraphicsAnalysis->BeginCapture();
    }

    void DxRenderer::endFrameCapture() {
        if (framesCapturesNr < FRAME_CAPTURES_NR_MAX && delay-- > 0) {
            pGraphicsAnalysis->EndCapture();
            framesCapturesNr++;
        }
    }

    HRESULT DxRenderer::addModel(std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, D3D_PRIMITIVE_TOPOLOGY primitiveTopology, UINT* modelIDOut) {
        HRESULT hr = S_OK;
        D3D11_BUFFER_DESC bd;
        D3D11_SUBRESOURCE_DATA initData;
        ModelRenderData modelBuffers;

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = modelVertices.size() * sizeof(VertexData);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        ZeroMemory(&initData, sizeof(initData));
        initData.pSysMem = &modelVertices[0];
        hr = dev->CreateBuffer(&bd, &initData, &modelBuffers.vertexBuffer);
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
        hr = dev->CreateBuffer(&bd, &initData, &modelBuffers.indexBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an index buffer.");
            modelBuffers.vertexBuffer->Release();

            return hr;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = INSTANCES_NR_MAX * sizeof(XMMATRIX);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &modelBuffers.instancesTransformatsBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an instances buffer..");
            modelBuffers.vertexBuffer->Release();
            modelBuffers.indexBuffer->Release();

            return hr;
        }

        modelBuffers.primitiveTopology = primitiveTopology;

        unsigned int modelID = modelsIDsPool.acquireIdx();
        *modelIDOut = modelID;
        modelsRenderData[modelID] = modelBuffers;

        return hr;
    }

    HRESULT DxRenderer::updateModelData(UINT modelID, std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, D3D_PRIMITIVE_TOPOLOGY primitiveTopology) {
        // TODO: Implement this
        return S_OK;
    }

    HRESULT DxRenderer::removeModel(UINT modelID) {
        for (std::list<Scene*>::iterator scenesIt = scenes.begin(); scenesIt != scenes.end(); scenesIt++) {
            std::list<Scene::SceneModelData>& sceneModelsData = (*scenesIt)->sceneModelsData;
            for (std::list<Scene::SceneModelData>::iterator sceneModelsDataIt = sceneModelsData.begin(); sceneModelsDataIt != sceneModelsData.end(); sceneModelsDataIt++) {
                if ((*sceneModelsDataIt).modelID == modelID) {
                    (*sceneModelsDataIt).sceneModelInstances.clear();
                    sceneModelsData.erase(sceneModelsDataIt);
                    break;
                }
            }
        }

        modelsRenderData[modelID].vertexBuffer->Release();
        modelsRenderData[modelID].indexBuffer->Release();
        modelsRenderData[modelID].instancesTransformatsBuffer->Release();
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
            for (std::list<Scene::SceneModelData>::iterator it = activeScene->sceneModelsData.begin(); it != activeScene->sceneModelsData.end(); it++) {                                
                devcon->PSSetShader(psScene, NULL, 0);
                devcon->VSSetShader(vsScene, NULL, 0);
                devcon->IASetInputLayout(vlScene);                
                unsigned int modelID = (*it).modelID;
                if (modelID == 1)
                    startFrameCapture();
                devcon->UpdateSubresource(cbModelID, 0, NULL, &modelID, 0, 0);

                ID3D11Buffer* constBuffers[3] = { cbViewMat, cbProjMat, cbModelID };
                devcon->VSSetConstantBuffers(0, 3, constBuffers); 
                
                ModelRenderData modelRenderData = modelsRenderData[modelID];
                ID3D11Buffer* vertexInstanceBuffers[2] = { modelRenderData.vertexBuffer, modelRenderData.instancesTransformatsBuffer };
                UINT strides[2] = { sizeof(VertexData), sizeof(XMMATRIX) };
                UINT offsets[2] = { 0, 0 };
                devcon->IASetVertexBuffers(0, 2, vertexInstanceBuffers, strides, offsets);

                ID3D11Buffer* indexBuffer = modelRenderData.indexBuffer;
                devcon->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);

                devcon->IASetPrimitiveTopology(modelRenderData.primitiveTopology);
                
                devcon->OMSetDepthStencilState(dsStateJustDepth, 0);
                
                ID3D11RenderTargetView* sceneRenderTargets[2] = { rtView , selectionRTViews[selectionTexIdxToUpdate] };
                devcon->OMSetRenderTargets(2, sceneRenderTargets, dsView);
                D3D11_BUFFER_DESC bd;
                indexBuffer->GetDesc(&bd);
                UINT instancesNr = (*it).sceneModelInstances.size();                
                devcon->DrawIndexedInstanced(bd.ByteWidth / sizeof(WORD), instancesNr, 0, 0, 0);   
                updatedSelectionTexIdx = selectionTexIdxToUpdate;
                if (modelID == 1)
                    endFrameCapture();

                if (activeScene->highlightedModelID == (*it).modelID) {                                                                                  
                    devcon->ClearRenderTargetView(blurRTView0, BLUR_TEXES_CLEAR_COLOR);
                    devcon->ClearRenderTargetView(blurRTView1, BLUR_TEXES_CLEAR_COLOR);

                    // render the highlighted object targeting blur texture0 with stencil write
                    devcon->OMSetDepthStencilState(dsStateWriteStencil, 0);
                    devcon->OMSetRenderTargets(1, &blurRTView0, dsView);
                    devcon->DrawIndexedInstanced(bd.ByteWidth / sizeof(WORD), 1, 0, 0, 0);
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
            }
            devcon->OMSetRenderTargets(1, &rtView, dsView);

            devcon->Flush();
        }        

        return hr;
    }    
}