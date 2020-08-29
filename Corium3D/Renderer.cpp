//
// Created by omer on 27/01/02018.
//

#include "Renderer.h"

#include "ServiceLocator.h"
#include "FilesOps.h"
#include "Timer.h"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/norm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <limits.h>
#include <string>
#include <fstream>

#if defined(__ANDROID__) || defined(ANDROID)
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif



namespace Corium3D {

	using namespace Corium3DUtils;
	using namespace std;
	using std::to_string;

	const GLchar* INSTANCE_DATA_IDX_ATTRIB_NAME = "aInstanceDataIdx";
	const GLchar* VERTEX_POS_ATTRIB_NAME = "aPos";
	const GLchar* VERTEX_BONES_IDS_ATTRIB_NAME = "aBonesIDs";
	const GLchar* VERTEX_BONES_WEIGHTS_ATTRIB_NAME = "aBonesWeights";
	const GLuint MVPS_UNIF_BUFFER_BINDING = 4;
	const GLuint BONES_TRANSFORMS_UNIF_BUFFER_BINDING = 5;
	const GLuint SELECTED_COLORS_IDXS_UNIF_BUFFER_BINDING = 6;
	const GLuint COLORS_UNIF_BUFFER_BINDING = 7;

	const char* debugVertexShaderCode =
		"#version 430 core \n"

		"uniform mat4 uVpMat; \n"
		"in vec4 aPos; \n"

		"out vec4 passColor; \n"

		"void main(void) { \n"
		"	passColor = vec4(1.0f, 1.0f, 1.0f, 1.0f); \n"
		"	gl_Position = uVpMat * aPos; \n"
		"} \n";
	
	const char* debugFragShaderCode =
		"#version 430 core \n"
		"precision mediump float; \n"

		"in vec4 passColor; \n"	
		"out vec4 fragColor; \n"

		"void main(void) { \n"
		"	fragColor = passColor; \n"	
		"} \n";

	const float EPSILON = FLT_EPSILON;

	inline string to_string(vec3 v) {
		return string("(") + to_string(v.x) + string(", ") + to_string(v.y) + string(", ") + to_string(v.z) + string(")");
	}

	inline string to_string(vec2 v) {
		return string("(") + to_string(v.x) + string(", ") + to_string(v.y) + string(")");
	}

	class Renderer::OpenGlContext {
	public:
		bool init(Corium3DEngineNativeWindowType _window);
		void destroy();
		bool swapBuffers();

	private:
	#if defined(__ANDROID__) || defined(ANDROID)
		const EGLint eglAattribs[17] = {
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RED_SIZE, RED_SZ,
			EGL_GREEN_SIZE, GREEN_SZ,
			EGL_BLUE_SIZE, BLUE_SZ,
			EGL_ALPHA_SIZE, ALPHA_SZ,
			EGL_DEPTH_SIZE, DEPTH_SZ,
			EGL_STENCIL_SIZE, STENCIL_SZ,
			EGL_NONE
		};

		const EGLint contextAttribs[3] = {
			EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_NONE
		};

		EGLDisplay display = NULL;
		EGLContext context = NULL;
		EGLSurface surface = NULL;
		Corium3DNativeWindowType window = NULL;
		EGLConfig config = NULL;	

	#elif defined(_WIN32) || defined(__VC32__) || defined(_WIN64) || defined(__VC64__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
		const int contextAttribs[7] = { WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			0 };

		HGLRC hglrc;	
		HDC hdc;
		HWND hwnd;
	
	#endif
	};

	class Renderer::ModelAnimator {
	public:
		friend InstanceAnimator;
		ModelAnimator(aiScene const* scene, GLuint _bonesTransformatsBuffer, unsigned int _bonesTransformatsBufferModelBaseIdx, unsigned int _bonesNr, glm::mat4* _meshesTransformsBuffer, unsigned int instancesNrMax);
		~ModelAnimator();	
		InstanceAnimator* acquireInstance(unsigned int instanceIdx);
		void releaseInstance(InstanceAnimator* instanceAnimator);

	private:
		struct Animation {	
			struct TransformatsHierarchyNode {
				//TODO: Find out if there is use for this in case the node is animated
				glm::mat4 transformat;			
				bool isNodeAnimated;
				unsigned int meshesNr;
				unsigned int* meshesIdxs;
				unsigned int boneIdx;
				unsigned int childrenNr;
				TransformatsHierarchyNode** children;
				TransformatsHierarchyNode* parent;
			};

			// root is at index 0
			TransformatsHierarchyNode* TransformatsHierarchyBuffer;		
			unsigned int nodesNr;
			unsigned int treeDepthMax;
			double dur;
			unsigned int ticksPerSecond;
			unsigned int channelsNr;
			unsigned int keyFramesNr;
			double* keyFramesTimes;
			glm::vec3** scales;
			glm::quat** rots;
			glm::vec3** translations;
		};
		
		GLuint bonesTransformatsBuffer;
		unsigned int bonesTransformatsBufferModelBaseIdx;
		ObjPool<InstanceAnimator>* instanceAnimatorsPool;
		unsigned int bonesNr;
		glm::mat4* meshesTransformsBuffer;
		glm::mat4* bonesOffsets;
		unsigned int animationsNr;
		Animation* animations;
	
		Animation::TransformatsHierarchyNode genTransformatHierarchyNode(aiNode* node, const aiScene* scene, unsigned int animationIdx, unsigned int nodeToChannelIdxMapping);
	 };

	class Renderer::InstanceAnimator {
	public:
		friend ObjPool<InstanceAnimator>;
		friend ModelAnimator;

		void start(unsigned int animationIdx);
		void updateRendererBuffer();

	private:
		struct InitData {
			ModelAnimator& modelAnimator;
			unsigned int instanceIdx;
		};

		ModelAnimator const& modelAnimator;
		unsigned int bonesTransformatsBufferInstanceBaseIdx;

		ModelAnimator::Animation& animation;
		float activeAnimationStartTime;
		unsigned int endKeyFrameIdxCache;
		unsigned int* childrenIdxsStack;
		glm::mat4* transformatsStack;
		unsigned int instanceIdx;

		InstanceAnimator(InitData const& initData);
		~InstanceAnimator();
	};

	class Renderer::VisibleNodesIt3D {
	public:
		VisibleNodesIt3D(Renderer const& _renderer) : renderer(_renderer) {}
	
		void init(BVH::Node<AABB3DRotatable>* root) {
			if (root != NULL && renderer.isBoundingSphereVisible(static_cast<BVH::Node3D*>(root)->getBoundingSphere())) {
				it = root;
				findNextVisibleLeaf();
			}		
		}

		BVH::Node<AABB3DRotatable>* getNext() {
			BVH::Node<AABB3DRotatable>* retNode = it;		
			it = it->getEscapeNode();			
			findNextVisibleLeaf();
		
			return retNode;
		}
																		
		bool isDone() { return it == NULL; }

	private:
		Renderer const& renderer;
		BVH::Node<AABB3DRotatable>* it = NULL;

		void findNextVisibleLeaf() {
			while (it != NULL) {
				if (it->isLeaf()) {
					if (renderer.isBoundingSphereVisible(static_cast<BVH::Node3D*>(it)->getBoundingSphere()))
						break;
					else
						it = it->getEscapeNode();
				}
				else {
					if (renderer.isBoundingSphereVisible(static_cast<BVH::Node3D*>(it)->getBoundingSphere()))
						it = it->getChild(0);
					else
						it = it->getEscapeNode();
				}			
			}		
		}
	};

	class Renderer::NodesIt2D {
	public:
		NodesIt2D(Renderer const& _renderer) : renderer(_renderer) {}
	
		void init(BVH::Node<AABB2DRotatable>* root) {
			if (root != NULL) {
				for (it = root; it->getChild(0) != NULL; it = it->getChild(0));
			}
		}

		BVH::Node<AABB2DRotatable>* getNext() {
			BVH::Node<AABB2DRotatable>* retNode = it;
			// get next sub tree
			for (BVH::Node<AABB2DRotatable>* parent = it->getParent();
				parent != NULL && parent->getChild(1) == it;
				it = parent, parent = it->getParent());

			if ((it = it->getParent()) != NULL) {			
				for (it = it->getChild(1); it->getChild(0) != NULL; it = it->getChild(0));
			}

			return retNode;
		}

		bool isDone() { return it == NULL; }

	private:
		Renderer const& renderer;
		BVH::Node<AABB2DRotatable>* it = NULL;	
	};

	inline void cpyCStrsToStrs(string* strsArr, const char** cStrsArr, unsigned int cStrsNr) {
		for (unsigned int strIdx = 0; strIdx < cStrsNr; strIdx++)
			strsArr[strIdx] = cStrsArr[strIdx];
	}

	Renderer::Renderer(const char* modelDescsFullPath, const char** _vertexShadersFullPaths, const char** _fragShadersFullPaths, unsigned int _shadersNr,
					   GUI& _gui, GUI::TxtControl& _fpsDisplay, GUI::TxtControl& _visiblesDisplay) :
			modelDescsFullPath(modelDescsFullPath), vertexShadersFullPaths(new string[shadersNr]), fragShadersFullPaths(new string[shadersNr]), shadersNr(_shadersNr),
			gui(_gui), fpsDisplay(_fpsDisplay), visiblesDisplay(_visiblesDisplay) {	
		cpyCStrsToStrs(vertexShadersFullPaths, _vertexShadersFullPaths, shadersNr);
		cpyCStrsToStrs(fragShadersFullPaths, _fragShadersFullPaths, shadersNr);	
		
		vaos = new GLuint[shadersNr];		
		progs = new GLuint[shadersNr];
		vertexShaders = new GLuint[shadersNr];
		fragShaders = new GLuint[shadersNr];		

		openGlContext = new OpenGlContext();	
		visibleNodesIt3D = new VisibleNodesIt3D(*this);
		nodesIt2D = new NodesIt2D(*this);	
	}

	Renderer::~Renderer() {    	
		unloadScene();

		delete nodesIt2D;
		delete visibleNodesIt3D;	
		delete openGlContext;				

		delete[] fragShaders;
		delete[] vertexShaders;
		delete[] progs;
		delete[] vaos;
			
		delete[] vertexShadersFullPaths;
		delete[] fragShadersFullPaths;	
	}

	bool Renderer::init(Corium3DEngineNativeWindowType window) {
		return openGlContext->init(window);		
	}

	void Renderer::destroy() {
		destroyOpenGlLmnts();
		needReinitGlLmnts = true;
		openGlContext->destroy();
	}

	bool Renderer::surfaceSzChanged(unsigned int _winWidth, unsigned int _winHeight) {
		ServiceLocator::getLogger().logd("Renderer", "surfaceChanged called.");
		winWidth = _winWidth;
		winHeight= _winHeight;
		glViewport(0, 0, winWidth, winHeight);    	
		refreshProjMat();
		updateDueFovOrWinSzChange();

		if (needReinitGlLmnts) {
			if (!initOpenGlLmnts()) {
				openGlContext->destroy();
				return false;
			}
			needReinitGlLmnts = false;
		}

		if (needReloadGlBuffers) {
			loadOpenGlBuffers();
			needReloadGlBuffers = false;
		}

		return true;
	}

	void Renderer::loadScene(unsigned int* modelDescsIdxs, unsigned int staticModelDescsNr, unsigned int mobileModelDescsNr,
		unsigned int* _modelsInstancesNrsMaxima, BVH& _bvh) {
		std::ifstream modelDescsFile(modelDescsFullPath, std::ios::in | std::ios::binary);
	#if DEBUG
		if (!modelDescsFile.is_open())
			throw std::ios_base::failure("The models descriptors file failed to open.");
	#endif
		unloadScene();
		staticModelsNr = staticModelDescsNr;
		mobileModelsNr = mobileModelDescsNr;
		modelsNrTotal = staticModelsNr + mobileModelsNr;
		this->modelsInstancesNrsMaxima = new unsigned int[modelsNrTotal];
		memcpy(modelsInstancesNrsMaxima, _modelsInstancesNrsMaxima, modelsNrTotal);
		modelDescsBuffer = new ModelDesc[modelsNrTotal]{};
		for (unsigned int modelIdx = 0; modelIdx < modelsNrTotal; modelIdx++) {
			
			modelDescsFile.read((char*)& modelDescsBuffer[modelIdx], sizeof(ModelDesc));
		}		

		instancesBaseIdxsPerModel = new unsigned int[modelsNrTotal];
		verticesColorsBaseIdxs = new unsigned int**[modelsNrTotal];
		modelsAnimators = new ModelAnimator*[modelsNrTotal];
		instancesAnimators = new InstanceAnimator**[modelsNrTotal];
		verticesNrTotal = 0;
		instancesNrsMaximaMax = 0;
		instancesNrMax = 0;
		bonesInstancedNrMax = 0;
		facesNrTotal = 0;
		verticesColorsNrTotal = 0;
		for (unsigned int modelIdx = 0; modelIdx < modelsNrTotal; modelIdx++) {
			verticesNrTotal += modelDescsBuffer[modelIdx].verticesNr;
			unsigned int modelInstancesNrMax = modelsInstancesNrsMaxima[modelIdx];
			if (modelInstancesNrMax > instancesNrsMaximaMax)
				instancesNrsMaximaMax = modelInstancesNrMax;
			instancesNrMax += modelInstancesNrMax;
			bonesInstancedNrMax += modelDescsBuffer[modelIdx].bonesNr * modelInstancesNrMax;
			facesNrTotal += modelDescsBuffer[modelIdx].facesNr;
			modelsAnimators[modelIdx] = NULL;
			instancesAnimators[modelIdx] = new InstanceAnimator*[modelInstancesNrMax];
			for (unsigned int instanceIdx = 0; instanceIdx < modelInstancesNrMax; instanceIdx++)
				instancesAnimators[modelIdx][instanceIdx] = NULL;
			verticesColorsNrTotal += modelDescsBuffer[modelIdx].verticesColorsNrTotal;
			verticesColorsBaseIdxs[modelIdx] = new unsigned int* [modelDescsBuffer[modelIdx].meshesNr];
			for (unsigned int meshIdx = 0; meshIdx < modelDescsBuffer[modelIdx].meshesNr; meshIdx++)
				verticesColorsBaseIdxs[modelIdx][meshIdx] = new unsigned int[modelDescsBuffer[modelIdx].extraColorsNrsPerMesh[meshIdx]]();
		}

		visibleStaticModelsInstancesIdxs = new unsigned int*[staticModelsNr];
		visibleStaticModelsInstancesNrs = new unsigned int[staticModelsNr]();
		for (unsigned int modelIdx = 0; modelIdx < staticModelsNr; modelIdx++) {
			unsigned int staticModelInstancesNrMax = modelsInstancesNrsMaxima[modelIdx];
			visibleStaticModelsInstancesIdxs[modelIdx] = new unsigned int[staticModelInstancesNrMax]();
		}

		mobileModelsTransformatsBuffers = new glm::mat4*[mobileModelsNr];
		visibleMobileModelsInstancesIdxs = new unsigned int*[mobileModelsNr]();
		visibleMobileModelsInstancesNrs = new unsigned int[mobileModelsNr]();
		for (unsigned int modelIdx = 0; modelIdx < mobileModelsNr; modelIdx++) {
			unsigned int mobileModelInstancesNrMax = modelsInstancesNrsMaxima[staticModelsNr + modelIdx];
			mobileModelsTransformatsBuffers[modelIdx] = new glm::mat4[mobileModelInstancesNrMax];
			visibleMobileModelsInstancesIdxs[modelIdx] = new unsigned int[mobileModelInstancesNrMax]();
		}

		viewMat = glm::lookAt(cameraPos, cameraPos + cameraLookDirection, cameraUp);
		if (!needReinitGlLmnts)
			loadOpenGlBuffers();	
		else
			needReloadGlBuffers = true;

		bvh = &_bvh;

		isSceneLoaded = true;
	}

	void Renderer::unloadScene() {	
		if (!isSceneLoaded)
			return;
	
		bvh = NULL;

		for (unsigned int modelIdx = 0; modelIdx < mobileModelsNr; modelIdx++) {		
			delete[] mobileModelsTransformatsBuffers[modelIdx];
			delete[] visibleMobileModelsInstancesIdxs[modelIdx];
		}
		delete[] mobileModelsTransformatsBuffers;
		delete[] visibleMobileModelsInstancesIdxs;
		delete[] visibleMobileModelsInstancesNrs;
	
		for (unsigned int modelIdx = 0; modelIdx < staticModelsNr; modelIdx++)		
			delete[] visibleStaticModelsInstancesIdxs[modelIdx];	
		delete[] visibleStaticModelsInstancesIdxs;
		delete[] visibleStaticModelsInstancesNrs;

		for (unsigned int modelIdx = 0; modelIdx < modelsNrTotal; modelIdx++) {		
			delete[] instancesAnimators[modelIdx];				
			for (unsigned int meshIdx = 0; meshIdx < modelDescsBuffer[modelIdx].meshesNr; meshIdx++)
				delete[] verticesColorsBaseIdxs[modelIdx][meshIdx];
			delete[] verticesColorsBaseIdxs[modelIdx];
		}	
		delete[] verticesColorsBaseIdxs;	
		delete[] instancesAnimators;
		delete[] instancesBaseIdxsPerModel;
		delete[] modelsAnimators;

		delete[] modelsInstancesNrsMaxima;

		isSceneLoaded = false;
		needReloadGlBuffers = false;	
	}

	void Renderer::translateCamera(glm::vec3 const& translation) {	
		cameraPivot += translation;
		cameraPos += translation;	
		viewMat = glm::lookAt(cameraPos, cameraPos + cameraLookDirection, cameraUp);
		vpMat = projMat * viewMat;
	
		updateFrustumDs();
	}

	void Renderer::resetCameraPivot() {
		cameraPivot = glm::vec3(0.0f, 0.0f, 0.0f);
		cameraPivotShiftAmount = 0.0F;
	}

	void Renderer::translateCameraPivot(glm::vec3 const& translation) {
		cameraPivot += translation;
		cameraPivotShiftAmount += glm::length(translation);
	}

	// cameraUp old derivation:
	// (isCameraFlippedFactor*(0,1,0) + t*cameraLookDirection)*cameraLookDirection = 0
	// =>  [found t] = - cameraLookDirection.y / length2(cameraLookDirection)
	// cameraUp = isCameraFlippedFactor*(0,1,0) + [found t]*cameraLookDirection	
	// **Reminder: cameraUp is not normalized

	// cameraRight derivation:
	// newly rotated cameraRight + t*(newly rotated cameraUp) = (corrected camera right x, 0.0f, corrected camera right z)
	// => t = - newly rotated cameraRight Y / - newly rotated cameraUp Y 
	// => fixed cameraRight.x = newly rotated cameraRight X + [found t]*newly rotated cameraUp X
	// => fixed cameraRight.z = newly rotated cameraRight Z + [found t]*newly rotated cameraUp Z
	void Renderer::rotCamera(float rotAng, glm::vec3 const&  rotAx) {	
		glm::quat rotQuat = glm::angleAxis(rotAng, rotAx);
		cameraLookDirection = rotQuat * cameraLookDirection;		
		glm::vec3 cameraRightRotated = rotQuat * cameraRight;	
		glm::vec3 cameraUpRotated = glm::cross(cameraRight, cameraLookDirection);
		float t = -cameraRightRotated.y / cameraUpRotated.y;
		cameraRight.x = cameraRightRotated.x + t*cameraUpRotated.x;
		cameraRight.y = 0.0f;
		cameraRight.z = cameraRightRotated.z + t*cameraUpRotated.z;
		cameraRight = glm::normalize(cameraRight);
		cameraUp = glm::cross(cameraRight, cameraLookDirection);
		if (cameraPivotShiftAmount > EPSILON)
			cameraPos = cameraPivot - cameraPivotShiftAmount*cameraLookDirection;

		viewMat = glm::lookAt(cameraPos, cameraPos + cameraLookDirection, cameraUp);    
		vpMat = projMat * viewMat;

		updateFrustumSidePlanesNormals();
		frustumNearPlane.normal = -cameraLookDirection;
		frustumFarPlane.normal = cameraLookDirection;
		updateFrustumDs();	
	}

	void Renderer::panCamera(glm::vec2 const& panVec) {	
		translateCamera(glm::mat3(glm::cross(cameraLookDirection, cameraUp), cameraUp, cameraLookDirection)*glm::vec3(panVec, 0.0f));
	}

	void Renderer::rotCameraAroundViewportContainedAx(float rotAng, glm::vec2& rotAx) {
		if (abs(rotAx.x) > EPSILON || abs(rotAx.y) > EPSILON)
			rotCamera(rotAng, glm::mat3(cameraRight, cameraUp, cameraLookDirection)*glm::vec3(rotAx, 0.0f));	
	}

	void Renderer::translateCameraInViewDirection(float translation) {
		translateCamera(translation*cameraLookDirection);
	}

	void Renderer::zoom(float factor) {
		if (fov / factor <= M_PI) {
			fov /= factor;
			refreshProjMat();
			updateDueFovOrWinSzChange();
		}						
	}

	void Renderer::setStaticModelInstanceTransform(unsigned int modelIdx, unsigned int instanceIdx, glm::mat4 const& transformat) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mvpMatsBuffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, (instancesBaseIdxsPerModel[modelIdx] + instanceIdx) * sizeof(glm::mat4), sizeof(glm::mat4), &transformat);	
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	void Renderer::changeModelInstanceColorsArr(unsigned int modelIdx, unsigned int instanceIdx, unsigned int meshIdx, unsigned int colorsArrIdx) {		
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedVerticesColorsIdxsBuffer);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, (instancesBaseIdxsPerModel[modelIdx] + instanceIdx) * sizeof(unsigned int), sizeof(unsigned int), &(verticesColorsBaseIdxs[modelIdx][meshIdx][colorsArrIdx]));	
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	Renderer::InstanceAnimationInterface* Renderer::activateAnimation(unsigned int modelIdx, unsigned int instanceIdx) {	
		InstanceAnimator* instanceAnimator = modelsAnimators[modelIdx]->acquireInstance(instanceIdx);
		instancesAnimators[modelIdx][instanceIdx] = instanceAnimator;
		return new InstanceAnimationInterface(instanceAnimator, modelIdx, instanceIdx);
	}

	void Renderer::deactivateAnimation(InstanceAnimationInterface* instanceAnimatorAPI) {
		instancesAnimators[instanceAnimatorAPI->modelIdx][instanceAnimatorAPI->instanceIdx] = NULL;
		modelsAnimators[instanceAnimatorAPI->modelIdx]->releaseInstance(instanceAnimatorAPI->instanceAnimator);
		delete instanceAnimatorAPI;
	}

	bool Renderer::render(double lag) {	
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		glUseProgram(debugProg);
		glBindVertexArray(debugVAO);
		glBindBuffer(GL_ARRAY_BUFFER, debugVertexBuffer);	
		CHECK_GL_ERROR("glBindBuffer");	
		glUniformMatrix4fv(debugVpMatUniformLoc, 1, GL_FALSE, (float*)&(vpMat));
		CHECK_GL_ERROR("glUniformMatrix4fv");
		glm::vec3 debugBuffer3D[100];
		glm::vec2 debugBuffer2D[100];
		visibleNodesIt3D->init(bvh->getStaticNodes3DRoot());
		while (!visibleNodesIt3D->isDone()) {
			// register leaf's model instance
			BVH::DataNode3D* node = static_cast<BVH::DataNode3D*>(visibleNodesIt3D->getNext());
			unsigned int modelIdx = node->getModelIdx();
			unsigned int instanceIdx = node->getInstanceIdx();		
			InstanceAnimator* instanceAnimator = instancesAnimators[modelIdx][instanceIdx];
			if (instanceAnimator)
				instanceAnimator->updateRendererBuffer();
			visibleStaticModelsInstancesIdxs[modelIdx][visibleStaticModelsInstancesNrs[modelIdx]++] = instanceIdx;
		
			AABB3D const& aabb = node->getAABB();
			glm::vec3 cubeMin = aabb.getMinVertex();
			glm::vec3 cubeMax = aabb.getMaxVertex();
			debugBuffer3D[0] = cubeMin;							  debugBuffer3D[1] = { cubeMax.x, cubeMin.y, cubeMin.z };
			debugBuffer3D[2] = { cubeMax.x, cubeMax.y, cubeMin.z }; debugBuffer3D[3] = { cubeMin.x, cubeMax.y, cubeMin.z };
			debugBuffer3D[4] = { cubeMin.x, cubeMin.y, cubeMax.z }; debugBuffer3D[5] = { cubeMax.x, cubeMin.y, cubeMax.z };
			debugBuffer3D[6] = cubeMax;							  debugBuffer3D[7] = { cubeMin.x, cubeMax.y, cubeMax.z };
			glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(glm::vec3), (void*)debugBuffer3D);
			CHECK_GL_ERROR("glBufferSubData");
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugIndicesBuffer);
			CHECK_GL_ERROR("glBindBuffer");
			glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (void*)0);
			CHECK_GL_ERROR("glDrawElements");
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			if (node->collisionData3D && node->collisionData3D->contactManifold.pointsNr > 0) {
				CollisionVolume::ContactManifold& contactManifold = node->collisionData3D->contactManifold;
				if (contactManifold.pointsNr == 1) {
					debugBuffer3D[0] = contactManifold.points[0];
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_POINTS, 0, 1);
					CHECK_GL_ERROR("glDrawArrays");
				}
				else {
					//unsigned int pointsNr = min(contactManifold.pointsNr, 4u);
					unsigned int pointsNr = contactManifold.pointsNr;
					for (unsigned int pIdx = 0; pIdx < pointsNr - 1; pIdx++) {
						debugBuffer3D[2 * pIdx] = contactManifold.points[pIdx];
						debugBuffer3D[2 * pIdx + 1] = contactManifold.points[pIdx + 1];
					}
					debugBuffer3D[2 * pointsNr - 2] = debugBuffer3D[2 * pointsNr - 3];
					debugBuffer3D[2 * pointsNr - 1] = debugBuffer3D[0];
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * pointsNr * sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2 * contactManifold.pointsNr);
					CHECK_GL_ERROR("glDrawArrays");

					vec3 centroid;
					for (unsigned int pIdx = 0; pIdx < pointsNr; pIdx++)
						centroid += contactManifold.points[pIdx];
					centroid /= pointsNr;
					debugBuffer3D[0] = centroid;
					debugBuffer3D[1] = centroid + contactManifold.normal;
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2);
					CHECK_GL_ERROR("glDrawArrays");
				}
			}
			if (node->collisionData2D && node->collisionData2D->contactManifold.pointsNr > 0) {
				CollisionPerimeter::ContactManifold& contactManifold = node->collisionData2D->contactManifold;
				if (contactManifold.pointsNr == 1) {
					debugBuffer2D[0] = contactManifold.points[0];
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2), (void*)debugBuffer2D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_POINTS, 0, 1);
					CHECK_GL_ERROR("glDrawArrays");
				}
				else {
					//unsigned int pointsNr = min(contactManifold.pointsNr, 4u);
					unsigned int pointsNr = contactManifold.pointsNr;
					for (unsigned int pIdx = 0; pIdx < pointsNr - 1; pIdx++) {
						debugBuffer2D[2 * pIdx] = contactManifold.points[pIdx];
						debugBuffer2D[2 * pIdx + 1] = contactManifold.points[pIdx + 1];
					}
					debugBuffer2D[2 * pointsNr - 2] = debugBuffer2D[2 * pointsNr - 3];
					debugBuffer2D[2 * pointsNr - 1] = debugBuffer2D[0];
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * pointsNr * sizeof(glm::vec2), (void*)debugBuffer2D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2 * contactManifold.pointsNr);
					CHECK_GL_ERROR("glDrawArrays");

					vec2 centroid;
					for (unsigned int pIdx = 0; pIdx < pointsNr; pIdx++)
						centroid += contactManifold.points[pIdx];
					centroid /= pointsNr;
					debugBuffer2D[0] = centroid;
					debugBuffer2D[1] = centroid + contactManifold.normal;
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec2), (void*)debugBuffer2D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2);
					CHECK_GL_ERROR("glDrawArrays");
				}
			}
		}		

		std::string visibles("");
		visibleNodesIt3D->init(bvh->getMobileNodes3DRoot());
		while (!visibleNodesIt3D->isDone()) {
			// register leaf's model instance
			BVH::MobileGameLmntDataNode3D* node = static_cast<BVH::MobileGameLmntDataNode3D*>(visibleNodesIt3D->getNext());
			visibles += std::string("(") + std::string(std::to_string(node->getModelIdx())) + std::string(", ") + std::string(std::to_string(node->getInstanceIdx())) + std::string(")");		
			unsigned int mobileModelIdx = node->getModelIdx() - staticModelsNr;		
			InstanceAnimator* instanceAnimator = instancesAnimators[node->getModelIdx()][node->getInstanceIdx()];
			if (instanceAnimator) {
				instanceAnimator->updateRendererBuffer();
				CHECK_GL_ERROR("updateRendererBuffer");
			}
			visibleMobileModelsInstancesIdxs[mobileModelIdx][visibleMobileModelsInstancesNrs[mobileModelIdx]] = node->getInstanceIdx();	
			mobileModelsTransformatsBuffers[mobileModelIdx][visibleMobileModelsInstancesNrs[mobileModelIdx]] = vpMat * node->getMobilityInterface().getTransformat();
			visibleMobileModelsInstancesNrs[mobileModelIdx]++;
		
			const AABB3D aabb = node->getAABB();
			glm::vec3 cubeMin = aabb.getMinVertex();
			glm::vec3 cubeMax = aabb.getMaxVertex();
			debugBuffer3D[0] = cubeMin;							   debugBuffer3D[1] = { cubeMax.x, cubeMin.y, cubeMin.z };
			debugBuffer3D[2] = { cubeMax.x, cubeMax.y, cubeMin.z }; debugBuffer3D[3] = { cubeMin.x, cubeMax.y, cubeMin.z };
			debugBuffer3D[4] = { cubeMin.x, cubeMin.y, cubeMax.z }; debugBuffer3D[5] = { cubeMax.x, cubeMin.y, cubeMax.z };
			debugBuffer3D[6] = cubeMax;							   debugBuffer3D[7] = { cubeMin.x, cubeMax.y, cubeMax.z };
			glBufferSubData(GL_ARRAY_BUFFER, 0, 8 * sizeof(glm::vec3), (void*)debugBuffer3D);
			CHECK_GL_ERROR("glBufferSubData");
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugIndicesBuffer);
			CHECK_GL_ERROR("glBindBuffer");
			glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (void*)0);
			CHECK_GL_ERROR("glDrawElements");
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			CHECK_GL_ERROR("glBindBuffer");
			if (node->collisionData3D && node->collisionData3D->contactManifold.pointsNr > 0) {
				CollisionVolume::ContactManifold& contactManifold = node->collisionData3D->contactManifold;
				if (contactManifold.pointsNr == 1) {				
					debugBuffer3D[0] = contactManifold.points[0];
					//debugBuffer3D[1] = contactManifold.points[1];
					//debugBuffer3D[2] = contactManifold.points[2];
					//debugBuffer3D[3] = contactManifold.points[3];
					//debugBuffer3D[4] = contactManifold.points[4];
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_POINTS, 0, 1);
					CHECK_GL_ERROR("glDrawArrays");
					debugBuffer3D[1] = debugBuffer3D[0] + contactManifold.normal;
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2*sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2);
					CHECK_GL_ERROR("glDrawArrays");
				}
				else if (contactManifold.pointsNr > 1) {
					//unsigned int pointsNr = min(contactManifold.pointsNr, 4u);
					unsigned int pointsNr = contactManifold.pointsNr;
					for (unsigned int pIdx = 0; pIdx < pointsNr - 1; pIdx++) {
						debugBuffer3D[2 * pIdx] = contactManifold.points[pIdx];
						debugBuffer3D[2 * pIdx + 1] = contactManifold.points[pIdx + 1];
					}
					debugBuffer3D[2 * pointsNr - 2] = debugBuffer3D[2 * pointsNr - 3];
					debugBuffer3D[2 * pointsNr - 1] = debugBuffer3D[0];
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * pointsNr * sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2 * pointsNr);
					CHECK_GL_ERROR("glDrawArrays");

					vec3 centroid;
					for (unsigned int pIdx = 0; pIdx < pointsNr; pIdx++)
						centroid += contactManifold.points[pIdx];
					centroid /= pointsNr;
					debugBuffer3D[0] = centroid;
					debugBuffer3D[1] = centroid + contactManifold.normal;
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2);
					CHECK_GL_ERROR("glDrawArrays");
				}
			}		
		}
		visiblesDisplay.setTxt(visibles);

		glBindBuffer(GL_ARRAY_BUFFER, debugVertexBuffer);
		nodesIt2D->init(bvh->getMobileNodes2DRoot());
		while (!nodesIt2D->isDone()) {		
			BVH::MobileGameLmntDataNode2D* node = static_cast<BVH::MobileGameLmntDataNode2D*>(nodesIt2D->getNext());		
			if (node->collisionData2D && node->collisionData2D->contactManifold.pointsNr > 0) {
				CollisionPerimeter::ContactManifold& contactManifold = node->collisionData2D->contactManifold;
				if (contactManifold.pointsNr == 1) {
					debugBuffer3D[0] = { contactManifold.points[0], -7.0f };
					//debugBuffer3D[1] = contactManifold.points[1];
					//debugBuffer3D[2] = contactManifold.points[2];
					//debugBuffer3D[3] = contactManifold.points[3];
					//debugBuffer3D[4] = contactManifold.points[4];
					glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_POINTS, 0, 1);
					CHECK_GL_ERROR("glDrawArrays");
					debugBuffer3D[1] = debugBuffer3D[0] + glm::vec3(contactManifold.normal, 0.0f);
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2);
					CHECK_GL_ERROR("glDrawArrays");
				}
				else if (contactManifold.pointsNr > 1) {
					//unsigned int pointsNr = min(contactManifold.pointsNr, 4u);
					unsigned int pointsNr = contactManifold.pointsNr;
					for (unsigned int pIdx = 0; pIdx < pointsNr - 1; pIdx++) {
						debugBuffer3D[2 * pIdx] = { contactManifold.points[pIdx].x, contactManifold.points[pIdx].y, -7.0f };
						debugBuffer3D[2 * pIdx + 1] = { contactManifold.points[pIdx + 1].x, contactManifold.points[pIdx + 1].y, -7.0f };
					}
					debugBuffer3D[2 * pointsNr - 2] = debugBuffer3D[2 * pointsNr - 3];
					debugBuffer3D[2 * pointsNr - 1] = debugBuffer3D[0];
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * pointsNr * sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2 * pointsNr);
					CHECK_GL_ERROR("glDrawArrays");

					vec3 centroid;
					for (unsigned int pIdx = 0; pIdx < pointsNr; pIdx++)
						centroid += glm::vec3(contactManifold.points[pIdx], -7.0f);
					centroid /= pointsNr;
					debugBuffer3D[0] = centroid;
					debugBuffer3D[1] = centroid + glm::vec3(contactManifold.normal, 0.0f);
					glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec3), (void*)debugBuffer3D);
					CHECK_GL_ERROR("glBufferSubData");
					glDrawArrays(GL_LINES, 0, 2);
					CHECK_GL_ERROR("glDrawArrays");
				}
			}
		}
			
		unsigned int processedVerticesNr = 0;
		unsigned int processedIndicesNr = 0;
		for (unsigned int modelIdx = 0; modelIdx < staticModelsNr; modelIdx++) {		
			glBindVertexArray(vaos[modelDescsBuffer[modelIdx].progIdx]);
			glUseProgram(progs[modelDescsBuffer[modelIdx].progIdx]);				

			unsigned int visibleInstancesNr = visibleStaticModelsInstancesNrs[modelIdx];
			visibleStaticModelsInstancesNrs[modelIdx] = 0;
			if (visibleInstancesNr) {											
				glBindBuffer(GL_ARRAY_BUFFER, instanceDataIdxsBuffer);
				unsigned int* instanceDataIdxsBufferPtr = (unsigned int*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(unsigned int) * visibleInstancesNr, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
				CHECK_GL_ERROR("glMapBufferRange");				
				for (unsigned int visibleInstanceIdxIdx = 0; visibleInstanceIdxIdx < visibleInstancesNr; visibleInstanceIdxIdx++)
					instanceDataIdxsBufferPtr[visibleInstanceIdxIdx] = visibleStaticModelsInstancesIdxs[modelIdx][visibleInstanceIdxIdx] + instancesBaseIdxsPerModel[modelIdx];
				glUnmapBuffer(GL_ARRAY_BUFFER);
				
				// REMINDER: doesnt work in opengl es 3
				for (unsigned int meshIdx = 0; meshIdx < modelDescsBuffer[modelIdx].meshesNr; meshIdx++) {
					//glUniformMatrix4fv(meshTransformUniformLoc, 1, GL_FALSE, (float*)&(meshesTransformsBuffer[meshIdx]));					
					CHECK_GL_ERROR("glUniformMatrix4fv");
					glDrawElementsInstancedBaseVertex(GL_TRIANGLES, 3 * modelDescsBuffer[modelIdx].facesNrsPerMesh[meshIdx], GL_UNSIGNED_INT,
						(GLvoid*)(processedIndicesNr * sizeof(unsigned int)), visibleInstancesNr, processedVerticesNr);
					CHECK_GL_ERROR("glDrawElementsInstancedBaseVertex");
					processedVerticesNr += modelDescsBuffer[modelIdx].verticesNrsPerMesh[meshIdx];
					processedIndicesNr += 3 * modelDescsBuffer[modelIdx].facesNrsPerMesh[meshIdx];
				}
			}
			else {
				processedVerticesNr += modelDescsBuffer[modelIdx].verticesNr;
				processedIndicesNr += 3 * modelDescsBuffer[modelIdx].facesNr;
			}	
		}
	
		unsigned int modelIdxOverall = staticModelsNr;
		for (unsigned int mobileModelIdx = 0; mobileModelIdx < mobileModelsNr; mobileModelIdx++) {		
			glBindVertexArray(vaos[modelDescsBuffer[modelIdxOverall].progIdx]);
			glUseProgram(progs[modelDescsBuffer[modelIdxOverall].progIdx]);		
		
			unsigned int visibleInstancesNr = visibleMobileModelsInstancesNrs[mobileModelIdx];
			visibleMobileModelsInstancesNrs[mobileModelIdx] = 0;
			if (visibleInstancesNr) {							
				glBindBuffer(GL_ARRAY_BUFFER, instanceDataIdxsBuffer);
				unsigned int* instanceDataIdxsBufferPtr = (unsigned int*)glMapBufferRange(GL_ARRAY_BUFFER, 0, sizeof(unsigned int) * visibleInstancesNr, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);
				CHECK_GL_ERROR("glMapBufferRange");
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, mvpMatsBuffer);
				glm::mat4* mvpMatsBufferPtr = (glm::mat4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::mat4) * modelsInstancesNrsMaxima[modelIdxOverall], GL_MAP_WRITE_BIT);
				CHECK_GL_ERROR("glMapBufferRange");
				for (unsigned int visibleInstanceIdxIdx = 0; visibleInstanceIdxIdx < visibleInstancesNr; visibleInstanceIdxIdx++) {	
					unsigned int modelInstanceIdx = visibleMobileModelsInstancesIdxs[mobileModelIdx][visibleInstanceIdxIdx];
					instanceDataIdxsBufferPtr[visibleInstanceIdxIdx] = modelInstanceIdx + instancesBaseIdxsPerModel[modelIdxOverall];
					mvpMatsBufferPtr[modelInstanceIdx] = mobileModelsTransformatsBuffers[mobileModelIdx][visibleInstanceIdxIdx];
				}
				glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				CHECK_GL_ERROR("glUnmapBuffer");
				glUnmapBuffer(GL_ARRAY_BUFFER);
				CHECK_GL_ERROR("glUnmapBuffer");
				//glUniformMatrix4fv(blahUniformLoc, 1, GL_FALSE, (float*)mobileModelsTransformatsBuffers[0]);
				//CHECK_GL_ERROR("glUniformMatrix4fv");

				//glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesTransformsBuffers[descIdx]);
				//glm::mat4* bonesTransformsPtr = (glm::mat4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, sizeof(glm::mat4) * instancesBaseIdxsPerDescPerModel[descIdx][modelIdx], sizeof(glm::mat4) * modelsDescsBuffer[descIdx].bonesNrs[modelIdx] * modelsDescsBuffer[descIdx].instancesNrsMaxima[modelIdx], GL_MAP_WRITE_BIT);
				//CHECK_GL_ERROR("glMapBufferRange");
				//for (unsigned int visibleInstanceIdxIdx = 0; visibleInstanceIdxIdx < visibleInstancesNr; visibleInstanceIdxIdx++)
				//	bonesTransformsPtr[instanceDataIdxsBufferPtr[visibleInstanceIdxIdx]] = identity;
				//glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				//CHECK_GL_ERROR("glUnmapBuffer");

				for (unsigned int meshIdx = 0; meshIdx < modelDescsBuffer[modelIdxOverall].meshesNr; meshIdx++) {
					//glUniformMatrix4fv(meshTransformUniformLoc, 1, GL_FALSE, (float*)&(meshesTransformsBuffer[meshIdx]));					
					//CHECK_GL_ERROR("glUniformMatrix4fv");
					//glUniform1ui(baseVertexUniformLoc, processedVerticesNr);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
					glDrawElementsInstancedBaseVertex(GL_TRIANGLES, 3 * modelDescsBuffer[modelIdxOverall].facesNrsPerMesh[meshIdx], GL_UNSIGNED_INT,
						(GLvoid*)(processedIndicesNr * sizeof(unsigned int)), visibleInstancesNr, processedVerticesNr);
					CHECK_GL_ERROR("glDrawElementsInstancedBaseVertex");	
					processedVerticesNr += modelDescsBuffer[modelIdxOverall].verticesNrsPerMesh[meshIdx];
					processedIndicesNr += 3 * modelDescsBuffer[modelIdxOverall].facesNrsPerMesh[meshIdx];
				}
			}
			else {
				processedVerticesNr += modelDescsBuffer[modelIdxOverall].verticesNr;
				processedIndicesNr += 3 * modelDescsBuffer[modelIdxOverall].facesNr;
			}

			modelIdxOverall++;		
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);	
		glBindVertexArray(0);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glUseProgram(0);
			
		if (++framesCount == framesNrForFpsUpdate) {
			double currRenderTime = ServiceLocator::getTimer().getCurrentTime();
			fpsDisplay.setTxt( (unsigned int)(framesNrForFpsUpdate / (currRenderTime - prevRenderTime)) );
			prevRenderTime = currRenderTime;
			framesCount = 0;
		}	
		gui.render();

		return openGlContext->swapBuffers();    
	}

	inline void Renderer::refreshProjMat() {	
		projMat = glm::perspectiveFov(fov, (float)winWidth, (float)winHeight, frustumNear, frustumFar);
		//projMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, frustumNear, frustumFar);
		vpMat = projMat * viewMat;			
	}

	inline void Renderer::updateDueFovOrWinSzChange() {
		// update fov trigs
		fovSin = sinf(fov);
		float QuartFov = fov / 4;
		vertQuartFovSin = sinf(QuartFov);
		vertQuartFovCos = cosf(QuartFov);
		float horizonQuartFov = atanf(vertQuartFovSin / vertQuartFovCos * winWidth / winHeight);
		horizonQuartFovCos = cosf(horizonQuartFov);
		horizonQuartFovSin = sinf(horizonQuartFov);
		updateFrustumSidePlanesNormals();
	}

	inline void Renderer::updateFrustumDs() {
		frustumTopPlane.updateD(cameraPos);
		frustumBotPlane.updateD(cameraPos);
		frustumLeftPlane.updateD(cameraPos);
		frustumRightPlane.updateD(cameraPos);
		frustumNearPlane.updateD(cameraPos + FRUSTUM_NEAR_INIT * cameraLookDirection);
		frustumFarPlane.updateD(cameraPos + FRUSTUM_FAR_INIT * cameraLookDirection);
	}

	void Renderer::updateFrustumSidePlanesNormals() {	
		frustumTopPlane.normal = glm::quat(vertQuartFovCos, vertQuartFovSin * cameraRight) * cameraUp;
		frustumBotPlane.normal = glm::quat(vertQuartFovCos, vertQuartFovSin * -cameraRight) * -cameraUp;
		frustumLeftPlane.normal = glm::quat(horizonQuartFovCos, horizonQuartFovSin * cameraUp) * -cameraRight;
		frustumRightPlane.normal = glm::quat(horizonQuartFovCos, horizonQuartFovSin * -cameraUp) * cameraRight;

		//ServiceLocator::getLogger().logd("renderer", (string("top normal = ") + to_string(frustumTopPlane.normal)).c_str());
		//ServiceLocator::getLogger().logd("renderer", (string("camera up = ") + to_string(cameraUp)).c_str());
		//ServiceLocator::getLogger().logd("renderer", "-------------------------------------------");
	}

	struct VertexData {
		aiVector3D pos;	
		unsigned int bonesIDs[BONES_NR_PER_VERTEX_MAX];
		float bonesWeights[BONES_NR_PER_VERTEX_MAX] = { 0 };
		//aiVector3D normal;
		//aiVector2D uvs;
	};

	inline unsigned int arrDotArr(unsigned int* arr1, unsigned int* arr2, unsigned int arrsLen) {
		unsigned int res = 0;
		for (unsigned int lmntIdx = 0; lmntIdx < arrsLen; lmntIdx++)
			res += arr1[lmntIdx] * arr2[lmntIdx];

		return res;
	}

	inline glm::mat4 assimp2glm(aiMatrix4x4 const& mat) {
		return glm::mat4(mat.a1, mat.b1, mat.c1, mat.d1, mat.a2, mat.b2, mat.c2, mat.d2, mat.a3, mat.b3, mat.c3, mat.d3, mat.a4, mat.b4, mat.c4, mat.d4);
	}

	inline glm::quat assimp2glm(aiQuaternion const& quat) {
		return glm::quat(quat.w, quat.x, quat.y, quat.z);
	}

	inline glm::vec3 assimp2glm(aiVector3D const& vec) {
		return glm::vec3(vec.x, vec.y, vec.z);
	}

	void loadMeshesTransformsArrRecurse(aiScene const* scene, glm::mat4* meshesTransformsArr, aiNode const* node, glm::mat4& parentTransform) {
		glm::mat4 nodeTransform = parentTransform * assimp2glm(node->mTransformation);
		for (unsigned int meshIdx = 0; meshIdx < node->mNumMeshes; meshIdx++)
			meshesTransformsArr[node->mMeshes[meshIdx]] = nodeTransform;

		for (unsigned int childIdx = 0; childIdx < node->mNumChildren; childIdx++)
			loadMeshesTransformsArrRecurse(scene, meshesTransformsArr, node->mChildren[childIdx], nodeTransform);
	}

	void loadMeshesTransformsArr(aiScene const* scene, glm::mat4* meshesTransformsArr) {
		loadMeshesTransformsArrRecurse(scene, meshesTransformsArr, scene->mRootNode, assimp2glm(scene->mRootNode->mTransformation));
	}

	bool Renderer::initOpenGlLmnts() {
		ServiceLocator::getLogger().logd("Renderer", "initializing gl components.");
	#if DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(glErrorCallback, NULL);
	#endif
		glClearColor(BKG_COLOR_R, BKG_COLOR_G, BKG_COLOR_B, 1.0f);
		glEnable(GL_DEPTH_TEST);
		//glFrontFace(GL_CCW);
		//glCullFace(GL_BACK);
		//glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPointSize(5.0f);

		glGenBuffers(1, &instanceDataIdxsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &vertexBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &mvpMatsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &bonesTransformsBaseIdxsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &bonesTransformsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &selectedVerticesColorsIdxsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &verticesColorsBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenBuffers(1, &indicesBuffer);
		CHECK_GL_ERROR("glGenBuffers");
		glGenVertexArrays(shadersNr, vaos);
		CHECK_GL_ERROR("glGenVertexArrays");

		for (unsigned int shaderIdx = 0; shaderIdx < shadersNr; shaderIdx++) {
			std::string vertexShaderCode, fragShaderCode;
			readFileToStr(vertexShadersFullPaths[shaderIdx], vertexShaderCode);
			readFileToStr(fragShadersFullPaths[shaderIdx], fragShaderCode);
			if (!createGlProg(vertexShaderCode.c_str(), fragShaderCode.c_str(), &progs[shaderIdx], &vertexShaders[shaderIdx], &fragShaders[shaderIdx]))
				return false;	

			GLuint instanceDataIdxAttribLoc = glGetAttribLocation(progs[shaderIdx], INSTANCE_DATA_IDX_ATTRIB_NAME);
			GLuint vertexPosAttribLoc = glGetAttribLocation(progs[shaderIdx], VERTEX_POS_ATTRIB_NAME);
			//blahUniformLoc = glGetUniformLocation(progs[shaderIdx], "uVpMat");
			//GLuint vertexBonesIdxsAttribLoc = glGetAttribLocation(progs[descIdx], VERTEX_BONES_IDS_ATTRIB_NAME);
			//GLuint vertexBonesWeightsAttribLoc = glGetAttribLocation(progs[descIdx], VERTEX_BONES_WEIGHTS_ATTRIB_NAME);	
			CHECK_GL_ERROR("glGetAttribLocation");
			//meshTransformUniformLoc = glGetUniformLocation(progs[shaderIdx], "uMeshTransform");
			//CHECK_GL_ERROR("glGetUniformLocation");
			//***baseVertexUniformLoc = glGetUniformLocation(progs[shaderIdx], "uBaseVertex");
			//***CHECK_GL_ERROR("glGetUniformLocation");

			glBindVertexArray(vaos[shaderIdx]);
			CHECK_GL_ERROR("glBindVertexArray");
			glBindBuffer(GL_ARRAY_BUFFER, instanceDataIdxsBuffer);
			glVertexAttribIPointer(instanceDataIdxAttribLoc, 1, GL_UNSIGNED_INT, 0, (void*)0);
			CHECK_GL_ERROR("glVertexAttribPointer");
			glEnableVertexAttribArray(instanceDataIdxAttribLoc);
			CHECK_GL_ERROR("glEnableVertexAttribArray");
			glVertexAttribDivisor(instanceDataIdxAttribLoc, 1);
			CHECK_GL_ERROR("glVertexAttribDivisor");

			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glVertexAttribPointer(vertexPosAttribLoc, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
			//glVertexAttribPointer(vertexPosAttribLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			CHECK_GL_ERROR("glVertexAttribPointer");
			glEnableVertexAttribArray(vertexPosAttribLoc);
			CHECK_GL_ERROR("glEnableVertexAttribArray");
			//glVertexAttribIPointer(vertexBonesIdxsAttribLoc, BONES_NR_PER_VERTEX_MAX, GL_UNSIGNED_INT, sizeof(VertexData), (void*)sizeof(aiVector3D));
			//CHECK_GL_ERROR("glVertexAttribPointer");
			//glEnableVertexAttribArray(vertexBonesIdxsAttribLoc);
			//CHECK_GL_ERROR("glEnableVertexAttribArray");
			//glVertexAttribPointer(vertexBonesWeightsAttribLoc, BONES_NR_PER_VERTEX_MAX, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(sizeof(aiVector3D) + BONES_NR_PER_VERTEX_MAX*sizeof(unsigned int)));
			//CHECK_GL_ERROR("glVertexAttribPointer");
			//glEnableVertexAttribArray(vertexBonesWeightsAttribLoc);
			//CHECK_GL_ERROR("glEnableVertexAttribArray");		

			glBindBuffer(GL_SHADER_STORAGE_BUFFER, mvpMatsBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mvpMatsBuffer);
			CHECK_GL_ERROR("glBindBufferBase");
			//glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesTransformsBaseIdxsBuffers[descIdx]);
			//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bonesTransformsBaseIdxsBuffers[descIdx]);
			//CHECK_GL_ERROR("glBindBufferBase");
			//glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesTransformsBuffers[descIdx]);
			//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, bonesTransformsBuffers[descIdx]);
			//CHECK_GL_ERROR("glBindBufferBase");
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedVerticesColorsIdxsBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, selectedVerticesColorsIdxsBuffer);
			CHECK_GL_ERROR("glBindBufferBase");
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesColorsBuffer);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, verticesColorsBuffer);
			CHECK_GL_ERROR("glBindBufferBase");

			/*
			mvpMatAttribLocs[descIdx] = glGetAttribLocation(progs[descIdx], MVP_MAT_ATTRIB_NAME);
			glBindBuffer(GL_ARRAY_BUFFER, mvpMatsBuffer);
			CHECK_GL_ERROR("glBindBuffer");
			for (unsigned int matColIdx = 0; matColIdx < 4; matColIdx++) {
				glVertexAttribPointer(mvpMatAttribLocs[descIdx] + matColIdx, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)*matColIdx) );
				CHECK_GL_ERROR("glVertexAttribPointer");
				glEnableVertexAttribArray(mvpMatAttribLocs[descIdx] + matColIdx);
				CHECK_GL_ERROR("glEnableVertexAttribArray");
				glVertexAttribDivisor(mvpMatAttribLocs[descIdx] + matColIdx, 1);
				CHECK_GL_ERROR("glVertexAttribDivisor");
			}
			*/
			//glGenBuffers(1, &blahBuffer);
			//glBindBuffer(GL_ARRAY_BUFFER, blahBuffer);
			//glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
		}

		glGenVertexArrays(1, &debugVAO);
		glGenBuffers(1, &debugVertexBuffer);
		glGenBuffers(1, &debugVpMatBuffer);
		glGenBuffers(1, &debugIndicesBuffer);

		unsigned int cubeVertexIndices[24] = { 0, 1, 0, 3, 1, 2, 2, 3,
											   4, 5, 4, 7, 5, 6, 6, 7,
											   0, 4, 1, 5, 2, 6, 3, 7 };
		glBindBuffer(GL_ARRAY_BUFFER, debugVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::vec3), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_ARRAY_BUFFER, debugVpMatBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugIndicesBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 24 * sizeof(unsigned int), cubeVertexIndices, GL_STATIC_DRAW);
		CHECK_GL_ERROR("glBufferData");

		if (!createGlProg(debugVertexShaderCode, debugFragShaderCode, &debugProg, &debugVertexShader, &debugFragShader))
			return false;

		glBindVertexArray(debugVAO);
		GLuint vertexAttribLoc = glGetAttribLocation(debugProg, "aPos");
		glBindBuffer(GL_ARRAY_BUFFER, debugVertexBuffer);
		glVertexAttribPointer(vertexAttribLoc, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(vertexAttribLoc);
		debugVpMatUniformLoc = glGetUniformLocation(debugProg, "uVpMat");
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	bool Renderer::loadOpenGlBuffers() {			
		glBindBuffer(GL_ARRAY_BUFFER, instanceDataIdxsBuffer);
		glBufferData(GL_ARRAY_BUFFER, instancesNrsMaximaMax * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, verticesNrTotal * sizeof(VertexData), NULL, GL_STATIC_DRAW);
		CHECK_GL_ERROR("glBufferData");			
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, mvpMatsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, instancesNrsMaximaMax * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesTransformsBaseIdxsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, instancesNrsMaximaMax * sizeof(unsigned int), NULL, GL_STATIC_DRAW);
		CHECK_GL_ERROR("glBufferData");		
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesTransformsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, bonesInstancedNrMax * sizeof(glm::mat4), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedVerticesColorsIdxsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, instancesNrMax * sizeof(unsigned int), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesColorsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, verticesColorsNrTotal * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		CHECK_GL_ERROR("glBufferData");
		glBindBuffer(GL_ARRAY_BUFFER, indicesBuffer);
		glBufferData(GL_ARRAY_BUFFER, facesNrTotal * 3 * sizeof(unsigned int), NULL, GL_STATIC_DRAW);
		CHECK_GL_ERROR("glBufferData");

		// load colladas to the buffers
		unsigned int processedVerticesNr = 0;
		unsigned int processedInstancedBonesNr = 0;
		unsigned int processedInstancesNr = 0;
		unsigned int processedVerticesColorsNr = 0;
		unsigned int processedIndicesNr = 0;			
		for (unsigned int modelIdx = 0; modelIdx < modelsNrTotal; modelIdx++) {
			Assimp::Importer* importer = new Assimp::Importer();
			// reminder : the importer object keeps ownership of the scene data and will destroy it upon destruction
			const aiScene *scene = importer->ReadFile(modelDescsBuffer[modelIdx].colladaPath, aiProcessPreset_TargetRealtime_Quality | aiProcess_JoinIdenticalVertices);
			if (!scene) {
				std::string errorString = importer->GetErrorString();
				ServiceLocator::getLogger().loge("loadOpenGlBuffers", "Scene import failed: %s", errorString.c_str());
				return false;
			}
			else
				ServiceLocator::getLogger().logd("Renderer", "Scene import succeeded.");

			if (scene->HasMeshes()) {
				// TODO: remove meshesTransformsBuffer from class scope and leave it in this scope.
				//		 these two will be relevant for unanimated scenes only.				
				meshesTransformsBuffer = new glm::mat4[scene->mNumMeshes]();		
				loadMeshesTransformsArr(scene, meshesTransformsBuffer);
				unsigned int processedModelBonesNr = 0;
				for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
					const aiMesh *mesh = scene->mMeshes[meshIdx];
					if (!mesh->HasPositions() || !mesh->HasFaces()) {
						std::string lmntIdxsPrefix(std::string("model #") + std::to_string(modelIdx) + std::string(": Mesh #") + std::to_string(meshIdx));
						if (!mesh->HasPositions())
							ServiceLocator::getLogger().loge("initOpenGlLmnts", (lmntIdxsPrefix + std::string(" has no positions !")).c_str());
						else if (!mesh->HasFaces())
							ServiceLocator::getLogger().loge("initOpenGlLmnts", (lmntIdxsPrefix + std::string(" has no faces !")).c_str());

						delete importer;
						return false;
					}

					// load vertex data				
					VertexData* verticesData = new VertexData[mesh->mNumVertices];
					if (scene->HasAnimations()) {
						for (unsigned int vertexIdx = 0; vertexIdx < mesh->mNumVertices; vertexIdx++) {
							verticesData[vertexIdx].pos = mesh->mVertices[vertexIdx];
							//verticesData[vertexIdx].normal = sceneFirstMesh->mNormals[vertexIdx];
							//verticesData[vertexIdx].uvs = sceneFirstMesh->mTextureCoords[0][vertexIdx];			
						}
					}
					else {
						//TODO: update the vertices data with the mesh transform
						for (unsigned int vertexIdx = 0; vertexIdx < mesh->mNumVertices; vertexIdx++) {
							verticesData[vertexIdx].pos = mesh->mVertices[vertexIdx];
							//verticesData[vertexIdx].normal = sceneFirstMesh->mNormals[vertexIdx];
							//verticesData[vertexIdx].uvs = sceneFirstMesh->mTextureCoords[0][vertexIdx];			
						}
					}
																						
					if (mesh->mNumBones) {
						unsigned int* verticesProcessedWeightsNrs = new unsigned int[mesh->mNumVertices]();
						for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; boneIdx++) {
							aiBone* bone = mesh->mBones[boneIdx];
							for (unsigned int vertexIdx = 0; vertexIdx < bone->mNumWeights; vertexIdx++) {
								unsigned int vertexID = bone->mWeights[vertexIdx].mVertexId;
								float vertexWeight = bone->mWeights[vertexIdx].mWeight;
								unsigned int vertexProcessedWeightsNr = verticesProcessedWeightsNrs[vertexID];
								verticesData[vertexID].bonesIDs[vertexProcessedWeightsNr] = processedModelBonesNr;
								verticesData[vertexID].bonesWeights[vertexProcessedWeightsNr] = vertexWeight;
								verticesProcessedWeightsNrs[vertexID]++;
							}
							processedModelBonesNr++;
						}
						delete[] verticesProcessedWeightsNrs;
					}
					glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
					CHECK_GL_ERROR("glBindBuffer");
					glBufferSubData(GL_ARRAY_BUFFER, sizeof(VertexData) * processedVerticesNr,
						sizeof(VertexData) * mesh->mNumVertices, verticesData);
					CHECK_GL_ERROR("glBufferSubData");
					delete[] verticesData;
					processedVerticesNr += mesh->mNumVertices;

					// load colors
					/* possibly iterate over indices 0 through AI_MAX_NUMBER_OF_COLOR_SETS
					unsigned int verticesNr = sceneFirstMesh->mNumVertices;
					float *vertexColorsArr = new float[verticesNr * 4];
					for (int vertexIdx = 0; vertexIdx < verticesNr; vertexIdx++) {
					vertexColorsArr[vertexIdx*4] = sceneFirstMesh->mColors[0][vertexIdx].r;
					vertexColorsArr[vertexIdx*4 + 1] = sceneFirstMesh->mColors[0][vertexIdx].g;
					vertexColorsArr[vertexIdx*4 + 2] = sceneFirstMesh->mColors[0][vertexIdx].b;
					vertexColorsArr[vertexIdx*4 + 3] = sceneFirstMesh->mColors[0][vertexIdx].a;
					} */
					
					// load faces
					unsigned int facesNr = mesh->mNumFaces;
					unsigned int *vertexIndicesArr = new unsigned int[facesNr * 3];
					unsigned int vertexIdxIdx = 0;
					for (unsigned int faceIdx = 0; faceIdx < facesNr; faceIdx++) {
						// read a face from assimp's mesh and copy it into faceArr
						const aiFace *face = &mesh->mFaces[faceIdx];
						memcpy(&vertexIndicesArr[vertexIdxIdx], face->mIndices, 3 * sizeof(unsigned int));
						vertexIdxIdx += 3;
					}

					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer);
					CHECK_GL_ERROR("glBindBuffer");
					glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * processedIndicesNr,
						sizeof(unsigned int) * facesNr * 3, vertexIndicesArr);
					CHECK_GL_ERROR("glBufferSubData");
					delete[] vertexIndicesArr;
					processedIndicesNr += facesNr * 3;

					unsigned int* initVerticesColorsIdx = new unsigned int[modelsInstancesNrsMaxima[modelIdx]];
					for (unsigned int instanceIdx = 0; instanceIdx < modelsInstancesNrsMaxima[modelIdx]; instanceIdx++)
						initVerticesColorsIdx[instanceIdx] = processedVerticesColorsNr;
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, selectedVerticesColorsIdxsBuffer);
					CHECK_GL_ERROR("glBindBuffer");
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * processedInstancesNr,
						sizeof(unsigned int) * modelsInstancesNrsMaxima[modelIdx], initVerticesColorsIdx);
					CHECK_GL_ERROR("glBufferSubData");
					delete[] initVerticesColorsIdx;

					glBindBuffer(GL_SHADER_STORAGE_BUFFER, verticesColorsBuffer);
					CHECK_GL_ERROR("glBindBuffer");
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 4 * processedVerticesColorsNr,
						sizeof(float) * 4 * mesh->mNumVertices, mesh->mColors[0]);
					CHECK_GL_ERROR("glBufferSubData");
					for (unsigned int colorsArrIdx = 0; colorsArrIdx < modelDescsBuffer[modelIdx].extraColorsNrsPerMesh[meshIdx]; colorsArrIdx++) {
						glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 4 * (processedVerticesColorsNr + (colorsArrIdx + 1)*mesh->mNumVertices),
							sizeof(float) * 4 * mesh->mNumVertices, modelDescsBuffer[modelIdx].extraColors[meshIdx][colorsArrIdx]);
						CHECK_GL_ERROR("glBufferSubData");
					}
					for (unsigned int verticesColorIdx = 0; verticesColorIdx < modelDescsBuffer[modelIdx].extraColorsNrsPerMesh[meshIdx] + 1; verticesColorIdx++)
						verticesColorsBaseIdxs[modelIdx][meshIdx][verticesColorIdx] = processedVerticesColorsNr + verticesColorIdx*mesh->mNumVertices;
					processedVerticesColorsNr += (modelDescsBuffer[modelIdx].extraColorsNrsPerMesh[meshIdx] + 1) * mesh->mNumVertices;

					// buffer for vertex texture coordinates
					// ***ASSUMPTION*** -- handle only one texture for each mesh
					/*if (mesh->HasTextureCoords(0)) {
					float *texCoords = new float[2 * mesh->mNumVertices];
					for (unsigned int texCoordsIdx = 0; texCoordsIdx < mesh->mNumVertices; ++texCoordsIdx) {
					texCoords[texCoordsIdx * 2] = mesh->mTextureCoords[0][texCoordsIdx].x;
					texCoords[texCoordsIdx * 2 + 1] = mesh->mTextureCoords[0][texCoordsIdx].y;
					}
					glGenBuffers(1, &buffer);
					glBindBuffer(GL_ARRAY_BUFFER, buffer);
					glBufferData(GL_ARRAY_BUFFER,
					sizeof(float) * 2 * mesh->mNumVertices, texCoords,
					GL_STATIC_DRAW);
					meshes[meshIdx].textureCoordBuffer = buffer;
					delete[] texCoords;

					// unbind buffers
					glBindBuffer(GL_ARRAY_BUFFER, 0);
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

					// copy texture index (= texture name in GL) for the mesh from textureNameMap
					aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
					aiString texPath;    //contains filename of texture
					if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath)) {
					unsigned int textureId = textureNameMap[texturePath.data];
					newMeshInfo.textureIndex = textureId;
					} else {
					newMeshInfo.textureIndex = 0;
					}
					}*/
					glBindBuffer(GL_ARRAY_BUFFER, 0);					
					glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				}
								
				if (scene->HasAnimations())
					modelsAnimators[modelIdx] = new ModelAnimator(scene, bonesTransformsBuffer, processedInstancedBonesNr, modelDescsBuffer[modelIdx].bonesNr, meshesTransformsBuffer, modelsInstancesNrsMaxima[modelIdx]);
					
				if (unsigned int modelBonesNr = modelDescsBuffer[modelIdx].bonesNr) {
					unsigned int* bonesTransformsBaseIdxs = new unsigned int[modelsInstancesNrsMaxima[modelIdx]]();
					for (unsigned int instanceIdx = 0; instanceIdx < modelsInstancesNrsMaxima[modelIdx]; instanceIdx++)
						bonesTransformsBaseIdxs[instanceIdx] = processedInstancedBonesNr + modelBonesNr*instanceIdx;
					glBindBuffer(GL_SHADER_STORAGE_BUFFER, bonesTransformsBaseIdxsBuffer);
					CHECK_GL_ERROR("glBindBuffer");
					glBufferSubData(GL_SHADER_STORAGE_BUFFER, processedInstancedBonesNr * sizeof(unsigned int),
						modelsInstancesNrsMaxima[modelIdx] * sizeof(unsigned int), bonesTransformsBaseIdxs);
					CHECK_GL_ERROR("glBufferSubData");
					delete[] bonesTransformsBaseIdxs;

					processedInstancedBonesNr += modelBonesNr * modelsInstancesNrsMaxima[modelIdx];
				}												
			}
			else {
				ServiceLocator::getLogger().loge("initOpenGlLmnts", "The scene has no meshes !");
				delete importer;
				return false;
			}

			instancesBaseIdxsPerModel[modelIdx] = processedInstancesNr;
			processedInstancesNr += modelsInstancesNrsMaxima[modelIdx];

			delete importer;																	
		}
					
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
			
		gui.initOpenGlLmnts(winWidth, winHeight);
		fpsDisplay.setTxt((unsigned int)0);
		prevRenderTime = ServiceLocator::getTimer().getCurrentTime();

		return true;
	}

	unsigned int currFrame = 0;
	inline glm::mat4 calcKeyframeTransformat(aiNodeAnim* node) {
		unsigned int frameIdx;
		if (currFrame < node->mNumPositionKeys)
			frameIdx = currFrame;
		else
			frameIdx = node->mNumPositionKeys - 1;
		glm::mat4 translate = glm::translate(assimp2glm(node->mPositionKeys[frameIdx].mValue));
	
		if (currFrame < node->mNumRotationKeys)
			frameIdx = currFrame;
		else
			frameIdx = node->mNumRotationKeys - 1;
		glm::mat4 rot = glm::mat4_cast(assimp2glm(node->mRotationKeys[frameIdx].mValue));

		if (currFrame < node->mNumScalingKeys)
			frameIdx = currFrame;
		else
			frameIdx = node->mNumScalingKeys - 1;
		glm::mat4 scale = glm::scale(assimp2glm(node->mScalingKeys[frameIdx].mValue));

		return translate * rot * scale;
	}

	void Renderer::destroyOpenGlLmnts() {
		if (isSceneLoaded)		
			needReloadGlBuffers = true;	

		for (unsigned int shaderIdx = 0; shaderIdx < shadersNr; shaderIdx++) {
			glDeleteShader(vertexShaders[shaderIdx]);
			glDeleteShader(fragShaders[shaderIdx]);
			glDeleteProgram(progs[shaderIdx]);
		}

		gui.destroyOpenGlLmnts();
		glDeleteShader(debugVertexShader);
		glDeleteShader(debugFragShader);
		glDeleteProgram(debugProg);
		glDeleteBuffers(1, &debugVertexBuffer);
		glDeleteBuffers(1, &debugVpMatBuffer);
		glDeleteBuffers(1, &debugIndicesBuffer);
		glDeleteBuffers(1, &verticesColorsBuffer);
		glDeleteBuffers(1, &selectedVerticesColorsIdxsBuffer);
		glDeleteBuffers(1, &bonesTransformsBuffer);
		glDeleteBuffers(1, &bonesTransformsBaseIdxsBuffer);
		glDeleteBuffers(1, &mvpMatsBuffer);
		glDeleteBuffers(1, &vertexBuffer);
		glDeleteBuffers(1, &instanceDataIdxsBuffer);
		glDeleteVertexArrays(shadersNr, vaos);
		delete[] meshesTransformsBuffer;
	}

	bool Renderer::isBoundingSphereVisible(BoundingSphere const& boundingSphere) const {
		//return true;
		glm::vec3 const& sphereC = boundingSphere.getCenter();
		float sphereR = boundingSphere.getRadius();
		if (frustumFarPlane.signedDistFromPoint(sphereC) > sphereR)
			return false;

		// cone to sphere test
		glm::vec3 u = cameraPos - boundingSphere.getRadius()*cameraLookDirection/fovSin;
		glm::vec3 d = sphereC - u;	
		float e = dot(cameraLookDirection, d);
		if ( e > 0 && e*e >= length2(d)*cos(fov)) {
			d = sphereC - cameraPos;
			e = -dot(cameraLookDirection, d);
			if (e > 0 && e*e >= length2(d)*sin(fov) && length2(d) > sphereR * sphereR)
				return false;
		}
		else 
			return false;	

		// frustum to sphere test
		return frustumNearPlane.signedDistFromPoint(sphereC) <= sphereR &&
			   frustumLeftPlane.signedDistFromPoint(sphereC) <= sphereR &&
			   frustumRightPlane.signedDistFromPoint(sphereC) <= sphereR &&
			   frustumTopPlane.signedDistFromPoint(sphereC) <= sphereR &&
			   frustumBotPlane.signedDistFromPoint(sphereC) <= sphereR;
	}

	Renderer::InstanceAnimationInterface::InstanceAnimationInterface(InstanceAnimator* _instanceAnimator, unsigned int _modelIdx, unsigned int _instanceIdx) :
		instanceAnimator(_instanceAnimator), modelIdx(_modelIdx), instanceIdx(_instanceIdx) {}

	void Renderer::InstanceAnimationInterface::start(unsigned int animationIdx) {
		instanceAnimator->start(animationIdx);
	}

	#if defined(__ANDROID__) || defined(ANDROID)
	bool Renderer::OpenGlContext::init(Corium3DNativeWindowType _window) {
		EGLDisplay _display;
		EGLConfig _config;
		EGLContext _context;
		EGLSurface _surface;
		EGLint configsNr;
		EGLint format;

		if ((_display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
			ServiceLocator::getLogger().loge("Renderer", "eglGetDisplay() returned error 0x%08x", eglGetError());
			return false;
		}
		EGLint majorVersion, minorVersion;
		if (!eglInitialize(_display, &majorVersion, &minorVersion)) {
			ServiceLocator::getLogger().loge("Renderer", "eglInitialize() returned error 0x%08x", eglGetError());
			return false;
		}

		if (!eglChooseConfig(_display, eglAattribs, &_config, 1, &configsNr)) {
			ServiceLocator::getLogger().loge("Renderer", "eglChooseConfig() returned error 0x%08x", eglGetError());
			destroy();
			return false;
		}

		if (!eglGetConfigAttrib(_display, _config, EGL_NATIVE_VISUAL_ID, &format)) {
			ServiceLocator::getLogger().loge("Renderer", "eglGetConfigAttrib() returned error 0x%08x", eglGetError());
			destroy();
			return false;
		}
		ANativeWindow_setBuffersGeometry(_window, 0, 0, format);

		if ((_context = eglCreateContext(_display, _config, EGL_NO_CONTEXT, contextAttribList)) == EGL_NO_CONTEXT) {
			ServiceLocator::getLogger().loge("Renderer", "eglCreateContext() returned error 0x%08x", eglGetError());
			destroy();
			return false;
		}

		if (surface != EGL_NO_SURFACE) {
			eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); // not testing return value
			eglDestroySurface(display, surface);
		}

		if ((_surface = eglCreateWindowSurface(display, config, window, NULL)) == EGL_NO_SURFACE) {
			ServiceLocator::getLogger().loge("Renderer", "eglCreateWindowSurface() returned error 0x%08x", eglGetError());
			destroy();
			return false;
		}

		if (!eglMakeCurrent(display, _surface, _surface, context)) {
			ServiceLocator::getLogger().loge("Renderer", "eglMakeCurrent() returned error 0x%08x", eglGetError());
			destroy();
			return false;
		}

		display = _display;
		context = _context;
		config = _config;
		window = _window;
		surface = _surface;
		return true;
	}

	void Renderer::OpenGlContext::destroy() {
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		eglDestroySurface(display, surface);
		eglTerminate(display);

		display = EGL_NO_DISPLAY;
		context = EGL_NO_CONTEXT;
		surface = EGL_NO_SURFACE;
	}

	bool Renderer::OpenGlContext::swapBuffers() {
		if (!eglSwapBuffers(display, surface)) {
			ServiceLocator::getLogger().loge("Renderer", "eglSwapBuffers() returned error %d", eglGetError());
			return false;
		else
			return true;
		}

	#elif defined(_WIN32) || defined(__VC32__) || defined(_WIN64) || defined(__VC64__) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
	bool Renderer::OpenGlContext::init(Corium3DEngineNativeWindowType _window) {
		hdc = GetDC(_window);
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = RED_SZ + GREEN_SZ + BLUE_SZ + ALPHA_SZ;
		pfd.cDepthBits = DEPTH_SZ;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int nPixelFormat = ChoosePixelFormat(hdc, &pfd);
		if (nPixelFormat == 0)
			return false;
		if (!SetPixelFormat(hdc, nPixelFormat, &pfd))
			return false;

		HGLRC tempOpenGlContext = wglCreateContext(hdc);
		wglMakeCurrent(hdc, tempOpenGlContext);
		if (glewInit() != GLEW_OK)
			return false;

		if (wglewIsSupported("WGL_ARB_create_context")) {
			hglrc = wglCreateContextAttribsARB(hdc, NULL, contextAttribs);
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(tempOpenGlContext);
			wglMakeCurrent(hdc, hglrc);
			wglSwapIntervalEXT(1);
		}
		else {
			wglDeleteContext(tempOpenGlContext);
			return false;
		}

		int glVersion[2];
		glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]);
		glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]);
		ServiceLocator::getLogger().logd("OpenGlContext", "OpenGL version created: %d.%d", glVersion[0], glVersion[1]);

		return true;
	}

	void Renderer::OpenGlContext::destroy() {
		wglMakeCurrent(hdc, NULL);
		wglDeleteContext(hglrc);
	}

	bool Renderer::OpenGlContext::swapBuffers() {
		SwapBuffers(hdc);
		return true;
	}

	#endif

	struct NodesCount {
		unsigned int nodesNr = 1;
		unsigned int treeDepthMax = 0;
	};

	// REMINDER: accumulating childrens' number onto nodesCount.nodesNr, and not the node itself (that is why nodesNr starts from 1 -> counts the root)
	void genNodesCountRecurse(aiNode* node, NodesCount& nodesCount, unsigned int depth) {
		nodesCount.treeDepthMax = depth > nodesCount.treeDepthMax ? depth : nodesCount.treeDepthMax;
		if (node->mNumChildren) {
			nodesCount.nodesNr += node->mNumChildren;
			for (unsigned int childIdx = 0; childIdx < node->mNumChildren; childIdx++)
				genNodesCountRecurse(node->mChildren[childIdx], nodesCount, depth + 1);
		}
	}

	NodesCount genNodesCount(aiNode* root) {
		NodesCount nodesCount;

		genNodesCountRecurse(root, nodesCount, 0);
		return nodesCount;
	}

	unsigned int mapNodesToChannelsIdxsRecurse(aiNode* node, unsigned int nodeIdx, const aiAnimation* animation, unsigned int* nodesToChannelsIdxsMap) {
		if (!node)
			return 0;

		unsigned int channelIdx = 0;
		while (channelIdx < animation->mNumChannels && animation->mChannels[channelIdx]->mNodeName != node->mName)
			channelIdx++;
		nodesToChannelsIdxsMap[nodeIdx] = channelIdx;
		unsigned int processedSubtreeNodesNr = 0;
		for (unsigned int childIdx = 0; childIdx < node->mNumChildren; childIdx++)
			processedSubtreeNodesNr += mapNodesToChannelsIdxsRecurse(node->mChildren[childIdx], processedSubtreeNodesNr + nodeIdx + 1, animation, nodesToChannelsIdxsMap);

		return processedSubtreeNodesNr + 1;
	}

	void mapNodesToChannelsIdxs(aiNode* root, const aiAnimation* animation, unsigned int* nodesToChannelsIdxsMap) {
		mapNodesToChannelsIdxsRecurse(root, 0, animation, nodesToChannelsIdxsMap);
	}

	Renderer::ModelAnimator::ModelAnimator(aiScene const* scene, GLuint _bonesTransformatsBuffer, unsigned int _bonesTransformatsBufferModelBaseIdx,
										   unsigned int _bonesNr, glm::mat4* _meshesTransformsBuffer, unsigned int instancesNrMax) {
		bonesTransformatsBuffer = _bonesTransformatsBuffer;
		bonesTransformatsBufferModelBaseIdx = _bonesTransformatsBufferModelBaseIdx;
		bonesNr = _bonesNr;
		meshesTransformsBuffer = _meshesTransformsBuffer;
		instanceAnimatorsPool = new ObjPool<InstanceAnimator>(instancesNrMax);	
		// REMINDER: Supposed to be bonesNrOverall (over all meshes)
		bonesOffsets = new glm::mat4[bonesNr]; 
		unsigned int boneIdxOverall = 0;
		for (unsigned meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
			for (unsigned int boneIdx = 0; boneIdx < scene->mMeshes[meshIdx]->mNumBones; boneIdx++) {
				bonesOffsets[boneIdxOverall] = assimp2glm(scene->mMeshes[meshIdx]->mBones[boneIdx]->mOffsetMatrix);			
				boneIdxOverall++;
			}
		}
	
		animationsNr = scene->mNumAnimations;
		// TODO: Calculate the following data statically and move it to an AnimationDesc from which it will be copied here
		animations = new Animation[animationsNr];
		for (unsigned int animationIdx = 0; animationIdx < animationsNr; animationIdx++) {
			aiAnimation const* animation = scene->mAnimations[animationIdx];
			animations[animationIdx].dur = animation->mDuration;
			animations[animationIdx].ticksPerSecond = animation->mTicksPerSecond;
		
			aiNode* nodesIt = scene->mRootNode;
			NodesCount nodesCount = genNodesCount(nodesIt);	
			animations[animationIdx].nodesNr = nodesCount.nodesNr;
			animations[animationIdx].treeDepthMax = nodesCount.treeDepthMax;
			unsigned int* nodesToChannelsIdxsMap = new unsigned int[nodesCount.nodesNr];
			mapNodesToChannelsIdxs(nodesIt, animation, nodesToChannelsIdxsMap);
			animations[animationIdx].TransformatsHierarchyBuffer = new Animation::TransformatsHierarchyNode[nodesCount.nodesNr];		
			unsigned int* childrenIdxsStack = new unsigned int[nodesCount.treeDepthMax]();
			Animation::TransformatsHierarchyNode** transformatsHierarchyNodesStack = new Animation::TransformatsHierarchyNode*[nodesCount.treeDepthMax + 1];
			unsigned int depth = 0;
			unsigned int nodesCounter = 0;
			animations[animationIdx].channelsNr = 0;
			while (1) {
				// REMINDER: Depends on having at least 2 bones in the hierarchy
				while (nodesIt->mNumChildren) {
					animations[animationIdx].TransformatsHierarchyBuffer[nodesCounter] = genTransformatHierarchyNode(nodesIt, scene, animationIdx, nodesToChannelsIdxsMap[nodesCounter]);
					transformatsHierarchyNodesStack[depth] = &animations[animationIdx].TransformatsHierarchyBuffer[nodesCounter++];								
					nodesIt = nodesIt->mChildren[childrenIdxsStack[depth]];
					childrenIdxsStack[depth]++;	
					depth++;
				}
				animations[animationIdx].TransformatsHierarchyBuffer[nodesCounter] = genTransformatHierarchyNode(nodesIt, scene, animationIdx, nodesToChannelsIdxsMap[nodesCounter]);
				transformatsHierarchyNodesStack[depth] = &animations[animationIdx].TransformatsHierarchyBuffer[nodesCounter++];						

				// REMINDER: Depends on having at least 2 bones in the hierarchy
				while (nodesIt->mParent && childrenIdxsStack[depth - 1] == nodesIt->mParent->mNumChildren) {
					nodesIt = nodesIt->mParent;
					transformatsHierarchyNodesStack[depth - 1]->children[childrenIdxsStack[depth - 1] - 1] = transformatsHierarchyNodesStack[depth];
					transformatsHierarchyNodesStack[depth]->parent = transformatsHierarchyNodesStack[depth - 1];
					depth--;
					childrenIdxsStack[depth] = 0;
				}

				if (nodesIt->mParent) {
					transformatsHierarchyNodesStack[depth - 1]->children[childrenIdxsStack[depth - 1] - 1] = transformatsHierarchyNodesStack[depth];
					transformatsHierarchyNodesStack[depth]->parent = transformatsHierarchyNodesStack[depth - 1];
					nodesIt = nodesIt->mParent->mChildren[childrenIdxsStack[depth - 1]];
					childrenIdxsStack[depth - 1]++;				
				}
				else {
					transformatsHierarchyNodesStack[depth]->parent = NULL;
					break;
				}
			}
			delete[] transformatsHierarchyNodesStack;
			delete[] childrenIdxsStack;		
						
			//if (animations[animationIdx].channelsNr) {
				unsigned int assimpChannelsNr = animation->mNumChannels;
				unsigned int keyFramesNrByAssimpFormat = 0;
				for (unsigned int channelIdx = 0; channelIdx < assimpChannelsNr; channelIdx++) {
					keyFramesNrByAssimpFormat += animation->mChannels[channelIdx]->mNumScalingKeys;
					keyFramesNrByAssimpFormat += animation->mChannels[channelIdx]->mNumRotationKeys;
					keyFramesNrByAssimpFormat += animation->mChannels[channelIdx]->mNumPositionKeys;
				}
				double* keyFramesTimesBuffer = new double[keyFramesNrByAssimpFormat];
				animations[animationIdx].keyFramesNr = 0;
				unsigned int* scalingKeysIts = new unsigned int[assimpChannelsNr]();
				unsigned int* rotKeysIts = new unsigned int[assimpChannelsNr]();
				unsigned int* translationKeysIts = new unsigned int[assimpChannelsNr]();
				unsigned int keyFrameIdxByAssimpFormatIt = 0;
				while (keyFrameIdxByAssimpFormatIt < keyFramesNrByAssimpFormat) {
					double minTime = std::numeric_limits<double>::max();
					for (unsigned int channelIdx = 0; channelIdx < assimpChannelsNr; channelIdx++) {
						aiNodeAnim* channel = animation->mChannels[channelIdx];
						double scalingKeyTime = scalingKeysIts[channelIdx] < channel->mNumScalingKeys ? channel->mScalingKeys[scalingKeysIts[channelIdx]].mTime : std::numeric_limits<double>::max();
						double rotKeyTime = rotKeysIts[channelIdx] < channel->mNumRotationKeys ? channel->mRotationKeys[rotKeysIts[channelIdx]].mTime : std::numeric_limits<double>::max();
						double translationKeyTime = translationKeysIts[channelIdx] < channel->mNumPositionKeys ? channel->mPositionKeys[translationKeysIts[channelIdx]].mTime : std::numeric_limits<double>::max();
						if (scalingKeyTime < minTime)
							minTime = scalingKeyTime;
						if (rotKeyTime < minTime)
							minTime = rotKeyTime;
						if (translationKeyTime < minTime)
							minTime = translationKeyTime;
					}
					keyFramesTimesBuffer[animations[animationIdx].keyFramesNr++] = minTime;

					for (unsigned int channelIdx = 0; channelIdx < assimpChannelsNr; channelIdx++) {
						aiNodeAnim* channel = animation->mChannels[channelIdx];
						if (scalingKeysIts[channelIdx] < channel->mNumScalingKeys && channel->mScalingKeys[scalingKeysIts[channelIdx]].mTime == minTime) {
							keyFrameIdxByAssimpFormatIt++;
							scalingKeysIts[channelIdx]++;
						}
						if (rotKeysIts[channelIdx] < channel->mNumRotationKeys && channel->mRotationKeys[rotKeysIts[channelIdx]].mTime == minTime) {
							keyFrameIdxByAssimpFormatIt++;
							rotKeysIts[channelIdx]++;
						}
						if (translationKeysIts[channelIdx] < channel->mNumPositionKeys && channel->mPositionKeys[translationKeysIts[channelIdx]].mTime == minTime) {
							keyFrameIdxByAssimpFormatIt++;
							translationKeysIts[channelIdx]++;
						}
					}
				}
				delete[] scalingKeysIts;
				delete[] rotKeysIts;
				delete[] translationKeysIts;
			
			
				if (keyFramesTimesBuffer[0] == 0) {
					animations[animationIdx].keyFramesTimes = new double[animations[animationIdx].keyFramesNr];
					memcpy(animations[animationIdx].keyFramesTimes, keyFramesTimesBuffer, animations[animationIdx].keyFramesNr * sizeof(double));
				}
				else {
					animations[animationIdx].keyFramesTimes = new double[animations[animationIdx].keyFramesNr + 1];
					animations[animationIdx].keyFramesTimes[0] = 0.0;
					memcpy(animations[animationIdx].keyFramesTimes + 1, keyFramesTimesBuffer, animations[animationIdx].keyFramesNr * sizeof(double));
				}

				animations[animationIdx].keyFramesNr++;
				delete[] keyFramesTimesBuffer;
				unsigned int keyFramesNr = animations[animationIdx].keyFramesNr;

				unsigned int channelsNr = animations[animationIdx].channelsNr;
				animations[animationIdx].scales = new glm::vec3*[channelsNr];
				animations[animationIdx].rots = new glm::quat*[channelsNr];
				animations[animationIdx].translations = new glm::vec3*[channelsNr];
				unsigned int nodeIdx = 0;
				for (unsigned int channelIdx = 0; channelIdx < channelsNr; channelIdx++) {
					while (!animations[animationIdx].TransformatsHierarchyBuffer[nodeIdx].isNodeAnimated)
						nodeIdx++;
					aiNodeAnim* channel = animation->mChannels[nodesToChannelsIdxsMap[nodeIdx++]];
					animations[animationIdx].scales[channelIdx] = new glm::vec3[keyFramesNr];
					animations[animationIdx].rots[channelIdx] = new glm::quat[keyFramesNr];
					animations[animationIdx].translations[channelIdx] = new glm::vec3[keyFramesNr];
					unsigned int channelScaleKeyFrameIdx = 0;
					unsigned int channelRotKeyFrameIdx = 0;
					unsigned int channelPosKeyFrameIdx = 0;
					for (unsigned int keyFrameIdx = 0; keyFrameIdx < keyFramesNr; keyFrameIdx++) {
						if (channelScaleKeyFrameIdx < channel->mNumScalingKeys) {
							if (channel->mScalingKeys[channelScaleKeyFrameIdx].mTime < animations[animationIdx].keyFramesTimes[keyFrameIdx]) {
								aiVectorKey& startKey = channel->mScalingKeys[channelScaleKeyFrameIdx - 1];
								aiVectorKey& endKey = channel->mScalingKeys[channelScaleKeyFrameIdx];
								animations[animationIdx].scales[channelIdx][keyFrameIdx] =
									(assimp2glm(endKey.mValue) - assimp2glm(startKey.mValue)) * (float)((animations[animationIdx].keyFramesTimes[keyFrameIdx] - startKey.mTime) / (endKey.mTime - startKey.mTime));
							}
							else if (channel->mScalingKeys[channelScaleKeyFrameIdx].mTime == animations[animationIdx].keyFramesTimes[keyFrameIdx])
								animations[animationIdx].scales[channelIdx][keyFrameIdx] = assimp2glm(channel->mScalingKeys[channelScaleKeyFrameIdx++].mValue);
							else // channel->mScalingKeys[channelScaleKeyFrameIdx].mTime > (animations[animationIdx].keyFramesTimes[keyFrameIdx] == 0))
								animations[animationIdx].scales[channelIdx][keyFrameIdx] = assimp2glm(channel->mScalingKeys[0].mValue);
						}
						else
							animations[animationIdx].scales[channelIdx][keyFrameIdx] = assimp2glm(channel->mScalingKeys[channelScaleKeyFrameIdx - 1].mValue);

						if (channelRotKeyFrameIdx < channel->mNumRotationKeys) {
							if (channel->mRotationKeys[channelRotKeyFrameIdx].mTime < animations[animationIdx].keyFramesTimes[keyFrameIdx]) {
								aiQuatKey& startKey = channel->mRotationKeys[channelRotKeyFrameIdx - 1];
								aiQuatKey& endKey = channel->mRotationKeys[channelRotKeyFrameIdx];
								animations[animationIdx].rots[channelIdx][keyFrameIdx] =
									glm::slerp(assimp2glm(startKey.mValue), assimp2glm(endKey.mValue), (float)((animations[animationIdx].keyFramesTimes[keyFrameIdx] - startKey.mTime) / (endKey.mTime - startKey.mTime)));
							}
							else if (channel->mRotationKeys[channelRotKeyFrameIdx].mTime == animations[animationIdx].keyFramesTimes[keyFrameIdx])
								animations[animationIdx].rots[channelIdx][keyFrameIdx] = assimp2glm(channel->mRotationKeys[channelRotKeyFrameIdx++].mValue);
							else // channel->mRotationKeys[channelRotKeyFrameIdx].mTime > (animations[animationIdx].keyFramesTimes[keyFrameIdx] == 0))
								animations[animationIdx].rots[channelIdx][keyFrameIdx] = assimp2glm(channel->mRotationKeys[0].mValue);
						}
						else
							animations[animationIdx].rots[channelIdx][keyFrameIdx] = assimp2glm(channel->mRotationKeys[channelRotKeyFrameIdx - 1].mValue);

						if (channelPosKeyFrameIdx < channel->mNumPositionKeys) {
							if (channel->mPositionKeys[channelPosKeyFrameIdx].mTime < animations[animationIdx].keyFramesTimes[keyFrameIdx]) {
								aiVectorKey& startKey = channel->mPositionKeys[channelScaleKeyFrameIdx - 1];
								aiVectorKey& endKey = channel->mPositionKeys[channelScaleKeyFrameIdx];
								animations[animationIdx].scales[channelIdx][keyFrameIdx] =
									(assimp2glm(endKey.mValue) - assimp2glm(startKey.mValue)) * (float)((animations[animationIdx].keyFramesTimes[keyFrameIdx] - startKey.mTime) / (endKey.mTime - startKey.mTime));
							}
							else if (channel->mPositionKeys[channelPosKeyFrameIdx].mTime == animations[animationIdx].keyFramesTimes[keyFrameIdx])
								animations[animationIdx].translations[channelIdx][keyFrameIdx] = assimp2glm(channel->mPositionKeys[channelPosKeyFrameIdx++].mValue);
							else // channel->mPositionKeys[channelPosKeyFrameIdx].mTime > (animations[animationIdx].keyFramesTimes[keyFrameIdx] == 0))
								animations[animationIdx].translations[channelIdx][keyFrameIdx] = assimp2glm(channel->mPositionKeys[0].mValue);
						}
						else
							animations[animationIdx].translations[channelIdx][keyFrameIdx] = assimp2glm(channel->mPositionKeys[channelPosKeyFrameIdx - 1].mValue);
					}
				}
			//}		
		}
	}

	Renderer::ModelAnimator::~ModelAnimator() {
		for (unsigned int animationIdx = 0; animationIdx < animationsNr; animationIdx++) {
			Animation& animation = animations[animationIdx];				
			for (unsigned int channelIdx = 0; channelIdx < animation.channelsNr; channelIdx++) {
				delete[] animation.scales[channelIdx];
				delete[] animation.rots[channelIdx];
				delete[] animation.translations[channelIdx];
			}
			delete[] animation.scales;
			delete[] animation.rots;
			delete[] animation.translations;
			for (unsigned int nodeIdx = 0; nodeIdx < animation.nodesNr; nodeIdx++) {
				if (animation.TransformatsHierarchyBuffer[nodeIdx].meshesNr)
					delete[] animation.TransformatsHierarchyBuffer[nodeIdx].meshesIdxs;
			}
			delete[] animation.TransformatsHierarchyBuffer;
			delete[] animation.keyFramesTimes;
		}
		delete[] animations;
		delete[] bonesOffsets;
		delete instanceAnimatorsPool;
	}

	Renderer::ModelAnimator::Animation::TransformatsHierarchyNode Renderer::ModelAnimator::genTransformatHierarchyNode(aiNode* node, const aiScene* scene, unsigned int animationIdx, unsigned int nodeToChannelIdxMapping) {
		Animation::TransformatsHierarchyNode transformatsHierarchyNode;
		transformatsHierarchyNode.transformat = assimp2glm(node->mTransformation);
		transformatsHierarchyNode.childrenNr = node->mNumChildren;
		transformatsHierarchyNode.children = new Animation::TransformatsHierarchyNode*[node->mNumChildren];
		if (nodeToChannelIdxMapping < scene->mAnimations[animationIdx]->mNumChannels) {
			aiNodeAnim* channel = scene->mAnimations[animationIdx]->mChannels[nodeToChannelIdxMapping];
			transformatsHierarchyNode.isNodeAnimated = true; // channel->mNumScalingKeys > 1 || channel->mNumRotationKeys > 1 || channel->mNumPositionKeys > 1;
			if (transformatsHierarchyNode.isNodeAnimated)
				animations[animationIdx].channelsNr++;		
		}
		else
			transformatsHierarchyNode.isNodeAnimated = false;			
	
		if (node->mNumMeshes) {
			transformatsHierarchyNode.meshesNr = node->mNumMeshes;
			transformatsHierarchyNode.meshesIdxs = new unsigned int[node->mNumMeshes];
			memcpy(transformatsHierarchyNode.meshesIdxs, node->mMeshes, node->mNumMeshes * sizeof(unsigned int));		
		}
		else {
			transformatsHierarchyNode.meshesNr = 0;
			unsigned int boneIdxOverall = 0;
			for (unsigned meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
				for (unsigned int boneIdx = 0; boneIdx < scene->mMeshes[meshIdx]->mNumBones; boneIdx++) {
					if (scene->mMeshes[meshIdx]->mBones[boneIdx]->mName == node->mName) {					
						transformatsHierarchyNode.boneIdx = boneIdxOverall;
						return transformatsHierarchyNode;
					}
					boneIdxOverall++;
				}
			}
		}
			
		transformatsHierarchyNode.boneIdx = std::numeric_limits<unsigned int>::max();
		return transformatsHierarchyNode;
	}

	Renderer::InstanceAnimator* Renderer::ModelAnimator::acquireInstance(unsigned int instanceIdx) {
		return instanceAnimatorsPool->acquire<InstanceAnimator::InitData>(InstanceAnimator::InitData({*this, instanceIdx}));
	}

	void Renderer::ModelAnimator::releaseInstance(InstanceAnimator* instanceAnimator) {
		instanceAnimatorsPool->release(instanceAnimator);
	}

	Renderer::InstanceAnimator::InstanceAnimator(InitData const& initData) : 
			modelAnimator(initData.modelAnimator), 
			bonesTransformatsBufferInstanceBaseIdx(modelAnimator.bonesTransformatsBufferModelBaseIdx + initData.instanceIdx*modelAnimator.bonesNr),
			animation(modelAnimator.animations[0]), 
			activeAnimationStartTime(ServiceLocator::getTimer().getCurrentTime()),
			endKeyFrameIdxCache(1),
			childrenIdxsStack(new unsigned int[animation.treeDepthMax]()),
			transformatsStack(new glm::mat4[animation.treeDepthMax + 2]()),
			instanceIdx(initData.instanceIdx) {}

	Renderer::InstanceAnimator::~InstanceAnimator() {
		delete[] childrenIdxsStack;
		delete[] transformatsStack;
	}

	void Renderer::InstanceAnimator::start(unsigned int animationIdx) {
		animation = modelAnimator.animations[animationIdx];
		activeAnimationStartTime = (float)ServiceLocator::getTimer().getCurrentTime();	
		endKeyFrameIdxCache = 1;
	}

	/*
		glm::mat4 inverseBindTransform = assimp2glm(scene->mRootNode->mTransformation); //glm::inverse(
		aiMesh* sceneFirstMesh = scene->mMeshes[0];
		glm::mat4 armature = assimp2glm(scene->mRootNode->mChildren[0]->mTransformation);
		currFrame = (currFrame + 1) % 3;

		glm::mat4 base = armature * calcKeyframeTransformat(scene->mAnimations[0]->mChannels[0]);
		bonesTransformsPtr[0] = inverseBindTransform * base * assimp2glm(sceneFirstMesh->mBones[0]->mOffsetMatrix);
		glm::mat4 second = base * calcKeyframeTransformat(scene->mAnimations[0]->mChannels[1]);
		bonesTransformsPtr[1] = inverseBindTransform * second * assimp2glm(sceneFirstMesh->mBones[1]->mOffsetMatrix);
		glm::mat4 third = second * calcKeyframeTransformat(scene->mAnimations[0]->mChannels[2]);
		bonesTransformsPtr[2] = inverseBindTransform * third * assimp2glm(sceneFirstMesh->mBones[2]->mOffsetMatrix);
		glm::mat4 fourth = third * calcKeyframeTransformat(scene->mAnimations[0]->mChannels[3]);
		bonesTransformsPtr[3] = inverseBindTransform * fourth * assimp2glm(sceneFirstMesh->mBones[3]->mOffsetMatrix);


		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		delete importer;
	*/

	// derived from: ( ((1-t)^3)*(0,0) + 3*((1-t)^2)*t*(0.25,0) + 3*(1-t)*(t^2)*(0.75,1) + (t^3)*(1,1) ).y
	inline float bezierify(float t) {	
		return t*t*(3 - 2*t);
	}

	void Renderer::InstanceAnimator::updateRendererBuffer() {
		glm::mat4* bonesTransformsPtr = NULL;
		if (modelAnimator.bonesNr) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelAnimator.bonesTransformatsBuffer);
			bonesTransformsPtr = (glm::mat4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, sizeof(glm::mat4) * bonesTransformatsBufferInstanceBaseIdx, sizeof(glm::mat4) * modelAnimator.bonesNr, GL_MAP_WRITE_BIT);
		}

		double currAnimationTime = fmod(ServiceLocator::getTimer().getCurrentTime() - activeAnimationStartTime, animation.dur) * animation.ticksPerSecond;	
		if (currAnimationTime < animation.keyFramesTimes[1])
			endKeyFrameIdxCache = 1;
		while (animation.keyFramesTimes[endKeyFrameIdxCache] < currAnimationTime)	
			endKeyFrameIdxCache++;	
		
		float interpolationFactor = (currAnimationTime - animation.keyFramesTimes[endKeyFrameIdxCache - 1]) / (animation.keyFramesTimes[endKeyFrameIdxCache] - animation.keyFramesTimes[endKeyFrameIdxCache - 1]);
		if (instanceIdx == 0)
			interpolationFactor = bezierify(interpolationFactor);
		
		ModelAnimator::Animation::TransformatsHierarchyNode* nodesIt = animation.TransformatsHierarchyBuffer;
		unsigned int channelIt = 0;	
		unsigned int depth = 0;		
		while (1) {
			while (1) {
				if (nodesIt->isNodeAnimated) {
					glm::vec3 scale = animation.scales[channelIt][endKeyFrameIdxCache - 1] * (1 - interpolationFactor) + animation.scales[channelIt][endKeyFrameIdxCache] * interpolationFactor;
					glm::quat rot = glm::slerp(animation.rots[channelIt][endKeyFrameIdxCache - 1], animation.rots[channelIt][endKeyFrameIdxCache], interpolationFactor);
					glm::vec3 translation = animation.translations[channelIt][endKeyFrameIdxCache - 1] * (1 - interpolationFactor) + animation.translations[channelIt][endKeyFrameIdxCache] * interpolationFactor;				
					transformatsStack[depth + 1] = transformatsStack[depth] * glm::translate(translation) * glm::mat4_cast(rot) * glm::scale(scale);												
					channelIt++;
				
					if (nodesIt->boneIdx != std::numeric_limits<unsigned int>::max())
						bonesTransformsPtr[nodesIt->boneIdx] = transformatsStack[depth + 1] * modelAnimator.bonesOffsets[nodesIt->boneIdx];				
				}
				else
					transformatsStack[depth + 1] = transformatsStack[depth] * nodesIt->transformat;				
			
				if (nodesIt->boneIdx == std::numeric_limits<unsigned int>::max() && nodesIt->meshesNr) {
					for (unsigned int meshIdx = 0; meshIdx < nodesIt->meshesNr; meshIdx++)
						modelAnimator.meshesTransformsBuffer[nodesIt->meshesIdxs[meshIdx]] = transformatsStack[depth + 1];
				}			 

				if (childrenIdxsStack[depth] < nodesIt->childrenNr) {
					nodesIt = nodesIt->children[childrenIdxsStack[depth]];
					childrenIdxsStack[depth]++;
					depth++;
				}
				else
					break;
			}	

			do {
				nodesIt = nodesIt->parent;
				childrenIdxsStack[depth] = 0;
				depth--;			
			} while (nodesIt && childrenIdxsStack[depth] == nodesIt->childrenNr);

			if (nodesIt) {
				nodesIt = nodesIt->children[childrenIdxsStack[depth]];
				childrenIdxsStack[depth]++;
				depth++;
			}
			else
				break;
		}

		//for (unsigned int depthIdx = 0; depthIdx < animation.treeDepthMax; depthIdx++)
		//	childrenIdxsStack[depthIdx] = 0;

		if (modelAnimator.bonesNr)
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}

	inline void Renderer::Plane::updateD(glm::vec3 const& newPointOnPlane) {
		d = -glm::dot(normal, newPointOnPlane);
	}

	inline float Renderer::Plane::signedDistFromPoint(glm::vec3 const& point) const {
		return glm::dot(normal, point) + d;
	}

} // namespace Corium3D

