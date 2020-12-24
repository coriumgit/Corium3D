#include "AssetsOps.h"

#include <fstream>

namespace Corium3D {	

	typedef unsigned int file_loc_t;
	typedef unsigned short collection_sz_t;

	template <class T>
	inline T readVal(std::ifstream& file) {
		T buffer;
		file.read((char*)&buffer, sizeof(T));		
		return buffer;
	}

	template <class T>
	inline void writeVal(std::ofstream& file, T val) {
		file.write((char*)&val, sizeof(T));		
	}

	inline void readStr(std::ifstream& file, std::string& outStr) {
		unsigned int sz = readVal<collection_sz_t>(file);
		outStr.resize(sz);		
		//std::istreambuf_iterator<char> startIt(file);
		//std::istreambuf_iterator<char> endIt(std::next(startIt, sz));		
		//outStr.assign(startIt, endIt);	
		if (sz > 0)
			file.read(&outStr[0], sz);				
	}
	
	inline void writeStr(std::ofstream& file, std::string const& outStr) {
		collection_sz_t sz = outStr.length();
		writeVal<collection_sz_t>(file, sz);
		if (sz > 0)
			file.write(outStr.data(), sz);			
	}

	template <class T, int SZ>
	inline void readArr(std::ifstream& file, std::array<T,SZ>& outArr) {		
		file.read((char*)&outArr[0], sizeof(T) * SZ);
	}

	template <class T, int SZ>
	inline void writeArr(std::ofstream& file, std::array<T, SZ> const& arr) {
		file.write((char*)&arr[0], sizeof(T) * SZ);		
	}

	template <class T>
	inline void readVec(std::ifstream& file, std::vector<T>& outVec) {		
		collection_sz_t sz = readVal<collection_sz_t>(file);
		outVec.resize(sz);
		if (sz > 0)
			file.read((char*)&outVec[0], sizeof(T) * sz);
	}	

	template <class T>
	inline void writeVec(std::ofstream& file, std::vector<T> const& vec) {
		collection_sz_t sz = vec.size();
		writeVal<collection_sz_t>(file, sz);
		if (sz > 0)
			file.write((char*)&vec[0], sizeof(T) * sz);
	}

	template <class T>
	inline void readValsSeq(std::ifstream& file, collection_sz_t valsNr, std::vector<T>& outVec) {
		outVec.resize(valsNr);
		if (valsNr > 0)
			file.read((char*)&outVec[0], sizeof(T) * valsNr);
	}	

	template <class T>
	inline void writeValsSeq(std::ofstream& file, T const* valsArr, collection_sz_t arrSz) {
		if (arrSz > 0) 
			file.write((char*)valsArr, sizeof(T) * arrSz);
	}

	void readModelDesc(std::ifstream& modelDescFile, ModelDesc& outSceneModl);

	void writeModelDesc(std::ofstream& modelDescFile, ModelDesc const& modelDesc);
	
	void readSceneData(std::ifstream& sceneDataFile, SceneData& outSceneData);
	
	void writeSceneData(std::ofstream& sceneDataFile, SceneData const& sceneDesc);
	
	void readSceneAssets(std::string const& assetsFileFullPath, unsigned int sceneIdx, SceneData& outSceneData, std::vector<ModelDesc>& outModelDescs, std::vector<unsigned int>& outModelSceneModelIdxsMap)
	{
		std::ifstream assetsFile(assetsFileFullPath, std::ios::binary);
#if DEBUG || _DEBUG
		if (!assetsFile.is_open())
			throw std::ios_base::failure(assetsFileFullPath + " failed to open.");
#endif	

		// readout models number
		collection_sz_t modelsNr = readVal<collection_sz_t>(assetsFile);
		assetsFile.seekg(sizeof(collection_sz_t) + modelsNr * sizeof(file_loc_t) + sceneIdx * sizeof(file_loc_t));		
		assetsFile.seekg(readVal<file_loc_t>(assetsFile));
		readSceneData(assetsFile, outSceneData);
		unsigned int sceneModelsNr = outSceneData.sceneModelsData.size();
		outModelDescs.resize(sceneModelsNr);
		outModelSceneModelIdxsMap.resize(modelsNr);		

		unsigned int staticModelIdx = 0;
		unsigned int mobileModelIdx = 0;
		for (unsigned int modelIdx = 0; modelIdx < sceneModelsNr; ++modelIdx) {
			assetsFile.seekg(sizeof(collection_sz_t) + outSceneData.sceneModelsData[modelIdx].modelIdx * sizeof(file_loc_t));
			assetsFile.seekg(readVal<file_loc_t>(assetsFile));			
			if (outSceneData.sceneModelsData[modelIdx].isStatic) {				
				outModelSceneModelIdxsMap[outSceneData.sceneModelsData[modelIdx].modelIdx] = staticModelIdx;
				readModelDesc(assetsFile, outModelDescs[staticModelIdx++]);
			}				
			else {
				unsigned int modelIdxMapped = sceneModelsNr - 1 - mobileModelIdx++;
				outModelSceneModelIdxsMap[outSceneData.sceneModelsData[modelIdx].modelIdx] = modelIdxMapped;
				readModelDesc(assetsFile, outModelDescs[modelIdxMapped]);				
			}						
		}

		assetsFile.close();
	}

	void writeAssetsFile(std::string const& assetsFileFullPath, std::vector<ModelDesc const*> const& modelDescs, std::vector<SceneData const*> const& scenesData)
	{
		std::ofstream assetsFile(assetsFileFullPath, std::ios::binary);
#if DEBUG || _DEBUG
		if (!assetsFile.is_open())
			throw std::ios_base::failure(assetsFileFullPath + " failed to open.");
#endif	

		unsigned int modelsNr = modelDescs.size();
		unsigned int scenesNr = scenesData.size();
		std::vector<file_loc_t> modelDescsFileLocs(modelsNr);
		std::vector<file_loc_t> scenesDataFileLocs(scenesNr);
		writeVec(assetsFile, modelDescsFileLocs);
		writeValsSeq(assetsFile, &scenesDataFileLocs[0], scenesNr);
		for (unsigned int modelIdx = 0; modelIdx < modelsNr; ++modelIdx) {
			modelDescsFileLocs[modelIdx] = assetsFile.tellp();
			writeModelDesc(assetsFile, *modelDescs[modelIdx]);						
		}
		for (unsigned int sceneIdx = 0; sceneIdx < scenesNr; ++sceneIdx) {
			scenesDataFileLocs[sceneIdx] = assetsFile.tellp();
			writeSceneData(assetsFile, *scenesData[sceneIdx]);
		}

		assetsFile.seekp(0);
		writeVec(assetsFile, modelDescsFileLocs);
		writeValsSeq(assetsFile, &scenesDataFileLocs[0], scenesNr);

		assetsFile.close();
	}

	void readModelDesc(std::ifstream& modelDescFile, ModelDesc& outModelDesc) {
		readStr(modelDescFile, outModelDesc.colladaPath);
		if (!outModelDesc.colladaPath.empty()) {
			outModelDesc.verticesNr = readVal<unsigned int>(modelDescFile);
			outModelDesc.meshesNr = readVal<unsigned int>(modelDescFile);
			readValsSeq<unsigned int>(modelDescFile, outModelDesc.meshesNr, outModelDesc.verticesNrsPerMesh);
			outModelDesc.verticesColorsNrTotal = readVal<unsigned int>(modelDescFile);
			readValsSeq<unsigned int>(modelDescFile, outModelDesc.meshesNr, outModelDesc.extraColorsNrsPerMesh);
			outModelDesc.extraColors.resize(outModelDesc.meshesNr);
			for (unsigned int meshIdx = 0; meshIdx < outModelDesc.meshesNr; meshIdx++) {
				unsigned int extraColorsNr = outModelDesc.extraColorsNrsPerMesh[meshIdx];
				outModelDesc.extraColors[meshIdx].resize(extraColorsNr);
				for (unsigned int extraColorIdx = 0; extraColorIdx < extraColorsNr; extraColorIdx++)
					readArr<float, 4>(modelDescFile, outModelDesc.extraColors[meshIdx][extraColorIdx]);
			}
			outModelDesc.texesNr = readVal<unsigned int>(modelDescFile);
			readValsSeq<unsigned int>(modelDescFile, outModelDesc.meshesNr, outModelDesc.texesNrsPerMesh);
			outModelDesc.facesNr = readVal<unsigned int>(modelDescFile);
			readValsSeq<unsigned int>(modelDescFile, outModelDesc.meshesNr, outModelDesc.facesNrsPerMesh);
			outModelDesc.progIdx = readVal<unsigned int>(modelDescFile);
			outModelDesc.bonesNr = readVal<unsigned int>(modelDescFile);
			readValsSeq<unsigned int>(modelDescFile, outModelDesc.meshesNr, outModelDesc.bonesNrsPerMesh);
			readVec<ModelDesc::AnimationDesc>(modelDescFile, outModelDesc.animationsDescs);
		}

		outModelDesc.boundingSphereCenter = readVal<glm::vec3>(modelDescFile);
		outModelDesc.boundingSphereRadius = readVal<float>(modelDescFile);
		
		outModelDesc.colliderData.collisionPrimitive3DType = readVal<enum CollisionPrimitive3DType>(modelDescFile);		
		if (outModelDesc.colliderData.collisionPrimitive3DType != CollisionPrimitive3DType::NO_3D_COLLIDER) {
			outModelDesc.colliderData.aabb3DMinVertex = readVal<glm::vec3>(modelDescFile);
			outModelDesc.colliderData.aabb3DMaxVertex = readVal<glm::vec3>(modelDescFile);
			switch (outModelDesc.colliderData.collisionPrimitive3DType) {
			case CollisionPrimitive3DType::BOX:
				outModelDesc.colliderData.collisionPrimitive3dData.collisionBoxData = readVal<ColliderData::CollisionBoxData>(modelDescFile);
				break;
			case CollisionPrimitive3DType::SPHERE:
				outModelDesc.colliderData.collisionPrimitive3dData.collisionSphereData = readVal<ColliderData::CollisionSphereData>(modelDescFile);
				break;
			case CollisionPrimitive3DType::CAPSULE:
				outModelDesc.colliderData.collisionPrimitive3dData.collisionCapsuleData = readVal<ColliderData::CollisionCapsuleData>(modelDescFile);
				break;
			}
		}

		outModelDesc.colliderData.collisionPrimitive2DType = readVal<enum CollisionPrimitive2DType>(modelDescFile);
		if (outModelDesc.colliderData.collisionPrimitive2DType != CollisionPrimitive2DType::NO_2D_COLLIDER) {
			outModelDesc.colliderData.aabb2DMinVertex = readVal<glm::vec2>(modelDescFile);
			outModelDesc.colliderData.aabb2DMaxVertex = readVal<glm::vec2>(modelDescFile);
			switch (outModelDesc.colliderData.collisionPrimitive2DType) {
			case CollisionPrimitive2DType::RECT:
				outModelDesc.colliderData.collisionPrimitive2dData.collisionRectData = readVal<ColliderData::CollisionRectData>(modelDescFile);
				break;
			case CollisionPrimitive2DType::CIRCLE:
				outModelDesc.colliderData.collisionPrimitive2dData.collisionCircleData = readVal<ColliderData::CollisionCircleData>(modelDescFile);
				break;
			case CollisionPrimitive2DType::STADIUM:
				outModelDesc.colliderData.collisionPrimitive2dData.collisionStadiumData = readVal<ColliderData::CollisionStadiumData>(modelDescFile);
				break;
			}
		}		
	}
		
	void writeModelDesc(std::ofstream& modelDescFile, ModelDesc const& modelDesc) {		
		writeStr(modelDescFile, modelDesc.colladaPath);
		if (!modelDesc.colladaPath.empty()) {			
			writeVal<unsigned int>(modelDescFile, modelDesc.verticesNr);
			writeVal<unsigned int>(modelDescFile, modelDesc.meshesNr);
			writeValsSeq<unsigned int>(modelDescFile, &modelDesc.verticesNrsPerMesh[0], modelDesc.meshesNr);
			writeVal<unsigned int>(modelDescFile, modelDesc.verticesColorsNrTotal);
			writeValsSeq<unsigned int>(modelDescFile, &modelDesc.extraColorsNrsPerMesh[0], modelDesc.meshesNr);
			for (unsigned int meshIdx = 0; meshIdx < modelDesc.meshesNr; meshIdx++) {
				for (unsigned int extraColorIdx = 0; extraColorIdx < modelDesc.extraColorsNrsPerMesh[meshIdx]; extraColorIdx++)
					writeArr<float, 4>(modelDescFile, modelDesc.extraColors[meshIdx][extraColorIdx]);
			}
			writeVal<unsigned int>(modelDescFile, modelDesc.texesNr);
			writeValsSeq<unsigned int>(modelDescFile, &modelDesc.texesNrsPerMesh[0], modelDesc.meshesNr);
			writeVal<unsigned int>(modelDescFile, modelDesc.facesNr);
			writeValsSeq<unsigned int>(modelDescFile, &modelDesc.facesNrsPerMesh[0], modelDesc.meshesNr);
			writeVal<unsigned int>(modelDescFile, modelDesc.progIdx);
			writeVal<unsigned int>(modelDescFile, modelDesc.bonesNr);
			writeValsSeq<unsigned int>(modelDescFile, &modelDesc.bonesNrsPerMesh[0], modelDesc.meshesNr);
			writeVec<ModelDesc::AnimationDesc>(modelDescFile, modelDesc.animationsDescs);
		}

		writeVal<glm::vec3>(modelDescFile, modelDesc.boundingSphereCenter);
		writeVal<float>(modelDescFile, modelDesc.boundingSphereRadius);
		
		writeVal<CollisionPrimitive3DType>(modelDescFile, modelDesc.colliderData.collisionPrimitive3DType);		
		if (modelDesc.colliderData.collisionPrimitive3DType != CollisionPrimitive3DType::NO_3D_COLLIDER) {
			writeVal<glm::vec3>(modelDescFile, modelDesc.colliderData.aabb3DMinVertex);
			writeVal<glm::vec3>(modelDescFile, modelDesc.colliderData.aabb3DMaxVertex);
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
		}

		writeVal<CollisionPrimitive2DType>(modelDescFile, modelDesc.colliderData.collisionPrimitive2DType);
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
	}

	void readSceneData(std::ifstream& sceneDataFile, SceneData& outSceneData) {
		outSceneData.staticModelsNr = readVal<unsigned int>(sceneDataFile);		
		unsigned int modelsNr = readVal<unsigned int>(sceneDataFile);
		outSceneData.sceneModelsData.resize(modelsNr);		
		for (unsigned int modelIdx = 0; modelIdx < modelsNr; ++modelIdx) {
			SceneData::SceneModelData& sceneModelData = outSceneData.sceneModelsData[modelIdx];
			sceneModelData.modelIdx = readVal<unsigned int>(sceneDataFile);
			sceneModelData.isStatic = readVal<bool>(sceneDataFile);
			sceneModelData.instancesNrMax = readVal<unsigned int>(sceneDataFile);			
			readVec<Transform3D>(sceneDataFile, sceneModelData.instancesTransformsInit);
		}

		readArr<unsigned int, CollisionPrimitive3DType::__PRIMITIVE3D_TYPES_NR__>(sceneDataFile, outSceneData.collisionPrimitives3DInstancesNrsMaxima);
		readArr<unsigned int, CollisionPrimitive2DType::__PRIMITIVE2D_TYPES_NR__>(sceneDataFile, outSceneData.collisionPrimitives2DInstancesNrsMaxima);		
	}

	void writeSceneData(std::ofstream& sceneDataFile, SceneData const& sceneData) {		
		writeVal<unsigned int>(sceneDataFile, sceneData.staticModelsNr);
		unsigned int modelsNr = sceneData.sceneModelsData.size();
		writeVal<unsigned int>(sceneDataFile, modelsNr);
		for (unsigned int modelIdx = 0; modelIdx < modelsNr; ++modelIdx) {
			SceneData::SceneModelData const& sceneModelData = sceneData.sceneModelsData[modelIdx];
			writeVal<unsigned int>(sceneDataFile, sceneModelData.modelIdx);
			writeVal<bool>(sceneDataFile, sceneModelData.isStatic);
			writeVal<unsigned int>(sceneDataFile, sceneModelData.instancesNrMax);
			writeVec<Transform3D>(sceneDataFile, sceneModelData.instancesTransformsInit);
		}

		writeArr<unsigned int, CollisionPrimitive3DType::__PRIMITIVE3D_TYPES_NR__>(sceneDataFile, sceneData.collisionPrimitives3DInstancesNrsMaxima);
		writeArr<unsigned int, CollisionPrimitive2DType::__PRIMITIVE2D_TYPES_NR__>(sceneDataFile, sceneData.collisionPrimitives2DInstancesNrsMaxima);		
	}
			
	void readFileToStr(std::string const& fileName, std::string& strOut) {
		std::ifstream file(fileName);
#if DEBUG || _DEBUG
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