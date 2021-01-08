#pragma once

#include "Camera.h"
#include "IdxPool.h"

#include <d3d11.h>
#include <list>
#include <vector>
#include <DXGItype.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <DXProgrammableCapture.h>
#include <limits>
#include <memory>
#include <functional>

namespace CoriumDirectX {
	const unsigned int MODELS_NR_MAX = 100;
	const unsigned int INSTANCES_NR_MAX = 1024;
	const unsigned int INSTANCES_BUFFERS_CAPACITY_INIT = 20;
	const unsigned int INSTANCES_BUFFERS_CAPACITY_INC_FACTOR = 2;
	const int GRAPHICS_DEBUG_MODEL_ID = -1;
    const int FRAMES_NR_TO_CAPTURE = 0;

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
        public:
            friend DxRenderer;

            class SceneModelInstance {
            public:
                friend Scene;

                // selectionHandler in:
                //	x -> cursor x coord on selection
                //	y -> cursor y coord on selection
                typedef void(*SelectionHandler)(float x, float y);                

                DirectX::XMFLOAT3 getTranslation();
                DirectX::FXMMATRIX& getModelTransformat() { return modelTransformat; }
                void addToTransformGrp();
                void removeFromTransformGrp();
                void highlight();
                void dim();
                void show();
                void hide();
                // retrun value: false if parent is a descendant of this SceneModelInstance, true otherwise				
                bool assignParent(SceneModelInstance* parent);
                void unparent();
                void release();

            private:
                class KDTreeDataNodeHolder;

                Scene& scene;
                unsigned int modelID;
                UINT instanceIdx;
                DirectX::XMVECTOR instanceColorMask;
                DirectX::XMVECTOR pos;
                DirectX::XMVECTOR scaleFactor;
                DirectX::XMVECTOR rot;
                DirectX::XMMATRIX modelTransformat;
                KDTreeDataNodeHolder& kdtreeDataNodeHolder;
                SelectionHandler selectionHandler;
                bool isShown = true;
                bool isHighlighted = false;
                UINT transformatsBufferOffset = (std::numeric_limits<UINT>::max)();
                std::list<SceneModelInstance*>::iterator transformGrpIt;

                SceneModelInstance* parent = NULL;
                std::list<SceneModelInstance*> children = std::list<SceneModelInstance*>();

                SceneModelInstance(Scene& Scene, UINT modelID, DirectX::CXMVECTOR _instanceColorMask, Transform const& transformInit, SelectionHandler callbackHandlers);
                ~SceneModelInstance();
                void addInstanceToKdtree();
                //void translate(DirectX::XMFLOAT3 const& translation);
                //void setTranslation(DirectX::XMFLOAT3 const& translation);            
                void translate(DirectX::CXMVECTOR translation);
                void setTranslation(DirectX::CXMVECTOR translation);
                //void scale(DirectX::XMFLOAT3 const& scaleFactorQ);
                //void setScale(DirectX::XMFLOAT3 const& scaleFactor);
                void scale(DirectX::CXMVECTOR scaleFactorQ);
                void setScale(DirectX::CXMVECTOR scaleFactor);
                //void rotate(DirectX::XMFLOAT3 const& ax, float ang);
                //void setRotation(DirectX::XMFLOAT3 const& ax, float ang);
                void rotate(DirectX::CXMVECTOR rot);
                void setRotation(DirectX::CXMVECTOR rot);
                void loadInstanceTransformatToBuffer();
                void unloadInstanceTransformatFromBuffer();
                void updateInstanceTransformatInBuffer();
                void updateBuffers();
                void recompTransformat();
                bool isInstanceDescendant(SceneModelInstance* instance);
            };            

            struct TransformCallbackHandlers {			            
                void(*translationHandler)(float x, float y, float z);
                void(*scaleHandler)(float x, float y, float z);
                void(*rotationHandler)(float axX, float axY, float axZ, float ang);
            };

            void activate();
            SceneModelInstance* createModelInstance(unsigned int modelID, DirectX::XMFLOAT4 const& instanceColorMask, Transform const& transformInit, SceneModelInstance::SelectionHandler selectionHandler);
            void transformGrpTranslate(DirectX::XMFLOAT3 const& translation);
            void transformGrpSetTranslation(DirectX::XMFLOAT3 const& translation);
            void transformGrpScale(DirectX::XMFLOAT3 const& scaleFactorQ);
            void transformGrpSetScale(DirectX::XMFLOAT3 const& scaleFactor);
            void transformGrpRotate(DirectX::XMFLOAT3 const& ax, float ang);
            void transformGrpSetRotation(DirectX::XMFLOAT3 const& ax, float ang);
            void panCamera(float x, float y);
            void rotateCamera(float x, float y);
            void zoomCamera(float amount);
            DirectX::XMFLOAT3 getCameraPos();
            float getCameraFOV();
            bool cursorSelect(float x, float y);
            DirectX::XMFLOAT3 screenVecToWorldVec(float x, float y);
            DirectX::XMFLOAT3 cursorPosToRayDirection(float x, float y);
            void release();

        private:
            class KDTreeImpl;

            class SceneModelInstanceManipulator {
            public:
                struct HandleModelData {
                    std::vector<VertexData> const& vertexData;
                    std::vector<WORD> const& vertexIndices;
                    const D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
                };                                

                struct HandleInstanceData {
                    DirectX::XMMATRIX transformat;
                    UINT instanceIdx;
                    DirectX::XMVECTOR color;
                };

                static const std::vector<HandleModelData> handlesModelData;
                static const std::vector<std::array<HandleInstanceData, 3>> handlesInstancesData;
                const std::vector<std::array<std::function<void(float x, float y)>, 3>> selectionHandlers;
                static void initModels3D();

                SceneModelInstanceManipulator(Scene& scene);                
                void onMouseMove(float cursorPosX, float cursorPosY);
                void onMouseUp();
                void onCameraTranslation();
                void onTransformGrpTranslated(DirectX::CXMVECTOR translation);
                void onTransformGrpTranslationSet(DirectX::CXMVECTOR translation);
                void onTransformGrpRotated(DirectX::CXMVECTOR rot);
                void onTransformGrpRotationSet(DirectX::CXMVECTOR rot);
                DirectX::CXMMATRIX getTransformat() const { 
                    return transformat; 
                }
                
            private:
                enum class Handle {
                    TranslateX, TranslateY, TranslateZ, Translate1D,
                    ScaleX, ScaleZ, ScaleY, Scale,
                    TranslateXY, TranslateXZ, TranslateYZ, Translate2D,
                    RotateX, RotateY, RotateZ, Rotate,
                    None
                };

                const float EPSILON = 1E-3f;
                static constexpr unsigned int HANDLES_CIRCLES_SEGS_NR = 20;
                static constexpr float TASL = 1.0f; // := Translation Arrow Shaft Length
                static constexpr float TRANSLATION_ARROW_HEAD_HEIGHT_TO_TASL_RATIO = 0.125f;
                static constexpr float TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO = 0.05f;
                static constexpr float SCALE_ARROW_SHAFT_LEN_TO_TASL_RATIO = 0.5f;
                // scale arrow head side computed below : scale arrow head is a cube whose surface plane is confined within translation arrow head's base
                static constexpr float TRANSLATION_RECT_SIDE_TO_TASL_RATIO = 0.15f;
                static constexpr float ROTATION_RING_RADIUS_TO_TASL_RATIO = 1.2f;
                // rotation ring envelope thickness computed below: equals to half the translation arrow head radius
                static constexpr float TASL_IN_VISUAL_DEGS_TO_FOV_RATIO = 0.2f;

                static constexpr float SCALE_ARROW_HEAD_HALF_SIDE = 0.5f * 1.41421356f * TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO * TASL;
                static constexpr float ROTATION_RING_ENVELOPE_THICKNESS_RADIUS = 0.5f * TRANSLATION_ARROW_HEAD_RADIUS_TO_TASL_RATIO * TASL;

                static std::vector<VertexData> translateArrowShaftVertices;
                static std::vector<VertexData> scaleArrowShaftVertices;
                static std::vector<WORD> arrowsShaftVertexIndices;

                static std::vector<float> handlesCirclesSegsCosines;
                static std::vector<float> handlesCirclesSegsSines;
                static std::vector<VertexData> translationArrowHeadVertices;
                static std::vector<WORD> translationArrowHeadVertexIndices;
                
                static std::vector<VertexData> scaleArrowHeadVertices;
                static std::vector<WORD> scaleArrowHeadVertexIndices;

                static std::vector<VertexData> translationArrowEnvelopeVertices;
                static std::vector<VertexData> scaleArrowEnvelopeVertices;
                static std::vector<WORD> arrowEnvelopesVertexIndices;

                static std::vector<VertexData> translationRectVertices;
                static std::vector<WORD> translationRectVertexIndices;

                static std::vector<VertexData> rotationRingVertices;
                static std::vector<WORD> rotationRingVertexIndices;
                static std::vector<VertexData> rotationRingEnvelopeVertices;
                static std::vector<WORD> rotationRingEnvelopeVertexIndices;

                static const std::array<HandleInstanceData, 3> translationHandlesTransformats;
                static const std::array<HandleInstanceData, 3> scaleHandlesTransformats;
                static const std::array<HandleInstanceData, 3> translationRectHandlesTransformats;
                static const std::array<HandleInstanceData, 3> rotationRingHandlesTransformats;

                Scene& scene;
                Handle activeHandle = Handle::None;                
                DirectX::XMVECTOR activeTranslationAxis{ 0.0f, 0.0f, 0.0f, 0.0f };                
                DirectX::XMVECTOR activeScaleAxis{ 0.0f, 0.0f, 0.0f, 0.0f };
                DirectX::XMVECTOR activeScaleAxisRotated{ 0.0f, 0.0f, 0.0f, 0.0f };
                DirectX::XMVECTOR activeTranslationPlaneNormal{ 0.0f, 0.0f, 0.0f, 0.0f };
                DirectX::XMVECTOR prevTransformPlaneDragPoint{ 0.0f, 0.0f, 0.0f, 0.0f };                
                DirectX::XMVECTOR activeRotationAxis{ 0.0f, 0.0f, 0.0f, 0.0f };
                DirectX::XMVECTOR instancePosToRotationStartPosVec{ 0.0f, 0.0f, 0.0f, 0.0f };
                DirectX::XMFLOAT2 prevCursorPos{ 0.0f, 0.0f };
                DirectX::XMVECTOR pos{ 0.0f, 0.0f, 0.0f, 0.0f };
                DirectX::XMVECTOR scale{ 1.0f, 1.0f, 1.0f, 0.0f };
                DirectX::XMVECTOR rot = DirectX::XMQuaternionIdentity();
                DirectX::XMMATRIX transformat = DirectX::XMMatrixIdentity();                                                                                                                                                                                                                   
                                           
                void activateTranslateOnX(float x, float y);
                void activateTranslateOnY(float x, float y);
                void activateTranslateOnZ(float x, float y);
                void activateScaleOnX(float x, float y);
                void activateScaleOnY(float x, float y);
                void activateScaleOnZ(float x, float y);
                void activateTranslateOnXY(float x, float y);
                void activateTranslateOnXZ(float x, float y);
                void activateTranslateOnYZ(float x, float y);
                void activateRotateRoundX(float x, float y);
                void activateRotateRoundY(float x, float y);
                void activateRotateRoundZ(float x, float y);
                //void onBoundInstanceRotate(DirectX::XMVECTOR const& rot);
                DirectX::XMVECTOR transformAxisContainingPlaneDragPoint(DirectX::XMVECTOR const& transformAxis, float cursorPosX, float cursorPosY);
                //void updateActiveScaleHandleVec();                        
                DirectX::XMVECTOR translationPlaneDragPoint(DirectX::XMVECTOR const& planeNormal, float cursorPosX, float cursorPosY);                                                           
                void updateHandlesScale();
                void updateTranslationRects();
                void recompTransformat();
            };

            enum class TransformSrc { Manipulator, Outside };

            struct SceneModelData {
                unsigned int modelID;
                IdxPool instanceIdxsPool = IdxPool();
                std::vector<SceneModelInstance*> sceneModelInstances = std::vector<SceneModelInstance*>(INSTANCES_BUFFERS_CAPACITY_INIT);
            };

            DxRenderer& renderer;
            Camera camera;
            KDTreeImpl* const kdtree;
            SceneModelInstanceManipulator instancesManipulator;
            std::list<SceneModelData*> sceneModelsData;
            std::list<SceneModelInstance*> transformGrp;
            TransformCallbackHandlers transformCallbackHandlers;

            IdxPool parentIdxsPool = IdxPool();

            Scene(DxRenderer& renderer, TransformCallbackHandlers const& transformCallbackHandlers);
            ~Scene();
            void loadViewMatToBuffer();
            void loadProjMatToBuffer();
            void loadVisibleInstancesDataToBuffers();
            void unloadVisibleInstancesDataToBuffers();
            void eraseSceneModelData(unsigned int modelID);
            void transformGrpTranslateEXE(DirectX::CXMVECTOR translation, TransformSrc src);
            void transformGrpSetTranslationEXE(DirectX::CXMVECTOR translation);
            void transformGrpScaleEXE(DirectX::CXMVECTOR scaleFactorQ, TransformSrc src);
            void transformGrpSetScaleEXE(DirectX::CXMVECTOR scaleFactor);
            void transformGrpRotateEXE(DirectX::CXMVECTOR rotAx, float rotAng, TransformSrc src);
            void transformGrpSetRotationEXE(DirectX::CXMVECTOR rot);
            bool testBoundingSphereVisibility(std::array<float, 3> const& boundingSphereCenter, float boundingSphereRadius);
            bool testAabbVisibility(std::array<float, 3> const& aabbMin, std::array<float, 3> const& aabbMax);
        };

        struct MouseCallbacks {
            std::function<void(float cursorPosX, float cursorPosY)> onMouseMoveCallback;
            std::function<void()> onMouseUpCallback;
        };        

		DxRenderer(float fov, float nearZ, float farZ);
		~DxRenderer();

		HRESULT initDirectXLmnts(void* resource);
		HRESULT addModel(std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, DirectX::XMFLOAT3 const& boundingSphereCenter, float boundingSphereRadius, D3D_PRIMITIVE_TOPOLOGY primitiveTopology, UINT* modelIDOut);
		HRESULT updateModelData(UINT modelID, std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, D3D_PRIMITIVE_TOPOLOGY primitiveTopology);
		HRESULT removeModel(UINT modelID);
		Scene* createScene(Scene::TransformCallbackHandlers const& transformCallbackHandlers, MouseCallbacks& outMouseCallbacks);
		HRESULT render();
		void captureFrame();

	private:		
        static const DirectX::XMVECTOR X_AXIS_COLOR;
        static const DirectX::XMVECTOR Y_AXIS_COLOR;
        static const DirectX::XMVECTOR Z_AXIS_COLOR;
		static const int BLUR_SPAN = 15;		
		static const float BLUR_STD;
		static const float BLUR_FACTOR;
		static const FLOAT CLEAR_COLOR[4];
		static const FLOAT SELECTION_TEXES_CLEAR_COLOR[4];		
		static const FLOAT BLUR_TEXES_CLEAR_COLOR[4];

        class SceneModelInstanceManipulator;

        struct ModelGeoData {
            ID3D11Buffer* vertexBuffer;
            ID3D11Buffer* indexBuffer;
            D3D_PRIMITIVE_TOPOLOGY primitiveTopology;
        };

        struct ManipulatorHandleModelRenderData {
            ModelGeoData modelGeoData;
            ID3D11Buffer* instancesDataBuffer;
        };
        
		struct ModelRenderData {
            ModelGeoData modelGeoData;
			BoundingSphere boundingSphere;						
			UINT visibleInstancesNr = 0;
			ID3D11Buffer* visibleInstancesDataBuffer;
			std::vector<UINT> visibleInstancesIdxs = std::vector<UINT>(INSTANCES_BUFFERS_CAPACITY_INIT);			
			UINT visibleHighlightedInstancesNr = 0;
			ID3D11Buffer* visibleHighlightedInstancesDataBuffer;	
			std::vector<UINT> visibleHighlightedInstancesIdxs = std::vector<UINT>(INSTANCES_BUFFERS_CAPACITY_INIT);			
			UINT visibleInstancesBuffersCapacity = INSTANCES_BUFFERS_CAPACITY_INIT;							
		};

		struct ScreenQuadVertexData {
			DirectX::XMFLOAT4 pos;
			DirectX::XMFLOAT2 tex;
		};
		
		struct VisibleInstanceData {
			DirectX::XMMATRIX transformat;
			//INT parentIdx;
            DirectX::XMVECTOR colorMask;
			UINT instanceIdx;            
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
        ID3D11Buffer* cbGlobalTransformat;
		ID3D11Buffer* cbBlur;
		UINT viewportWidth = 0;
		UINT viewportHeight = 0;
		ID3D11BlendState* blendStateTransparency = NULL;

		// scene shaders resources
        std::vector<ManipulatorHandleModelRenderData> manipulatorHandlesModelRenderData;
		std::vector<ModelRenderData> modelsRenderData = std::vector<ModelRenderData>(MODELS_NR_MAX);
		ID3D11RenderTargetView* rtView = NULL;
		ID3D11Texture2D* selectionTexes[3] = { NULL, NULL, NULL };
		ID3D11RenderTargetView* selectionRTViews[3] = { NULL, NULL, NULL };
		unsigned int updatedSelectionTexIdx = 3;
		ID3D11Texture2D* stagingSelectionTex = NULL;
		ID3D11Texture2D* dsTex = NULL;
		ID3D11DepthStencilView* dsView = NULL;
		ID3D11DepthStencilState* dsStateJustDepth = NULL;
		ID3D11DepthStencilState* dsStateDisable = NULL;

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
        ID3D11DepthStencilState* dsStateWriteMaskedStencil = NULL;

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
		unsigned int framesNrToCapture = FRAMES_NR_TO_CAPTURE;
        
		HRESULT loadShaders();	
		HRESULT initShaderResources(void* resource);		
        HRESULT loadGeoData(std::vector<VertexData> const& modelVertices, std::vector<WORD> const& modelVertexIndices, D3D_PRIMITIVE_TOPOLOGY primitiveTopology, ModelGeoData& modelGeoData);
		void startFrameCapture();
		void endFrameCapture();        
	};   	    
}