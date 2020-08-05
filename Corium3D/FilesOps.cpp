#include "FilesOps.h"

#include <fstream>

namespace Corium3D {	

	template <class T>
	inline T readVal(std::ifstream& file) {
		T buffer;
		file.read((char*)&buffer, sizeof(T));
		return buffer;
	}

	template <class T>
	inline T* readValsArr(std::ifstream& file, unsigned int arrSz) {
		T* buffer = new T[arrSz];
		file.read((char*)buffer, sizeof(T) * arrSz);

		return buffer;
	}

	template <class T>
	inline void writeVal(std::ofstream& file, T val) {
		file.write((char*)&val, sizeof(T));
	}

	template <class T>
	inline void writeValsArr(std::ofstream& file, T const* valsArr, unsigned int arrSz) {
		file.write((char*)valsArr, sizeof(T) * arrSz);
	}

	void readSceneDesc(std::string const& sceneDescFileName, SceneDesc& sceneDescOut) {
		std::ifstream sceneDescFile(sceneDescFileName, std::ios::in | std::ios::binary);
#if DEBUG
		if (!sceneDescFile.is_open())
			throw std::ios_base::failure(sceneDescFileName + "failed to open.");
#endif	

		sceneDescOut.staticModelsNr = readVal<unsigned int>(sceneDescFile);
		sceneDescOut.mobileModelsNr = readVal<unsigned int>(sceneDescFile);
		unsigned int modelsNr = sceneDescOut.staticModelsNr + sceneDescOut.mobileModelsNr;
		sceneDescOut.modelsIdxs = readValsArr<unsigned int>(sceneDescFile, modelsNr);
		sceneDescOut.modelsInstancesNrsMaxima = readValsArr<unsigned int>(sceneDescFile, modelsNr);		
		sceneDescOut.collisionPrimitives3DInstancesNrsMaxima = readValsArr<unsigned int>(sceneDescFile, CollisionPrimitive3DType::__PRIMITIVE3D_TYPES_NR__);
		sceneDescOut.collisionPrimitives2DInstancesNrsMaxima = readValsArr<unsigned int>(sceneDescFile, CollisionPrimitive2DType::__PRIMITIVE2D_TYPES_NR__);

		sceneDescFile.close();
	}

	void readModelDesc(std::string const& modelDescFileName, ModelDesc& modelDescOut) {
		std::ifstream modelDescFile(modelDescFileName, std::ios::binary);
#if DEBUG
		if (!modelDescFile.is_open())
			throw std::ios_base::failure(modelDescFileName + "failed to open.");
#endif	

		unsigned int colladaPathSz = readVal<unsigned int>(modelDescFile);
		modelDescOut.colladaPath.reserve(colladaPathSz);
		modelDescFile.read(&modelDescOut.colladaPath[0], colladaPathSz);
		modelDescOut.verticesNr = readVal<unsigned int>(modelDescFile);
		modelDescOut.meshesNr = readVal<unsigned int>(modelDescFile);
		modelDescOut.verticesNrsPerMesh = readValsArr<unsigned int>(modelDescFile, modelDescOut.meshesNr);
		modelDescOut.verticesColorsNrTotal = readVal<unsigned int>(modelDescFile);
		modelDescOut.extraColorsNrsPerMesh = readValsArr<unsigned int>(modelDescFile, modelDescOut.meshesNr);
		modelDescOut.extraColors = new float** [modelDescOut.meshesNr];
		for (unsigned int meshIdx = 0; meshIdx < modelDescOut.meshesNr; meshIdx++) {
			unsigned int extraColorsNr = modelDescOut.extraColorsNrsPerMesh[meshIdx];
			modelDescOut.extraColors[meshIdx] = new float* [extraColorsNr];
			for (unsigned int extraColorIdx = 0; extraColorIdx < extraColorsNr; extraColorIdx++)
				modelDescOut.extraColors[meshIdx][extraColorIdx] = readValsArr<float>(modelDescFile, 4);
		}
		modelDescOut.texesNr = readVal<unsigned int>(modelDescFile);
		modelDescOut.texesNrsPerMesh = readValsArr<unsigned int>(modelDescFile, modelDescOut.meshesNr);
		modelDescOut.facesNr = readVal<unsigned int>(modelDescFile);
		modelDescOut.facesNrsPerMesh = readValsArr<unsigned int>(modelDescFile, modelDescOut.meshesNr);
		modelDescOut.progIdx = readVal<unsigned int>(modelDescFile);
		modelDescOut.bonesNr = readVal<unsigned int>(modelDescFile);
		modelDescOut.bonesNrsPerMesh = readValsArr<unsigned int>(modelDescFile, modelDescOut.meshesNr);
		modelDescOut.animationsNr = readVal<unsigned int>(modelDescFile);
		modelDescOut.animationsDescs = readValsArr<ModelDesc::AnimationDesc>(modelDescFile, modelDescOut.animationsNr);

		modelDescFile.close();
	}	

	void readColliderData(std::string const& colliderDataFileName, ColliderData& colliderDataOut) {
		std::ifstream colliderDataFile(colliderDataFileName, std::ios::in | std::ios::binary);
#if DEBUG
		if (!colliderDataFile.is_open())
			throw std::ios_base::failure(colliderDataFileName + "failed to open.");
#endif	

		colliderDataOut.boundingSphereCenter = readVal<glm::vec3>(colliderDataFile);
		colliderDataOut.boundingSphereRadius = readVal<float>(colliderDataFile);
		colliderDataOut.aabb3DMinVertex = readVal<glm::vec3>(colliderDataFile);
		colliderDataOut.aabb3DMaxVertex = readVal<glm::vec3>(colliderDataFile);
		colliderDataOut.collisionPrimitive3DType = readVal<CollisionPrimitive3DType>(colliderDataFile);
		colliderDataOut.collisionPrimitive2DType = readVal<CollisionPrimitive2DType>(colliderDataFile);
		switch (colliderDataOut.collisionPrimitive3DType) {
		case CollisionPrimitive3DType::BOX:
			colliderDataOut.collisionBoxData = readVal<ColliderData::CollisionBoxData>(colliderDataFile);
			break;
		case CollisionPrimitive3DType::SPHERE:
			colliderDataOut.collisionSphereData = readVal<ColliderData::CollisionSphereData>(colliderDataFile);
			break;
		case CollisionPrimitive3DType::CAPSULE:
			colliderDataOut.collisionCapsuleData = readVal<ColliderData::CollisionCapsuleData>(colliderDataFile);
			break;
		}

		if (colliderDataOut.collisionPrimitive2DType != CollisionPrimitive2DType::NO_2D_COLLIDER) {
			colliderDataOut.aabb2DMinVertex = readVal<glm::vec2>(colliderDataFile);
			colliderDataOut.aabb2DMaxVertex = readVal<glm::vec2>(colliderDataFile);
			switch (colliderDataOut.collisionPrimitive2DType) {
			case CollisionPrimitive2DType::RECT:
				colliderDataOut.collisionRectData = readVal<ColliderData::CollisionRectData>(colliderDataFile);
				break;
			case CollisionPrimitive2DType::CIRCLE:
				colliderDataOut.collisionCircleData = readVal<ColliderData::CollisionCircleData>(colliderDataFile);
				break;
			case CollisionPrimitive2DType::STADIUM:
				colliderDataOut.collisionStadiumData = readVal<ColliderData::CollisionStadiumData>(colliderDataFile);
				break;
			}
		}

		colliderDataFile.close();
	}

	void writeSceneDesc(std::string const& sceneDescFileName, SceneDesc const& sceneDesc) {
		std::ofstream sceneDescFile(sceneDescFileName, std::ios::in | std::ios::binary);
#if DEBUG
		if (!sceneDescFile.is_open())
			throw std::ios_base::failure(sceneDescFileName + "failed to open.");
#endif

		writeVal<unsigned int>(sceneDescFile, sceneDesc.staticModelsNr);
		writeVal<unsigned int>(sceneDescFile, sceneDesc.mobileModelsNr);		
		unsigned int modelsNr = sceneDesc.staticModelsNr + sceneDesc.mobileModelsNr;
		writeValsArr<unsigned int>(sceneDescFile, sceneDesc.modelsIdxs, modelsNr);
		writeValsArr<unsigned int>(sceneDescFile, sceneDesc.modelsInstancesNrsMaxima, modelsNr);
		writeValsArr<unsigned int>(sceneDescFile, sceneDesc.collisionPrimitives3DInstancesNrsMaxima, CollisionPrimitive3DType::__PRIMITIVE3D_TYPES_NR__);
		writeValsArr<unsigned int>(sceneDescFile, sceneDesc.collisionPrimitives2DInstancesNrsMaxima, CollisionPrimitive2DType::__PRIMITIVE2D_TYPES_NR__);		

		sceneDescFile.close();
	}

	void writeModelDesc(std::string const& modelDescFileName, ModelDesc const& modelDesc) {
		std::ofstream modelDescFile(modelDescFileName, std::ios::binary);
#if DEBUG
		if (!modelDescFile.is_open())
			throw std::ios_base::failure(modelDescFileName + "failed to open.");
#endif

		writeVal<unsigned int>(modelDescFile, modelDesc.colladaPath.size());
		writeValsArr<char>(modelDescFile, modelDesc.colladaPath.c_str(), modelDesc.colladaPath.size());
		writeVal<unsigned int>(modelDescFile, modelDesc.verticesNr);
		writeVal<unsigned int>(modelDescFile, modelDesc.meshesNr);
		writeValsArr<unsigned int>(modelDescFile, modelDesc.verticesNrsPerMesh, modelDesc.meshesNr);
		writeVal<unsigned int>(modelDescFile, modelDesc.verticesColorsNrTotal);
		writeValsArr<unsigned int>(modelDescFile, modelDesc.extraColorsNrsPerMesh, modelDesc.meshesNr);
		for (unsigned int meshIdx = 0; meshIdx < modelDesc.meshesNr; meshIdx++) {
			for (unsigned int extraColorIdx = 0; extraColorIdx < modelDesc.extraColorsNrsPerMesh[meshIdx]; extraColorIdx++)
				writeValsArr<float>(modelDescFile, modelDesc.extraColors[meshIdx][extraColorIdx], 4);
		}
		writeVal<unsigned int>(modelDescFile, modelDesc.texesNr);
		writeValsArr<unsigned int>(modelDescFile, modelDesc.texesNrsPerMesh, modelDesc.meshesNr);
		writeVal<unsigned int>(modelDescFile, modelDesc.facesNr);
		writeValsArr<unsigned int>(modelDescFile, modelDesc.facesNrsPerMesh, modelDesc.meshesNr);
		writeVal<unsigned int>(modelDescFile, modelDesc.progIdx);
		writeVal<unsigned int>(modelDescFile, modelDesc.bonesNr);
		writeValsArr<unsigned int>(modelDescFile, modelDesc.bonesNrsPerMesh, modelDesc.meshesNr);
		writeVal<unsigned int>(modelDescFile, modelDesc.animationsNr);
		writeValsArr<ModelDesc::AnimationDesc>(modelDescFile, modelDesc.animationsDescs, modelDesc.animationsNr);

		modelDescFile.close();
	}

	void writeColliderData(std::string const& colliderDataFileName, ColliderData const& colliderData) {
		std::ofstream colliderDataFile(colliderDataFileName, std::ios::out | std::ios::binary);
#if DEBUG
		if (!colliderDataFile.is_open())
			throw std::ios_base::failure(colliderDataFileName + "failed to open.");
#endif	

		writeVal<glm::vec3>(colliderDataFile, colliderData.boundingSphereCenter);
		writeVal<float>(colliderDataFile, colliderData.boundingSphereRadius);
		writeVal<glm::vec3>(colliderDataFile, colliderData.aabb3DMinVertex);
		writeVal<glm::vec3>(colliderDataFile, colliderData.aabb3DMaxVertex);
		writeVal<CollisionPrimitive3DType>(colliderDataFile, colliderData.collisionPrimitive3DType);
		writeVal<CollisionPrimitive2DType>(colliderDataFile, colliderData.collisionPrimitive2DType);
		switch (colliderData.collisionPrimitive3DType) {
		case CollisionPrimitive3DType::BOX:
			writeVal<ColliderData::CollisionBoxData>(colliderDataFile, colliderData.collisionBoxData);
			break;
		case CollisionPrimitive3DType::SPHERE:
			writeVal<ColliderData::CollisionSphereData>(colliderDataFile, colliderData.collisionSphereData);
			break;
		case CollisionPrimitive3DType::CAPSULE:
			writeVal<ColliderData::CollisionCapsuleData>(colliderDataFile, colliderData.collisionCapsuleData);
			break;
		}

		if (colliderData.collisionPrimitive2DType != CollisionPrimitive2DType::NO_2D_COLLIDER) {
			writeVal<glm::vec2>(colliderDataFile, colliderData.aabb2DMinVertex);
			writeVal<glm::vec2>(colliderDataFile, colliderData.aabb2DMaxVertex);
			switch (colliderData.collisionPrimitive2DType) {
			case CollisionPrimitive2DType::RECT:
				writeVal<ColliderData::CollisionRectData>(colliderDataFile, colliderData.collisionRectData);
				break;
			case CollisionPrimitive2DType::CIRCLE:
				writeVal<ColliderData::CollisionCircleData>(colliderDataFile, colliderData.collisionCircleData);
				break;
			case CollisionPrimitive2DType::STADIUM:
				writeVal<ColliderData::CollisionStadiumData>(colliderDataFile, colliderData.collisionStadiumData);
				break;
			}
		}

		colliderDataFile.close();
	}

	void readFileToStr(std::string const& fileName, std::string& strOut) {
		std::ifstream file(fileName);
#if DEBUG
		if (!file.is_open())
			throw std::ios_base::failure(fileName + std::string(" failed to open !"));
#endif

		file.seekg(0, std::ios::end);
		strOut.reserve(file.tellg());
		file.seekg(0, std::ios::beg);
		strOut.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		file.close();
	}

} // namespace Corium3D