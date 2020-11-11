#include "DxRenderer.h"
#include "DxUtils.h"
#include "KDTree.h"

#include <exception>
#include <string>
#include <limits>
#include <ostream>

#pragma warning( disable : 3146081)

using namespace DirectX;

namespace CoriumDirectX {    
    const XMVECTOR DxRenderer::X_AXIS_COLOR = DirectX::XMVectorSet(1.0f, 0.37f, 0.37f, 1.0f);
    const XMVECTOR DxRenderer::Y_AXIS_COLOR = DirectX::XMVectorSet(0.0f, 1.00f, 0.37f, 1.0f);
    const XMVECTOR DxRenderer::Z_AXIS_COLOR = DirectX::XMVectorSet(0.0f, 0.82f, 1.00f, 1.0f);
    const float DxRenderer::BLUR_STD = 5.0f;
    const float DxRenderer::BLUR_FACTOR = 1.75f;
    const FLOAT DxRenderer::CLEAR_COLOR[4] = { 224.0f / 255, 224.0f / 255, 224.0f / 255, 1.0f };
    const FLOAT DxRenderer::SELECTION_TEXES_CLEAR_COLOR[4] = { MODELS_NR_MAX, 0, 0, 0}; //TODO: update the selection texes to hold INT instead of UINT
    const FLOAT DxRenderer::BLUR_TEXES_CLEAR_COLOR[4] = { CLEAR_COLOR[0], CLEAR_COLOR[1], CLEAR_COLOR[2], 0.0f };

    inline float gaussDistrib(float x, float y, float rho) {
        return 1.0f / sqrtf(XM_2PI * rho * rho) * expf(-(x * x + y * y) / (2 * rho * rho));
    }

    inline std::array<float, 3> float3ToArr(XMFLOAT3 const& float3) {
        return std::array<float, 3>{float3.x, float3.y, float3.z};
    }

    inline std::ostream& operator<<(std::ostream& out, DirectX::CXMVECTOR vec) {
        return out << "(" << XMVectorGetX(vec) << ", " << XMVectorGetY(vec) << ", " << XMVectorGetZ(vec) << ", " << XMVectorGetW(vec) << ")";
    }

    DxRenderer::DxRenderer(float _fov, float _nearZ, float _farZ) :
            fov(_fov), nearZ(_nearZ), farZ(_farZ), manipulatorHandlesModelRenderData(Scene::SceneModelInstanceManipulator::handlesModelData.size()) {   
        Scene::SceneModelInstanceManipulator::initModels3D();
    }

    DxRenderer::~DxRenderer() {
        for (std::vector<ModelRenderData>::iterator it = modelsRenderData.begin(); it != modelsRenderData.end(); it++) {
            (*it).modelGeoData.vertexBuffer->Release();
            (*it).modelGeoData.indexBuffer->Release();
            (*it).visibleInstancesDataBuffer->Release();
            (*it).visibleHighlightedInstancesDataBuffer->Release();
        }

        modelsRenderData.clear();

        SAFE_RELEASE(dev);
        SAFE_RELEASE(devcon);
        SAFE_RELEASE(cbViewMat);
        SAFE_RELEASE(cbProjMat);
        SAFE_RELEASE(cbModelID);
        SAFE_RELEASE(cbGlobalTransformat);
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
        SAFE_RELEASE(dsStateDisable);
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
        bd.ByteWidth = 4 * sizeof(UINT);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &cbModelID);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not create the model ID constant buffer.");
            goto CbModelIDFailed;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(XMMATRIX);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &cbGlobalTransformat);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Could not create the global transformat constant buffer.");
            goto CbGlobalTransformatFailed;
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

        for (unsigned int manipulatorHandleIdx = 0; manipulatorHandleIdx < manipulatorHandlesModelRenderData.size(); ++manipulatorHandleIdx) {
            Scene::SceneModelInstanceManipulator::HandleModelData const& handleModelData = Scene::SceneModelInstanceManipulator::handlesModelData[manipulatorHandleIdx];
            hr = loadGeoData(handleModelData.vertexData, handleModelData.vertexIndices, handleModelData.primitiveTopology, manipulatorHandlesModelRenderData[manipulatorHandleIdx].modelGeoData);
            if (FAILED(hr)) {
                OutputDebugStringW(L"Failed to create a manipulator handle model.");
                goto ManipulatorHandleBuffersCreationFailed;
            }

            int handleInstancesNr = Scene::SceneModelInstanceManipulator::handlesInstancesData[manipulatorHandleIdx].size();
            ZeroMemory(&bd, sizeof(bd));
            bd.Usage = D3D11_USAGE_DEFAULT;
            bd.ByteWidth = handleInstancesNr * sizeof(VisibleInstanceData);
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = 0;         
            D3D11_SUBRESOURCE_DATA handlesInstancesData;
            ZeroMemory(&handlesInstancesData, sizeof(handlesInstancesData));            
            std::vector<VisibleInstanceData> handlesInstancesVisibleInstanceData(handleInstancesNr);            
            for (unsigned int handleInstanceIdx = 0; handleInstanceIdx < handleInstancesNr; ++handleInstanceIdx) {
                Scene::SceneModelInstanceManipulator::HandleInstanceData handleInstanceData = 
                    Scene::SceneModelInstanceManipulator::handlesInstancesData[manipulatorHandleIdx][handleInstanceIdx];
                handlesInstancesVisibleInstanceData[handleInstanceIdx].transformat = handleInstanceData.transformat;
                handlesInstancesVisibleInstanceData[handleInstanceIdx].instanceIdx = handleInstanceData.instanceIdx;
                handlesInstancesVisibleInstanceData[handleInstanceIdx].colorMask = handleInstanceData.color;
            }
            handlesInstancesData.pSysMem = &handlesInstancesVisibleInstanceData[0];
            hr = dev->CreateBuffer(&bd, &handlesInstancesData, &manipulatorHandlesModelRenderData[manipulatorHandleIdx].instancesDataBuffer);
            if (FAILED(hr)) {
                OutputDebugStringW(L"Failed to create a manipulator handle instances buffer.");
                manipulatorHandlesModelRenderData[manipulatorHandleIdx].modelGeoData.vertexBuffer->Release();
                manipulatorHandlesModelRenderData[manipulatorHandleIdx].modelGeoData.indexBuffer->Release();
                goto ManipulatorHandleBuffersCreationFailed;
            }
        }        

        if (GRAPHICS_DEBUG_MODEL_ID >= 0) {
            hr = DXGIGetDebugInterface1(0, __uuidof(pGraphicsAnalysis), reinterpret_cast<void**>(&pGraphicsAnalysis));
            if (FAILED(hr))
                goto PGraphicsAnalysisFailed;
        }

        return hr;

    PGraphicsAnalysisFailed:
    ManipulatorHandleBuffersCreationFailed:
        //TODO: release all successfuly created manipulator handle models
    InitShaderResourcesFailed:
    LoadShadersFailed:
        blendStateTransparency->Release();

    BlendStateTransparencyFailed:
        vbScreenQuad->Release();

    VbScreenQuadFailed:
        cbBlur->Release();    

    CbBlurFailed:
        cbGlobalTransformat->Release();

    CbGlobalTransformatFailed:
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
            //{"PARENT_IDX", 0, DXGI_FORMAT_R32_SINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"INSTANCE_IDX", 0, DXGI_FORMAT_R32_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1}            
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

        hr = dev->CreateDepthStencilState(&dsStateDesc, &dsStateDisable);
        if (FAILED(hr))
            goto DsStateDisableFail;

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

        return hr;

    DsStateMaskStencilFail:
        dsStateWriteStencil->Release();

    DsStateWriteStencilFail:
        dsStateDisable->Release();

    DsStateDisableFail:
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

        hr = loadGeoData(modelVertices, modelVertexIndices, primitiveTopology, modelRenderData.modelGeoData);
        if (FAILED(hr))
            return hr;

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = INSTANCES_BUFFERS_CAPACITY_INIT * sizeof(VisibleInstanceData);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &modelRenderData.visibleInstancesDataBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an instances buffer.");
            modelRenderData.modelGeoData.vertexBuffer->Release();
            modelRenderData.modelGeoData.indexBuffer->Release();

            return hr;
        }

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = INSTANCES_BUFFERS_CAPACITY_INIT * sizeof(VisibleInstanceData);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer(&bd, NULL, &modelRenderData.visibleHighlightedInstancesDataBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an instances buffer.");
            modelRenderData.modelGeoData.vertexBuffer->Release();
            modelRenderData.modelGeoData.indexBuffer->Release();
            modelRenderData.visibleInstancesDataBuffer->Release();

            return hr;
        }

        modelRenderData.boundingSphere = BoundingSphere(XMLoadFloat3(&boundingSphereCenter), boundingSphereRadius);        
        modelRenderData.modelGeoData.primitiveTopology = primitiveTopology;

        unsigned int modelID = modelsIDsPool.acquireIdx();
        *modelIDOut = modelID;
        modelsRenderData[modelID] = modelRenderData;

        return hr;
    }

    HRESULT DxRenderer::loadGeoData(std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, D3D_PRIMITIVE_TOPOLOGY primitiveTopology, ModelGeoData& modelGeoData)
    {
        HRESULT hr = S_OK;
        D3D11_BUFFER_DESC bd;
        D3D11_SUBRESOURCE_DATA initData;

        ZeroMemory(&bd, sizeof(bd));
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = modelVertices.size() * sizeof(VertexData);
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;

        ZeroMemory(&initData, sizeof(initData));
        initData.pSysMem = &modelVertices[0];
        hr = dev->CreateBuffer(&bd, &initData, &modelGeoData.vertexBuffer);
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
        hr = dev->CreateBuffer(&bd, &initData, &modelGeoData.indexBuffer);
        if (FAILED(hr)) {
            OutputDebugStringW(L"Failed to create an index buffer.");
            modelGeoData.vertexBuffer->Release();

            return hr;
        }

        modelGeoData.primitiveTopology = primitiveTopology;

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

        modelsRenderData[modelID].modelGeoData.vertexBuffer->Release();
        modelsRenderData[modelID].modelGeoData.indexBuffer->Release();
        modelsRenderData[modelID].visibleInstancesDataBuffer->Release();
        modelsRenderData[modelID].visibleHighlightedInstancesDataBuffer->Release();
        modelsIDsPool.releaseIdx(modelID);

        return S_OK;
    }

    DxRenderer::Scene* DxRenderer::createScene(Scene::TransformCallbackHandlers const& transformCallbackHandlers, MouseCallbacks& outMouseCallbacks) {
        Scene* scene = new Scene(*this, transformCallbackHandlers);        
        scenes.push_back(scene);
        outMouseCallbacks.onMouseMoveCallback= std::bind(&Scene::SceneModelInstanceManipulator::onMouseMove, &scene->instancesManipulator, std::placeholders::_1, std::placeholders::_2);
        outMouseCallbacks.onMouseUpCallback = std::bind(&Scene::SceneModelInstanceManipulator::onMouseUp, &scene->instancesManipulator);

        return scene;
    }

    HRESULT DxRenderer::render() {
        HRESULT hr = S_OK;

        devcon->ClearRenderTargetView(rtView, CLEAR_COLOR);
        UINT selectionTexIdxToUpdate = (updatedSelectionTexIdx + 1) % 3;        
        devcon->ClearRenderTargetView(selectionRTViews[selectionTexIdxToUpdate], SELECTION_TEXES_CLEAR_COLOR);
        devcon->ClearDepthStencilView(dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        XMMATRIX identityMat = XMMatrixIdentity();
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
                    devcon->UpdateSubresource(cbGlobalTransformat, 0, NULL, &identityMat, 0, 0);

                    ID3D11Buffer* constBuffers[4] = { cbViewMat, cbProjMat, cbModelID, cbGlobalTransformat };
                    devcon->VSSetConstantBuffers(0, 4, constBuffers);

                    ID3D11Buffer* vertexBuffers[2] = { modelRenderData.modelGeoData.vertexBuffer, modelRenderData.visibleInstancesDataBuffer };
                    UINT strides[2] = { sizeof(VertexData), sizeof(VisibleInstanceData) };
                    UINT offsets[2] = { 0, 0 };
                    devcon->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);

                    ID3D11Buffer* indexBuffer = modelRenderData.modelGeoData.indexBuffer;
                    devcon->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
                    D3D11_BUFFER_DESC bd;
                    indexBuffer->GetDesc(&bd);

                    devcon->IASetPrimitiveTopology(modelRenderData.modelGeoData.primitiveTopology);
                    
                    devcon->OMSetDepthStencilState(dsStateJustDepth, 0);                    

                    if (modelID == GRAPHICS_DEBUG_MODEL_ID)
                        startFrameCapture();

                    if (modelRenderData.visibleInstancesNr) {
                        ID3D11RenderTargetView* sceneRenderTargets[2] = { rtView , selectionRTViews[selectionTexIdxToUpdate] };
                        devcon->OMSetRenderTargets(2, sceneRenderTargets, dsView);
                        devcon->DrawIndexedInstanced(bd.ByteWidth / sizeof(WORD), modelRenderData.visibleInstancesNr, 0, 0, 0);                        
                    }

                    if (modelRenderData.visibleHighlightedInstancesNr) {
                        devcon->ClearRenderTargetView(blurRTView0, BLUR_TEXES_CLEAR_COLOR);
                        devcon->ClearRenderTargetView(blurRTView1, BLUR_TEXES_CLEAR_COLOR);

                        // render the highlighted object targeting blur texture0 with stencil write
                        vertexBuffers[1] = modelRenderData.visibleHighlightedInstancesDataBuffer;
                        devcon->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);

                        devcon->OMSetDepthStencilState(dsStateWriteStencil, 0);
                        ID3D11RenderTargetView* sceneRenderTargets[3] = { rtView, selectionRTViews[selectionTexIdxToUpdate], blurRTView0 };
                        devcon->OMSetRenderTargets(3, sceneRenderTargets, dsView);
                        devcon->DrawIndexedInstanced(bd.ByteWidth / sizeof(WORD), modelRenderData.visibleHighlightedInstancesNr, 0, 0, 0);                        
                        devcon->OMSetRenderTargets(1, &rtView, NULL); // temporarily rebind the back buffer

                        // render the texture with the blur algo twice, and the result to the back buffer: blurRTView0 -> blurRTView1 -> blurRTView0 -> back buffer
                        devcon->PSSetShader(psBlur, NULL, 0);
                        devcon->VSSetShader(vsBlur, NULL, 0);
                        devcon->IASetInputLayout(vlBlur);

                        UINT stride = sizeof(ScreenQuadVertexData);
                        UINT offset = 0;
                        devcon->IASetVertexBuffers(0, 1, &vbScreenQuad, &stride, &offset);

                        devcon->PSSetConstantBuffers(4, 1, &cbBlur);

                        devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                        devcon->PSSetSamplers(0, 1, &blurTexSampState);

                        // blur pass 1                                        
                        devcon->UpdateSubresource(cbBlur, 0, NULL, &blurVarsHorizon, 0, 0); // TODO: create a cbBlurHorizon and cbBlurVert and change the constant buffer set
                        devcon->PSSetShaderResources(0, 1, &blurSRView0);
                        devcon->OMSetRenderTargets(1, &blurRTView1, NULL);
                        devcon->Draw(4, 0);
                        devcon->OMSetRenderTargets(1, &rtView, NULL); // temporarily rebind the back buffer

                        // blur pass 2                    
                        devcon->UpdateSubresource(cbBlur, 0, NULL, &blurVarsVert, 0, 0); // TODO: create a cbBlurHorizon and cbBlurVert and change the constant buffer set
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

                    if (modelID == GRAPHICS_DEBUG_MODEL_ID)
                        endFrameCapture();
                }
            }

            devcon->ClearDepthStencilView(dsView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
            devcon->OMSetDepthStencilState(dsStateJustDepth, 0);
            if (!activeScene->transformGrp.empty()) { 
                if (GRAPHICS_DEBUG_MODEL_ID > 100)
                    startFrameCapture();
                for (unsigned int manipulatorHandleModelIdx = 0; manipulatorHandleModelIdx < manipulatorHandlesModelRenderData.size(); ++manipulatorHandleModelIdx) {
                    ManipulatorHandleModelRenderData const& handleModelRenderData = manipulatorHandlesModelRenderData[manipulatorHandleModelIdx];
                    devcon->PSSetShader(psScene, NULL, 0);
                    devcon->VSSetShader(vsScene, NULL, 0);
                    devcon->IASetInputLayout(vlScene);
                    
                    UINT handleModelID = MODELS_NR_MAX + manipulatorHandleModelIdx + 1;
                    devcon->UpdateSubresource(cbModelID, 0, NULL, &handleModelID, 0, 0);
                    devcon->UpdateSubresource(cbGlobalTransformat, 0, NULL, &activeScene->instancesManipulator.getTransformat(), 0, 0);
                    ID3D11Buffer* vertexBuffers[2] = { handleModelRenderData.modelGeoData.vertexBuffer, handleModelRenderData.instancesDataBuffer };
                    UINT strides[2] = { sizeof(VertexData), sizeof(VisibleInstanceData) };
                    UINT offsets[2] = { 0, 0 };
                    devcon->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
                                        
                    devcon->IASetIndexBuffer(handleModelRenderData.modelGeoData.indexBuffer, DXGI_FORMAT_R16_UINT, 0);
                    D3D11_BUFFER_DESC bd;
                    handleModelRenderData.modelGeoData.indexBuffer->GetDesc(&bd);

                    devcon->IASetPrimitiveTopology(handleModelRenderData.modelGeoData.primitiveTopology);                    

                    ID3D11RenderTargetView* sceneRenderTargets[2] = { rtView , selectionRTViews[selectionTexIdxToUpdate] };
                    devcon->OMSetRenderTargets(2, sceneRenderTargets, dsView);
                    devcon->DrawIndexedInstanced(bd.ByteWidth / sizeof(WORD), 3, 0, 0, 0);                    
                }       
                if (GRAPHICS_DEBUG_MODEL_ID > 100)
                    endFrameCapture();
            }

            updatedSelectionTexIdx = selectionTexIdxToUpdate;
            devcon->OMSetRenderTargets(1, &rtView, dsView);

            devcon->Flush();
        }

        return hr;
    }

    void DxRenderer::captureFrame() {
        framesNrToCapture++;
    }
    
    class DxRenderer::Scene::KDTreeImpl : public KDTree<SceneModelInstance, 3> {
    public:
        KDTreeImpl(TestBoundingSphereVisibility testBoundingSphereVisibility, TestAabbVisibility testAabbVisibility) :
            KDTree<SceneModelInstance, 3>(testBoundingSphereVisibility, testAabbVisibility) {}        
    };    

    class DxRenderer::Scene::SceneModelInstance::KDTreeDataNodeHolder {
    public:
        KDTreeDataNodeHolder() {}
        KDTreeDataNodeHolder(KDTree<SceneModelInstance, 3>::DataNode* _dataNode) : dataNode(_dataNode) {}
        operator KDTree<SceneModelInstance, 3>::DataNode*() { return dataNode; }     
        operator bool() { return dataNode != NULL; }
        KDTree<SceneModelInstance, 3>::DataNode* operator->() { return dataNode; }
        KDTreeDataNodeHolder& operator=(KDTree<SceneModelInstance, 3>::DataNode* _dataNode) { dataNode = _dataNode; return *this; }

    private:
        KDTree<SceneModelInstance, 3>::DataNode* dataNode = NULL;
    };

    XMMATRIX DxRenderer::Transform::genTransformat() const {
        XMMATRIX transformat = XMMatrixScaling(scaleFactor.x, scaleFactor.y, scaleFactor.z);
        transformat = XMMatrixMultiply(XMMatrixRotationAxis(XMLoadFloat3(&rotAx), rotAng), transformat);
        transformat = XMMatrixMultiply(XMMatrixTranslationFromVector(XMLoadFloat3(&translation)), transformat);

        return transformat;
    }

    /*
    void DxRenderer::Scene::SceneModelInstance::translate(XMFLOAT3 const& _translation) {        
        setTranslation(pos + XMLoadFloat3(&_translation));
    }

    void DxRenderer::Scene::SceneModelInstance::setTranslation(XMFLOAT3 const& _translation) {
        setTranslation(XMLoadFloat3(&_translation));
    }
    */

    void DxRenderer::Scene::SceneModelInstance::translate(DirectX::CXMVECTOR translation) {
        setTranslation(pos + translation);
    }
       
    void DxRenderer::Scene::SceneModelInstance::setTranslation(DirectX::CXMVECTOR translation) {
        pos = translation;        
        if (kdtreeDataNodeHolder) {
            XMFLOAT3 _pos;
            XMStoreFloat3(&_pos, pos);
            scene.kdtree->setTranslationForNodeBoundingSphere(kdtreeDataNodeHolder, float3ToArr(_pos));
        }

        recompTransformat();
        updateBuffers();
    }

    DirectX::XMFLOAT3 DxRenderer::Scene::SceneModelInstance::getTranslation() {
        XMFLOAT3 ret;
        XMStoreFloat3(&ret, pos);
        
        return ret;
    }   

    // TODO: Verify that instance is not in the transform group
    void DxRenderer::Scene::SceneModelInstance::addToTransformGrp() {
        //kdtree->destroy(instance->kdtreeDataNodeHolder);
        scene.transformGrp.push_back(this);
        transformGrpIt = scene.transformGrp.end();
        --transformGrpIt;
        scene.instancesManipulator.onTransformGrpTranslationSet(pos);
    }

    // TODO: Verify that instance is in the transform group
    void DxRenderer::Scene::SceneModelInstance::removeFromTransformGrp() {
        scene.transformGrp.erase(transformGrpIt);
        //instance->addInstanceToKdtree();  
    }

    /*
    void DxRenderer::Scene::SceneModelInstance::scale(XMFLOAT3 const& _scaleFactorQ) {        
        setScale(scaleFactor * XMLoadFloat3(&_scaleFactorQ));        
    }
    
    void DxRenderer::Scene::SceneModelInstance::setScale(XMFLOAT3 const& _scaleFactor) {
        setScale(XMLoadFloat3(&_scaleFactor));         
    }
    */
    void DxRenderer::Scene::SceneModelInstance::scale(DirectX::CXMVECTOR scaleFactorQ) {
        setScale(scaleFactor + scaleFactorQ);
    }
    
    void DxRenderer::Scene::SceneModelInstance::setScale(DirectX::CXMVECTOR scaleFactor) {
        this->scaleFactor = scaleFactor;        
        if (kdtreeDataNodeHolder) {
            XMFLOAT3 _scaleFactor;
            XMStoreFloat3(&_scaleFactor, scaleFactor);
            scene.kdtree->setRadiusForNodeBoundingSphere(kdtreeDataNodeHolder, (std::max)((std::max)(_scaleFactor.x, _scaleFactor.y), _scaleFactor.z));
        }
        recompTransformat();
        updateBuffers();
    }

    /*
    void DxRenderer::Scene::SceneModelInstance::rotate(XMFLOAT3 const& ax, float ang) {         
		setRotation(XMQuaternionMultiply(rot, XMQuaternionRotationAxis(XMVectorSet(ax.x, ax.y, ax.z, 0.0f), ang / 180.0f * XM_PI)));
    }

    void DxRenderer::Scene::SceneModelInstance::setRotation(XMFLOAT3 const& ax, float ang) {
        setRotation(XMQuaternionRotationAxis(XMVectorSet(ax.x, ax.y, ax.z, 0.0f), ang / 180.0f * XM_PI));        
    }
    */

    void DxRenderer::Scene::SceneModelInstance::rotate(DirectX::CXMVECTOR _rot) {
        setRotation(XMQuaternionMultiply(rot, _rot));        
    }

    void DxRenderer::Scene::SceneModelInstance::setRotation(DirectX::CXMVECTOR _rot) {
        rot = _rot;
        if (kdtreeDataNodeHolder) {
            XMFLOAT3 rotatedBoundSphereC;
            XMStoreFloat3(&rotatedBoundSphereC, pos + XMVector3Rotate(scene.renderer.modelsRenderData[modelID].boundingSphere.getCenter(), rot));
            scene.kdtree->setTranslationForNodeBoundingSphere(kdtreeDataNodeHolder, float3ToArr(rotatedBoundSphereC));
        }
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

    bool DxRenderer::Scene::SceneModelInstance::assignParent(SceneModelInstance* parent)
    {
        if (isInstanceDescendant(parent))
            return false;
        
        this->parent = parent;
        parent->children.push_back(this);
        //scene.renderer.devcon->UpdateSubresource()
        return true;
    }

    void DxRenderer::Scene::SceneModelInstance::unparent() {

    }

    void DxRenderer::Scene::SceneModelInstance::release() {
        // TODO: if (kdtreeDataNodeHolder)
        scene.kdtree->destroy(kdtreeDataNodeHolder);

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

    inline BoundingSphere calcTransformedBoundingSphere(BoundingSphere const& boundingSphere, XMVECTOR const& translation, XMVECTOR const& scaleFactor, XMVECTOR rot) {
        return BoundingSphere::calcTransformedBoundingSphere(boundingSphere, translation + XMVector3Rotate(boundingSphere.getCenter(), rot), max(max(XMVectorGetX(scaleFactor), XMVectorGetY(scaleFactor)), XMVectorGetZ(scaleFactor)));
    }

    DxRenderer::Scene::SceneModelInstance::SceneModelInstance(Scene& _scene, UINT _modelID, CXMVECTOR _instanceColorMask, Transform const& transform, SelectionHandler _selectionHandler) :
            scene(_scene), modelID(_modelID), instanceColorMask(_instanceColorMask),
            selectionHandler(_selectionHandler),
            pos(XMLoadFloat3(&transform.translation)), 
            scaleFactor(XMLoadFloat3(&transform.scaleFactor)), 
            rot(XMQuaternionRotationAxis(XMLoadFloat3(&transform.rotAx), transform.rotAng / 180.0f * XM_PI)),
            modelTransformat(transform.genTransformat()),
            kdtreeDataNodeHolder(*(new KDTreeDataNodeHolder())) {

        addInstanceToKdtree();

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
            scene.sceneModelsData.push_front(sceneModelData);            
        }

        instanceIdx = sceneModelData->instanceIdxsPool.acquireIdx();
#if _DEBUG
        kdtreeDataNodeHolder->setName(std::string("(") + std::to_string(modelID) + std::string(",") + std::to_string(instanceIdx) + std::string(")"));
#endif
        // TODO: if acquired indices number == visibleInstancesBuffersCapacity, reallocate:
        // visibleInstancesIdxsBuffer
        // visibleHighlightedInstancesIdxsBuffer        
        // visibleInstancesIdxsStaging
        // visibleHighlightedInstancesIdxsStaging

        sceneModelData->sceneModelInstances[instanceIdx] = this;                           
        updateBuffers();
    }

    DxRenderer::Scene::SceneModelInstance::~SceneModelInstance() { delete &kdtreeDataNodeHolder; }

    void DxRenderer::Scene::SceneModelInstance::addInstanceToKdtree() {
        BoundingSphere boundingSphere = calcTransformedBoundingSphere(scene.renderer.modelsRenderData[modelID].boundingSphere, pos, scaleFactor, rot);
        XMFLOAT3 boundingSphereCenter;
        XMStoreFloat3(&boundingSphereCenter, boundingSphere.getCenter());
        kdtreeDataNodeHolder = scene.kdtree->insert(float3ToArr(boundingSphereCenter), boundingSphere.getRadius(), *this);        
    }

    void DxRenderer::Scene::SceneModelInstance::loadInstanceTransformatToBuffer() {             
        if (isHighlighted) 
            transformatsBufferOffset = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesNr++;        
        else 
            transformatsBufferOffset = scene.renderer.modelsRenderData[modelID].visibleInstancesNr++;
                
        updateInstanceTransformatInBuffer();
    }

    void DxRenderer::Scene::SceneModelInstance::unloadInstanceTransformatFromBuffer() {
        ID3D11Buffer* instancesDataBuffer;        
        std::vector<UINT>* visibleInstancesIdxs;        
        UINT visibleInstancesNr;        
        if (isHighlighted) {
            instancesDataBuffer = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesDataBuffer;
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesIdxs;
            visibleInstancesNr = --scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesNr;
        }
        else {
            instancesDataBuffer = scene.renderer.modelsRenderData[modelID].visibleInstancesDataBuffer;
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleInstancesIdxs;
            visibleInstancesNr = --scene.renderer.modelsRenderData[modelID].visibleInstancesNr;
        }
                
        if (transformatsBufferOffset < visibleInstancesNr) {
            for (std::list<SceneModelData*>::iterator it = scene.sceneModelsData.begin(); it != scene.sceneModelsData.end(); it++) {
                SceneModelData* sceneModelData = *it;
                if (sceneModelData->modelID == modelID) {
                    D3D11_BOX rangeBox = { visibleInstancesNr * sizeof(VisibleInstanceData), 0U, 0U, (visibleInstancesNr + 1) * sizeof(VisibleInstanceData), 1U, 1U };
                    scene.renderer.devcon->CopySubresourceRegion(instancesDataBuffer, 0, transformatsBufferOffset * sizeof(VisibleInstanceData), 0, 0, instancesDataBuffer, 0, &rangeBox);                    

                    SceneModelInstance* instanceToSwapWith = sceneModelData->sceneModelInstances[(*visibleInstancesIdxs)[visibleInstancesNr]];
                    (*visibleInstancesIdxs)[transformatsBufferOffset] = instanceToSwapWith->instanceIdx;
                    instanceToSwapWith->transformatsBufferOffset = transformatsBufferOffset;
                }
            }
        }

        transformatsBufferOffset = (std::numeric_limits<UINT>::max)();
    }

    void DxRenderer::Scene::SceneModelInstance::updateInstanceTransformatInBuffer() {
        ID3D11Buffer* instancesDataBuffer;
        std::vector<UINT>* visibleInstancesIdxs;        
        if (isHighlighted) {
            instancesDataBuffer = scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesDataBuffer;
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleHighlightedInstancesIdxs;            
        }
        else {
            instancesDataBuffer = scene.renderer.modelsRenderData[modelID].visibleInstancesDataBuffer;
            visibleInstancesIdxs = &scene.renderer.modelsRenderData[modelID].visibleInstancesIdxs;            
        }

        VisibleInstanceData instanceData = { modelTransformat, instanceColorMask, instanceIdx };
        D3D11_BOX rangeBox = { transformatsBufferOffset * sizeof(VisibleInstanceData), 0U, 0U, (transformatsBufferOffset + 1) * sizeof(VisibleInstanceData), 1U, 1U };
        scene.renderer.devcon->UpdateSubresource(instancesDataBuffer, 0, &rangeBox, &instanceData, 0, 0);

        (*visibleInstancesIdxs)[transformatsBufferOffset] = instanceIdx;
    }

    void DxRenderer::Scene::SceneModelInstance::updateBuffers() {        
        if (isShown && scene.kdtree->isNodeVisible(kdtreeDataNodeHolder)) {
            if (transformatsBufferOffset == (std::numeric_limits<UINT>::max)())
                loadInstanceTransformatToBuffer();
            else
                updateInstanceTransformatInBuffer();
        }
        else if (transformatsBufferOffset < (std::numeric_limits<UINT>::max)())
            unloadInstanceTransformatFromBuffer();
    }       

    // modelTransformat = XMMatrixAffineTransformation(scaleFactor, XMVectorZero(), rot, pos);
    void DxRenderer::Scene::SceneModelInstance::recompTransformat() {
        modelTransformat = XMMatrixTranslationFromVector(pos);        
        modelTransformat = XMMatrixMultiply(XMMatrixRotationQuaternion(rot) , modelTransformat);
        modelTransformat = XMMatrixMultiply(XMMatrixScalingFromVector(scaleFactor), modelTransformat);
    }

    bool DxRenderer::Scene::SceneModelInstance::isInstanceDescendant(SceneModelInstance* instance)
    {
        for (std::list<SceneModelInstance*>::iterator childrenIt = children.begin(); childrenIt != children.end(); childrenIt++) {
            if (instance == *childrenIt || isInstanceDescendant(*childrenIt))
                return true;            
        }

        return false;
    }

    void DxRenderer::Scene::activate() {        
        loadViewMatToBuffer();
        loadProjMatToBuffer();
        loadVisibleInstancesDataToBuffers();
        renderer.activeScene = this;
    }

    DxRenderer::Scene::SceneModelInstance* DxRenderer::Scene::createModelInstance(unsigned int modelID, DirectX::XMFLOAT4 const& instanceColorMask, Transform const& transformInit, SceneModelInstance::SelectionHandler selectionHandler) {
        return new SceneModelInstance(*this, modelID, XMLoadFloat4(&instanceColorMask), transformInit, selectionHandler);
    }   

    void DxRenderer::Scene::transformGrpTranslate(DirectX::XMFLOAT3 const& translation) {        
        transformGrpTranslateEXE(XMLoadFloat3(&translation), TransformSrc::Outside);        
    }

    void DxRenderer::Scene::transformGrpSetTranslation(DirectX::XMFLOAT3 const& translation) {
        transformGrpSetTranslationEXE(XMLoadFloat3(&translation));        
    }

    void DxRenderer::Scene::transformGrpScale(DirectX::XMFLOAT3 const& scaleFactorQ) {
        transformGrpScaleEXE(XMLoadFloat3(&scaleFactorQ), TransformSrc::Outside);
    }

    void DxRenderer::Scene::transformGrpSetScale(DirectX::XMFLOAT3 const& scaleFactor) {
        transformGrpSetScaleEXE(XMLoadFloat3(&scaleFactor));
    }

    void DxRenderer::Scene::transformGrpRotate(DirectX::XMFLOAT3 const& ax, float ang) {
        transformGrpRotateEXE(XMLoadFloat3(&ax), ang, TransformSrc::Outside);
    }

    void DxRenderer::Scene::transformGrpSetRotation(DirectX::XMFLOAT3 const& ax, float ang) {
        transformGrpSetRotationEXE(XMQuaternionRotationAxis(XMLoadFloat3(&ax), ang / 180 * XM_PI));
    }

    void DxRenderer::Scene::panCamera(float x, float y) {
        camera.panViaViewportDrag(x, y);
        loadViewMatToBuffer();
        loadVisibleInstancesDataToBuffers();
        if (!transformGrp.empty())
            instancesManipulator.onCameraTranslation();        
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
        if (!transformGrp.empty())
            instancesManipulator.onCameraTranslation();
    }

    XMFLOAT3 DxRenderer::Scene::getCameraPos() {
        XMFLOAT3 ret;
        XMStoreFloat3(&ret, camera.getPos());
        return ret;
    }
    
    float DxRenderer::Scene::getCameraFOV() {
        return camera.getFOV();
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
                        instance->selectionHandler(x, y);
                    
                    return true;
                }
            }

            assert(false);            
        }
        else if (MODELS_NR_MAX < sceneModelInstanceIdxs.modelID) {
            instancesManipulator.selectionHandlers[sceneModelInstanceIdxs.modelID - MODELS_NR_MAX - 1][sceneModelInstanceIdxs.instanceIdx](x, y);
            return true;
        }
        else
            return false;
    }

    DirectX::XMFLOAT3 DxRenderer::Scene::screenVecToWorldVec(float x, float y) {
        XMFLOAT3 ret;
        XMStoreFloat3(&ret, camera.screenVecToWorldVec(x, y));

        return ret;
    }

    XMFLOAT3 DxRenderer::Scene::cursorPosToRayDirection(float x, float y) {        
        XMFLOAT3 ret;
        XMStoreFloat3(&ret, camera.cursorPosToRayDirection(x, y));
        
        return ret;
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

    DxRenderer::Scene::Scene(DxRenderer& _renderer, Scene::TransformCallbackHandlers const& _transformCallbackHandlers) :
        renderer(_renderer), camera(renderer.fov, renderer.viewportWidth, renderer.viewportHeight, renderer.nearZ, renderer.farZ),
        kdtree(new DxRenderer::Scene::KDTreeImpl(std::bind(&DxRenderer::Scene::testBoundingSphereVisibility, this, std::placeholders::_1, std::placeholders::_2),
                                                 std::bind(&DxRenderer::Scene::testAabbVisibility, this, std::placeholders::_1, std::placeholders::_2))),
        instancesManipulator(*this),
        transformCallbackHandlers(_transformCallbackHandlers) {}

    DxRenderer::Scene::~Scene()
    {
        delete kdtree;
    }
    
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
            
        kdtree->initVisibleNodesIt();     
        while (SceneModelInstance* visibleInstance = kdtree->getNextVisibleNodeData()) {
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
                std::vector<VisibleInstanceData> visibleInstancesDataStaging(modelRenderData.visibleInstancesBuffersCapacity);
                for (unsigned int transformatsBufferOffset = 0; transformatsBufferOffset < modelRenderData.visibleInstancesNr; transformatsBufferOffset++) {
                    UINT instanceIdx = modelRenderData.visibleInstancesIdxs[transformatsBufferOffset];
                    visibleInstancesDataStaging[transformatsBufferOffset] = { sceneModelData->sceneModelInstances[instanceIdx]->getModelTransformat(), sceneModelData->sceneModelInstances[instanceIdx]->instanceColorMask, instanceIdx };
                }
                D3D11_BOX rangeBox = { 0U, 0U, 0U, modelRenderData.visibleInstancesNr * sizeof(VisibleInstanceData), 1U, 1U };
				renderer.devcon->UpdateSubresource(modelRenderData.visibleInstancesDataBuffer, 0, &rangeBox, &visibleInstancesDataStaging[0], 0, 0);                
            }
            if (modelRenderData.visibleHighlightedInstancesNr) {
                std::vector<VisibleInstanceData> visibleHighlightedInstancesDataStaging(modelRenderData.visibleInstancesBuffersCapacity);
                for (unsigned int transformatsBufferOffset = 0; transformatsBufferOffset < modelRenderData.visibleHighlightedInstancesNr; transformatsBufferOffset++) {
                    UINT instanceIdx = modelRenderData.visibleHighlightedInstancesIdxs[transformatsBufferOffset];
                    visibleHighlightedInstancesDataStaging[transformatsBufferOffset] = { sceneModelData->sceneModelInstances[instanceIdx]->getModelTransformat(), sceneModelData->sceneModelInstances[instanceIdx]->instanceColorMask, instanceIdx };
                }
                D3D11_BOX rangeBox = { 0U, 0U, 0U, modelRenderData.visibleHighlightedInstancesNr * sizeof(VisibleInstanceData), 1U, 1U };
                renderer.devcon->UpdateSubresource(modelRenderData.visibleHighlightedInstancesDataBuffer, 0, &rangeBox, &visibleHighlightedInstancesDataStaging[0], 0, 0);                
            }            
        }
    }    

    void DxRenderer::Scene::transformGrpTranslateEXE(DirectX::CXMVECTOR translation, TransformSrc src) {
        for (std::list<SceneModelInstance*>::iterator it = transformGrp.begin(); it != transformGrp.end(); ++it)
            (*it)->translate(translation);
        instancesManipulator.onTransformGrpTranslated(translation);

        if (src == TransformSrc::Manipulator) {
            if (transformCallbackHandlers.translationHandler) {
                XMFLOAT3 _translation;
                XMStoreFloat3(&_translation, translation);            
                transformCallbackHandlers.translationHandler(_translation.x, _translation.y, _translation.z);
            }
        }                             
    }

    void DxRenderer::Scene::transformGrpSetTranslationEXE(DirectX::CXMVECTOR translation) {
        for (std::list<SceneModelInstance*>::iterator it = transformGrp.begin(); it != transformGrp.end(); ++it)
            (*it)->setTranslation(translation);         
        instancesManipulator.onTransformGrpTranslationSet(translation);
    }

    void DxRenderer::Scene::transformGrpScaleEXE(DirectX::CXMVECTOR scaleFactorQ, TransformSrc src) {
        for (std::list<SceneModelInstance*>::iterator it = transformGrp.begin(); it != transformGrp.end(); ++it)
            (*it)->scale(scaleFactorQ);

        if (src == TransformSrc::Manipulator) {
            if (transformCallbackHandlers.scaleHandler) {
                XMFLOAT3 _scaleFactorQ;
                XMStoreFloat3(&_scaleFactorQ, scaleFactorQ);            
                transformCallbackHandlers.scaleHandler(_scaleFactorQ.x, _scaleFactorQ.y, _scaleFactorQ.z);
            }
        }
    }

    void DxRenderer::Scene::transformGrpSetScaleEXE(DirectX::CXMVECTOR scaleFactor) {
        for (std::list<SceneModelInstance*>::iterator it = transformGrp.begin(); it != transformGrp.end(); ++it)
            (*it)->setScale(scaleFactor);
    }

    void DxRenderer::Scene::transformGrpRotateEXE(DirectX::CXMVECTOR rotAx, float rotAng, TransformSrc src) {
        XMVECTOR quat = XMQuaternionRotationAxis(rotAx, rotAng / 180 * XM_PI);
        for (std::list<SceneModelInstance*>::iterator it = transformGrp.begin(); it != transformGrp.end(); ++it)
            (*it)->rotate(quat);
        instancesManipulator.onTransformGrpRotated(quat);

        if (src == TransformSrc::Manipulator) {
            if (transformCallbackHandlers.rotationHandler) {
                XMFLOAT3 _rotAx;
                XMStoreFloat3(&_rotAx, rotAx);
                transformCallbackHandlers.rotationHandler(_rotAx.x, _rotAx.y, _rotAx.z, rotAng);
            }
        }
    }

    void DxRenderer::Scene::transformGrpSetRotationEXE(DirectX::CXMVECTOR rot) {
        for (std::list<SceneModelInstance*>::iterator it = transformGrp.begin(); it != transformGrp.end(); ++it)
            (*it)->setRotation(rot);
        instancesManipulator.onTransformGrpRotationSet(rot);
    }

    bool DxRenderer::Scene::testBoundingSphereVisibility(std::array<float, 3> const& boundingSphereCenter, float boundingSphereRadius)
    {
        XMFLOAT3 _boundingSphereCenter(boundingSphereCenter[0], boundingSphereCenter[1], boundingSphereCenter[2]);
        return camera.isBoundingSphereVisible(BoundingSphere(XMLoadFloat3(&_boundingSphereCenter), boundingSphereRadius));
    }

    bool DxRenderer::Scene::testAabbVisibility(std::array<float, 3> const& aabbMin, std::array<float, 3> const& aabbMax)
    {
        XMFLOAT3 _aabbMin(aabbMin[0], aabbMin[1], aabbMin[2]);
        XMFLOAT3 _aabbMax(aabbMax[0], aabbMax[1], aabbMax[2]);
        return camera.isAabbVisible(XMLoadFloat3(&_aabbMin), XMLoadFloat3(&_aabbMax));
    }                        
    
    const std::vector<DxRenderer::Scene::SceneModelInstanceManipulator::HandleModelData> DxRenderer::Scene::SceneModelInstanceManipulator::handlesModelData = {
        {translateArrowShaftVertices, arrowsShaftVertexIndices, D3D_PRIMITIVE_TOPOLOGY_LINELIST},
        {translationArrowHeadVertices, translationArrowHeadVertexIndices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST},        
        {scaleArrowShaftVertices, arrowsShaftVertexIndices, D3D_PRIMITIVE_TOPOLOGY_LINELIST},
        {scaleArrowHeadVertices, scaleArrowHeadVertexIndices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST},        
        {translationRectVertices, translationRectVertexIndices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST},
        {rotationRingVertices, rotationRingVertexIndices, D3D_PRIMITIVE_TOPOLOGY_LINELIST},
        {translationArrowEnvelopeVertices, arrowEnvelopesVertexIndices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST},
        {scaleArrowEnvelopeVertices, arrowEnvelopesVertexIndices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST},
        {rotationRingEnvelopeVertices, rotationRingEnvelopeVertexIndices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST}
    };

    const std::array<DxRenderer::Scene::SceneModelInstanceManipulator::HandleInstanceData, 3> DxRenderer::Scene::SceneModelInstanceManipulator::translationHandlesTransformats = {
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.0f), 0, DxRenderer::X_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XM_PIDIV2), 1, DxRenderer::Y_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), -XM_PIDIV2), 2, DxRenderer::Z_AXIS_COLOR},
    };

    const std::array<DxRenderer::Scene::SceneModelInstanceManipulator::HandleInstanceData, 3> DxRenderer::Scene::SceneModelInstanceManipulator::scaleHandlesTransformats = {
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.0f), 0, DxRenderer::X_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), XM_PIDIV2), 1, DxRenderer::Y_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), -XM_PIDIV2), 2, DxRenderer::Z_AXIS_COLOR}
    };

    const std::array<DxRenderer::Scene::SceneModelInstanceManipulator::HandleInstanceData, 3> DxRenderer::Scene::SceneModelInstanceManipulator::translationRectHandlesTransformats = {
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.0f), 0, DxRenderer::Z_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XM_PIDIV2), 1, DxRenderer::Y_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), -XM_PIDIV2), 2, DxRenderer::X_AXIS_COLOR}
    };

    const std::array<DxRenderer::Scene::SceneModelInstanceManipulator::HandleInstanceData, 3> DxRenderer::Scene::SceneModelInstanceManipulator::rotationRingHandlesTransformats = {
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), XM_PIDIV2), 0, DxRenderer::X_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), XM_PIDIV2), 1, DxRenderer::Y_AXIS_COLOR},
        HandleInstanceData{XMMatrixRotationAxis(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), 0.0f), 2, DxRenderer::Z_AXIS_COLOR}
    };

    const std::vector<std::array<DxRenderer::Scene::SceneModelInstanceManipulator::HandleInstanceData, 3>> DxRenderer::Scene::SceneModelInstanceManipulator::handlesInstancesData = {
        translationHandlesTransformats,
        translationHandlesTransformats,         
        scaleHandlesTransformats,
        scaleHandlesTransformats,        
        translationRectHandlesTransformats,
        rotationRingHandlesTransformats,
        translationHandlesTransformats,
        scaleHandlesTransformats,
        rotationRingHandlesTransformats
    };
   
    /*    
    private void onBoundInstanceRotate(Quaternion rot)
    {
        // rotate scaling handles                 
        for (uint dxHandleIdx = 9; dxHandleIdx <= 11; dxHandleIdx++)
            dxHandles[dxHandleIdx].setRotation(boundInstance.Rot.Axis.Vector3DCpy, (float)boundInstance.Rot.Angle);

        Quaternion scaleYHandleRot = boundInstance.Rot.QuaternionCpy * new Quaternion(new Vector3D(0.0f, 0.0f, 1.0f), 90.0);
        for (uint dxHandleIdx = 12; dxHandleIdx <= 14; dxHandleIdx++)
            dxHandles[dxHandleIdx].setRotation(scaleYHandleRot.Axis, (float)scaleYHandleRot.Angle);
        Quaternion scaleZHandleRot = boundInstance.Rot.QuaternionCpy * new Quaternion(new Vector3D(0.0f, 1.0f, 0.0f), -90.0);
        for (uint dxHandleIdx = 15; dxHandleIdx <= 17; dxHandleIdx++)
            dxHandles[dxHandleIdx].setRotation(scaleZHandleRot.Axis, (float)scaleZHandleRot.Angle);
    }

    private void updateActiveScaleHandleVec()
    {
        Matrix3D rotMat = Matrix3D.Identity;
        rotMat.Rotate(boundInstance.Rot.QuaternionCpy);
        activeScaleHandleVec = rotMat.Transform(activeScaleAxis);
    }

    
    */

    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::translateArrowShaftVertices = std::vector<DxRenderer::VertexData>(2);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::Scene::SceneModelInstanceManipulator::scaleArrowShaftVertices = std::vector<DxRenderer::VertexData>(2);
    std::vector<WORD> DxRenderer::Scene::SceneModelInstanceManipulator::arrowsShaftVertexIndices = std::vector<WORD>(2);
    std::vector<float> DxRenderer::Scene::SceneModelInstanceManipulator::handlesCirclesSegsCosines = std::vector<float>(HANDLES_CIRCLES_SEGS_NR);
    std::vector<float> DxRenderer::Scene::SceneModelInstanceManipulator::handlesCirclesSegsSines = std::vector<float>(HANDLES_CIRCLES_SEGS_NR);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::translationArrowHeadVertices = std::vector<VertexData>(HANDLES_CIRCLES_SEGS_NR + 2);
    std::vector<WORD> DxRenderer::Scene::SceneModelInstanceManipulator::translationArrowHeadVertexIndices = std::vector<WORD>(6 * HANDLES_CIRCLES_SEGS_NR);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::scaleArrowHeadVertices = std::vector<DxRenderer::VertexData>(8);
    std::vector<WORD> DxRenderer::Scene::SceneModelInstanceManipulator::scaleArrowHeadVertexIndices = std::vector<WORD>(36);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::translationArrowEnvelopeVertices = std::vector<DxRenderer::VertexData>(6 * HANDLES_CIRCLES_SEGS_NR);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::scaleArrowEnvelopeVertices = std::vector<DxRenderer::VertexData>(2 * HANDLES_CIRCLES_SEGS_NR);
    std::vector<WORD> DxRenderer::Scene::SceneModelInstanceManipulator::arrowEnvelopesVertexIndices = std::vector<WORD>(6 * HANDLES_CIRCLES_SEGS_NR);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::translationRectVertices = std::vector<DxRenderer::VertexData>(4);
    std::vector<WORD> DxRenderer::Scene::SceneModelInstanceManipulator::translationRectVertexIndices = std::vector<WORD>(12);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::rotationRingVertices = std::vector<VertexData>(HANDLES_CIRCLES_SEGS_NR);
    std::vector<WORD> DxRenderer::Scene::SceneModelInstanceManipulator::rotationRingVertexIndices = std::vector<WORD>(2 * HANDLES_CIRCLES_SEGS_NR);
    std::vector<DxRenderer::VertexData> DxRenderer::Scene::SceneModelInstanceManipulator::rotationRingEnvelopeVertices = std::vector<DxRenderer::VertexData>(HANDLES_CIRCLES_SEGS_NR * HANDLES_CIRCLES_SEGS_NR);
    std::vector<WORD> DxRenderer::Scene::SceneModelInstanceManipulator::rotationRingEnvelopeVertexIndices = std::vector<WORD>(6 * HANDLES_CIRCLES_SEGS_NR * HANDLES_CIRCLES_SEGS_NR);

    void DxRenderer::Scene::SceneModelInstanceManipulator::initModels3D() {
        // translation & scale arrows shaft
        // vertices
        translateArrowShaftVertices = { { { 0.0f, 0.0f, 0.0f }, {1.0f, 0.0f, 0.0f, 1.0f} } , { { TASL, 0.0f, 0.0f }, {1.0f, 0.0f, 0.0f, 1.0f} } };        
        scaleArrowShaftVertices = { { { 0.0f, 0.0f, 0.0f }, {0.0f, 1.0f, 0.0f, 0.0f} } , { { SCALE_ARROW_SHAFT_LEN_TO_TASL_RATIO * TASL, 0.0f, 0.0f }, {0.0f, 0.0f, 1.0f, 0.0f} } };
        // vertex indices
        arrowsShaftVertexIndices = { 0, 1 };

        // precompute cosines and sines for round handle features
        static const float handlesCirclesSegsArg = DirectX::XM_2PI / HANDLES_CIRCLES_SEGS_NR;
        for (unsigned int segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++) {
            handlesCirclesSegsCosines[segIdx] = (float)std::cos(segIdx * handlesCirclesSegsArg);
            handlesCirclesSegsSines[segIdx] = std::sin(segIdx * handlesCirclesSegsArg);
        }                            
        
        // translation arrow head
        // vertices
        static const float translationArrowHeadRadius = TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO * TASL;
        for (unsigned int vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
            translationArrowHeadVertices[vertexIdx] = { { TASL, translationArrowHeadRadius * handlesCirclesSegsSines[vertexIdx] , translationArrowHeadRadius * handlesCirclesSegsCosines[vertexIdx]}, {0.0f, 1.0f, 0.0f, 1.0f} };
        translationArrowHeadVertices[HANDLES_CIRCLES_SEGS_NR] = { {TASL * (1 + TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO), 0.0f, 0.0f }, {0.0f, 1.0f, 0.0f, 1.0f} };
        translationArrowHeadVertices[HANDLES_CIRCLES_SEGS_NR + 1] = { {TASL, 0.0f, 0.0f }, {0.0f, 1.0f, 0.0f, 1.0f} };
        // vertex indices
        for (unsigned int segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
        {
            translationArrowHeadVertexIndices[6u * segIdx] = translationArrowHeadVertexIndices[6u * segIdx + 3] = (WORD)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
            translationArrowHeadVertexIndices[6u * segIdx + 1] = translationArrowHeadVertexIndices[6u * segIdx + 4] = segIdx;
            translationArrowHeadVertexIndices[6u * segIdx + 2] = (WORD)HANDLES_CIRCLES_SEGS_NR;
            translationArrowHeadVertexIndices[6u * segIdx + 3] = segIdx;
            translationArrowHeadVertexIndices[6u * segIdx + 4] = (WORD)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
            translationArrowHeadVertexIndices[6u * segIdx + 5] = (WORD)HANDLES_CIRCLES_SEGS_NR + 1;
        }
        
        // scale arrow head
        static const float scaleArrowShaftLen = SCALE_ARROW_SHAFT_LEN_TO_TASL_RATIO * TASL;
        // vertices
        scaleArrowHeadVertices[0] = { {scaleArrowShaftLen, SCALE_ARROW_HEAD_HALF_SIDE, SCALE_ARROW_HEAD_HALF_SIDE}, {1.0f, 0.0f, 1.0f, 1.0f} };
        scaleArrowHeadVertices[1] = { {scaleArrowShaftLen, SCALE_ARROW_HEAD_HALF_SIDE, -SCALE_ARROW_HEAD_HALF_SIDE}, {1.0f, 0.0f, 1.0f, 1.0f} };
        scaleArrowHeadVertices[2] = { {scaleArrowShaftLen, -SCALE_ARROW_HEAD_HALF_SIDE, -SCALE_ARROW_HEAD_HALF_SIDE}, {1.0f, 0.0f, 1.0f, 1.0f} };
        scaleArrowHeadVertices[3] = { {scaleArrowShaftLen, -SCALE_ARROW_HEAD_HALF_SIDE, SCALE_ARROW_HEAD_HALF_SIDE}, {1.0f, 0.0f, 1.0f, 1.0f} };
        for (unsigned int vertexIdx = 0; vertexIdx < 4; vertexIdx++)
            scaleArrowHeadVertices[4 + vertexIdx] = { {scaleArrowHeadVertices[vertexIdx].pos.x + 2.0f * SCALE_ARROW_HEAD_HALF_SIDE, scaleArrowHeadVertices[vertexIdx].pos.y, scaleArrowHeadVertices[vertexIdx].pos.z }, {1.0f, 0.0f, 1.0f, 1.0f} };
        // vertex indices
        // left face
        scaleArrowHeadVertexIndices[0] = 0; scaleArrowHeadVertexIndices[1] = 1; scaleArrowHeadVertexIndices[2] = 3;
        scaleArrowHeadVertexIndices[3] = 3; scaleArrowHeadVertexIndices[4] = 1; scaleArrowHeadVertexIndices[5] = 2;
        // top face
        scaleArrowHeadVertexIndices[6] = 0; scaleArrowHeadVertexIndices[7] = 4; scaleArrowHeadVertexIndices[8] = 1;
        scaleArrowHeadVertexIndices[9] = 1; scaleArrowHeadVertexIndices[10] = 4; scaleArrowHeadVertexIndices[11] = 5;
        // front face
        scaleArrowHeadVertexIndices[12] = 1; scaleArrowHeadVertexIndices[13] = 5; scaleArrowHeadVertexIndices[14] = 6;
        scaleArrowHeadVertexIndices[15] = 6; scaleArrowHeadVertexIndices[16] = 2; scaleArrowHeadVertexIndices[17] = 1;
        // right face
        scaleArrowHeadVertexIndices[18] = 5; scaleArrowHeadVertexIndices[19] = 4; scaleArrowHeadVertexIndices[20] = 7;
        scaleArrowHeadVertexIndices[21] = 7; scaleArrowHeadVertexIndices[22] = 6; scaleArrowHeadVertexIndices[23] = 5;
        // bottom face
        scaleArrowHeadVertexIndices[24] = 3; scaleArrowHeadVertexIndices[25] = 2; scaleArrowHeadVertexIndices[26] = 6;
        scaleArrowHeadVertexIndices[27] = 6; scaleArrowHeadVertexIndices[28] = 7; scaleArrowHeadVertexIndices[29] = 3;
        // back face
        scaleArrowHeadVertexIndices[30] = 0; scaleArrowHeadVertexIndices[31] = 3; scaleArrowHeadVertexIndices[32] = 7;
        scaleArrowHeadVertexIndices[33] = 7; scaleArrowHeadVertexIndices[34] = 4; scaleArrowHeadVertexIndices[35] = 0;

        // translation & scale arrows envelope
        static const float translationArrowEnvelopeLen = TASL * (1 + TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO);
        static const float scaleArrowEnvelopeLen = scaleArrowShaftLen + 2.0f * SCALE_ARROW_HEAD_HALF_SIDE;
        // vertices
        for (unsigned int vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
        {
            translationArrowEnvelopeVertices[vertexIdx] = { {0.0f, 0.5f * translationArrowHeadVertices[vertexIdx].pos.y, 0.5f * translationArrowHeadVertices[vertexIdx].pos.z} , {1.0f, 1.0f, 1.0f, 0.0f} };
            translationArrowEnvelopeVertices[vertexIdx + HANDLES_CIRCLES_SEGS_NR] = { {translationArrowEnvelopeLen, translationArrowHeadVertices[vertexIdx].pos.y, translationArrowHeadVertices[vertexIdx].pos.z}, {1.0f, 1.0f, 1.0f, 0.0f} };
            scaleArrowEnvelopeVertices[vertexIdx + HANDLES_CIRCLES_SEGS_NR] = { {scaleArrowEnvelopeLen, translationArrowHeadVertices[vertexIdx].pos.y, translationArrowHeadVertices[vertexIdx].pos.z}, {1.0f, 1.0f, 1.0f, 0.0f} };
        }
        // vertex indices
        for (unsigned int segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
        {
            arrowEnvelopesVertexIndices[6u * segIdx] = segIdx;
            arrowEnvelopesVertexIndices[6u * segIdx + 1] = (WORD)(HANDLES_CIRCLES_SEGS_NR + segIdx);
            arrowEnvelopesVertexIndices[6u * segIdx + 2] = (WORD)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
            arrowEnvelopesVertexIndices[6u * segIdx + 3] = (WORD)(HANDLES_CIRCLES_SEGS_NR + segIdx);
            arrowEnvelopesVertexIndices[6u * segIdx + 4] = (WORD)(HANDLES_CIRCLES_SEGS_NR + ((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR));
            arrowEnvelopesVertexIndices[6u * segIdx + 5] = (WORD)((segIdx + 1) % HANDLES_CIRCLES_SEGS_NR);
        }

        // translation rect
        // vertices
        const float translationRectSide = TRANSLATION_RECT_SIDE_TO_TASL_RATIO * TASL;
        translationRectVertices[0] = { { 0.0f, 0.0f, 0.0f }, {0.0f, 1.0f, 1.0f, 1.0f} };
        translationRectVertices[1] = { {translationRectSide, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} };
        translationRectVertices[2] = { {translationRectSide, translationRectSide, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} };
        translationRectVertices[3] = { {0.0f, translationRectSide, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f} };
        // vertex indices
        translationRectVertexIndices[0] = 0; translationRectVertexIndices[1] = 3; translationRectVertexIndices[2] = 2;
        translationRectVertexIndices[3] = 2; translationRectVertexIndices[4] = 1; translationRectVertexIndices[5] = 0;
        translationRectVertexIndices[6] = 0; translationRectVertexIndices[7] = 1; translationRectVertexIndices[8] = 2;
        translationRectVertexIndices[9] = 2; translationRectVertexIndices[10] = 3; translationRectVertexIndices[11] = 0;

        // rotation ring
        // vertices
        const float rotationRingRadius = ROTATION_RING_RADIUS_TO_TASL_RATIO * TASL;
        for (unsigned int vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
            rotationRingVertices[vertexIdx] = { { rotationRingRadius * handlesCirclesSegsCosines[vertexIdx], rotationRingRadius * handlesCirclesSegsSines[vertexIdx], 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f} };                  
        // vertex indices
        for (unsigned int segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
        {
            rotationRingVertexIndices[2 * segIdx] = segIdx;
            rotationRingVertexIndices[2 * segIdx + 1] = (segIdx + 1) % HANDLES_CIRCLES_SEGS_NR;
        }

        // rotation ring envelope
        // vertices
        for (unsigned int vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++) {
            rotationRingEnvelopeVertices[vertexIdx] = 
                { 
                    { ROTATION_RING_ENVELOPE_THICKNESS_RADIUS * handlesCirclesSegsCosines[vertexIdx] + rotationRingRadius, 
                      0.0f, 
                      ROTATION_RING_ENVELOPE_THICKNESS_RADIUS * handlesCirclesSegsSines[vertexIdx]}, 
                    {1.0f, 1.0f, 1.0f, 0.0f} 
                };
        }
        for (unsigned int segIdx = 1; segIdx < HANDLES_CIRCLES_SEGS_NR; segIdx++)
        {
            for (unsigned int vertexIdx = 0; vertexIdx < HANDLES_CIRCLES_SEGS_NR; vertexIdx++)
            {
                rotationRingEnvelopeVertices[segIdx * HANDLES_CIRCLES_SEGS_NR + vertexIdx] =
                    { 
                        { rotationRingEnvelopeVertices[vertexIdx].pos.x * handlesCirclesSegsCosines[segIdx],
                          rotationRingEnvelopeVertices[vertexIdx].pos.x * handlesCirclesSegsSines[segIdx],
                          rotationRingEnvelopeVertices[vertexIdx].pos.z},
                        {1.0f, 1.0f, 1.0f, 0.0f} 
                    };
            }
        }

        // vertex indices
        for (unsigned int segIdx = 0; segIdx < HANDLES_CIRCLES_SEGS_NR - 1; segIdx++)
        {
            WORD facesBaseIdx = 6 * segIdx * HANDLES_CIRCLES_SEGS_NR;
            WORD vertexBaseIdx = segIdx * HANDLES_CIRCLES_SEGS_NR;
            for (WORD faceIdx = 0; faceIdx < HANDLES_CIRCLES_SEGS_NR; faceIdx++)
            {
                rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx] = vertexBaseIdx + faceIdx;
                rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 1] = vertexBaseIdx + HANDLES_CIRCLES_SEGS_NR + faceIdx;
                rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 2] = vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR;
                rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 3] = vertexBaseIdx + HANDLES_CIRCLES_SEGS_NR + faceIdx;
                rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 4] = vertexBaseIdx + HANDLES_CIRCLES_SEGS_NR + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR;
                rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 5] = vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR;
            }
        }
        
        WORD facesBaseIdx = 6 * (HANDLES_CIRCLES_SEGS_NR - 1) * HANDLES_CIRCLES_SEGS_NR;
        WORD vertexBaseIdx = (HANDLES_CIRCLES_SEGS_NR - 1) * HANDLES_CIRCLES_SEGS_NR;
        for (WORD faceIdx = 0; faceIdx < HANDLES_CIRCLES_SEGS_NR; faceIdx++)
        {
            rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx] = vertexBaseIdx + faceIdx;
            rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 1] = faceIdx;
            rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 2] = vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR;
            rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 3] = faceIdx;
            rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 4] = (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR;
            rotationRingEnvelopeVertexIndices[facesBaseIdx + 6 * faceIdx + 5] = vertexBaseIdx + (faceIdx + 1) % HANDLES_CIRCLES_SEGS_NR;
        }        
    }

    DxRenderer::Scene::SceneModelInstanceManipulator::SceneModelInstanceManipulator(Scene& _scene) : 
            scene(_scene),
            selectionHandlers{ {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnZ, this, std::placeholders::_1, std::placeholders::_2)},
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnZ, this, std::placeholders::_1, std::placeholders::_2)},                               
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnZ, this, std::placeholders::_1, std::placeholders::_2)},
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnZ, this, std::placeholders::_1, std::placeholders::_2)},
                               
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnXY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnXZ, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnYZ, this, std::placeholders::_1, std::placeholders::_2)},
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundZ, this, std::placeholders::_1, std::placeholders::_2)},
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnZ, this, std::placeholders::_1, std::placeholders::_2)},
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnZ, this, std::placeholders::_1, std::placeholders::_2)},
                               {std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundX, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundY, this, std::placeholders::_1, std::placeholders::_2),
                                std::bind(&DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundZ, this, std::placeholders::_1, std::placeholders::_2)}
                            } {}                

    void DxRenderer::Scene::SceneModelInstanceManipulator::onMouseMove(float cursorPosX, float cursorPosY) {        
        if (activeHandle == Handle::None)
            return;
        else
        {
            if (activeHandle == Handle::TranslateX || activeHandle == Handle::TranslateY || activeHandle == Handle::TranslateZ) {
                XMVECTOR currTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, cursorPosX, cursorPosY);
                XMVECTOR translationVec = XMVector3Dot(activeTranslationAxis, currTransformPlaneDragPoint - prevTransformPlaneDragPoint) * activeTranslationAxis;
                scene.transformGrpTranslateEXE(translationVec, TransformSrc::Manipulator);

                prevTransformPlaneDragPoint = currTransformPlaneDragPoint;
            }
            else if (activeHandle == Handle::ScaleX || activeHandle == Handle::ScaleY || activeHandle == Handle::ScaleZ) {                
                XMVECTOR currTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleAxis, cursorPosX, cursorPosY);
                XMVECTOR scaleVec = XMVector3Dot(activeScaleAxis, currTransformPlaneDragPoint - prevTransformPlaneDragPoint) * activeScaleAxisRotated;
                scene.transformGrpScaleEXE(scaleVec, TransformSrc::Manipulator);

                prevTransformPlaneDragPoint = currTransformPlaneDragPoint;
            }
            else if (activeHandle == Handle::TranslateXY || activeHandle == Handle::TranslateXZ || activeHandle == Handle::TranslateYZ) {
                XMVECTOR currTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, cursorPosX, cursorPosY);
                XMVECTOR translationVec = currTransformPlaneDragPoint - prevTransformPlaneDragPoint;
                scene.transformGrpTranslateEXE(translationVec, TransformSrc::Manipulator);

                prevTransformPlaneDragPoint = currTransformPlaneDragPoint;
            }
            else // activeHandle == Handle::Rotate
            {                
                XMVECTOR cursorMoveVecInWorldSpace = scene.camera.screenVecToWorldVec(0.1f * (cursorPosX - prevCursorPos.x), -0.1f * (cursorPosY - prevCursorPos.y));
                XMVECTOR c = XMVector3Cross(instancePosToRotationStartPosVec, cursorMoveVecInWorldSpace);
                XMVECTOR d = XMVector3Dot(c, activeRotationAxis);                                
                XMVECTOR q = XMQuaternionRotationAxis(activeRotationAxis, XMVectorGetX(XMVector3Dot(XMVector3Cross(instancePosToRotationStartPosVec, cursorMoveVecInWorldSpace), activeRotationAxis)) / 180.0f * XM_PI);
                scene.transformGrpRotateEXE(activeRotationAxis, XMVectorGetX(XMVector3Dot(XMVector3Cross(instancePosToRotationStartPosVec, cursorMoveVecInWorldSpace), activeRotationAxis)), TransformSrc::Manipulator);

                prevCursorPos = { cursorPosX, cursorPosY };
            }
        }
    }
    
    void DxRenderer::Scene::SceneModelInstanceManipulator::onMouseUp() {                
        activeHandle = Handle::None;        
    }

    void DxRenderer::Scene::SceneModelInstanceManipulator::onCameraTranslation() {
        updateHandlesScale();
        updateTranslationRects();
        recompTransformat();        
    }

    void DxRenderer::Scene::SceneModelInstanceManipulator::onTransformGrpTranslated(CXMVECTOR translation) {
        onTransformGrpTranslationSet(pos + translation);        
    }

    void DxRenderer::Scene::SceneModelInstanceManipulator::onTransformGrpTranslationSet(CXMVECTOR translation) {
        pos = translation;        
        updateHandlesScale();
        updateTranslationRects();
        recompTransformat();
    }

    void DxRenderer::Scene::SceneModelInstanceManipulator::onTransformGrpRotated(CXMVECTOR rot) {
        onTransformGrpRotationSet(XMQuaternionMultiply(this->rot, rot));
    }

    void DxRenderer::Scene::SceneModelInstanceManipulator::onTransformGrpRotationSet(CXMVECTOR rot) {
        this->rot = rot;
        updateTranslationRects();
    }

    inline DirectX::XMVECTOR linePlaneIntersection(DirectX::XMVECTOR const& lineVec, DirectX::XMVECTOR const& linePoint, DirectX::XMVECTOR const& planeNormal, DirectX::XMVECTOR const& planePoint) {
        return linePoint + lineVec * XMVector3Dot(planePoint - linePoint, planeNormal) / XMVector3Dot(lineVec, planeNormal);
    }    
    
    XMVECTOR DxRenderer::Scene::SceneModelInstanceManipulator::transformAxisContainingPlaneDragPoint(XMVECTOR const& transformAxis, float cursorPosX, float cursorPosY) {
        CXMVECTOR cameraPos = scene.camera.getPos();           
        XMVECTOR containingPlaneNormal = XMVector3Cross(XMVector3Cross(transformAxis, cameraPos - pos), transformAxis);

        return linePlaneIntersection(scene.camera.cursorPosToRayDirection(cursorPosX, cursorPosY), cameraPos, containingPlaneNormal, pos);
    }
    
    void DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnX(float x, float y) {        
        activeHandle = Handle::TranslateX;       
        activeTranslationAxis = { 1.0f, 0.0f, 0.0f, 0.0f };
        prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, x, y);
    }

    void DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnY(float x, float y) {
        activeHandle = Handle::TranslateY;        
        activeTranslationAxis = { 0.0f, 1.0f, 0.0f, 0.0f };
        prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, x, y);
    }

    void DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnZ(float x, float y) {
        activeHandle = Handle::TranslateZ;        
        activeTranslationAxis = { 0.0f, 0.0f, 1.0f, 0.0f };
        prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeTranslationAxis, x, y);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnX(float x, float y) {
        activeHandle = Handle::ScaleX;
        activeScaleAxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        activeScaleAxisRotated = XMVectorAbs(XMVector3Rotate(activeScaleAxis, XMQuaternionInverse(rot)));
        prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleAxis, x, y);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnY(float x, float y) {
        activeHandle = Handle::ScaleY;
        activeScaleAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        activeScaleAxisRotated = XMVectorAbs(XMVector3Rotate(activeScaleAxis, XMQuaternionInverse(rot)));
        prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleAxis, x, y);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateScaleOnZ(float x, float y) {
        activeHandle = Handle::ScaleZ;
        activeScaleAxis = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        activeScaleAxisRotated = XMVectorAbs(XMVector3Rotate(activeScaleAxis, XMQuaternionInverse(rot)));
        prevTransformPlaneDragPoint = transformAxisContainingPlaneDragPoint(activeScaleAxis, x, y);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnXY(float x, float y) {
        activeHandle = Handle::TranslateXY;
        activeTranslationPlaneNormal = { 0.0f, 0.0f, 1.0f, 0.0f };
        prevTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, x, y);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnXZ(float x, float y) {
        activeHandle = Handle::TranslateXZ;
        activeTranslationPlaneNormal = { 0.0f, 1.0f, 0.0f, 0.0f };
        prevTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, x, y);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateTranslateOnYZ(float x, float y) {
        activeHandle = Handle::TranslateYZ;
        activeTranslationPlaneNormal = { 1.0f, 0.0f, 0.0f, 0.0f };
        prevTransformPlaneDragPoint = translationPlaneDragPoint(activeTranslationPlaneNormal, x, y);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundX(float x, float y) {
        activeHandle = Handle::RotateX;
        activeRotationAxis = { 1.0f, 0.0f, 0.0f };

        XMFLOAT3 rayDirection = scene.cursorPosToRayDirection(x, y);
        XMFLOAT3 cameraPos = scene.getCameraPos(); 
        XMFLOAT3 boundInstancePos;
        XMStoreFloat3(&boundInstancePos, pos);
        XMFLOAT3 rotationStartPos = { boundInstancePos.x, 0.0f, 0.0f };
        XMFLOAT3 _instancePosToRotationStartPosVec;
        if (fabs(rayDirection.x) > EPSILON)
        {
            float t = (float)((boundInstancePos.x - cameraPos.x) / rayDirection.x);
            rotationStartPos.y = cameraPos.y + t * rayDirection.y;
            rotationStartPos.z = cameraPos.z + t * rayDirection.z;
			_instancePosToRotationStartPosVec = { rotationStartPos.x - boundInstancePos.x, rotationStartPos.y - boundInstancePos.y, rotationStartPos.z - boundInstancePos.z };
        }
        else
            _instancePosToRotationStartPosVec = { cameraPos.x - boundInstancePos.x, cameraPos.y - boundInstancePos.y, cameraPos.z - boundInstancePos.z };

        instancePosToRotationStartPosVec = XMLoadFloat3(&_instancePosToRotationStartPosVec);
        instancePosToRotationStartPosVec = XMVector3Normalize(instancePosToRotationStartPosVec);

        prevCursorPos = { x, y };
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundY(float x, float y) {
        activeHandle = Handle::RotateY;
        activeRotationAxis = { 0.0f, 1.0f, 0.0f };

        XMFLOAT3 rayDirection = scene.cursorPosToRayDirection(x, y);
        XMFLOAT3 cameraPos = scene.getCameraPos();
        XMFLOAT3 boundInstancePos;
        XMStoreFloat3(&boundInstancePos, pos);
        XMFLOAT3 rotationStartPos = { 0.0f, boundInstancePos.y, 0.0f };
        XMFLOAT3 _instancePosToRotationStartPosVec;
        if (fabs(rayDirection.y) > EPSILON)
        {
            float t = (float)((boundInstancePos.y - cameraPos.y) / rayDirection.y);
            rotationStartPos.x = cameraPos.x + t * rayDirection.x;
            rotationStartPos.z = cameraPos.z + t * rayDirection.z;
            _instancePosToRotationStartPosVec = { rotationStartPos.x - boundInstancePos.x, rotationStartPos.y - boundInstancePos.y, rotationStartPos.z - boundInstancePos.z };
        }
        else
            _instancePosToRotationStartPosVec = { cameraPos.x - boundInstancePos.x, cameraPos.y - boundInstancePos.y, cameraPos.z - boundInstancePos.z };

        instancePosToRotationStartPosVec = XMLoadFloat3(&_instancePosToRotationStartPosVec);
        instancePosToRotationStartPosVec = XMVector3Normalize(instancePosToRotationStartPosVec);

        prevCursorPos = { x, y };
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::activateRotateRoundZ(float x, float y) {
        activeHandle = Handle::RotateZ;
        activeRotationAxis = { 0.0f, 0.0f, 1.0f };

        XMFLOAT3 rayDirection = scene.cursorPosToRayDirection(x, y);
        XMFLOAT3 cameraPos = scene.getCameraPos();
        XMFLOAT3 boundInstancePos;
        XMStoreFloat3(&boundInstancePos, pos);
        XMFLOAT3 rotationStartPos = { 0.0f, 0.0f, boundInstancePos.z };
        XMFLOAT3 _instancePosToRotationStartPosVec;
        if (fabs(rayDirection.z) > EPSILON)
        {
            float t = (float)((boundInstancePos.z - cameraPos.z) / rayDirection.z);
            rotationStartPos.x = cameraPos.x + t * rayDirection.x;
            rotationStartPos.y = cameraPos.y + t * rayDirection.y;
            _instancePosToRotationStartPosVec = { rotationStartPos.x - boundInstancePos.x, rotationStartPos.y - boundInstancePos.y, rotationStartPos.z - boundInstancePos.z };
        }
        else
            _instancePosToRotationStartPosVec = { cameraPos.x - boundInstancePos.x, cameraPos.y - boundInstancePos.y, cameraPos.z - boundInstancePos.z };

        instancePosToRotationStartPosVec = XMLoadFloat3(&_instancePosToRotationStartPosVec);
        instancePosToRotationStartPosVec = XMVector3Normalize(instancePosToRotationStartPosVec);

        prevCursorPos = { x, y };
    }

    inline DirectX::XMVECTOR DxRenderer::Scene::SceneModelInstanceManipulator::translationPlaneDragPoint(DirectX::XMVECTOR const& planeNormal, float cursorPosX, float cursorPosY) {
        return linePlaneIntersection(scene.camera.cursorPosToRayDirection(cursorPosX, cursorPosY), scene.camera.getPos(), planeNormal, pos);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::updateHandlesScale() {
        scale = XMVector3Length(tan(TASL_IN_VISUAL_DEGS_TO_FOV_RATIO * scene.camera.getFOV()) * (scene.camera.getPos() - pos));        
    }

    // convert quadrent code (quadrent defined by the horizontal dimension sign and the vertical dimension sign. 
    // dimension >= 0 => code = 0, dimension < 0 =? code = 1) to the rotation starting at 0 and ending at the quadrent starting axis    
    inline float quadrentCodeToRot(int horizonCode, int vertCode) {
        return XM_PI * vertCode + XM_PIDIV2 * (horizonCode ^ vertCode);
    }

    inline unsigned int floatSignCode(float f) {
        unsigned int i = *(unsigned int*)&f;
        return i >> 8 * sizeof(i) - 1;
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::updateTranslationRects() {
        XMVECTOR manipulatorCameraVec = XMVector3Rotate(scene.camera.getPos() - pos, XMQuaternionInverse(rot));
        XMFLOAT3 _manipulatorCameraVec; 
        XMStoreFloat3(&_manipulatorCameraVec, scene.camera.getPos() - pos);
        unsigned int xSignBit = floatSignCode(_manipulatorCameraVec.x);
        unsigned int ySignBit = floatSignCode(_manipulatorCameraVec.y);
        unsigned int zSignBit = floatSignCode(_manipulatorCameraVec.z);
        
        XMMATRIX xyTranslationRectTransformat = XMMatrixRotationNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), quadrentCodeToRot(xSignBit, ySignBit));
        D3D11_BOX rangeBox = { 0U, 0U, 0U, sizeof(XMMATRIX), 1U, 1U };
        scene.renderer.devcon->UpdateSubresource(scene.renderer.manipulatorHandlesModelRenderData[4].instancesDataBuffer, 0, &rangeBox, &xyTranslationRectTransformat, 0, 0);

        XMMATRIX xzTranslationRectTransformat = XMMatrixMultiply(translationRectHandlesTransformats[1].transformat, XMMatrixRotationNormal(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), -quadrentCodeToRot(xSignBit, zSignBit)));
        rangeBox = { sizeof(VisibleInstanceData), 0U, 0U, sizeof(VisibleInstanceData) + sizeof(XMMATRIX), 1U, 1U };
        scene.renderer.devcon->UpdateSubresource(scene.renderer.manipulatorHandlesModelRenderData[4].instancesDataBuffer, 0, &rangeBox, &xzTranslationRectTransformat, 0, 0);

        XMMATRIX yzTranslationRectTransformat = XMMatrixMultiply(translationRectHandlesTransformats[2].transformat, XMMatrixRotationNormal(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), -quadrentCodeToRot(zSignBit, ySignBit)));
        rangeBox = { 2*sizeof(VisibleInstanceData), 0U, 0U, 2*sizeof(VisibleInstanceData) + sizeof(XMMATRIX), 1U, 1U };
        scene.renderer.devcon->UpdateSubresource(scene.renderer.manipulatorHandlesModelRenderData[4].instancesDataBuffer, 0, &rangeBox, &yzTranslationRectTransformat, 0, 0);
    }

    inline void DxRenderer::Scene::SceneModelInstanceManipulator::recompTransformat() {
        transformat = XMMatrixTranspose(XMMatrixMultiply(XMMatrixScalingFromVector(scale), XMMatrixTranslationFromVector(pos)));
    }
}
    