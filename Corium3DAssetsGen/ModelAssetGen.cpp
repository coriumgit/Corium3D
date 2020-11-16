#include "ModelAssetGen.h"

#include "../Corium3D/AssetsOps.h"
#include "../Corium3D/BoundingSphere.h"
#include "../Corium3D/AABB.h"
#include "../Corium3D/ServiceLocator.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <windows.h>
#include <fbxsdk.h>

#define IMPORTER_FBX_SDK true

using namespace System::Runtime::InteropServices;
using namespace System::Windows::Media::Media3D;
using namespace System::Windows::Media;
using namespace System::Windows;

//extern "C" __declspec(dllexport) bool PerformTest()

namespace Corium3D {	
	
	inline glm::vec3 marshalPoint3D(Point3D^ src) {
		return glm::vec3{ src->X, src->Y ,src->Z };			
	}

	inline glm::vec3 marshalVector3D(Vector3D^ src) {
		return glm::vec3{ src->X, src->Y ,src->Z };
	}

	inline glm::vec2 marshalPoint(Point^ src) {
		return glm::vec2{ src->X, src->Y };
	}

	inline glm::vec2 marshalVector(Vector^ src) {
		return glm::vec2{ src->X, src->Y };
	}

	inline char* systemStringToAnsiString(System::String^ str) {
		return static_cast<char*>(Marshal::StringToHGlobalAnsi(str).ToPointer());
	}

	void genNodesCountRecurse(aiNode* node, unsigned int depth, unsigned int& treeDepthMaxOut, unsigned int& nodesNrOut) {
		treeDepthMaxOut = depth > treeDepthMaxOut ? depth : treeDepthMaxOut;
		if (node->mNumChildren) {
			nodesNrOut += node->mNumChildren;
			for (unsigned int childIdx = 0; childIdx < node->mNumChildren; childIdx++)
				genNodesCountRecurse(node->mChildren[childIdx], depth + 1, treeDepthMaxOut, nodesNrOut);
		}
	}

	ModelAssetGen::ModelAssetGen(System::String^ modelPath)
	{	
		modelDesc = new ModelDesc;
		modelDesc->colladaPath = systemStringToAnsiString(modelPath);
		modelDesc->verticesNr = 0;
		modelDesc->verticesColorsNrTotal = 0;
		modelDesc->texesNr = 0;
		modelDesc->bonesNr = 0;
		modelDesc->facesNr = 0;		
		modelDesc->progIdx = 0;		

#if IMPORTER_FBX_SDK
		FbxManager* fbxManager = FbxManager::Create();
		FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
		fbxManager->SetIOSettings(ios);
		FbxImporter* fbxImporter = FbxImporter::Create(fbxManager, "");
		if (!fbxImporter->Initialize(modelDesc->colladaPath.c_str(), -1, fbxManager->GetIOSettings())) {
			OutputDebugStringA((std::string("Failed to import \"") + modelDesc->colladaPath + std::string("\": ") + fbxImporter->GetStatus().GetErrorString()).c_str());
			throw std::exception();
		}
		else
			OutputDebugStringA((std::string("Imported \"") + modelDesc->colladaPath + std::string("\"\n")).c_str());

		FbxScene* fbxScene = FbxScene::Create(fbxManager, "myScene");
		fbxImporter->Import(fbxScene);
		fbxImporter->Destroy();

		FbxNode* fbxNodeRoot = fbxScene->GetRootNode();
		for (int i = 0; i < fbxNodeRoot->GetChildCount(); i++) {
			FbxNode* childNode = fbxNodeRoot->GetChild(i);
			FbxMesh* mesh = childNode->GetMesh();
			if (mesh) {
				FbxGeometryElementNormal* lNormalElement = mesh->GetElementNormal();
				if (lNormalElement)
				{
					//mapping mode is by control points. The mesh should be smooth and soft.
					//we can get normals by retrieving each control point
					if (lNormalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
					{
						//Let's get normals of each vertex, since the mapping mode of normal element is by control point
						for (int lVertexIndex = 0; lVertexIndex < mesh->GetControlPointsCount(); lVertexIndex++)
						{
							int lNormalIndex = 0;
							//reference mode is direct, the normal index is same as vertex index.
							//get normals by the index of control vertex
							if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
								lNormalIndex = lVertexIndex;

							//reference mode is index-to-direct, get normals by the index-to-direct
							if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
								lNormalIndex = lNormalElement->GetIndexArray().GetAt(lVertexIndex);

							//Got normals of each vertex.
							FbxVector4 lNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
							//FBXSDK_printf("normals for vertex[%d]: %f %f %f %f \n", lVertexIndex, lNormal[0], lNormal[1], lNormal[2], lNormal[3]);
							//add your custom code here, to output normals or get them into a list, such as KArrayTemplate<FbxVector4>
							//. . .
						}//end for lVertexIndex
					}//end eByControlPoint
					//mapping mode is by polygon-vertex.
					//we can get normals by retrieving polygon-vertex.
					else if (lNormalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
					{
						int lIndexByPolygonVertex = 0;
						//Let's get normals of each polygon, since the mapping mode of normal element is by polygon-vertex.
						for (int lPolygonIndex = 0; lPolygonIndex < mesh->GetPolygonCount(); lPolygonIndex++)
						{
							//get polygon size, you know how many vertices in current polygon.
							int lPolygonSize = mesh->GetPolygonSize(lPolygonIndex);
							//retrieve each vertex of current polygon.
							for (int i = 0; i < lPolygonSize; i++)
							{
								int lNormalIndex = 0;
								//reference mode is direct, the normal index is same as lIndexByPolygonVertex.
								if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
									lNormalIndex = lIndexByPolygonVertex;

								//reference mode is index-to-direct, get normals by the index-to-direct
								if (lNormalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect)
									lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndexByPolygonVertex);

								//Got normals of each polygon-vertex.
								FbxVector4 lNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
								//FBXSDK_printf("normals for polygon[%d]vertex[%d]: %f %f %f %f \n", lPolygonIndex, i, lNormal[0], lNormal[1], lNormal[2], lNormal[3]);
								//add your custom code here, to output normals or get them into a list, such as KArrayTemplate<FbxVector4>
								//. . .

								lIndexByPolygonVertex++;
							}//end for i //lPolygonSize
						}//end for lPolygonIndex //PolygonCount

					}//end eByPolygonVertex
				}
			}
		}
//#elif

		importer = new Assimp::Importer();	
		//importer->SetPropertyFloat("PP_GSN_MAX_SMOOTHING_ANGLE", 120);
		aiScene const* scene = importer->ReadFile(modelDesc->colladaPath, aiProcessPreset_TargetRealtime_Fast);
		if (!scene) {
			std::string errorString = importer->GetErrorString();
			OutputDebugStringA((std::string("Failed to import \"") + modelDesc->colladaPath + std::string("\": ") + errorString).c_str());
			throw std::exception();
		}
		else
			OutputDebugStringA((std::string("Imported \"") + modelDesc->colladaPath + std::string("\"\n")).c_str());

		if (!scene->HasMeshes()) {
			OutputDebugStringA(std::string("The scene has no meshes !").c_str());
			delete importer;
			throw std::exception();
		}

		modelDesc->meshesNr = scene->mNumMeshes;
		modelDesc->verticesNrsPerMesh = new unsigned int[scene->mNumMeshes]();
		modelDesc->texesNrsPerMesh = new unsigned int[scene->mNumMeshes];
		modelDesc->bonesNrsPerMesh = new unsigned int[scene->mNumMeshes];
		modelDesc->facesNrsPerMesh = new unsigned int[scene->mNumMeshes];
		modelDesc->extraColorsNrsPerMesh = new unsigned int[scene->mNumMeshes]();
		modelDesc->extraColors = new float** [scene->mNumMeshes]();
		
		unsigned int vertexIdxOverall = 0;
		managedImportedData = gcnew ManagedImportedData;
		managedImportedData->meshesGeometries = gcnew array<MeshGeometry3D^>(scene->mNumMeshes);
		managedImportedData->meshesVertices = gcnew array<array<Point3D>^>(scene->mNumMeshes);
		managedImportedData->meshesVertexIndices = gcnew array<array<unsigned short>^>(scene->mNumMeshes);
		for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
			const aiMesh* mesh = scene->mMeshes[meshIdx];
			if (!mesh->HasPositions() || !mesh->HasFaces()) {
				if (!mesh->HasPositions())
					OutputDebugStringA(std::string("The mesh has no positions !").c_str());
				else if (!mesh->HasFaces())
					OutputDebugStringA(std::string("The mesh has no faces !").c_str());

				delete importer;
				throw std::exception();
			}

			modelDesc->verticesNrsPerMesh[meshIdx] += mesh->mNumVertices;
			modelDesc->verticesNr += mesh->mNumVertices;
			managedImportedData->meshesGeometries[meshIdx] = gcnew MeshGeometry3D();
			managedImportedData->meshesVertices[meshIdx] = gcnew array<Point3D>(mesh->mNumVertices);			
			for (unsigned int vertexIdx = 0; vertexIdx < mesh->mNumVertices; vertexIdx++) {
				aiVector3D vertex = mesh->mVertices[vertexIdx];				
				managedImportedData->meshesVertices[meshIdx][vertexIdx] = Point3D(vertex.x, vertex.y, vertex.z);
				managedImportedData->meshesGeometries[meshIdx]->Positions->Add(Point3D(vertex.x, vertex.y, vertex.z));
			}			

			
			if (mesh->HasNormals()) {
				managedImportedData->meshesGeometries[meshIdx]->Normals = gcnew Vector3DCollection(modelDesc->verticesNr);
				for (unsigned int vertexIdx = 0; vertexIdx < mesh->mNumVertices; vertexIdx++) {
					aiVector3D normal = mesh->mNormals[vertexIdx];
					managedImportedData->meshesGeometries[meshIdx]->Normals->Add(Vector3D(normal.x, normal.y, normal.z));
				}
			}		

			modelDesc->facesNrsPerMesh[meshIdx] = mesh->mNumFaces;
			modelDesc->facesNr += mesh->mNumFaces;									
			managedImportedData->meshesVertexIndices[meshIdx] = gcnew array<unsigned short>(modelDesc->facesNr * 3);							
			for (unsigned int faceIdx = 0; faceIdx < mesh->mNumFaces; faceIdx++) {
				unsigned int* indices = mesh->mFaces[faceIdx].mIndices;
				managedImportedData->meshesGeometries[meshIdx]->TriangleIndices->Add(indices[0]);
				managedImportedData->meshesGeometries[meshIdx]->TriangleIndices->Add(indices[1]);
				managedImportedData->meshesGeometries[meshIdx]->TriangleIndices->Add(indices[2]);
				managedImportedData->meshesVertexIndices[meshIdx][faceIdx*3	  ] = indices[0];
				managedImportedData->meshesVertexIndices[meshIdx][faceIdx*3 + 1] = indices[1];
				managedImportedData->meshesVertexIndices[meshIdx][faceIdx*3 + 2] = indices[2];				
			}
			
			modelDesc->progIdx = 0;

			modelDesc->bonesNrsPerMesh[meshIdx] = mesh->mNumBones;
			modelDesc->bonesNr += mesh->mNumBones;
		}

		modelDesc->animationsNr = 0; // scene->mNumAnimations;
		if (modelDesc->animationsNr > 0) {
			modelDesc->animationsDescs = new ModelDesc::AnimationDesc[modelDesc->animationsNr];
			/*
			for (unsigned int animationIdx = 0; animationIdx < modelDesc->animationsNr; animationIdx++) {
				aiAnimation const* animation = scene->mAnimations[animationIdx];
				animations[animationIdx].dur = animation->mDuration;
				animations[animationIdx].ticksPerSecond = animation->mTicksPerSecond;

				aiNode* nodesIt = scene->mRootNode;
				genNodesCountRecurse(scene->mRootNode, NodesCount & nodesCount, 0);
				NodesCount nodesCount = genNodesCount(nodesIt);
				animations[animationIdx].nodesNr = nodesCount.nodesNr;
				animations[animationIdx].treeDepthMax = nodesCount.treeDepthMax;
				unsigned int* nodesToChannelsIdxsMap = new unsigned int[nodesCount.nodesNr];
				mapNodesToChannelsIdxs(nodesIt, animation, nodesToChannelsIdxsMap);
				animations[animationIdx].TransformatsHierarchyBuffer = new Animation::TransformatsHierarchyNode[nodesCount.nodesNr];
				unsigned int* childrenIdxsStack = new unsigned int[nodesCount.treeDepthMax]();
				Animation::TransformatsHierarchyNode** transformatsHierarchyNodesStack = new Animation::TransformatsHierarchyNode * [nodesCount.treeDepthMax + 1];
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
				animations[animationIdx].scales = new glm::vec3 * [channelsNr];
				animations[animationIdx].rots = new glm::quat * [channelsNr];
				animations[animationIdx].translations = new glm::vec3 * [channelsNr];
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
			*/
		}
		else
			modelDesc->animationsDescs = nullptr;																

		glm::vec3* vec3Arr = new glm::vec3[modelDesc->verticesNr];
		for (unsigned int meshIdx = 0; meshIdx < scene->mNumMeshes; meshIdx++) {
			aiMesh* mesh = scene->mMeshes[meshIdx];
			for (unsigned int vertexIdx = 0; vertexIdx < mesh->mNumVertices; vertexIdx++) {
				aiVector3D vertex = mesh->mVertices[vertexIdx];
				vec3Arr[vertexIdxOverall++] = { vertex.x, vertex.y, vertex.z };
			}
		}
		
		BoundingSphere bs = BoundingSphere::calcBoundingSphereEfficient(vec3Arr, modelDesc->verticesNr);
		modelDesc->colliderData.boundingSphereCenter = bs.getCenter();
		managedImportedData->boundingSphereCenter = Point3D(modelDesc->colliderData.boundingSphereCenter.x, modelDesc->colliderData.boundingSphereCenter.y, modelDesc->colliderData.boundingSphereCenter.z);
		managedImportedData->boundingSphereRadius = modelDesc->colliderData.boundingSphereRadius = bs.getRadius();

		AABB3D aabb3D = AABB3D::calcAABB(vec3Arr, modelDesc->verticesNr);
		modelDesc->colliderData.aabb3DMinVertex = aabb3D.getMinVertex();
		managedImportedData->aabb3DMinVertex = Point3D(modelDesc->colliderData.aabb3DMinVertex.x, modelDesc->colliderData.aabb3DMinVertex.y, modelDesc->colliderData.aabb3DMinVertex.z);
		modelDesc->colliderData.aabb3DMaxVertex = aabb3D.getMaxVertex();
		managedImportedData->aabb3DMaxVertex = Point3D(modelDesc->colliderData.aabb3DMaxVertex.x, modelDesc->colliderData.aabb3DMaxVertex.y, modelDesc->colliderData.aabb3DMaxVertex.z);

		float verticesSpreadX = modelDesc->colliderData.aabb3DMaxVertex.x - modelDesc->colliderData.aabb3DMinVertex.x;
		float verticesSpreadY = modelDesc->colliderData.aabb3DMaxVertex.y - modelDesc->colliderData.aabb3DMinVertex.y;
		float verticesSpreadZ = modelDesc->colliderData.aabb3DMaxVertex.z - modelDesc->colliderData.aabb3DMinVertex.z;
		managedImportedData->boundingCapsuleAxisVec = Vector3D(0, 0, 0);
		if (verticesSpreadX > verticesSpreadY) {
			if (verticesSpreadX > verticesSpreadZ) {				
				if (verticesSpreadZ > verticesSpreadY) {
					// largest: X, 2nd: Z						
					managedImportedData->boundingCapsuleAxisVec.X = 1;					
					managedImportedData->boundingCapsuleHeight = verticesSpreadX;
					managedImportedData->boundingCapsuleRadius = 0.5 * verticesSpreadZ;
				}
				else {
					// largest: X, 2nd: Y
					managedImportedData->boundingCapsuleAxisVec.X = 1;
					managedImportedData->boundingCapsuleHeight = verticesSpreadX;
					managedImportedData->boundingCapsuleRadius = 0.5 * verticesSpreadY;
				}
			}
			else {
				// largest: Z, 2nd: X
				managedImportedData->boundingCapsuleAxisVec.Z = 1;
				managedImportedData->boundingCapsuleHeight = verticesSpreadZ;
				managedImportedData->boundingCapsuleRadius = 0.5 * verticesSpreadX;
			}			
		}
		else if (verticesSpreadY > verticesSpreadZ) {
			if (verticesSpreadX > verticesSpreadZ) {
				// largest: Y, 2nd: X
				managedImportedData->boundingCapsuleAxisVec.Y = 1;
				managedImportedData->boundingCapsuleHeight = verticesSpreadY;
				managedImportedData->boundingCapsuleRadius = 0.5 * verticesSpreadX;
			}
			else { // largest: Y, 2nd: Z
				managedImportedData->boundingCapsuleAxisVec.Y = 1;
				managedImportedData->boundingCapsuleHeight = verticesSpreadY;
				managedImportedData->boundingCapsuleRadius = 0.5 * verticesSpreadZ;
			}
		}
		else {
			// largest: Z, 2nd: Y
			managedImportedData->boundingCapsuleAxisVec.Z = 1;
			managedImportedData->boundingCapsuleHeight = verticesSpreadZ;
			managedImportedData->boundingCapsuleRadius = 0.5 * verticesSpreadY;
		}		
		managedImportedData->boundingCapsuleCenter = Point3D(0.5 * (modelDesc->colliderData.aabb3DMinVertex.x + modelDesc->colliderData.aabb3DMaxVertex.x),
															 0.5 * (modelDesc->colliderData.aabb3DMinVertex.y + modelDesc->colliderData.aabb3DMaxVertex.y),
															 0.5 * (modelDesc->colliderData.aabb3DMinVertex.z + modelDesc->colliderData.aabb3DMaxVertex.z));

		modelDesc->colliderData.collisionPrimitive3DType = CollisionPrimitive3DType::NO_3D_COLLIDER;
		modelDesc->colliderData.collisionPrimitive2DType = CollisionPrimitive2DType::NO_2D_COLLIDER;
		AABB2D aabb2D = AABB2D::calcAABB(vec3Arr, modelDesc->verticesNr);		
		modelDesc->colliderData.aabb2DMinVertex = aabb2D.getMinVertex();
		modelDesc->colliderData.aabb2DMaxVertex = aabb2D.getMaxVertex();		

		delete[] vec3Arr;					
#endif
	}	

	ModelAssetGen::~ModelAssetGen()
	{
		delete importer;		
		delete modelDesc;		
	}

	void ModelAssetGen::assignExtraColors(array<array<array<float>^>^>^ extraColors)
	{
		modelDesc->verticesColorsNrTotal = 0;
		for (unsigned int meshIdx = 0; meshIdx < extraColors->Length; meshIdx++) {
			if (modelDesc->extraColorsNrsPerMesh[meshIdx] > 0) {
				for (unsigned int extraColorIdx = 0; extraColorIdx < modelDesc->extraColorsNrsPerMesh[meshIdx]; extraColorIdx++)
					delete[] modelDesc->extraColors[meshIdx][extraColorIdx];
				delete[] modelDesc->extraColors[meshIdx];
			}

			modelDesc->extraColorsNrsPerMesh[meshIdx] = extraColors[meshIdx]->GetLength(0);
			if (modelDesc->extraColorsNrsPerMesh[meshIdx] > 0) {				
				modelDesc->extraColors[meshIdx] = new float* [modelDesc->extraColorsNrsPerMesh[meshIdx]];
				for (unsigned int extraColorIdx = 0; extraColorIdx < modelDesc->extraColorsNrsPerMesh[meshIdx]; extraColorIdx++) {
					modelDesc->extraColors[meshIdx][extraColorIdx] = new float[4];
					Marshal::Copy(extraColors[meshIdx][extraColorIdx], 0, System::IntPtr((void*)modelDesc->extraColors[meshIdx][extraColorIdx]), 4);
				}				
			}

			modelDesc->verticesColorsNrTotal += (modelDesc->extraColorsNrsPerMesh[meshIdx] + 1) * importer->GetScene()->mMeshes[meshIdx]->mNumVertices;
		}
	}

	void ModelAssetGen::assignProgIdx(unsigned int progIdx)
	{
		modelDesc->progIdx = progIdx;
	}

	void ModelAssetGen::clearCollisionPrimitive3D()
	{
		modelDesc->colliderData.collisionPrimitive3DType = CollisionPrimitive3DType::NO_3D_COLLIDER;
	}

	void ModelAssetGen::assignCollisionBox(Point3D^ center, Point3D^ scale)
	{
		modelDesc->colliderData.collisionPrimitive3DType = CollisionPrimitive3DType::BOX;
		modelDesc->colliderData.collisionPrimitive3dData.collisionBoxData = { marshalPoint3D(center), marshalPoint3D(scale) };
	}

	void ModelAssetGen::assignCollisionSphere(Point3D^ center, float radius)
	{
		modelDesc->colliderData.collisionPrimitive3DType = CollisionPrimitive3DType::SPHERE;
		modelDesc->colliderData.collisionPrimitive3dData.collisionSphereData = { marshalPoint3D(center), radius };
	}

	void ModelAssetGen::assignCollisionCapsule(Point3D^ center1, Vector3D^ axisVec, float radius)
	{
		modelDesc->colliderData.collisionPrimitive3DType = CollisionPrimitive3DType::CAPSULE;
		modelDesc->colliderData.collisionPrimitive3dData.collisionCapsuleData = { marshalPoint3D(center1), marshalVector3D(axisVec), radius };
	}

	void ModelAssetGen::clearCollisionPrimitive2D()
	{
		modelDesc->colliderData.collisionPrimitive2DType = CollisionPrimitive2DType::NO_2D_COLLIDER;
	}

	void ModelAssetGen::assignCollisionRect(Point^ center, Point^ scale)
	{
		modelDesc->colliderData.collisionPrimitive2DType = CollisionPrimitive2DType::RECT;
		modelDesc->colliderData.collisionPrimitive2dData.collisionRectData = { marshalPoint(center), marshalPoint(scale) };
	}

	void ModelAssetGen::assignCollisionCircle(Point^ center, float radius)
	{
		modelDesc->colliderData.collisionPrimitive2DType = CollisionPrimitive2DType::CIRCLE;
		modelDesc->colliderData.collisionPrimitive2dData.collisionCircleData = { marshalPoint(center), radius };
	}

	void ModelAssetGen::assignCollisionStadium(Point^ center1, Vector^ axisVec, float radius)
	{
		modelDesc->colliderData.collisionPrimitive2DType = CollisionPrimitive2DType::STADIUM;
		modelDesc->colliderData.collisionPrimitive2dData.collisionStadiumData = { marshalPoint(center1), marshalVector(axisVec), radius };
	}

	void ModelAssetGen::genCorium3dAsset(System::String^ path)
	{
		std::filesystem::path savePath(systemStringToAnsiString(path));
		writeModelDesc((savePath / std::filesystem::path(modelDesc->colladaPath).stem()).string() + ".moddesc", *modelDesc);		
	}	

} // namespace Corium3D