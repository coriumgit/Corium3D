#pragma once

#include <string>
#include <glm/glm.hpp>

namespace Corium3D {

	struct SceneDesc {
		unsigned int staticModelsNr;
		unsigned int mobileModelsNr;
		unsigned int* modelsIdxs;
		unsigned int* modelsInstancesNrsMaxima;
		bool* modelsMobilityLogicalVec;
		unsigned int* collisionPrimitives3DInstancesNrsMaxima;
		unsigned int* collisionPrimitives2DInstancesNrsMaxima;
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
		std::string collbinFileName; // TODO: incorporate this -data- to the collada
	};
	
	enum CollisionPrimitive3DType{ BOX, SPHERE, CAPSULE, __PRIMITIVE3D_TYPES_NR__, NO_3D_COLLIDER };
	enum CollisionPrimitive2DType{ RECT, CIRCLE, STADIUM, __PRIMITIVE2D_TYPES_NR__, NO_2D_COLLIDER };

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

		union {
			CollisionBoxData collisionBoxData;
			CollisionSphereData collisionSphereData;
			CollisionCapsuleData collisionCapsuleData;
		};

		union {
			CollisionRectData collisionRectData;
			CollisionCircleData collisionCircleData;
			CollisionStadiumData collisionStadiumData;
		};

		glm::vec3 boundingSphereCenter;
		float boundingSphereRadius;
		glm::vec3 aabb3DMinVertex;
		glm::vec3 aabb3DMaxVertex;

		CollisionPrimitive3DType collisionPrimitive3DType;		

		CollisionPrimitive2DType collisionPrimitive2DType;
		glm::vec2 aabb2DMinVertex;
		glm::vec2 aabb2DMaxVertex;
	};

} // namespace Corium3D