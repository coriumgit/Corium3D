//
// Created by omer on 13/01/02018.
//

#pragma once

#include "SystemDefs.h"
#include "ObjPool.h"
#include "BoundingSphere.h"
#include "AABB.h"
#include "BVH.h"
#include "OpenGL.h"
#include "GUI.h"
#include "AssetsOps.h"
#include <glm/glm.hpp>
#include <math.h>

namespace Corium3D {

	// TODO: move to gui-bytecode-loader
	const int RED_SZ = 8;
	const int GREEN_SZ = 8;
	const int BLUE_SZ = 8;
	const int ALPHA_SZ = 8;
	const int DEPTH_SZ = 8;
	const int STENCIL_SZ = 8;

	const unsigned int BONES_NR_PER_VERTEX_MAX = 4;

	constexpr GLfloat BKG_COLOR_R = 100.0f / 256;
	constexpr GLfloat BKG_COLOR_G = 149.0f / 256;
	constexpr GLfloat BKG_COLOR_B = 237.0f / 256;

	constexpr GLfloat CAMERA_FOV_INIT = (float)M_PI / 3;
	const GLfloat FRUSTUM_NEAR_INIT = 1.0f;
	const GLfloat FRUSTUM_FAR_INIT = 20.0f;

	const glm::vec3 CAMERA_POS_INIT(0.0f, 0.0f, 0.0f);
	const glm::vec3 CAMERA_PIVOT_INIT(0.0f, 0.0f, 0.0f);
	const glm::vec3 CAMERA_UP_INIT(0.0f, 1.0f, 0.0f);
	const glm::vec3 CAMERA_LOOK_DIRECTION_INIT(0.0f, 0.0f, -1.0f);

	const unsigned int FRAMES_NR_FOR_FPS_UPDATE = 20;

	class Renderer {
	public:
		class InstanceAnimationInterface;

		// TODO: Add differentiation between permanent and impermanent static models descs - 
		// will demand adding and removing impermanent static instances	
		Renderer(const char** vertexShadersFullPaths, const char** fragShadersFullPaths, unsigned int shadersNr,
			GUI& gui, GUI::TxtControl& fpsDisplay, GUI::TxtControl& visiblesDisplay);
		Renderer(Renderer const&) = delete;
		~Renderer();
		bool init(Corium3DEngineNativeWindowType window);
		void destroy();
		bool surfaceSzChanged(unsigned int width, unsigned int height);
		void loadScene(std::vector<ModelDesc>&& modelDescs, unsigned int staticModelDescsNr, unsigned int mobileModelDescsNr, unsigned int* modelsInstancesNrsMaxima, BVH& bvh);
		void unloadScene();
		void translateCamera(glm::vec3 const& translation);
		void resetCameraPivot();
		void translateCameraPivot(glm::vec3 const& translation);
		void rotCamera(float rotAng, glm::vec3 const& rotAx);
		void panCamera(glm::vec2 const& panVec);
		void rotCameraAroundViewportContainedAx(float rotAng, glm::vec2& rotAx);
		void translateCameraInViewDirection(float translation);
		void zoom(float factor);
		glm::vec3 const& getCameraPos() const { return cameraPos; }
		glm::vec3 const& getCameraLookDirection() const { return cameraLookDirection; }
		glm::vec3 getCameraUp() const { return glm::normalize(cameraUp); }

		float getVirtualScreenWidth() const { return virtualScreenWidth; }
		float getVirtualScreenHeight() const { return virtualScreenHeight; }
		float getFrustumNear() const { return frustumNear; }
		unsigned int getWinWidth() const { return winWidth; }
		unsigned int getWinHeight() const { return winHeight; }

		// REMINDER: modelIdx relates to indexing according to order given to Renderer with static models first
		void setStaticModelInstanceTransform(unsigned int modelIdx, unsigned int instanceIdx, glm::mat4 const& transformat);
		// TODO: Adopt into Material implementation	
		void changeModelInstanceColorsArr(unsigned int modelIdx, unsigned int instanceIdx, unsigned int meshIdx, unsigned int colorsArrIdx);
		InstanceAnimationInterface* activateAnimation(unsigned int modelIdx, unsigned int instanceIdx);
		void deactivateAnimation(InstanceAnimationInterface* instanceAnimatorAPI);

		bool render(double lag);

	private:
		class OpenGlContext;
		class VisibleNodesIt3D;
		class NodesIt2D;
		class ModelAnimator;
		class InstanceAnimator;

		struct Plane {
			glm::vec3 normal;
			float d;

			void updateD(glm::vec3 const& newPointOnPlane);
			float signedDistFromPoint(glm::vec3 const& point) const;
		};

		OpenGlContext* openGlContext;
		VisibleNodesIt3D* visibleNodesIt3D;
		NodesIt2D* nodesIt2D;

		unsigned int winWidth = 0;
		unsigned int winHeight = 0;
		bool needReinitGlLmnts = true;
		bool needReloadGlBuffers = false;
		bool isSceneLoaded = false;

		BVH* bvh = NULL;
		GUI& gui;

		GUI::TxtControl& fpsDisplay;
		GUI::TxtControl& visiblesDisplay;
		unsigned int framesCount = 0;
		unsigned int framesNrForFpsUpdate = FRAMES_NR_FOR_FPS_UPDATE;
		double prevRenderTime = 0;
		
		std::string* vertexShadersFullPaths;
		std::string* fragShadersFullPaths;
		unsigned int shadersNr;
		std::vector<ModelDesc> modelDescsBuffer;
		unsigned int staticModelsNr;
		unsigned int mobileModelsNr;
		unsigned int modelsNrTotal;		
		unsigned int* modelsInstancesNrsMaxima;		
		unsigned int verticesNrTotal;
		unsigned int instancesNrsMaximaMax;
		unsigned int staticInstancesNrMax;
		unsigned int bonesInstancedNrMax;
		unsigned int facesNrTotal;
		unsigned int verticesColorsNrTotal;
		unsigned int*** verticesColorsBaseIdxs;

		unsigned int* instancesBaseIdxsPerModel;

		std::string* vertexShaderCodesBuffer;
		std::string* fragShaderCodesBuffer;

		GLuint instanceDataIdxsBuffer;
		GLuint vertexBuffer;
		GLuint mvpMatsBuffer;
		GLuint bonesTransformsBaseIdxsBuffer;
		GLuint bonesTransformsBuffer;
		GLuint selectedVerticesColorsIdxsBuffer;
		GLuint verticesColorsBuffer;
		GLuint indicesBuffer;
		GLuint* vaos;
		GLuint* progs;
		GLuint* vertexShaders;
		GLuint* fragShaders;
		//GLuint* vertexPosAttribLocs;
		//GLuint* vertexColorAttribLocs;
		GLuint baseVertexUniformLoc;
		//GLuint* uvsAttribLocs;
		//GLuint* texSamplerUnifLocs;
		GLuint vpMatAttribLoc;
		GLuint meshTransformUniformLoc;
		glm::mat4* meshesTransformsBuffer;

		//<<TUNABLE SECTION>>
		//(*) Implementation for few sphoradic colors updates chosen among many per vertex colors arrays
		//TODO: Rename all of this (?)
		//unsigned int** instanceColorsIdxs;

		unsigned int** visibleStaticModelsInstancesIdxs;
		unsigned int* visibleStaticModelsInstancesNrs;

		glm::mat4** mobileModelsTransformatsBuffers;
		unsigned int** visibleMobileModelsInstancesIdxs;
		unsigned int* visibleMobileModelsInstancesNrs;

		ModelAnimator** modelsAnimators;
		InstanceAnimator*** instancesAnimators;

		glm::mat4 projMat = glm::mat4();
		glm::mat4 viewMat;
		glm::mat4 vpMat = glm::mat4();
		glm::vec3 cameraPos = CAMERA_POS_INIT;
		glm::vec3 cameraPivot = CAMERA_PIVOT_INIT;
		float cameraPivotShiftAmount = glm::distance(cameraPos, cameraPivot);
		glm::vec3 cameraLookDirection = CAMERA_LOOK_DIRECTION_INIT;
		glm::vec3 cameraRight = glm::normalize(glm::vec3(-CAMERA_LOOK_DIRECTION_INIT.z, 0.0f, CAMERA_LOOK_DIRECTION_INIT.x));
		glm::vec3 cameraUp = glm::cross(cameraRight, cameraLookDirection);
		float fov = CAMERA_FOV_INIT;
		float fovSin;
		float fovCos;
		float vertQuartFovCos;
		float vertQuartFovSin;
		float horizonQuartFovCos;
		float horizonQuartFovSin;
		float horizonHalfFovSin;
		float horizonHalfFovCos;
		float horizonHalfFovTan;
		//float fovReciprocal;
		float virtualScreenWidth;
		float virtualScreenHeight;
		const GLfloat frustumNear = FRUSTUM_NEAR_INIT;
		const GLfloat frustumFar = FRUSTUM_FAR_INIT;
		Plane frustumTopPlane;
		Plane frustumBotPlane;
		Plane frustumLeftPlane;
		Plane frustumRightPlane;
		Plane frustumNearPlane = { glm::vec3(0.0f, 0.0f, 1.0f), FRUSTUM_NEAR_INIT };
		Plane frustumFarPlane = { glm::vec3(0.0f, 0.0f, -1.0f), -FRUSTUM_FAR_INIT };

		////////////////////////////
		/////////// temp ///////////
		////////////////////////////
		GLuint debugVAO;
		GLuint debugVertexBuffer;
		GLuint debugVpMatBuffer;
		GLuint debugIndicesBuffer;
		GLuint debugProg;
		GLuint debugVertexShader;
		GLuint debugFragShader;
		GLuint debugVpMatUniformLoc;
		GLuint blahBuffer;		
		////////////////////////////

		void refreshProjMat();
		void refreshViewMat();
		void updateDueFovOrWinSzChange();
		void updateFrustumDs();
		void updateFrustumSidePlanesNormals();
		bool initOpenGlLmnts();
		bool loadOpenGlBuffers();
		void destroyOpenGlLmnts();
		bool isBoundingSphereVisible(BoundingSphere const& boundingSphere) const;
	};

	class Renderer::InstanceAnimationInterface {
	public:
		friend Renderer;

		void start(unsigned int animationIdx);

	private:
		InstanceAnimationInterface(InstanceAnimator* instanceAnimator, unsigned int modelIdx, unsigned int instanceIdx);
		~InstanceAnimationInterface() {}

		InstanceAnimator* instanceAnimator;
		unsigned int modelIdx;
		unsigned int instanceIdx;
	};

} // namespace Corium3D