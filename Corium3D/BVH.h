#pragma once

#include "AABB.h"
#include "BoundingSphere.h"
#include "ObjPool.h"
#include "SearchTreeAVL.h"
#include "PhysicsEngine.h"
#include "CollisionPrimitives.h"

#include <vector>
#include <array>

namespace Corium3D {

	// TODO: check for fattening factor according to displacement prediction
	const float FATTENING_FACTOR = 1.05f;

	//template <class T>
	class BVH {
	public:
		// temp - to be removed (needed it before Node's declaration)	
		template <class V>
		class CollisionData {
		public:
			friend BVH;
			friend Corium3DUtils::SearchTreeAVL<CollisionData<V>>;
			friend Corium3DUtils::ObjPool<CollisionData<V>>;

			unsigned int modelIdx1 = 0;
			unsigned int instanceIdx1 = 0;
			unsigned int modelIdx2 = 0;
			unsigned int instanceIdx2 = 0;
			typename CollisionPrimitive<V>::ContactManifold contactManifold;

		private:
			enum CollisionState { start, persist, end };
			CollisionState collisionState = CollisionState::start;

			CollisionData() {};
			CollisionData(unsigned int modelIdx1, unsigned int instanceIdx1, unsigned int modelIdx2, unsigned int instanceIdx2);
			bool operator>(CollisionData const& other) const;
			bool operator>=(CollisionData const& other) const;
			bool operator<(CollisionData const& other) const;
			bool operator<=(CollisionData const& other) const;
			bool operator==(CollisionData const& other) const;
		};

		template <class TAABB>
		class Node {
		public:
			friend BVH;
			friend Corium3DUtils::ObjPool<Node<TAABB>>;

			TAABB const& getAABB() const { return aabb; }
			Node* getParent() const { return parent; }
			Node* getChild(unsigned int childIdx) const {
				if (isLeaf())
					return NULL;
				else
					return children[childIdx];
			}
			Node* getEscapeNode() const { return escapeNode; }
			bool isLeaf() const { return children[0] == NULL; }

			BVH::CollisionData<glm::vec3>* collisionData3D = NULL;
			BVH::CollisionData<glm::vec2>* collisionData2D = NULL;

		protected:
			TAABB aabb;
			Node* parent = NULL;
			Node* escapeNode = NULL;
			Node* lastLeftChildAncestor = NULL;

			Node(TAABB const& _aabb);
			Node(Node* leftChild, Node* rightChild);
			Node(Node const& node);
			~Node() {}
			virtual void refitBVs();
			bool isLeftChild() const { return parent->children[0] == this; }
			bool isRightChild() const { return parent->children[1] == this; }

		private:
			Node* children[2] = { NULL, NULL };
			unsigned char depth = 0;

			void replaceChild(Node* child, unsigned int childIdx);
		};

		class Node3D : public BVH::Node<AABB3DRotatable> {
		public:
			friend BVH;
			friend Corium3DUtils::ObjPool<Node3D>;

			BoundingSphere const& getBoundingSphere() const { return boundingSphere; }

		protected:
			BoundingSphere boundingSphere;

			Node3D(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere);
			Node3D(Node3D const& dataNode);
			Node3D(Node3D* leftChild, Node3D* rightChild);
			~Node3D() {}
			void refitBVs() override;
		};

		class DataNode3D : public BVH::Node3D {
		public:
			friend BVH;
			friend Corium3DUtils::ObjPool<DataNode3D>;
			unsigned int getModelIdx() const { return modelIdx; }
			unsigned int getInstanceIdx() const { return instanceIdx; }

		protected:
			unsigned int modelIdx;
			unsigned int instanceIdx;
			CollisionVolume& collisionPrimitive;

			DataNode3D(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume);
			DataNode3D(DataNode3D const& dataNode);
			~DataNode3D() {}
		};

		class MobileGameLmntDataNode3D : public BVH::DataNode3D {
		public:
			friend BVH;
			friend Corium3DUtils::ObjPool<MobileGameLmntDataNode3D>;

			PhysicsEngine::MobilityInterface const& getMobilityInterface() { return mobilityInterface; }

		private:
			PhysicsEngine::MobilityInterface const& mobilityInterface;
			AABB3DRotatable aabbFattened;

			MobileGameLmntDataNode3D(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume, PhysicsEngine::MobilityInterface const& mobilityInterface);
			MobileGameLmntDataNode3D(MobileGameLmntDataNode3D const& node);
			~MobileGameLmntDataNode3D() {}
			void updateBVs(Transform3D const& transformDelta);
			void translateBVs(glm::vec3 const& translate);
			void scaleBVs(glm::vec3 const& scale);
			void rotateBVs(glm::quat const& rot);
			void refitBVs() override;
		};

		typedef Node<AABB2DRotatable> Node2D;

		class DataNode2D : public BVH::Node<AABB2DRotatable> {
		public:
			friend BVH;
			friend Corium3DUtils::ObjPool<DataNode2D>;
			unsigned int getModelIdx() const { return modelIdx; }
			unsigned int getInstanceIdx() const { return instanceIdx; }

		protected:
			unsigned int modelIdx;
			unsigned int instanceIdx;
			CollisionPerimeter& collisionPrimitive;

			DataNode2D(AABB2DRotatable const& aabb, unsigned int modelIdx, unsigned int instanceIdx, CollisionPerimeter& collisionPerimeter);
			DataNode2D(DataNode2D const& dataNode);
			~DataNode2D() {}
		};

		class MobileGameLmntDataNode2D : public BVH::DataNode2D {
		public:
			friend BVH;
			friend Corium3DUtils::ObjPool<MobileGameLmntDataNode2D>;

			PhysicsEngine::MobilityInterface const& getMobilityInterface() { return mobilityInterface; }

		private:
			PhysicsEngine::MobilityInterface const& mobilityInterface;
			AABB2DRotatable aabbFattened;

			MobileGameLmntDataNode2D(AABB2DRotatable const& aabb, unsigned int modelIdx, unsigned int instanceIdx, CollisionPerimeter& collisionPerimeter, PhysicsEngine::MobilityInterface const& mobilityInterface);
			MobileGameLmntDataNode2D(MobileGameLmntDataNode2D const& node);
			~MobileGameLmntDataNode2D() {}
			void updateBPs(Transform2D const& transformDelta);
			void translateBPs(glm::vec2 const& translate);
			void scaleBPs(glm::vec2 const& scale);
			void rotateBPs(std::complex<float> const& rot);
			void refitBPs();
		};

		// temp - to be restored
		// TODO: Optimize with integers concatinations and bitwise operators (or some other way)
		/*class CollisionData {
		public:
			friend BVH;
			friend Corium3DUtils::SearchTreeAVL<CollisionData>;
			friend Corium3DUtils::ObjPool<CollisionData>;

			unsigned int modelIdx1 = 0;
			unsigned int instanceIdx1 = 0;
			unsigned int modelIdx2 = 0;
			unsigned int instanceIdx2 = 0;
			CollisionVolume::ContactManifold contactManifold;

		private:
			enum CollisionState { start, persist, end };
			CollisionState collisionState = CollisionState::start;

			CollisionData() {};
			CollisionData(unsigned int modelIdx1, unsigned int instanceIdx1, unsigned int modelIdx2, unsigned int instanceIdx2);
			bool operator>(CollisionData const& other) const;
			bool operator>=(CollisionData const& other) const;
			bool operator<(CollisionData const& other) const;
			bool operator<=(CollisionData const& other) const;
			bool operator==(CollisionData const& other) const;
		};*/

		template <class V>
		struct CollisionsData {
			CollisionData<V>* collisionsDataBuffer;
			unsigned int collisionsNr = 0;
			CollisionData<V>* detachmentsDataBuffer;
			unsigned int detachmentsNr = 0;
		};

		struct RayCollisionData {
			bool hasCollided = false;
			unsigned int modelIdx = 0;
			unsigned int instanceIdx = 0;
		};

		BVH(unsigned int staticGameLmnts3DNrMax, unsigned int mobileGameLmnts3DNrMax, unsigned int staticGameLmnts2DNrMax, unsigned int mobileGameLmnts2DNrMax, unsigned int collisions2DNrMax, unsigned int collisions3DNrMax);
		BVH(BVH const&) = delete;
		~BVH();
		void refitBPsDueToUpdate();

		// 3D methods
		DataNode3D* insert(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume);
		MobileGameLmntDataNode3D* insert(AABB3DRotatable const& aabb, BoundingSphere const& boundingSphere, unsigned int modelIdx, unsigned int instanceIdx, CollisionVolume& collisionVolume, PhysicsEngine::MobilityInterface const& mobilityInterface);
		void remove(DataNode3D* node);
		void remove(MobileGameLmntDataNode3D* node);
		void updateNodeBPs(MobileGameLmntDataNode3D* node, Transform3D const& transformDelta) { node->updateBVs(transformDelta); }
		void translateNodeBPs(MobileGameLmntDataNode3D* node, glm::vec3 const& translate) { node->translateBVs(translate); }
		void scaleNodeBPs(MobileGameLmntDataNode3D* node, glm::vec3 const& scale) { node->scaleBVs(scale); }
		void rotateNodeBPs(MobileGameLmntDataNode3D* node, glm::quat const& rot) { node->rotateBVs(rot); }
		Node3D* getStaticNodes3DRoot() const { return staticNodes3DRoot; }
		Node3D* getMobileNodes3DRoot() const { return mobileNodes3DRoot; }
		// Reminder: Right now CollisionData is only 3D (to simplify debugging)
		CollisionsData<glm::vec3> const& getCollisionsData3D();
		RayCollisionData const& getRayCollisionData(glm::vec3 const& rayOrigin, glm::vec3 const& rayDirection);

		// 2D methods
		DataNode2D* insert(AABB2DRotatable const& aabb, unsigned int modelIdx, unsigned int instanceIdx, CollisionPerimeter& collisionPerimeter);
		MobileGameLmntDataNode2D* insert(AABB2DRotatable const& aabb, unsigned int modelIdx, unsigned int instanceIdx, CollisionPerimeter& collisionPerimeter, PhysicsEngine::MobilityInterface const& mobilityInterface);
		void remove(DataNode2D* node);
		void remove(MobileGameLmntDataNode2D* node);
		void updateNodeBPs(MobileGameLmntDataNode2D* node, Transform2D const& transformDelta) { node->updateBPs(transformDelta); }
		void translateNodeBPs(MobileGameLmntDataNode2D* node, glm::vec2 const& translate) { node->translateBPs(translate); }
		void scaleNodeBPs(MobileGameLmntDataNode2D* node, glm::vec2 const& scale) { node->scaleBPs(scale); }
		void rotateNodeBPs(MobileGameLmntDataNode2D* node, std::complex<float> const& rot) { node->rotateBPs(rot); }
		Node2D* getStaticNodes2DRoot() const { return staticNodes2DRoot; }
		Node2D* getMobileNodes2DRoot() const { return mobileNodes2DRoot; }
		// Reminder: Right now CollisionData is only 3D (to simplify debugging)
		CollisionsData<glm::vec2> const& getCollisionsData2D();

	private:
		template <class V>
		struct BroadPhaseCollisionsData {
			unsigned int collisionsNr = 0;
			std::array<CollisionPrimitive<V>*, 2>* collisionPrimitivesDuos;
			CollisionData<V>* collisionsData;
		};
		template <class V>
		struct CollisionsBuffers {
			CollisionsData<V> collisionsData;
			BroadPhaseCollisionsData<V> broadPhaseResBuffer;
			typename Corium3DUtils::SearchTreeAVL<CollisionData<V>>* collisionsRecord;
			typename Corium3DUtils::SearchTreeAVL<CollisionData<V>>::InOrderIt* collisionsRecordIt;
		};

		// 3D pools	
		Corium3DUtils::ObjPool<Node3D>* branchNodes3DPool;
		Corium3DUtils::ObjPool<DataNode3D>* staticNodes3DPool;
		Corium3DUtils::ObjPool<MobileGameLmntDataNode3D>* mobileNodes3DPool;
		// 2D pools
		Corium3DUtils::ObjPool<Node2D>* branchNodes2DPool;
		Corium3DUtils::ObjPool<DataNode2D>* staticNodes2DPool;
		Corium3DUtils::ObjPool<MobileGameLmntDataNode2D>* mobileNodes2DPool;

		// 3D trees roots
		Node3D* staticNodes3DRoot = NULL;
		unsigned int staticNodes3DNr = 0;
		Node3D* mobileNodes3DRoot = NULL;
		unsigned int mobileNodes3DNr = 0;
		// 2D trees roots
		Node2D* staticNodes2DRoot = NULL;
		unsigned int staticNodes2DNr = 0;
		Node2D* mobileNodes2DRoot = NULL;
		unsigned int mobileNodes2DNr = 0;

		// 3D collisions buffers
		CollisionsBuffers<glm::vec3> collisionsBuffers3D;
		//CollisionsData<glm::vec3> collisionsDataBuffer3D;	
		//BroadPhaseCollisionsData<glm::vec3> broadPhaseResBuffer3D;
		//Corium3DUtils::SearchTreeAVL<CollisionData<glm::vec3>>* collisionsRecord3D;
		//Corium3DUtils::SearchTreeAVL<CollisionData<glm::vec3>>::InOrderIt* collisionsRecord3DIt;
		RayCollisionData rayCollisionData;
		// 2D collisions buffers
		CollisionsBuffers<glm::vec2> collisionsBuffers2D;
		//CollisionsData<glm::vec2> collisionsDataBuffer2D;
		//BroadPhaseCollisionsData<glm::vec2> broadPhaseResBuffer2D;
		//Corium3DUtils::SearchTreeAVL<CollisionData<glm::vec2>>* collisionsRecord2D;
		//Corium3DUtils::SearchTreeAVL<CollisionData<glm::vec2>>::InOrderIt* collisionsRecord2DIt;

		template <class TAABB>
		void doRefitBPsDueToUpdate(Node<TAABB>* nodesRoot);
		template <class TAABB, class TNode>
		void doInsert(TNode** nodesRoot, TNode* newNode, Corium3DUtils::ObjPool<TNode>* nodesPool, unsigned int& nodesCounter);
		template <class TAABB, class TNode>
		void doRemove(TNode** nodesRoot, TNode* nodeToRemove, Corium3DUtils::ObjPool<TNode>* nodesPool, unsigned int& nodesCounter);
		template <class TAABB, class TDataNode, class V>
		CollisionsData<V> const& doCollisionsSearch(Node<TAABB>* staticNodesRoot, Node<TAABB>* mobileNodesRoot, CollisionsBuffers<V>& searchBuffers);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// TODO: move these statics to the anonymous part of the translation unit (it is an implementation detail) //
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template <class TAABB>
		static void setSubtreeDepthValues(Node<TAABB>* subtreeRoot);
		template <class TAABB>
		static void reduceSahAndRefitBVs(Node<TAABB>* node);
		template <class TAABB>
		static Node<TAABB>* findNewNodeSibling(Node<TAABB>* root, Node<TAABB>* newNode);
		template <class TAABB>
		static Node<TAABB>* heightUp(Node<TAABB>* origin, unsigned int height);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////
		template <class TAABB, class TDataNode, class V>
		static void runStacklessAlgoIteration(Node<TAABB>** leftSubtreeItPtr, Node<TAABB>** rightSubtreeItPtr, CollisionsBuffers<V>& searchBuffers);
		template <class TAABB, class TDataNode, class V>
		static void runLeafAlgo(Node<TAABB>* leaf, Node<TAABB>* treeRoot, CollisionsBuffers<V>& searchBuffers);
		template <class TDataNode, class V>
		static void recordBroadPhaseCollisionIdxsDuo(TDataNode* node1, TDataNode* node2, BroadPhaseCollisionsData<V>& broadPhaseResBuffer);
		RayCollisionData const& retEmptyRayCollisionData() {
			rayCollisionData.hasCollided = false;
			return rayCollisionData;
		}
	};

} // namespace Corium3D