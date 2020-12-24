#pragma once

#include "../Corium3D/TransformsStructs.h"

#include <string>
#include <vector>
#include <array>
#include <glm/glm.hpp>

namespace Corium3D {
	
	enum CollisionPrimitive3DType { BOX, SPHERE, CAPSULE, __PRIMITIVE3D_TYPES_NR__, NO_3D_COLLIDER };
	enum CollisionPrimitive2DType { RECT, CIRCLE, STADIUM, __PRIMITIVE2D_TYPES_NR__, NO_2D_COLLIDER };
	
	struct SceneData {
		struct SceneModelData {
			unsigned int modelIdx;
			bool isStatic;
			unsigned int instancesNrMax;			
			std::vector<Transform3D> instancesTransformsInit;
		};

		unsigned int staticModelsNr;		
		std::vector<SceneModelData> sceneModelsData;
		std::array<unsigned int, CollisionPrimitive3DType::__PRIMITIVE3D_TYPES_NR__> collisionPrimitives3DInstancesNrsMaxima{};
		std::array<unsigned int, CollisionPrimitive2DType::__PRIMITIVE2D_TYPES_NR__> collisionPrimitives2DInstancesNrsMaxima{};
	};

	struct ColliderData {
		struct CollisionBoxData {
			glm::vec3 center;
			glm::vec3 scale;
		};

		struct CollisionSphereData {
			glm::vec3 center;
			float radius;
		};

		struct CollisionCapsuleData {
			glm::vec3 center1;
			glm::vec3 axisVec;
			float radius;
		};

		struct CollisionRectData {
			glm::vec2 center;
			glm::vec2 scale;
		};

		struct CollisionCircleData {
			glm::vec2 center;
			float radius;
		};

		struct CollisionStadiumData {
			glm::vec2 center1;
			glm::vec2 axisVec;
			float radius;
		};
		
		glm::vec3 aabb3DMinVertex;
		glm::vec3 aabb3DMaxVertex;
		glm::vec2 aabb2DMinVertex;
		glm::vec2 aabb2DMaxVertex;

		CollisionPrimitive3DType collisionPrimitive3DType;
		union {
			CollisionBoxData collisionBoxData;
			CollisionSphereData collisionSphereData;
			CollisionCapsuleData collisionCapsuleData;
		} collisionPrimitive3dData{ glm::vec3{}, glm::vec3{} };

		CollisionPrimitive2DType collisionPrimitive2DType;
		union {
			CollisionRectData collisionRectData;
			CollisionCircleData collisionCircleData;
			CollisionStadiumData collisionStadiumData;
		} collisionPrimitive2dData{ glm::vec2{}, glm::vec2{} };
	};

	struct ModelDesc {
		struct AnimationDesc {
			unsigned int keyFramesNr;
			unsigned int channelsNr;
			unsigned int transformatsHierarchyNodesMeshesNrs;
			unsigned int transformatsHierarchyNodesChildrenNr;
			unsigned int transformatsHierarchyNodesNr;
			unsigned int transformatsHierarchyDepthMax;
		};

		std::string colladaPath;
		unsigned int verticesNr;
		unsigned int meshesNr;
		std::vector<unsigned int> verticesNrsPerMesh;
		unsigned int verticesColorsNrTotal;
		std::vector<unsigned int> extraColorsNrsPerMesh; // TODO: incorporate this -data- to the collada
		std::vector<std::vector<std::array<float, 4>>> extraColors; // TODO: incorporate this -data- to the collada
		unsigned int texesNr;
		std::vector<unsigned int> texesNrsPerMesh;
		unsigned int facesNr;
		std::vector<unsigned int> facesNrsPerMesh;
		unsigned int progIdx; // TODO: incorporate this -data- to the collada
		unsigned int bonesNr;
		std::vector<unsigned int> bonesNrsPerMesh;		
		std::vector<AnimationDesc> animationsDescs;

		glm::vec3 boundingSphereCenter;
		float boundingSphereRadius;

		ColliderData colliderData;
	};

	void readSceneAssets(std::string const& assetsFileFullPath, unsigned int sceneIdx, SceneData& outSceneData, std::vector<ModelDesc>& outModelDescs, std::vector<unsigned int>& outModelSceneModelIdxsMap);

	void writeAssetsFile(std::string const& fullPath, std::vector<ModelDesc const*> const& modelDescs, std::vector<SceneData const*> const& scenesData);

	void readFileToStr(std::string const& fileName, std::string& strOut);

} // namespace Corium3D
