#include "AssetsOps.h"

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

	void readSceneData(std::string const& sceneDescFileName, SceneData& sceneDataOut) {
		std::ifstream sceneDescFile(sceneDescFileName, std::ios::in | std::ios::binary);
#if DEBUG
		if (!sceneDescFile.is_open())
			throw std::ios_base::failure(sceneDescFileName + "failed to open.");
#endif	

		sceneDataOut.staticModelsNr = readVal<unsigned int>(sceneDescFile);
		sceneDataOut.mobileModelsNr = readVal<unsigned int>(sceneDescFile);
		unsigned int modelsNr = sceneDataOut.staticModelsNr + sceneDataOut.mobileModelsNr;
		//sceneDataOut.sceneModelsData.modelsIdxs = readValsArr<unsigned int>(sceneDescFile, modelsNr);
		//sceneDataOut.sceneModelsData.modelsInstancesNrsMaxima = readValsArr<unsigned int>(sceneDescFile, modelsNr);
		sceneDataOut.collisionPrimitives3DInstancesNrsMaxima = readValsArr<unsigned int>(sceneDescFile, CollisionPrimitive3DType::__PRIMITIVE3D_TYPES_NR__);
		sceneDataOut.collisionPrimitives2DInstancesNrsMaxima = readValsArr<unsigned int>(sceneDescFile, CollisionPrimitive2DType::__PRIMITIVE2D_TYPES_NR__);

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

		modelDescOut.colliderData.boundingSphereCenter = readVal<glm::vec3>(modelDescFile);
		modelDescOut.colliderData.boundingSphereRadius = readVal<float>(modelDescFile);
		modelDescOut.colliderData.aabb3DMinVertex = readVal<glm::vec3>(modelDescFile);
		modelDescOut.colliderData.aabb3DMaxVertex = readVal<glm::vec3>(modelDescFile);
		modelDescOut.colliderData.collisionPrimitive3DType = readVal<enum CollisionPrimitive3DType>(modelDescFile);
		modelDescOut.colliderData.collisionPrimitive2DType = readVal<enum CollisionPrimitive2DType>(modelDescFile);
		switch (modelDescOut.colliderData.collisionPrimitive3DType) {
		case CollisionPrimitive3DType::BOX:
			modelDescOut.colliderData.collisionPrimitive3dData.collisionBoxData = readVal<ColliderData::CollisionBoxData>(modelDescFile);
			break;
		case CollisionPrimitive3DType::SPHERE:
			modelDescOut.colliderData.collisionPrimitive3dData.collisionSphereData = readVal<ColliderData::CollisionSphereData>(modelDescFile);
			break;
		case CollisionPrimitive3DType::CAPSULE:
			modelDescOut.colliderData.collisionPrimitive3dData.collisionCapsuleData = readVal<ColliderData::CollisionCapsuleData>(modelDescFile);
			break;
		}

		if (modelDescOut.colliderData.collisionPrimitive2DType != CollisionPrimitive2DType::NO_2D_COLLIDER) {
			modelDescOut.colliderData.aabb2DMinVertex = readVal<glm::vec2>(modelDescFile);
			modelDescOut.colliderData.aabb2DMaxVertex = readVal<glm::vec2>(modelDescFile);
			switch (modelDescOut.colliderData.collisionPrimitive2DType) {
			case CollisionPrimitive2DType::RECT:
				modelDescOut.colliderData.collisionPrimitive2dData.collisionRectData = readVal<ColliderData::CollisionRectData>(modelDescFile);
				break;
			case CollisionPrimitive2DType::CIRCLE:
				modelDescOut.colliderData.collisionPrimitive2dData.collisionCircleData = readVal<ColliderData::CollisionCircleData>(modelDescFile);
				break;
			case CollisionPrimitive2DType::STADIUM:
				modelDescOut.colliderData.collisionPrimitive2dData.collisionStadiumData = readVal<ColliderData::CollisionStadiumData>(modelDescFile);
				break;
			}
		}

		modelDescFile.close();
	}	
	
	void writeSceneData(std::string const& sceneDescFileName, SceneData const& sceneData) {
		std::ofstream sceneDescFile(sceneDescFileName, std::ios::in | std::ios::binary);
#if DEBUG
		if (!sceneDescFile.is_open())
			throw std::ios_base::failure(sceneDescFileName + "failed to open.");
#endif

		writeVal<unsigned int>(sceneDescFile, sceneData.staticModelsNr);
		writeVal<unsigned int>(sceneDescFile, sceneData.mobileModelsNr);		
		unsigned int modelsNr = sceneData.staticModelsNr + sceneData.mobileModelsNr;
		//writeValsArr<unsigned int>(sceneDescFile, sceneData.modelsIdxs, modelsNr);
		//writeValsArr<unsigned int>(sceneDescFile, sceneData.modelsInstancesNrsMaxima, modelsNr);
		writeValsArr<unsigned int>(sceneDescFile, sceneData.collisionPrimitives3DInstancesNrsMaxima, CollisionPrimitive3DType::__PRIMITIVE3D_TYPES_NR__);
		writeValsArr<unsigned int>(sceneDescFile, sceneData.collisionPrimitives2DInstancesNrsMaxima, CollisionPrimitive2DType::__PRIMITIVE2D_TYPES_NR__);		

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

		writeVal<glm::vec3>(modelDescFile, modelDesc.colliderData.boundingSphereCenter);
		writeVal<float>(modelDescFile, modelDesc.colliderData.boundingSphereRadius);
		writeVal<glm::vec3>(modelDescFile, modelDesc.colliderData.aabb3DMinVertex);
		writeVal<glm::vec3>(modelDescFile, modelDesc.colliderData.aabb3DMaxVertex);
		writeVal<CollisionPrimitive3DType>(modelDescFile, modelDesc.colliderData.collisionPrimitive3DType);
		writeVal<CollisionPrimitive2DType>(modelDescFile, modelDesc.colliderData.collisionPrimitive2DType);
		switch (modelDesc.colliderData.collisionPrimitive3DType) {
		case CollisionPrimitive3DType::BOX:
			writeVal<ColliderData::CollisionBoxData>(modelDescFile, modelDesc.colliderData.collisionPrimitive3dData.collisionBoxData);
			break;
		case CollisionPrimitive3DType::SPHERE:
			writeVal<ColliderData::CollisionSphereData>(modelDescFile, modelDesc.colliderData.collisionPrimitive3dData.collisionSphereData);
			break;
		case CollisionPrimitive3DType::CAPSULE:
			writeVal<ColliderData::CollisionCapsuleData>(modelDescFile, modelDesc.colliderData.collisionPrimitive3dData.collisionCapsuleData);
			break;
		}

		if (modelDesc.colliderData.collisionPrimitive2DType != CollisionPrimitive2DType::NO_2D_COLLIDER) {
			writeVal<glm::vec2>(modelDescFile, modelDesc.colliderData.aabb2DMinVertex);
			writeVal<glm::vec2>(modelDescFile, modelDesc.colliderData.aabb2DMaxVertex);
			switch (modelDesc.colliderData.collisionPrimitive2DType) {
			case CollisionPrimitive2DType::RECT:
				writeVal<ColliderData::CollisionRectData>(modelDescFile, modelDesc.colliderData.collisionPrimitive2dData.collisionRectData);
				break;
			case CollisionPrimitive2DType::CIRCLE:
				writeVal<ColliderData::CollisionCircleData>(modelDescFile, modelDesc.colliderData.collisionPrimitive2dData.collisionCircleData);
				break;
			case CollisionPrimitive2DType::STADIUM:
				writeVal<ColliderData::CollisionStadiumData>(modelDescFile, modelDesc.colliderData.collisionPrimitive2dData.collisionStadiumData);
				break;
			}
		}

		modelDescFile.close();
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