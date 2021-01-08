#pragma once

#include "../Corium3D/AssetsOps.h"

#include <assimp/Importer.hpp>
#include <vector>

namespace Media3D = System::Windows::Media::Media3D;
namespace Win = System::Windows;
namespace Media = System::Windows::Media;
namespace Collections = System::Collections::Generic;

namespace Corium3D {

	public ref class AssetsGen abstract sealed {
	public:
		ref struct ManagedImportedData {
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

		interface class IModelAssetGen : System::IDisposable {
			property ManagedImportedData^ ManagedImportedDataRef {
				ManagedImportedData^ get();				
			}						

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
		};

		interface class ISceneAssetGen : System::IDisposable {
			interface class ISceneModelData : System::IDisposable {
				interface class ISceneModelInstanceData : System::IDisposable {
					void setTranslationInit(Media3D::Vector3D^ translationInit);
					void setScaleInit(Media3D::Vector3D^ scaleInit);
					void setRotInit(Media3D::Quaternion^ rotInit);
				};

				ISceneModelInstanceData^ addSceneModelInstanceData(Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleInit, Media3D::Quaternion^ rotInit);
				void setInstancesNrMax(unsigned int instancesNrMax);
				void setIsStatic(bool isStatic);								
			};

			ISceneModelData^ addSceneModelData(IModelAssetGen^ modelAssetGen, unsigned int instancesNrMax, bool isStatic);
		};

		static IModelAssetGen^ createModelAssetGen(System::String^ modelPath);		
		static ISceneAssetGen^ createSceneAssetGen();
		static void generateAssets(System::String^ outputFolder);

	private:
		ref class ModelAssetGen : IModelAssetGen {
		public:
			property ManagedImportedData^ ManagedImportedDataRef {
				public: virtual ManagedImportedData^ get() = IModelAssetGen::ManagedImportedDataRef::get{ return managedImportedData; }
				private: void set(ManagedImportedData^ val) { managedImportedData = val; }
			}

			property unsigned int ModelIdx {
				public: unsigned int get() { return modelIdx; }				
			}

			property CollisionPrimitive3DType CollisionPromitive3DTypeRef {
				public:	CollisionPrimitive3DType get() { return modelDesc->colliderData.collisionPrimitive3DType; }
			}

			property CollisionPrimitive2DType CollisionPromitive2DTypeRef {
				public:	CollisionPrimitive2DType get() { return modelDesc->colliderData.collisionPrimitive2DType; }
			}

			ModelAssetGen(System::String^ modelPath);
			~ModelAssetGen();
			!ModelAssetGen();			
			virtual void assignExtraColors(array<array<array<float>^>^>^ extraColors) = IModelAssetGen::assignExtraColors;
			virtual void assignProgIdx(unsigned int progIdx) = IModelAssetGen::assignProgIdx;
			virtual void clearCollisionPrimitive3D() = IModelAssetGen::clearCollisionPrimitive3D;
			virtual void assignCollisionBox(Media3D::Point3D^ center, Media3D::Point3D^ scale) = IModelAssetGen::assignCollisionBox;
			virtual void assignCollisionSphere(Media3D::Point3D^ center, float radius) = IModelAssetGen::assignCollisionSphere;
			virtual void assignCollisionCapsule(Media3D::Point3D^ center1, Media3D::Vector3D^ axisVec, float radius) = IModelAssetGen::assignCollisionCapsule;
			virtual void clearCollisionPrimitive2D() = IModelAssetGen::clearCollisionPrimitive2D;
			virtual void assignCollisionRect(Win::Point^ center, Win::Point^ scale) = IModelAssetGen::assignCollisionRect;
			virtual void assignCollisionCircle(Win::Point^ center, float radius) = IModelAssetGen::assignCollisionCircle;
			virtual void assignCollisionStadium(Win::Point^ center1, Win::Vector^ axisVec, float radius) = IModelAssetGen::assignCollisionStadium;
			ModelDesc const* getAssetsFileReadyModelDesc(unsigned int modelIdx);

		private:			
			System::String^ modelPath;
			ManagedImportedData^ managedImportedData;
			Assimp::Importer* importer;
			ModelDesc* modelDesc;
			unsigned int modelIdx;
			bool isDisposed = false;
		};

		ref class SceneAssetGen : ISceneAssetGen
		{
		public:
			SceneAssetGen();
			~SceneAssetGen();
			!SceneAssetGen();
			virtual ISceneAssetGen::ISceneModelData^ addSceneModelData(IModelAssetGen^ modelAssetGen, unsigned int instancesNrMax, bool isStatic) = ISceneAssetGen::addSceneModelData;
			void removeSceneModelData(ModelAssetGen^ modelRepresentedByModelData);
			SceneData const* getAssetsFileReadySceneData();

		private:
			ref class SceneModelData : ISceneAssetGen::ISceneModelData
			{
			public:
				property ModelAssetGen^ ModelAssetGenRef {
					public: ModelAssetGen^ get() { return modelAssetGen; }
				}

				SceneModelData(SceneAssetGen^ sceneAssetGen, IModelAssetGen^ modelAssetGen, unsigned int instancesNrMax, bool isStatic);
				~SceneModelData();				
				virtual ISceneModelData::ISceneModelInstanceData^ addSceneModelInstanceData(Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleInit, Media3D::Quaternion^ rotInit) = ISceneModelData::addSceneModelInstanceData;				
				virtual void setIsStatic(bool isStatic) = ISceneModelData::setIsStatic;
				virtual void setInstancesNrMax(unsigned int instancesNrMax) = ISceneModelData::setInstancesNrMax;								
				void updateSceneAndSceneModelDataStructs(std::vector<SceneData::SceneModelData>::iterator& sceneModelsDataIt);

			private:
				ref class SceneModelInstanceData : ISceneAssetGen::ISceneModelData::ISceneModelInstanceData
				{
				public:
					SceneModelInstanceData(SceneModelData^ sceneModelData, Media3D::Vector3D^ translationInit, Media3D::Vector3D^ scaleInit, Media3D::Quaternion^ rotInit);
					~SceneModelInstanceData();							
					virtual void setTranslationInit(Media3D::Vector3D^ translationInit) = ISceneModelData::ISceneModelInstanceData::setTranslationInit;
					virtual void setScaleInit(Media3D::Vector3D^ scaleInit) = ISceneModelData::ISceneModelInstanceData::setScaleInit;
					virtual void setRotInit(Media3D::Quaternion^ rotInit) = ISceneModelData::ISceneModelInstanceData::setRotInit;					
					void updateSceneModelDataInstanceStruct(std::vector<Transform3D>::iterator& sceneModelInstancesDataIt);

				private:					
					Media3D::Vector3D^ translationInit;
					Media3D::Vector3D^ scaleInit;
					Media3D::Quaternion^ rotInit;
					SceneModelData^ sceneModelData;					
					bool isDisposed = false;
				}; // class SceneModelInstanceData				
				
				bool isStatic;
				unsigned int instancesNrMax;
				ModelAssetGen^ modelAssetGen;
				SceneAssetGen^ sceneAssetGen;
				Collections::List<SceneModelInstanceData^>^ sceneModelInstancesData = gcnew Collections::List<SceneModelInstanceData^>();
				bool isDisposed = false;
			}; // class SceneModelData

			SceneData* sceneData;			
			Collections::List<SceneModelData^>^ sceneModelsData = gcnew Collections::List<SceneModelData^>();
			bool isDisposed = false;
		}; // class SceneAssetGen

		static Collections::List<ModelAssetGen^>^ modelAssetGens = gcnew Collections::List<ModelAssetGen^>();
		static Collections::List<SceneAssetGen^>^ sceneAssetGens = gcnew Collections::List<SceneAssetGen^>();
	};
}