#pragma once

#include "../Corium3D/TransformsStructs.h"

#include <string>
#include <glm/glm.hpp>

namespace Corium3D {

	struct SceneData {
		struct SceneModelData {
			unsigned int modelIdx;
			unsigned int instancesNrMax;
			unsigned int instancesNrInit;
			Transform3D* instancesTransformsInit;
		};

		unsigned int staticModelsNr;
		unsigned int mobileModelsNr;
		SceneModelData* sceneModelsData;
		unsigned int* collisionPrimitives3DInstancesNrsMaxima;
		unsigned int* collisionPrimitives2DInstancesNrsMaxima;
	};

	enum CollisionPrimitive3DType { BOX, SPHERE, CAPSULE, __PRIMITIVE3D_TYPES_NR__, NO_3D_COLLIDER };
	enum CollisionPrimitive2DType { RECT, CIRCLE, STADIUM, __PRIMITIVE2D_TYPES_NR__, NO_2D_COLLIDER };

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

		glm::vec3 boundingSphereCenter;
		float boundingSphereRadius;
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
		unsigned int* verticesNrsPerMesh;
		unsigned int verticesColorsNrTotal;
		unsigned int* extraColorsNrsPerMesh; // TODO: incorporate this -data- to the collada
		float*** extraColors; // TODO: incorporate this -data- to the collada
		unsigned int texesNr;
		unsigned int* texesNrsPerMesh;
		unsigned int facesNr;
		unsigned int* facesNrsPerMesh;
		unsigned int progIdx; // TODO: incorporate this -data- to the collada
		unsigned int bonesNr;
		unsigned int* bonesNrsPerMesh;
		unsigned int animationsNr;
		AnimationDesc* animationsDescs;
		ColliderData colliderData; // TODO: incorporate this -data- to the collada and convert ColliderData to ColliderDesc
	};

	void readSceneData(std::string const& sceneDescFileName, SceneData& sceneDescOut);

	void readModelDesc(std::string const& modelDescFileName, ModelDesc& modelDescOut);	

	void writeSceneData(std::string const& sceneDescFileName, SceneData const& sceneDesc);

	void writeModelDesc(std::string const& modelDescFileName, ModelDesc const& modelDesc);		

	void readFileToStr(std::string const& fileName, std::string& strOut);

} // namespace Corium3D
