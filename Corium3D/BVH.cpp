#include "BVH.h"

#include "ServiceLocator.h"
#include <math.h>
#include <glm/gtx/norm.hpp>
#include <limits.h>
#include <string>

using namespace Corium3DUtils;

namespace Corium3D {

const double EPSILON_ZERO = 1e-5;
const float RAY_DESTINATION_EXTRA_FACTOR = 0.01f;
// #define RAY_EXTENSION_FACTOR

	BVH::BVH(unsigned int staticGameLmnts3DNrMax, unsigned int mobileGameLmnts3DNrMax, unsigned int staticGameLmnts2DNrMax, unsigned int mobileGameLmnts2DNrMax, unsigned int collisions2DNrMax, unsigned int collisions3DNrMax) {
		// 3D pools	
		branchNodes3DPool = new ObjPool<Node3D>(staticGameLmnts3DNrMax + mobileGameLmnts3DNrMax - 2);
		staticNodes3DPool = new ObjPool<DataNode3D>(staticGameLmnts3DNrMax);
		mobileNodes3DPool = new ObjPool<MobileGameLmntDataNode3D>(mobileGameLmnts3DNrMax);		
		// 2D pools	
		branchNodes2DPool = new ObjPool<Node2D>(staticGameLmnts2DNrMax + mobileGameLmnts2DNrMax - 2);
		staticNodes2DPool = new ObjPool<DataNode2D>(staticGameLmnts2DNrMax);
		mobileNodes2DPool = new ObjPool<MobileGameLmntDataNode2D>(mobileGameLmnts2DNrMax);	
	
		// theoretically possible maximum collisions number: mobileGameLmntsNrMax*staticGameLmntsNrMax + mobileGameLmntsNrMax*[mobileGameLmntsNrMax - 1)]/2	
		collisionsBuffers3D.collisionsData.collisionsDataBuffer = new CollisionData<glm::vec3>[collisions3DNrMax];
		collisionsBuffers3D.collisionsData.detachmentsDataBuffer = new CollisionData<glm::vec3>[collisions3DNrMax];
		collisionsBuffers3D.broadPhaseResBuffer.collisionPrimitivesDuos = new std::array<CollisionPrimitive<glm::vec3>*, 2>[collisions3DNrMax];
		collisionsBuffers3D.broadPhaseResBuffer.collisionsData = new CollisionData<glm::vec3>[collisions3DNrMax];
		collisionsBuffers3D.collisionsRecord = new SearchTreeAVL<CollisionData<glm::vec3>>(collisions3DNrMax);
		collisionsBuffers3D.collisionsRecordIt = new SearchTreeAVL<CollisionData<glm::vec3>>::InOrderIt(*collisionsBuffers3D.collisionsRecord);

		collisionsBuffers2D.collisionsData.collisionsDataBuffer = new CollisionData<glm::vec2>[collisions2DNrMax];
		collisionsBuffers2D.collisionsData.detachmentsDataBuffer = new CollisionData<glm::vec2>[collisions2DNrMax];
		collisionsBuffers2D.broadPhaseResBuffer.collisionPrimitivesDuos = new std::array<CollisionPrimitive<glm::vec2>*, 2>[collisions2DNrMax];
		collisionsBuffers2D.broadPhaseResBuffer.collisionsData = new CollisionData<glm::vec2>[collisions2DNrMax];
		collisionsBuffers2D.collisionsRecord = new SearchTreeAVL<CollisionData<glm::vec2>>(collisions2DNrMax);
		collisionsBuffers2D.collisionsRecordIt = new SearchTreeAVL<CollisionData<glm::vec2>>::InOrderIt(*collisionsBuffers2D.collisionsRecord);
	}

	BVH::~BVH() {
		delete collisionsBuffers2D.collisionsRecordIt;
		delete collisionsBuffers2D.collisionsRecord;
		delete[] collisionsBuffers2D.broadPhaseResBuffer.collisionsData;
		delete[] collisionsBuffers2D.broadPhaseResBuffer.collisionPrimitivesDuos;
		delete[] collisionsBuffers2D.collisionsData.collisionsDataBuffer;
		delete[] collisionsBuffers2D.collisionsData.detachmentsDataBuffer;
		delete collisionsBuffers3D.collisionsRecordIt;
		delete collisionsBuffers3D.collisionsRecord;
		delete[] collisionsBuffers3D.broadPhaseResBuffer.collisionsData;
		delete[] collisionsBuffers3D.broadPhaseResBuffer.collisionPrimitivesDuos;
		delete[] collisionsBuffers3D.collisionsData.collisionsDataBuffer;
		delete[] collisionsBuffers3D.collisionsData.detachmentsDataBuffer;
		delete mobileNodes2DPool;
		delete staticNodes2DPool;	
		delete branchNodes2DPool;
		delete mobileNodes3DPool;
		delete staticNodes3DPool;	
		delete branchNodes3DPool;
	}

	void BVH::refitBPsDueToUpdate() {
		if (mobileNodes3DRoot != NULL && !mobileNodes3DRoot->isLeaf())
			doRefitBPsDueToUpdate<AABB3DRotatable>(mobileNodes3DRoot);
		if (mobileNodes2DRoot != NULL && !mobileNodes2DRoot->isLeaf())
			doRefitBPsDueToUpdate<AABB2DRotatable>(mobileNodes2DRoot);	
	}

	BVH::DataNode3D* BVH::insert(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume) {	
		DataNode3D* newNode = staticNodes3DPool->acquire(aabb, boundingSphere, modelIdx, instanceIdx, collisionVolume);
		doInsert<AABB3DRotatable, Node3D>(&staticNodes3DRoot, newNode, branchNodes3DPool, staticNodes3DNr);
	
		return newNode;
	}

	BVH::MobileGameLmntDataNode3D* BVH::insert(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume, PhysicsEngine::MobilityInterface const& mobilityInterface) {
		// T const& data
		MobileGameLmntDataNode3D* newNode = mobileNodes3DPool->acquire(aabb, boundingSphere, modelIdx, instanceIdx, collisionVolume, mobilityInterface);
		doInsert<AABB3DRotatable, Node3D>(&mobileNodes3DRoot, newNode, branchNodes3DPool, mobileNodes3DNr);

		return newNode;
	}

	void BVH::remove(DataNode3D* node) {
		doRemove<AABB3DRotatable, Node3D>(&staticNodes3DRoot, node, branchNodes3DPool, staticNodes3DNr);
		staticNodes3DPool->release(node);	
	}

	void BVH::remove(MobileGameLmntDataNode3D* node) {
		doRemove<AABB3DRotatable, Node3D>(&mobileNodes3DRoot, node, branchNodes3DPool, mobileNodes3DNr);
		mobileNodes3DPool->release(node);
	}

	/*
	void BVH::refitBPsDueToUpdate() {
		if (mobileNodes3DRoot == NULL || mobileNodes3DRoot->isLeaf())
			return;

		Node3D* nodesIt = mobileNodes3DRoot;
		while (!nodesIt->children[0]->isLeaf())
			nodesIt = nodesIt->children[0];

		do {
			if (!nodesIt->children[1]->isLeaf()) {
				nodesIt = nodesIt->children[1];
				while (!nodesIt->children[0]->isLeaf())
					nodesIt = nodesIt->children[0];
			}
			else {
				reduceSahAndRefitBVs<AABB3DRotatable>(nodesIt);
				while (nodesIt->parent != NULL && nodesIt == nodesIt->parent->children[1]) {
					nodesIt = nodesIt->parent;
					reduceSahAndRefitBVs<AABB3DRotatable>(nodesIt);
				}
				nodesIt = nodesIt->parent;
			}
		} while (nodesIt != NULL);
	}

	BVH::DataNode3D* BVH::insert(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume) {
		// T const& data
		DataNode3D* newNode = staticNodes3DPool->acquire(aabb, boundingSphere, modelIdx, instanceIdx, collisionVolume);
		if (staticNodes3DRoot != NULL) {
			if (!staticNodes3DRoot->isLeaf()) {
				Node3D* newNodeSibling = findNewNodeSibling<AABB3DRotatable>(staticNodes3DRoot, newNode);
				Node3D* newNodeSiblingParent = newNodeSibling->parent;
				Node3D* newBranch = branchNodes3DPool->acquire(newNodeSibling, newNode);
				if (newNodeSiblingParent != NULL) {
					if (newNodeSiblingParent->children[0] == newNodeSibling)
						newNodeSiblingParent->replaceChild(newBranch, 0);
					else
						newNodeSiblingParent->replaceChild(newBranch, 1);
				}
				else {
					staticNodes3DRoot = newBranch;
					setSubtreeDepthValues<AABB3DRotatable>(staticNodes3DRoot);
				}

				//balanceTreeUpwards(foundSiblingParent);
			}
			else
				staticNodes3DRoot = branchNodes3DPool->acquire(staticNodes3DRoot, newNode);

			staticNodes3DNr += 2;
		}
		else {
			staticNodes3DRoot = newNode;
			staticNodes3DNr++;
		}

		return newNode;
	}

	BVH::MobileGameLmntDataNode3D* BVH::insert(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume, PhysicsEngine::MobilityInterface const& mobilityInterface) {
		// T const& data
		MobileGameLmntDataNode3D* newNode = mobileNodes3DPool->acquire(aabb, boundingSphere, modelIdx, instanceIdx, collisionVolume, mobilityInterface);
		if (mobileNodes3DRoot != NULL) {
			if (!mobileNodes3DRoot->isLeaf()) {
				Node3D* newNodeSibling = findNewNodeSibling<AABB3DRotatable>(mobileNodes3DRoot, newNode);
				Node3D* newNodeSiblingParent = newNodeSibling->parent;
				Node3D* newBranch = branchNodes3DPool->acquire(newNodeSibling, newNode);
				if (newNodeSiblingParent != NULL) {
					if (newNodeSiblingParent->children[0] == newNodeSibling)
						newNodeSiblingParent->replaceChild(newBranch, 0);
					else
						newNodeSiblingParent->replaceChild(newBranch, 1);
				}
				else {
					mobileNodes3DRoot = newBranch;
					setSubtreeDepthValues(mobileNodes3DRoot);
				}

				//balanceTreeUpwards(foundSiblingParent);
			}
			else
				mobileNodes3DRoot = branchNodes3DPool->acquire(mobileNodes3DRoot, newNode);

			mobileNodes3DNr += 2;
		}
		else {
			mobileNodes3DRoot = newNode;
			mobileNodes3DNr++;
		}

		return newNode;
	}

	void BVH::remove(DataNode3D* node) {
		if (staticNodes3DRoot != node) {
			Node3D* nodeParent = node->parent;
			Node3D* nodeSibling;
			if (nodeParent->children[0] == node)
				nodeSibling = nodeParent->children[1];
			else
				nodeSibling = nodeParent->children[0];

			if (staticNodes3DRoot != nodeParent) {
				Node3D* nodeGrandParent = nodeParent->parent;
				if (nodeGrandParent->children[0] == nodeParent)
					nodeGrandParent->replaceChild(nodeSibling, 0);
				else
					nodeGrandParent->replaceChild(nodeSibling, 1);

				//balanceTreeUpwards(nodeParent->parent);
			}
			else {
				staticNodes3DRoot = nodeSibling;
				nodeSibling->parent = NULL;
			}

			staticNodes3DPool->release(node);
			branchNodes3DPool->release(nodeParent);
			staticNodes3DNr -= 2;
		}
		else {
			staticNodes3DRoot = NULL;
			staticNodes3DPool->release(node);
			staticNodes3DNr--;
		}
	}

	void BVH::remove(MobileGameLmntDataNode3D* node) {
		if (mobileNodes3DRoot != node) {
			Node3D* nodeParent = node->parent;
			Node3D* nodeSibling;
			if (nodeParent->children[0] == node)
				nodeSibling = nodeParent->children[1];
			else
				nodeSibling = nodeParent->children[0];

			if (mobileNodes3DRoot != nodeParent) {
				Node3D* nodeGrandParent = nodeParent->parent;
				if (nodeGrandParent->children[0] == nodeParent)
					nodeGrandParent->replaceChild(nodeSibling, 0);
				else
					nodeGrandParent->replaceChild(nodeSibling, 1);

				//balanceTreeUpwards(nodeParent->parent);
			}
			else {
				mobileNodes3DRoot = nodeSibling;
				nodeSibling->parent = NULL;
				nodeSibling->escapeNode = NULL;
			}

			mobileNodes3DPool->release(node);
			branchNodes3DPool->release(nodeParent);
			mobileNodes3DNr -= 2;
		}
		else {
			mobileNodes3DRoot = NULL;
			mobileNodes3DPool->release(node);
			mobileNodes3DNr--;
		}
	}

	BVH::CollisionsData const& BVH::getCollisionsData3D() {
		// BROAD PHASE //
		Node3D* leftSubtreeIt = mobileNodes3DRoot;
		Node3D* rightSubtreeIt = mobileNodes3DRoot;
		while (rightSubtreeIt) {
			if (leftSubtreeIt == rightSubtreeIt) {
				if (rightSubtreeIt->isLeaf()) {
					// can happen only after a last iteration of the stackless algorithm
					leftSubtreeIt = leftSubtreeIt->lastLeftChildAncestor;
					rightSubtreeIt = rightSubtreeIt->escapeNode;
				}
				else {
					Node3D* leftChild = leftSubtreeIt->children[0];
					Node3D* rightChild = leftSubtreeIt->children[1];
					if (leftChild->isLeaf()) {
						Node3D* rightChild = leftSubtreeIt->children[1];
						if (rightChild->isLeaf()) {
							if ((leftChild->aabb).doesIntersect(rightChild->aabb))
								recordBroadPhaseCollisionIdxsDuo<DataNode3D>(static_cast<DataNode3D*>(leftChild), static_cast<DataNode3D*>(rightChild));
							rightSubtreeIt = rightSubtreeIt->escapeNode;
							if (leftSubtreeIt != mobileNodes3DRoot && leftSubtreeIt->isRightChild())
								leftSubtreeIt = leftSubtreeIt->lastLeftChildAncestor;
						}
						else {
							runLeafAlgo<AABB3DRotatable, DataNode3D>(leftChild, rightChild);
							leftSubtreeIt = rightSubtreeIt = rightChild;
						}
					}
					else
						leftSubtreeIt = rightSubtreeIt = leftChild;
				}
			}
			else
				runStacklessAlgoIteration<AABB3DRotatable, DataNode3D>(&leftSubtreeIt, &rightSubtreeIt);
		}

		// find collisions between mobile and static game elements
	
		//leftSubtreeIt = staticNodesRoot;
		//rightSubtreeIt = mobileNodesRoot;
		//while (rightSubtreeIt)
		//	runStacklessAlgoIteration(collisionsDuos, &leftSubtreeIt, &rightSubtreeIt);
		//

		// ================================================================================================
		//										SEARCH TREE PRINTER
		// ================================================================================================
		// static const CollisionData troubleMaker(1, 7, 1, 6);
		// collisionsRecordIt->reset();
		// while (collisionsRecordIt->hasNext()) {
		// 	CollisionData gameLmntsIdxsDuo = collisionsRecordIt->next();
		// 	if (troubleMaker == gameLmntsIdxsDuo) {
		// 		collisionsRecordIt->reset();
		// 		ServiceLocator::getLogger().logd("", "");
		//		ServiceLocator::getLogger().logd("", "CollisionsDuos Search Tree Printout");
		// 		ServiceLocator::getLogger().logd("", "===================================");
		// 		while (collisionsRecordIt->hasNext()) {
		// 			gameLmntsIdxsDuo = collisionsRecordIt->next();
		// 			ServiceLocator::getLogger().logd("", (std::to_string(gameLmntsIdxsDuo.instanceIdx1) + "<->" + std::to_string(gameLmntsIdxsDuo.instanceIdx2) + ": " + std::to_string(gameLmntsIdxsDuo.collisionState)).c_str());
		// 		}
		//		break;
		// 	}
		// }
		//  ================================================================================================ 

		// NARROW PHASE //		
		for (unsigned int duoIdx = 0; duoIdx < broadPhaseResBuffer.collisionsNr; duoIdx++) {
			if (broadPhaseResBuffer.collisionVolumesDuos[duoIdx][0]->testCollision((broadPhaseResBuffer.collisionVolumesDuos[duoIdx][1]))) { //, broadPhaseResBuffer.collisionsData[duoIdx].contactManifold
				CollisionData& returnedCollisionDuo = collisionsRecord->insert(broadPhaseResBuffer.collisionsData[duoIdx]);
				if (returnedCollisionDuo.collisionState == CollisionData::CollisionState::end)
					returnedCollisionDuo.collisionState = CollisionData::CollisionState::persist;
			}
		}
		broadPhaseResBuffer.collisionsNr = 0;

		collisionsRecordIt->reset();
		collisionsData.collisionsNr = collisionsData.detachmentsNr = 0;
		while (collisionsRecordIt->hasNext()) {
			CollisionData& collisionData = collisionsRecordIt->next();
			switch (collisionData.collisionState) {
			case CollisionData::CollisionState::start:
				collisionsData.collisionsDataBuffer[collisionsData.collisionsNr++] = collisionData;
			case CollisionData::CollisionState::persist:
				collisionData.collisionState = CollisionData::CollisionState::end;
				break;
			case  CollisionData::CollisionState::end:
				collisionsData.detachmentsDataBuffer[collisionsData.detachmentsNr++] = collisionData;
				collisionsRecordIt->removeCurr();
				break;
			}
		}

		return collisionsData;
		}
	*/

	BVH::CollisionsData<glm::vec3> const& BVH::getCollisionsData3D() {
		return doCollisionsSearch<AABB3DRotatable, DataNode3D, glm::vec3>(staticNodes3DRoot, mobileNodes3DRoot, collisionsBuffers3D);
		//return collisionsBuffers3D.collisionsData;
	}

	/*
	inline unsigned char calcOutCode(glm::vec3 const& vertex, glm::vec3 const& boxMin, glm::vec3 const& boxMax) {
		return ((vertex.x < boxMin.x)	  )	| ((boxMax.x < vertex.x) << 1) |
			   ((vertex.y < boxMin.y) << 2) | ((boxMax.y < vertex.y) << 3) |
			   ((vertex.z < boxMin.z) << 4) | ((boxMax.z < vertex.z) << 5);
	}

	inline unsigned char calcOutCodeSpecificDim(glm::vec3 const& vertex, glm::vec3 const& boxMin, glm::vec3 const& boxMax, unsigned int dim) {	
		return ((vertex[dim] < boxMin[dim]) << 2 * dim) | ((boxMax[dim] < vertex[dim]) << 2*dim + 1);
	}
	*/

	inline bool testRayAabbIntersection(glm::vec3 const& rayOrigin, unsigned char rayOriginOutCode, glm::vec3 const& rayDest, unsigned char rayDestOutCode, glm::vec3 const& aabbMin, glm::vec3 const& aabbMax, float& rayCollisionFactorOut) {
	
		return false;
	}

	struct RayDirectionStandardAxesCrossProducts {
		glm::vec3 crossProducts[3];
		glm::vec3 crossProductsAbs[3];
	};

	inline bool testRayAabbIntersection(glm::vec3 const& rayOrigin, glm::vec3 const& rayDest, RayDirectionStandardAxesCrossProducts const& rayDirectionStandardAxesCrossProducts, glm::vec3 const& aabbCenter, glm::vec3 const& aabbExtent) {
		glm::vec3 rayOriginRel = rayOrigin - aabbCenter;
		glm::vec3 rayDestRel = rayDest - aabbCenter;
		for (unsigned int axIdx = 0; axIdx < 3; axIdx++) {
			float originProj = rayOriginRel[axIdx];
			float destProj = rayDestRel[axIdx];
			if (abs(originProj + destProj) > abs(originProj - destProj) + 2*aabbExtent[axIdx])
				return false;
		}

		//if ( (EPSILON_ZERO < rayDirectionAbs.y && EPSILON_ZERO < rayDirectionAbs.z) ||
		//	 (EPSILON_ZERO < rayDirectionAbs.x && (EPSILON_ZERO < rayDirectionAbs.y || EPSILON_ZERO < rayDirectionAbs.z)) ) {	
		for (unsigned int crossProductAxIdx = 0; crossProductAxIdx < 3; crossProductAxIdx++) {				
			if (fabs(dot(rayDirectionStandardAxesCrossProducts.crossProducts[crossProductAxIdx], rayOriginRel)) > dot(rayDirectionStandardAxesCrossProducts.crossProductsAbs[crossProductAxIdx], aabbExtent))
				return false;		
		}
		//}

		return true;	
	}

	BVH::RayCollisionData const& BVH::getRayCollisionData(glm::vec3 const& rayOrigin, glm::vec3 const& rayDirection) {
		rayCollisionData.hasCollided = false;

	#if DEBUG
		AABB3D sceneAABB;
		if (staticNodes3DRoot && mobileNodes3DRoot) {
			sceneAABB.combine(staticNodes3DRoot->aabb);
			sceneAABB.combine(mobileNodes3DRoot->aabb);
		}
		else if (staticNodes3DRoot)
			sceneAABB.combine(staticNodes3DRoot->aabb);
		else if (mobileNodes3DRoot)
			sceneAABB.combine(mobileNodes3DRoot->aabb);
		else
			return rayCollisionData;	

	#else
		AABB3D sceneAABB(staticNodes3DRoot->aabb);
		sceneAABB.combine(mobileNodes3DRoot->aabb);	
	#endif
		
		// if origin + direction is outside box return empty	
		// glm::vec3 isRayOutsideBoxTestVec = rayDirection * (sceneAABB.calcCenter() - rayOrigin - sceneAABB.calcExtents());
		// if (isRayOutsideBoxTestVec.x > 0.0f || isRayOutsideBoxTestVec.y > 0.0f || isRayOutsideBoxTestVec.z > 0.0f)
		//	return rayCollisionData;
		glm::vec3 sceneAabbMaxVertex(sceneAABB.getMaxVertex());
		glm::vec3 sceneAabbMinVertex(sceneAABB.getMinVertex());		
		if ((rayDirection.x > 0.0f && rayOrigin.x > sceneAabbMaxVertex.x) ||
			(rayDirection.x < 0.0f && rayOrigin.x < sceneAabbMinVertex.x) ||
			(rayDirection.y > 0.0f && rayOrigin.y > sceneAabbMaxVertex.y) ||
			(rayDirection.y < 0.0f && rayOrigin.y < sceneAabbMinVertex.y) ||
			(rayDirection.z > 0.0f && rayOrigin.z > sceneAabbMaxVertex.z) ||
			(rayDirection.z < 0.0f && rayOrigin.z < sceneAabbMinVertex.z))
			return rayCollisionData;
			
		glm::vec3 rayDirectionAbs = glm::abs(rayDirection);
		unsigned int rayDirectionMaxDim = rayDirectionAbs.x > rayDirectionAbs.y ? 
										  (rayDirectionAbs.x > rayDirectionAbs.z ? 0 : 2) : 
										  (rayDirectionAbs.y > rayDirectionAbs.z ? 1 : 2);
		float destBoxPlane = rayDirection[rayDirectionMaxDim] > 0.0f ? sceneAabbMaxVertex[rayDirectionMaxDim] : sceneAabbMinVertex[rayDirectionMaxDim];
		glm::vec3 rayDest = rayOrigin + ((destBoxPlane - rayOrigin[rayDirectionMaxDim])/rayDirection[rayDirectionMaxDim] + RAY_DESTINATION_EXTRA_FACTOR)*rayDirection;
		
		//unsigned int rayDirectionNonMaxDim1 = (rayDirectionMaxDim + 1) % 3;
		//unsigned int rayDirectionNonMaxDim2 = (rayDirectionMaxDim + 2) % 3;			
		RayDirectionStandardAxesCrossProducts rayDirectionStandardAxesCrossProducts;
		glm::vec3 unitVec(0.0f, 0.0f, 0.0f);
		for (unsigned int crossProductAxIdx = 0; crossProductAxIdx < 3; crossProductAxIdx++) {
			unitVec[crossProductAxIdx] = 1.0f;
			rayDirectionStandardAxesCrossProducts.crossProducts[crossProductAxIdx] = cross(unitVec, rayDirection);
			rayDirectionStandardAxesCrossProducts.crossProductsAbs[crossProductAxIdx] = glm::abs(rayDirectionStandardAxesCrossProducts.crossProducts[crossProductAxIdx]);
			unitVec[crossProductAxIdx] = 0.0f;
		}
		Node<AABB3DRotatable>* nodesIt;
		float rayCollisionFactorMin = 1.0f;
		for (unsigned int treeIdx = 0; treeIdx < 2; treeIdx++) {
			if (treeIdx == 0)
				nodesIt = mobileNodes3DRoot;
			else
				nodesIt = staticNodes3DRoot;
				
			while (nodesIt) {
				AABB3D const& aabb = nodesIt->getAABB();
				glm::vec3 const& aabbMin = aabb.getMinVertex();
				glm::vec3 const& aabbMax = aabb.getMaxVertex();
				//unsigned char destOutCode = calcOutCodeSpecificDim(rayDest, aabbMin, aabbMax, rayDirectionNonMaxDim1) |
				//	calcOutCodeSpecificDim(rayDest, aabbMin, aabbMax, rayDirectionNonMaxDim2);
				float rayCollisionFactorOut;
				if (testRayAabbIntersection(rayOrigin, rayDest, rayDirectionStandardAxesCrossProducts, aabb.calcCenter(), aabb.calcExtents())) {
					if (!nodesIt->isLeaf())
						nodesIt = nodesIt->children[0];
					else {					
						if ( (static_cast<DataNode3D*>(nodesIt))->collisionPrimitive.testSegCollision(rayOrigin, rayDest, rayCollisionFactorOut) && rayCollisionFactorOut < rayCollisionFactorMin ) {
							rayCollisionFactorMin = rayCollisionFactorOut;
							rayCollisionData.hasCollided = true;
							rayCollisionData.modelIdx = static_cast<DataNode3D*>(nodesIt)->modelIdx;
							rayCollisionData.instanceIdx = static_cast<DataNode3D*>(nodesIt)->instanceIdx;
						}
						nodesIt = nodesIt->escapeNode;
					}
				}
				else
					nodesIt = nodesIt->escapeNode;
			}
		}

		return rayCollisionData;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	BVH::DataNode2D* BVH::insert(AABB2DRotatable const& aabb, unsigned int modelIdx, unsigned int instanceIdx, CollisionPerimeter& collisionPerimeter) {
		DataNode2D* newNode = staticNodes2DPool->acquire(aabb, modelIdx, instanceIdx, collisionPerimeter);
		doInsert<AABB2DRotatable, Node2D>(&staticNodes2DRoot, newNode, branchNodes2DPool, staticNodes2DNr);

		return newNode;
	}

	BVH::MobileGameLmntDataNode2D* BVH::insert(AABB2DRotatable const& aabb, unsigned int modelIdx, unsigned int instanceIdx, CollisionPerimeter& collisionPerimeter, PhysicsEngine::MobilityInterface const& mobilityInterface) {
		MobileGameLmntDataNode2D* newNode = mobileNodes2DPool->acquire(aabb, modelIdx, instanceIdx, collisionPerimeter, mobilityInterface);
		doInsert<AABB2DRotatable, Node2D>(&mobileNodes2DRoot, newNode, branchNodes2DPool, mobileNodes2DNr);

		return newNode;
	}

	void BVH::remove(DataNode2D* node) {
		doRemove<AABB2DRotatable, Node2D>(&staticNodes2DRoot, node, branchNodes2DPool, staticNodes2DNr);
		staticNodes2DPool->release(node);
	}

	void BVH::remove(MobileGameLmntDataNode2D* node) {
		doRemove<AABB2DRotatable, Node2D>(&mobileNodes2DRoot, node, branchNodes2DPool, mobileNodes2DNr);
		mobileNodes2DPool->release(node);
	}

	// Reminder: Right now CollisionData is only 3D (to simplify debugging)
	BVH::CollisionsData<glm::vec2> const& BVH::getCollisionsData2D() {
		//return doCollisionsSearch<AABB2DRotatable, DataNode2D, glm::vec2>(staticNodes2DRoot, mobileNodes2DRoot, collisionsBuffers2D);
		return collisionsBuffers2D.collisionsData;
	}

	template <class TAABB>
	void BVH::doRefitBPsDueToUpdate(Node<TAABB>* nodesRoot) {	
		Node<TAABB>* nodesIt = nodesRoot;
		while (!nodesIt->children[0]->isLeaf())
			nodesIt = nodesIt->children[0];

		do {
			if (!nodesIt->children[1]->isLeaf()) {
				nodesIt = nodesIt->children[1];
				while (!nodesIt->children[0]->isLeaf())
					nodesIt = nodesIt->children[0];
			}
			else {
				reduceSahAndRefitBVs<TAABB>(nodesIt);
				while (nodesIt->parent != NULL && nodesIt == nodesIt->parent->children[1]) {
					nodesIt = nodesIt->parent;
					reduceSahAndRefitBVs<TAABB>(nodesIt);
				}
				nodesIt = nodesIt->parent;
			}
		} while (nodesIt != NULL);
	}

	template <class TAABB, class TNode>
	void BVH::doInsert(TNode** nodesRoot, TNode* newNode, Corium3DUtils::ObjPool<TNode>* nodesPool, unsigned int& nodesCounter) {
		// T const& data	
		if (*nodesRoot != NULL) {
			if (!(*nodesRoot)->isLeaf()) {
				Node<TAABB>* newNodeSibling = findNewNodeSibling<TAABB>(*nodesRoot, newNode);
				Node<TAABB>* newNodeSiblingParent = newNodeSibling->parent;				
				TNode* newBranch = nodesPool->acquire(static_cast<TNode*>(newNodeSibling), newNode);
				if (newNodeSiblingParent != NULL) {
					if (newNodeSiblingParent->children[0] == newNodeSibling)
						newNodeSiblingParent->replaceChild(newBranch, 0);
					else
						newNodeSiblingParent->replaceChild(newBranch, 1);
				}
				else {
					*nodesRoot = newBranch;
					setSubtreeDepthValues<TAABB>(*nodesRoot);
				}

				//balanceTreeUpwards(foundSiblingParent);
			}
			else 				
				*nodesRoot = nodesPool->acquire(*nodesRoot, newNode);			

			nodesCounter += 2;
		}
		else {
			*nodesRoot = newNode;
			nodesCounter++;
		}
	}

	template <class TAABB, class TNode>
	void BVH::doRemove(TNode** nodesRoot, TNode* nodeToRemove, Corium3DUtils::ObjPool<TNode>* nodesPool, unsigned int& nodesCounter) {
		if (*nodesRoot != nodeToRemove) {
			Node<TAABB>* nodeParent = nodeToRemove->parent;
			Node<TAABB>* nodeSibling;
			if (nodeParent->children[0] == nodeToRemove)
				nodeSibling = nodeParent->children[1];
			else
				nodeSibling = nodeParent->children[0];

			if (*nodesRoot != nodeParent) {
				Node<TAABB>* nodeGrandParent = nodeParent->parent;
				if (nodeGrandParent->children[0] == nodeParent)
					nodeGrandParent->replaceChild(nodeSibling, 0);
				else
					nodeGrandParent->replaceChild(nodeSibling, 1);

				//balanceTreeUpwards(nodeParent->parent);
			}
			else {
				*nodesRoot = static_cast<TNode*>(nodeSibling);
				nodeSibling->parent = NULL;
				if (nodeSibling->escapeNode != NULL) {
					Node<TAABB>* nodesIt = nodeSibling;
					do {
						nodesIt->escapeNode = NULL;
						nodesIt = nodesIt->children[1];
					} while (nodesIt);
				}
			}
		
			nodesPool->release(static_cast<TNode*>(nodeParent));
			nodesCounter -= 2;
		}
		else {
			*nodesRoot = NULL;	
			nodesCounter--;
		}
	}

	template <class TAABB, class TDataNode, class V>
	BVH::CollisionsData<V> const& BVH::doCollisionsSearch(Node<TAABB>* staticNodesRoot, Node<TAABB>* mobileNodesRoot, CollisionsBuffers<V>& collisionsBuffers) {
		// BROAD PHASE //
		Node<TAABB>* leftSubtreeIt = mobileNodesRoot;
		Node<TAABB>* rightSubtreeIt = mobileNodesRoot;
		while (rightSubtreeIt) {
			if (leftSubtreeIt == rightSubtreeIt) {
				if (rightSubtreeIt->isLeaf()) {
					// can happen only after a last iteration of the stackless algorithm
					leftSubtreeIt = leftSubtreeIt->lastLeftChildAncestor;
					rightSubtreeIt = rightSubtreeIt->escapeNode;
				}
				else {
					Node<TAABB>* leftChild = leftSubtreeIt->children[0];
					Node<TAABB>* rightChild = leftSubtreeIt->children[1];
					if (leftChild->isLeaf()) {
						Node<TAABB>* rightChild = leftSubtreeIt->children[1];
						if (rightChild->isLeaf()) {
							if ((leftChild->aabb).doesIntersect(rightChild->aabb))
								recordBroadPhaseCollisionIdxsDuo<TDataNode, V>(static_cast<TDataNode*>(leftChild), static_cast<TDataNode*>(rightChild), collisionsBuffers.broadPhaseResBuffer);
							rightSubtreeIt = rightSubtreeIt->escapeNode;
							if (leftSubtreeIt != mobileNodesRoot && leftSubtreeIt->isRightChild())
								leftSubtreeIt = leftSubtreeIt->lastLeftChildAncestor;
						}
						else {
							runLeafAlgo<TAABB, TDataNode>(leftChild, rightChild, collisionsBuffers);
							leftSubtreeIt = rightSubtreeIt = rightChild;
						}
					}
					else
						leftSubtreeIt = rightSubtreeIt = leftChild;
				}
			}
			else
				runStacklessAlgoIteration<TAABB, TDataNode>(&leftSubtreeIt, &rightSubtreeIt, collisionsBuffers);
		}

		// find collisions between mobile and static game elements
		
		leftSubtreeIt = staticNodesRoot;
		rightSubtreeIt = mobileNodesRoot;
		while (rightSubtreeIt && leftSubtreeIt)
			runStacklessAlgoIteration<TAABB, TDataNode>(&leftSubtreeIt, &rightSubtreeIt, collisionsBuffers);
		
		/* ================================================================================================
		/*										SEARCH TREE PRINTER
		/* ================================================================================================
		/* static const CollisionData troubleMaker(1, 7, 1, 6);
		/* collisionsRecordIt->reset();
		/* while (collisionsRecordIt->hasNext()) {
		/* 	CollisionData gameLmntsIdxsDuo = collisionsRecordIt->next();
		/* 	if (troubleMaker == gameLmntsIdxsDuo) {
		/* 		collisionsRecordIt->reset();
		/* 		ServiceLocator::getLogger().logd("", "");
		/* 		ServiceLocator::getLogger().logd("", "CollisionsDuos Search Tree Printout");
		/* 		ServiceLocator::getLogger().logd("", "===================================");
		/* 		while (collisionsRecordIt->hasNext()) {
		/* 			gameLmntsIdxsDuo = collisionsRecordIt->next();
		/* 			ServiceLocator::getLogger().logd("", (std::to_string(gameLmntsIdxsDuo.instanceIdx1) + "<->" + std::to_string(gameLmntsIdxsDuo.instanceIdx2) + ": " + std::to_string(gameLmntsIdxsDuo.collisionState)).c_str());
		/* 		}
		/* 		break;
		/* 	}
		/* }
		/*  ================================================================================================ */

		// NARROW PHASE //	
		BroadPhaseCollisionsData<V>& broadPhaseResBuffer = collisionsBuffers.broadPhaseResBuffer;
		for (unsigned int duoIdx = 0; duoIdx < broadPhaseResBuffer.collisionsNr; duoIdx++) {		
			if (broadPhaseResBuffer.collisionPrimitivesDuos[duoIdx][0]->testCollision(broadPhaseResBuffer.collisionPrimitivesDuos[duoIdx][1], broadPhaseResBuffer.collisionsData[duoIdx].contactManifold)) {
				CollisionData<V>& returnedCollisionData = collisionsBuffers.collisionsRecord->insert(broadPhaseResBuffer.collisionsData[duoIdx]);
				if (returnedCollisionData.collisionState == CollisionData<V>::CollisionState::end)
					returnedCollisionData.collisionState = CollisionData<V>::CollisionState::persist;
			}
		}
		collisionsBuffers.broadPhaseResBuffer.collisionsNr = 0;

		collisionsBuffers.collisionsRecordIt->reset();
		collisionsBuffers.collisionsData.collisionsNr = collisionsBuffers.collisionsData.detachmentsNr = 0;
		while (collisionsBuffers.collisionsRecordIt->hasNext()) {
			CollisionData<V>& collisionData = collisionsBuffers.collisionsRecordIt->next();
			switch (collisionData.collisionState) {
				case CollisionData<V>::CollisionState::start:
					collisionsBuffers.collisionsData.collisionsDataBuffer[collisionsBuffers.collisionsData.collisionsNr++] = collisionData;
				case CollisionData<V>::CollisionState::persist:
					collisionData.collisionState = CollisionData<V>::CollisionState::end;
					break;
				case CollisionData<V>::CollisionState::end:
					collisionsBuffers.collisionsData.detachmentsDataBuffer[collisionsBuffers.collisionsData.detachmentsNr++] = collisionData;
					collisionsBuffers.collisionsRecordIt->removeCurr();
					break;
			}
		}

		return collisionsBuffers.collisionsData;
	}

	template <class TAABB>
	void BVH::setSubtreeDepthValues(Node<TAABB>* subtreeRoot) {
		Node<TAABB>* nodesIt = subtreeRoot->children[0];
		do {
			nodesIt->depth = nodesIt->parent->depth + 1;
			if (nodesIt->isLeaf())
				nodesIt = nodesIt->escapeNode;
			else
				nodesIt = nodesIt->children[0];
		} while (nodesIt);
	}

	enum RollTypes { LL_RL, LL_RR, NO_ROLL };

	template <class TAABB>
	void BVH::reduceSahAndRefitBVs(Node<TAABB>* node) {
		if (node->children[0]->isLeaf() || node->children[1]->isLeaf()) {
			node->refitBVs();
			return;
		}

		Node<TAABB> *childL = node->children[0], *childR = node->children[1];
		TAABB &childLAABB = childL->aabb, &childRAABB = childR->aabb;
		TAABB &childLLAABB = childL->children[0]->aabb, &childLRAABB = childL->children[1]->aabb;
		TAABB &childRLAABB = childR->children[0]->aabb, &childRRAABB = childR->children[1]->aabb;

		float rollsSAHs[2] = {		
			childRLAABB.calcCombinedAabbSurface(childLRAABB) + childLLAABB.calcCombinedAabbSurface(childRRAABB), //LL-RL roll
			childRRAABB.calcCombinedAabbSurface(childLRAABB) + childLLAABB.calcCombinedAabbSurface(childRLAABB) //LL-RR roll
		};

		float minSAH = childLAABB.getSurface() + childRAABB.getSurface();
		unsigned int minSahRollIdx = NO_ROLL;
		for (unsigned int rollIdx = 0; rollIdx < 2; rollIdx++) {
			if (rollsSAHs[rollIdx] < minSAH) {
				minSAH = rollsSAHs[rollIdx];
				minSahRollIdx = rollIdx;
			}
		}

		switch (minSahRollIdx) {		
			case LL_RL: {
				Node<TAABB>* childLL = childL->children[0];
				childL->replaceChild(childR->children[0], 0);
				childR->replaceChild(childLL, 0);
				childR->refitBVs();
				childL->refitBVs();
				break;
			}

			case LL_RR: {
				Node<TAABB>* childLL = childL->children[0];
				childL->replaceChild(childR->children[1], 0);
				childR->replaceChild(childLL, 1);
				childR->refitBVs();
				childL->refitBVs();
				break;
			}

			case NO_ROLL:
				node->refitBVs();
				break;		
		}
	}

	/*
	enum RollTypes { L_RL, L_RR, R_LR, R_LL, LL_RL, LL_RR, NO_ROLL };

	void BVH::reduceSahAndRefitBVs(Node* node) {
		if (node->children[0]->isLeaf() || node->children[1]->isLeaf())
			return;

		Node *childL = node->children[0], *childR = node->children[1];
		AABB &childLAABB = childL->aabb, &childRAABB = childR->aabb;
		AABB &childLLAABB = childL->children[0]->aabb, &childLRAABB = childL->children[1]->aabb;
		AABB &childRLAABB = childR->children[0]->aabb, &childRRAABB = childR->children[1]->aabb;

		float rollsSAHs[6] = { 
			childRLAABB.getSurface() + AABB::calcCombinedAABBSurface(childLAABB, childRRAABB), //L-RL roll
			childRRAABB.getSurface() + AABB::calcCombinedAABBSurface(childLAABB, childRLAABB), //L-RR roll
			childLRAABB.getSurface() + AABB::calcCombinedAABBSurface(childRAABB, childLLAABB), //R-LR roll
			childLLAABB.getSurface() + AABB::calcCombinedAABBSurface(childRAABB, childLLAABB), //R-LL roll
			AABB::calcCombinedAABBSurface(childRLAABB, childLRAABB) +
			AABB::calcCombinedAABBSurface(childLLAABB, childRRAABB), //LL-RL roll
			AABB::calcCombinedAABBSurface(childRRAABB, childLRAABB) +
			AABB::calcCombinedAABBSurface(childLLAABB, childRLAABB) //LL-RR roll
		}; 

		float minSAH = childLAABB.getSurface() + childRAABB.getSurface();
		unsigned int minSahRollIdx = NO_ROLL;
		for (unsigned int rollIdx = 0; rollIdx < 6; rollIdx++) {
			if (rollsSAHs[rollIdx] < minSAH) {
				minSAH = rollsSAHs[rollIdx];
				minSahRollIdx = rollIdx;
			}
		}

		switch (minSahRollIdx) {
			case L_RL: {
				Node* childRL = childR->children[0];
				childR->replaceChild(childL, 0);
				childR->refitBVs();
				node->replaceChild(childRL, 0);
				break;
			}

			case L_RR: {
				Node* childRR = childR->children[1];
				childR->replaceChild(childL, 1);
				childR->refitBVs();
				node->replaceChild(childRR, 0);
				break;
			}

			case R_LR: {
				Node* childLR = childL->children[1];
				childL->replaceChild(childR, 1);
				childL->refitBVs();
				node->replaceChild(childLR, 1);
				break;
			}

			case R_LL: {
				Node* childLL = childL->children[0];
				childL->replaceChild(childR, 0);
				childL->refitBVs();
				node->replaceChild(childLL, 1);
				break;
			}

			case LL_RL: {
				Node* childLL = childL->children[0];
				childL->replaceChild(childR->children[0], 0);
				childR->replaceChild(childLL, 0);
				childR->refitBVs();
				childL->refitBVs();			
				break;
			}

			case LL_RR: {
				Node* childLL = childL->children[0];
				childL->replaceChild(childR->children[1], 0);
				childR->replaceChild(childLL, 1);
				childR->refitBVs();
				childL->refitBVs();
				break;
			}
		}
	}
	*/

	template <class TAABB>
	static BVH::Node<TAABB>* BVH::findNewNodeSibling(Node<TAABB>* root, Node<TAABB>* newNode) {
		Node<TAABB>* nodesIt = root;
		bool wasInsertionPlaceFound = false;
		while (!nodesIt->isLeaf()) {
			float nodeAndLeafCombinedSurface = (nodesIt->aabb).calcCombinedAabbSurface(newNode->aabb);
			float costInsertingAsSibling = 2 * nodeAndLeafCombinedSurface;
			float inheritanceCost = 2 * (nodeAndLeafCombinedSurface - nodesIt->aabb.getSurface());
			Node<TAABB>* child0 = nodesIt->children[0];
			Node<TAABB>* child1 = nodesIt->children[1];
			float child0DescentCost = inheritanceCost;
			float child1DescentCost = inheritanceCost;
			if (child0->isLeaf())
				child0DescentCost += 2 * (child0->aabb).calcCombinedAabbSurface(newNode->aabb);
			else
				child0DescentCost += 2 * ((child0->aabb).calcCombinedAabbSurface(newNode->aabb) - (child0->aabb).getSurface());
			if (child1->isLeaf())
				child1DescentCost += 2 * (child1->aabb).calcCombinedAabbSurface(newNode->aabb);
			else
				child1DescentCost += 2 * ((child1->aabb).calcCombinedAabbSurface(newNode->aabb) - (child1->aabb).getSurface());

			if (costInsertingAsSibling < child0DescentCost && costInsertingAsSibling < child1DescentCost)
				return nodesIt;
			else if (child0DescentCost < child1DescentCost)
				nodesIt = child0;
			else
				nodesIt = child1;
		}

		return nodesIt;
	}

	template <class TAABB>
	static BVH::Node<TAABB>* BVH::heightUp(Node<TAABB>* retNode, unsigned int height) {
		while (height--) retNode = retNode->parent;	
		return retNode;
	}

	template <class TAABB, class TDataNode, class V>
	void BVH::runStacklessAlgoIteration(Node<TAABB>** leftSubtreeItPtr, Node<TAABB>** rightSubtreeItPtr, CollisionsBuffers<V>& collisionsBuffers) {
		Node<TAABB>* leftSubtreeIt = *leftSubtreeItPtr;
		Node<TAABB>* rightSubtreeIt = *rightSubtreeItPtr;
		if (leftSubtreeIt->aabb.doesIntersect(rightSubtreeIt->aabb)) {
			if (leftSubtreeIt->isLeaf() && rightSubtreeIt->isLeaf())
				recordBroadPhaseCollisionIdxsDuo<TDataNode, V>(static_cast<TDataNode*>(leftSubtreeIt), static_cast<TDataNode*>(rightSubtreeIt), collisionsBuffers.broadPhaseResBuffer);
			else if (rightSubtreeIt->isLeaf())
				runLeafAlgo<TAABB, TDataNode, V>(rightSubtreeIt, leftSubtreeIt, collisionsBuffers);
			else if (leftSubtreeIt->isLeaf()) {
				runLeafAlgo<TAABB, TDataNode, V>(leftSubtreeIt, rightSubtreeIt, collisionsBuffers);
			}
			else {
				if (leftSubtreeIt->depth < rightSubtreeIt->depth)
					*leftSubtreeItPtr = leftSubtreeIt->children[0];
				else
					*rightSubtreeItPtr = rightSubtreeIt->children[0];
				return;
			}
		}

		if (leftSubtreeIt->depth == rightSubtreeIt->depth) {
			if (leftSubtreeIt->isLeftChild()) {
				*leftSubtreeItPtr = leftSubtreeIt->escapeNode;
			}
			else if (leftSubtreeIt->isRightChild() && rightSubtreeIt->isLeftChild()) {
				*leftSubtreeItPtr = leftSubtreeIt->parent;
				*rightSubtreeItPtr = rightSubtreeIt->escapeNode;
			}
			else if (leftSubtreeIt->isRightChild() && rightSubtreeIt->isRightChild()) {
				unsigned int leftSubtreeRCL = leftSubtreeIt->depth - leftSubtreeIt->lastLeftChildAncestor->depth;
				unsigned int rightSubtreeRCL = rightSubtreeIt->depth - rightSubtreeIt->lastLeftChildAncestor->depth;
				if (leftSubtreeRCL <= rightSubtreeRCL) {
					*leftSubtreeItPtr = leftSubtreeIt->escapeNode;
					*rightSubtreeItPtr = heightUp<TAABB>(rightSubtreeIt, leftSubtreeRCL);
				}
				else {
					*leftSubtreeItPtr = heightUp<TAABB>(leftSubtreeIt, rightSubtreeRCL + 1);
					*rightSubtreeItPtr = rightSubtreeIt->escapeNode;
				}
			}
		}
		else {
			if (rightSubtreeIt->isLeftChild())
				*rightSubtreeItPtr = rightSubtreeIt->escapeNode;
			else if (leftSubtreeIt->isLeftChild()) {
				*leftSubtreeItPtr = leftSubtreeIt->escapeNode;
				*rightSubtreeItPtr = rightSubtreeIt->parent;
			}
			else { // if (leftSubtreeIt->isRightChild() && rightSubtreeIt->isRightChild()) {
				unsigned int leftSubtreeRCL = leftSubtreeIt->depth - leftSubtreeIt->lastLeftChildAncestor->depth;
				unsigned int rightSubtreeRCL = rightSubtreeIt->depth - rightSubtreeIt->lastLeftChildAncestor->depth;
				if (leftSubtreeRCL <= rightSubtreeRCL - 1) {
					*leftSubtreeItPtr = leftSubtreeIt->escapeNode;
					*rightSubtreeItPtr = heightUp(rightSubtreeIt, leftSubtreeRCL + 1);
				}
				else {
					*leftSubtreeItPtr = heightUp(leftSubtreeIt, rightSubtreeRCL);
					*rightSubtreeItPtr = rightSubtreeIt->escapeNode;
				}
			}
		}
	}

	template <class TAABB, class TDataNode, class V>
	void BVH::runLeafAlgo(Node<TAABB>* leaf, Node<TAABB>* treeRoot, CollisionsBuffers<V>& collisionsBuffers) {
		Node<TAABB>* treeIt = treeRoot;
		TAABB& leafAABB = leaf->aabb;
		do {
			if (leafAABB.doesIntersect(treeIt->aabb)) {
				if (treeIt->isLeaf()) {		
					recordBroadPhaseCollisionIdxsDuo<TDataNode, V>(static_cast<TDataNode*>(leaf), static_cast<TDataNode*>(treeIt), collisionsBuffers.broadPhaseResBuffer);
					treeIt = treeIt->escapeNode;
				}
				else
					treeIt = treeIt->children[0];
			}
			else 
				treeIt = treeIt->escapeNode;
		} while (treeIt != treeRoot->escapeNode);
	}

	template <class TDataNode, class V>
	void BVH::recordBroadPhaseCollisionIdxsDuo(TDataNode* node1, TDataNode* node2, BroadPhaseCollisionsData<V>& broadPhaseResBuffer) {
		broadPhaseResBuffer.collisionPrimitivesDuos[broadPhaseResBuffer.collisionsNr][0] = &(node1->collisionPrimitive);
		broadPhaseResBuffer.collisionPrimitivesDuos[broadPhaseResBuffer.collisionsNr][1] = &(node2->collisionPrimitive);
		broadPhaseResBuffer.collisionsData[broadPhaseResBuffer.collisionsNr] =
			CollisionData<V>(node1->modelIdx, node1->instanceIdx, node2->modelIdx, node2->instanceIdx);
		//node1->collisionData2D = node2->collisionData2D = &broadPhaseResBuffer.collisionsData[broadPhaseResBuffer.collisionsNr]; //temp: for debug
		node1->collisionData3D = node2->collisionData3D = &broadPhaseResBuffer.collisionsData[broadPhaseResBuffer.collisionsNr]; //temp: for debug
		broadPhaseResBuffer.collisionsNr++;
	}

	template <class TAABB>
	BVH::Node<TAABB>::Node(TAABB const& _aabb) : aabb(_aabb) {}

	// REMINDER: Leaves the depth member of this sub-tree's nodes at an invalid state (more efficient to update
	//			 the depth member after all changes to the BVH made on the current frame)
	template <class TAABB>
	BVH::Node<TAABB>::Node(Node<TAABB>* leftChild, Node<TAABB>* rightChild) :		
			aabb(TAABB::calcCombinedAABB(leftChild->aabb, rightChild->aabb)) {	
		leftChild->parent = rightChild->parent = this;
		rightChild->escapeNode = this->escapeNode;
		Node* nodesIt = leftChild;
		do {
			nodesIt->escapeNode = rightChild;
			nodesIt = nodesIt->children[1];
		} while (nodesIt);

		leftChild->lastLeftChildAncestor = NULL;
		nodesIt = leftChild->children[1];
		while (nodesIt) {
			nodesIt->lastLeftChildAncestor = leftChild;
			nodesIt = nodesIt->children[1];
		}
		nodesIt = rightChild;
		do {
			nodesIt->lastLeftChildAncestor = this;
			nodesIt = nodesIt->children[1];
		} while (nodesIt);	

		children[0] = leftChild;
		children[1] = rightChild;
	}

	template <class TAABB>
	BVH::Node<TAABB>::Node(Node<TAABB> const& node) : aabb(node.aabb), 
			parent(node.parent), escapeNode(node.escapeNode), lastLeftChildAncestor(node.lastLeftChildAncestor), depth(node.depth) {
		if (!node.isLeaf()) {
			children[0] = node.children[0];
			children[1] = node.children[1];
			children[0]->parent = children[1]->parent = this;
			if (children[1]->lastLeftChildAncestor == &node)
				children[1]->lastLeftChildAncestor = this;
		}

		if (node.parent) {
			if (node.isRightChild()) {
				node.parent->children[0]->escapeNode = this;
				node.parent->children[1] = this;
			}
			else {
				node.parent->children[0] = this;
			}		
		}
	}

	template <class TAABB>
	void BVH::Node<TAABB>::refitBVs() {		
		aabb = TAABB::calcCombinedAABB(children[0]->aabb, children[1]->aabb);
	}

	template <class TAABB>
	void BVH::Node<TAABB>::replaceChild(Node<TAABB>* child, unsigned int childIdx) {
		child->parent = this;
		Node<TAABB>* childNewLastLeftChildAncestor;
		Node<TAABB>* nodesIt;
		if (childIdx == 0) {
			child->escapeNode = children[1];
			child->lastLeftChildAncestor = NULL;
			childNewLastLeftChildAncestor = child;
		}
		else { // childIdx == 1
			child->escapeNode = this->escapeNode;
			nodesIt = children[0];	
			do {
				nodesIt->escapeNode = child;
				nodesIt = nodesIt->children[1];
			} while (nodesIt);
			if (this->lastLeftChildAncestor)
				childNewLastLeftChildAncestor = child->lastLeftChildAncestor = this->lastLeftChildAncestor;
			else
				childNewLastLeftChildAncestor = child->lastLeftChildAncestor = this;
		}

		nodesIt = child->children[1];
		while (nodesIt) {
			nodesIt->lastLeftChildAncestor = childNewLastLeftChildAncestor;
			nodesIt->escapeNode = child->escapeNode;
			nodesIt = nodesIt->children[1];
		}
	
		if (child->depth != this->depth + 1) {						
			nodesIt = child;
			do {
				nodesIt->depth = nodesIt->parent->depth + 1;
				if (nodesIt->isLeaf())
					nodesIt = nodesIt->escapeNode;
				else {
					nodesIt = nodesIt->children[0];
					continue;
				}
			} while (nodesIt != this->escapeNode);
			// TODO: improve the while condition
		}

		children[childIdx] = child;
		refitBVs();
	}

	BVH::Node3D::Node3D(AABB3DRotatable const& aabb, BoundingSphere const& _boundingSphere) : Node<AABB3DRotatable>(aabb), boundingSphere(_boundingSphere) {}

	BVH::Node3D::Node3D(Node3D const& node3D) : Node<AABB3DRotatable>(node3D), boundingSphere(node3D.boundingSphere) {}

	BVH::Node3D::Node3D(Node3D* leftChild, Node3D* rightChild) : 
		Node<AABB3DRotatable>(leftChild, rightChild), boundingSphere(BoundingSphere::calcCombinedBoundingSphere(leftChild->boundingSphere, rightChild->boundingSphere)) {}

	void BVH::Node3D::refitBVs() {
		Node<AABB3DRotatable>::refitBVs();
		boundingSphere = BoundingSphere::calcCombinedBoundingSphere(static_cast<Node3D*>(getChild(0))->boundingSphere, static_cast<Node3D*>(getChild(1))->boundingSphere);	
	}	

	BVH::DataNode3D::DataNode3D(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int _modelIdx, unsigned int _instanceIdx, CollisionVolume& _collisionVolume) :
		Node3D(aabb, boundingSphere), modelIdx(_modelIdx), instanceIdx(_instanceIdx), collisionPrimitive(_collisionVolume) {}

	BVH::DataNode3D::DataNode3D(DataNode3D const& dataNode) : Node3D(dataNode), modelIdx(dataNode.modelIdx), instanceIdx(dataNode.instanceIdx), collisionPrimitive(dataNode.collisionPrimitive) {}

	BVH::MobileGameLmntDataNode3D::MobileGameLmntDataNode3D(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume, PhysicsEngine::MobilityInterface const& _mobilityInterface) :
			BVH::DataNode3D(AABB3DRotatable::calcScaledAABB(aabb, FATTENING_FACTOR), boundingSphere, modelIdx, instanceIdx, collisionVolume), aabbFattened(aabb), mobilityInterface(_mobilityInterface) {}

	BVH::MobileGameLmntDataNode3D::MobileGameLmntDataNode3D(MobileGameLmntDataNode3D const& node) : BVH::DataNode3D(node),
		mobilityInterface(node.mobilityInterface), aabbFattened(node.aabbFattened) {}

	void BVH::MobileGameLmntDataNode3D::updateBVs(Transform3DUS const& transformDelta) {
		static_cast<AABB3DRotatable&>(aabb).transform(transformDelta);
		boundingSphere.transform(transformDelta);
		collisionPrimitive.transform(transformDelta);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB3DRotatable::calcScaledAABB(static_cast<AABB3DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode3D::translateBVs(glm::vec3 const& translate) {
		static_cast<AABB3DRotatable&>(aabb).translate(translate);
		boundingSphere.translate(translate);
		collisionPrimitive.translate(translate);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB3DRotatable::calcScaledAABB(static_cast<AABB3DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode3D::scaleBVs(float scaleFactor) {
		static_cast<AABB3DRotatable&>(aabb).scale(scaleFactor);
		boundingSphere.scale(scaleFactor);
		collisionPrimitive.scale(scaleFactor);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB3DRotatable::calcScaledAABB(static_cast<AABB3DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode3D::rotateBVs(glm::quat const& rot) {
		static_cast<AABB3DRotatable&>(aabb).rotate(rot);
		collisionPrimitive.rotate(rot);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB3DRotatable::calcScaledAABB(static_cast<AABB3DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode3D::refitBVs() {
		Node3D::refitBVs();
		aabbFattened = AABB3DRotatable::calcScaledAABB(static_cast<AABB3DRotatable&>(aabb), FATTENING_FACTOR);
	}

	BVH::DataNode2D::DataNode2D(AABB2DRotatable const& aabb, unsigned int _modelIdx, unsigned int _instanceIdx, CollisionPerimeter& _collisionPerimeter) :
		Node2D(aabb), modelIdx(_modelIdx), instanceIdx(_instanceIdx), collisionPrimitive(_collisionPerimeter) {}

	BVH::DataNode2D::DataNode2D(DataNode2D const& dataNode) :
		Node2D(dataNode), modelIdx(dataNode.modelIdx), instanceIdx(dataNode.instanceIdx), collisionPrimitive(dataNode.collisionPrimitive) {}

	BVH::MobileGameLmntDataNode2D::MobileGameLmntDataNode2D(AABB2DRotatable const& aabb, unsigned int modelIdx, unsigned int instanceIdx, CollisionPerimeter& collisionPerimeter, PhysicsEngine::MobilityInterface const& _mobilityInterface) :
		DataNode2D(aabb, modelIdx, instanceIdx, collisionPerimeter), mobilityInterface(_mobilityInterface) {}

	BVH::MobileGameLmntDataNode2D::MobileGameLmntDataNode2D(MobileGameLmntDataNode2D const& node) :
		DataNode2D(node.aabb, node.modelIdx, node.instanceIdx, node.collisionPrimitive), mobilityInterface(node.mobilityInterface) {}

	void BVH::MobileGameLmntDataNode2D::updateBPs(Transform2DUS const& transformDelta) {
		static_cast<AABB2DRotatable&>(aabb).transform(transformDelta);	
		collisionPrimitive.transform(transformDelta);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB2DRotatable::calcScaledAABB(static_cast<AABB2DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode2D::translateBPs(glm::vec2 const& translate) {
		static_cast<AABB2DRotatable&>(aabb).translate(translate);	
		collisionPrimitive.translate(translate);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB2DRotatable::calcScaledAABB(static_cast<AABB2DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode2D::scaleBPs(float scaleFactor) {
		static_cast<AABB2DRotatable&>(aabb).scale(scaleFactor);
		collisionPrimitive.scale(scaleFactor);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB2DRotatable::calcScaledAABB(static_cast<AABB2DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode2D::rotateBPs(std::complex<float> const& rot) {
		static_cast<AABB2DRotatable&>(aabb).rotate(rot);
		collisionPrimitive.rotate(rot);
		if (!aabbFattened.doesContain(aabb))
			aabbFattened = AABB2DRotatable::calcScaledAABB(static_cast<AABB2DRotatable&>(aabb), FATTENING_FACTOR);
	}

	void BVH::MobileGameLmntDataNode2D::refitBPs() {
		Node2D::refitBVs();
		aabbFattened = AABB2DRotatable::calcScaledAABB(static_cast<AABB2DRotatable&>(aabb), FATTENING_FACTOR);
	}

	template <class V>
	BVH::CollisionData<V>::CollisionData(unsigned int modelIdx1, unsigned int instanceIdx1, unsigned int modelIdx2, unsigned int instanceIdx2) {
		if (modelIdx1 > modelIdx2) {
			this->modelIdx1 = modelIdx1;
			this->instanceIdx1 = instanceIdx1;
			this->modelIdx2 = modelIdx2;
			this->instanceIdx2 = instanceIdx2;
		}
		else if (modelIdx1 < modelIdx2) {
			this->modelIdx1 = modelIdx2;
			this->instanceIdx1 = instanceIdx2;
			this->modelIdx2 = modelIdx1;
			this->instanceIdx2 = instanceIdx1;
		}
		else { // (modelIdx1 == modelIdx2) {
			this->modelIdx1 = modelIdx1;		
			this->modelIdx2 = modelIdx2;
			if (instanceIdx1 > instanceIdx2) {
				this->instanceIdx1 = instanceIdx1;
				this->instanceIdx2 = instanceIdx2;
			}
			else {
				this->instanceIdx1 = instanceIdx2;
				this->instanceIdx2 = instanceIdx1;
			}
		}
	}

	template <class V>
	bool BVH::CollisionData<V>::operator>(CollisionData const& other) const {
		if (modelIdx1 != other.modelIdx1)
			return modelIdx1 > other.modelIdx1;
		else if (instanceIdx1 != other.instanceIdx1)
			return instanceIdx1 > other.instanceIdx1;
		else if (modelIdx2 != other.modelIdx2)
			return modelIdx2 > other.modelIdx2;
		else if (instanceIdx2 != other.instanceIdx2)
			return instanceIdx2 > other.instanceIdx2;
		else
			return false;
	}

	template <class V>
	bool BVH::CollisionData<V>::operator>=(CollisionData const& other) const {
		return !operator<(other);
	}

	template <class V>
	bool BVH::CollisionData<V>::operator<(CollisionData const& other) const {
		if (modelIdx1 != other.modelIdx1)
			return modelIdx1 < other.modelIdx1;
		else if (instanceIdx1 != other.instanceIdx1)
			return instanceIdx1 < other.instanceIdx1;
		else if (modelIdx2 != other.modelIdx2)
			return modelIdx2 < other.modelIdx2;
		else if (instanceIdx2 != other.instanceIdx2)
			return instanceIdx2 < other.instanceIdx2;
		else
			return false;
	}

	template <class V>
	bool BVH::CollisionData<V>::operator<=(CollisionData const& other) const {
		return !operator>(other);
	}

	template <class V>
	bool BVH::CollisionData<V>::operator==(CollisionData const& other) const {
		return this->modelIdx1 == other.modelIdx1 &&
			   this->instanceIdx1 == other.instanceIdx1  &&
			   this->modelIdx2 == other.modelIdx2  &&
			   this->instanceIdx2 == other.instanceIdx2;
	}

} // namespace Corium3D