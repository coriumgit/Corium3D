#pragma once

#include "Camera.h"
#include "IdxPool.h"
#include "BSH.h"

#include <d3d11.h>
#include <list>
#include <vector>
#include <DXGItype.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <DXProgrammableCapture.h>
#include <limits>

namespace CoriumDirectX {
	const unsigned int MODELS_NR_MAX = 100;
	const unsigned int INSTANCES_NR_MAX = 1024;
	const unsigned int INSTANCES_BUFFERS_CAPACITY_INIT = 20;
	const unsigned int INSTANCES_BUFFERS_CAPACITY_INC_FACTOR = 2;
	const bool GRAPHICS_DBUG_RUN = false;
	
	class DxRenderer {
	public:	
		struct VertexData {
			DirectX::XMFLOAT3 pos;
			DirectX::XMFLOAT4 color;
		};

		struct Transform {
			DirectX::XMFLOAT3 translation;
			DirectX::XMFLOAT3 scaleFactor;
			DirectX::XMFLOAT3 rotAx;
			float rotAng;

			DirectX::XMMATRIX genTransformat() const;
		};

		class Scene {
		private:
			struct SceneModelData;

		public:
			friend DxRenderer;

			class SceneModelInstance {
			public:
				friend Scene;
				
				typedef void (*SelectionHandler)();				

				void translate(DirectX::XMFLOAT3 const& translation);
				void setTranslation(DirectX::XMFLOAT3 const& translation);
				void scale(DirectX::XMFLOAT3 const& scaleFactorQ);
				void setScale(DirectX::XMFLOAT3 const& scaleFactor);
				void rotate(DirectX::XMFLOAT3 const& ax, float ang);
				void setRotation(DirectX::XMFLOAT3 const& ax, float ang);
				DirectX::FXMMATRIX& getModelTransformat() { return modelTransformat; }
				void highlight();
				void dim();
				void show();
				void hide();
				void release();				

			private:
				Scene& const scene;				
				unsigned int modelID;
				UINT instanceIdx;				
				Transform transform;
				DirectX::XMVECTOR pos;
				DirectX::XMVECTOR scaleFactor;
				DirectX::XMVECTOR rot;
				DirectX::XMMATRIX modelTransformat;
				BSH<SceneModelInstance>::DataNode* const bshDataNode;
				SelectionHandler const selectionHandler;
				bool isShown = true;
				bool isHighlighted = false;
				UINT transformatsBufferOffset = (std::numeric_limits<UINT>::max)();

				SceneModelInstance(Scene& Scene, UINT modelID, Transform const& transformInit, SelectionHandler selectionHandler);
				~SceneModelInstance() {}
				void loadInstanceTransformatToBuffer();
				void unloadInstanceTransformatFromBuffer();
				void updateInstanceTransformatInBuffer();
				void updateBuffers();
				void recompTransformat();								
			};
							
			void activate();
			SceneModelInstance* createModelInstance(unsigned int modelID, Transform const& transformInit, SceneModelInstance::SelectionHandler selectionHandler);
			void panCamera(float x, float y);
			void rotateCamera(float x, float y);
			void zoomCamera(float amount);
			DirectX::XMFLOAT3 getCameraPos();
			bool cursorSelect(float x, float y);
			DirectX::XMFLOAT3 cursorPosToRayDirection(float x, float y);
			void release();

		private:
			class VisibleInstancesIt;

			struct SceneModelData {
				unsigned int modelID;				
				IdxPool instanceIdxsPool = IdxPool();
				std::vector<SceneModelInstance*> sceneModelInstances = std::vector<SceneModelInstance*>(INSTANCES_BUFFERS_CAPACITY_INIT);
			};

			DxRenderer& renderer;
			Camera camera;
			BSH<SceneModelInstance> bsh;
			std::list<SceneModelData*> sceneModelsData;

			Scene(DxRenderer& renderer);
			~Scene() {}
			void loadViewMatToBuffer();
			void loadProjMatToBuffer();		
			void loadVisibleInstancesDataToBuffers();
		};	

		DxRenderer(float fov, float nearZ, float farZ);
		~DxRenderer();

		HRESULT initDirectXLmnts(void* resource);
		HRESULT addModel(std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, DirectX::XMFLOAT3 const& boundingSphereCenter, float boundingSphereRadius, D3D_PRIMITIVE_TOPOLOGY primitiveTopology, UINT* modelIDOut);
		HRESULT updateModelData(UINT modelID, std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, D3D_PRIMITIVE_TOPOLOGY primitiveTopology);
		HRESULT removeModel(UINT modelID);
		Scene* createScene();		
		HRESULT render();
		void captureFrame();

	private:		
		static const int BLUR_SPAN = 15;		
		static const float BLUR_STD;
		static const float BLUR_FACTOR;
		static const FLOAT CLEAR_COLOR[4];
		static const FLOAT SELECTION_TEXES_CLEAR_COLOR[4];		
		static const FLOAT BLUR_TEXES_CLEAR_COLOR[4];

		struct ModelRenderData {
			ID3D11Buffer* vertexBuffer;
			ID3D11Buffer* indexBuffer;
			BoundingSphere boundingSphere;						
			UINT visibleInstancesNr = 0;
			ID3D11Buffer* visibleInstancesTransformatsBuffer;
			std::vector<UINT> visibleInstancesIdxs = std::vector<UINT>(INSTANCES_BUFFERS_CAPACITY_INIT);
			ID3D11Buffer* visibleInstancesIdxsBuffer;
			UINT visibleHighlightedInstancesNr = 0;
			ID3D11Buffer* visibleHighlightedInstancesTransformatsBuffer;	
			std::vector<UINT> visibleHighlightedInstancesIdxs = std::vector<UINT>(INSTANCES_BUFFERS_CAPACITY_INIT);
			ID3D11Buffer* visibleHighlightedInstancesIdxsBuffer;
			UINT visibleInstancesBuffersCapacity = INSTANCES_BUFFERS_CAPACITY_INIT;									
			D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
		};
		
		struct ScreenQuadVertexData {
			DirectX::XMFLOAT4 pos;
			DirectX::XMFLOAT2 tex;
		};
		
		struct BlurVars {
			DirectX::XMFLOAT4 offsets[BLUR_SPAN];
			DirectX::XMFLOAT4 weights[BLUR_SPAN];
		};
		
		struct SceneModelInstanceIdxs {
			USHORT modelID;
			USHORT instanceIdx;
		};

		BlurVars blurVarsHorizon;
		BlurVars blurVarsVert;

		ID3D11Device* dev = NULL;
		ID3D11DeviceContext* devcon = NULL;		
		ID3D11Buffer* cbProjMat;
		ID3D11Buffer* cbViewMat;
		ID3D11Buffer* cbModelID;
		ID3D11Buffer* cbBlur;
		UINT viewportWidth = 0;
		UINT viewportHeight = 0;
		ID3D11BlendState* blendStateTransparency = NULL;

		// scene shaders resources
		std::vector<ModelRenderData> modelsRenderData = std::vector<ModelRenderData>(MODELS_NR_MAX);
		ID3D11RenderTargetView* rtView = NULL;
		ID3D11Texture2D* selectionTexes[3] = { NULL, NULL, NULL };
		ID3D11RenderTargetView* selectionRTViews[3] = { NULL, NULL, NULL };
		unsigned int updatedSelectionTexIdx = 3;
		ID3D11Texture2D* stagingSelectionTex = NULL;
		ID3D11Texture2D* dsTex = NULL;
		ID3D11DepthStencilView* dsView = NULL;
		ID3D11DepthStencilState* dsStateJustDepth = NULL;
		// scene shaders
		ID3D11VertexShader* vsScene = NULL;
		ID3D11PixelShader* psScene = NULL;
		ID3D11InputLayout* vlScene = NULL;

		// selection shaders resources

		ID3D11VertexShader* vsOutline = NULL; // outline vertex shader
		ID3D11PixelShader* psOutline = NULL; // outline pixel shader			

		// blur shaders resources
		ID3D11Buffer* vbScreenQuad = NULL;
		ID3D11Texture2D* blurTex0 = NULL;
		ID3D11RenderTargetView* blurRTView0 = NULL;
		ID3D11ShaderResourceView* blurSRView0 = NULL;
		ID3D11Texture2D* blurTex1 = NULL;
		ID3D11RenderTargetView* blurRTView1 = NULL;
		ID3D11ShaderResourceView* blurSRView1 = NULL;
		ID3D11SamplerState* blurTexSampState = NULL;
		ID3D11DepthStencilState* dsStateWriteStencil = NULL;
		ID3D11DepthStencilState* dsStateMaskStencil = NULL;
		// blur shaders
		ID3D11VertexShader* vsBlur = NULL;
		ID3D11PixelShader* psBlur = NULL;
		ID3D11InputLayout* vlBlur = NULL;						
								
		IdxPool modelsIDsPool;		
		Scene* activeScene = NULL;
		std::list<Scene*> scenes;
		float fov; 
		float nearZ;
		float farZ;

		IDXGraphicsAnalysis* pGraphicsAnalysis = NULL;
		unsigned int framesNrToCapture = 0;

		HRESULT loadShaders();	
		HRESULT initShaderResources(void* resource);		
		void startFrameCapture();
		void endFrameCapture();
	};

}