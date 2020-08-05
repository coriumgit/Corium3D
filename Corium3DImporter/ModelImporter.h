#pragma once

#include "../Corium3D/FilesStructs.h"

#include <assimp/Importer.hpp>

namespace Media3D = System::Windows::Media::Media3D;
namespace Win = System::Windows;
namespace Media = System::Windows::Media;

namespace Corium3D {

	public ref class ModelImporter {
	public:
		ref struct ImportData {
			array<Media3D::MeshGeometry3D^>^ meshesGeometries;
			array<array<Media3D::Point3D>^>^ meshesVertices;			
			array<array<unsigned short>^>^ meshesVertexIndices;
			Media3D::Point3D boundingSphereCenter;
			float boundingSphereRadius;
			Media3D::Point3D aabb3DMinVertex;
			Media3D::Point3D aabb3DMaxVertex;
			Media3D::Point3D boundingCapsuleCenter;
			Media3D::Vector3D boundingCapsuleAxisVec;
			float boundingCapsuleHeight;
			float boundingCapsuleRadius;
		};

		ModelImporter(System::String^ modelPath, [System::Runtime::InteropServices::Out] ImportData^% importData);

		~ModelImporter();

		void assignExtraColors(array<array<array<float>^>^>^ extraColors);

		void assignProgIdx(unsigned int progIdx);

		void clearCollisionPrimitive3D();

		void assignCollisionBox(Media3D::Point3D^ center, Media3D::Point3D^ scale);

		void assignCollisionSphere(Media3D::Point3D^ center, float radius);

		void assignCollisionCapsule(Media3D::Point3D^ center1, Media3D::Vector3D^ axisVec, float radius);

		void clearCollisionPrimitive2D();

		void assignCollisionRect(Win::Point^ center, Win::Point^ scale);

		void assignCollisionCircle(Win::Point^ center, float radius);

		void assignCollisionStadium(Win::Point^ center1, Win::Vector^ axisVec, float radius);

		void genFiles(System::String^ path);

	private:

		Assimp::Importer* importer;
		ModelDesc* modelDesc;
		ColliderData* colliderData;
	};
} // namespace Corium3D