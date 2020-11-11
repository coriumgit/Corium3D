#pragma once

#include <array>
#include <string>
#include <iostream>
#include <list>
#include <queue>
#include <limits>
#include <functional>
#include <thread>
#include <mutex>

namespace CoriumDirectX {

	template <class T, unsigned int K>
	class KDTree {	
	//private:
	//	class Point;
	//	class Node;	
	//	class Node::KeySphereNode;
	public:										
		class DataNode;	

		typedef std::function<bool(std::array<float, K> const& boundingSphereCenter, float boundingSphereRadius)> TestBoundingSphereVisibility;
		typedef std::function<bool(std::array<float, K> const& aabbMin, std::array<float, K> const& aabbMax)> TestAabbVisibility;

		KDTree(TestBoundingSphereVisibility testBoundingSphereVisibility, TestAabbVisibility testAabbVisibility);
		KDTree(KDTree const&) = delete;
		~KDTree();

		DataNode* insert(std::array<float, K> const& boundingSphereCenter, float boundingSphereRadius, T& data);
		void destroy(DataNode* node);
		void translateNodeBoundingSphere(DataNode* node, std::array<float, K> vec);
		void setTranslationForNodeBoundingSphere(DataNode* node, std::array<float, K> vec);
		void scaleNodeBoundingSphere(DataNode* node, float scaleFactor);
		void setRadiusForNodeBoundingSphere(DataNode* node, float radius);
		bool isNodeVisible(DataNode* node);
		void initVisibleNodesIt();
		T* getNextVisibleNodeData();
		void rebuildTree();

#if _DEBUG
		void print();
#endif

	private:	
		class Point {
		public:
			Point() {}

			Point(std::array<float, K> _coords) : coords(_coords) {}
			void fill(float val) { coords.fill(val); }
			std::array<float, K> const& getArr() const { return coords; }

			Point operator+(Point const& other) const {
				Point res = this->coords;
				res += other;

				return res;
			}

			Point& operator+=(Point const& other) {
				for (unsigned int dim = 0; dim < K; dim++)
					coords[dim] += other.coords[dim];

				return *this;
			}

			float operator[](unsigned int dim) const {
				_ASSERT(dim < K);
				return coords[dim];
			}

			float& operator[](unsigned int dim) {
				_ASSERT(dim < K);
				return coords[dim];
			}

#if _DEBUG
			friend std::ostream& operator<<(std::ostream& out, KDTree<T, K>::Point const& keyPoint);
#endif

		private:
			std::array<float, K> coords;
		};

		class KeySphere {
		public:
			KeySphere() {}

			KeySphere(Point const& center, float radius) : c(center), r(radius) {}

			Point const& getCenter() const { return c; }
			Point& getCenter() { return c; }
			float getRadius() const { return r; }
			float& getRadius() { return r; }

		private:
			Point c; // center
			float r; // radius
		};

		class Region {
		public:
			struct Split {
				unsigned int dim;
				float coord;
			};

			enum Containment { Complete, Partial, None };

			Region();
			void genRegionsViaSplit(Split const& split, Region& minRegionOut, Region& maxRegionOut) const;
			bool doesContain(KeySphere const& keySphere) const;
			Point const& getMin() const { return min; }
			Point const& getMax() const { return max; }

		private:
			Point min;
			Point max;
		};

		class Node;		
		class RebuildNode;
		class VisibleDataNodesIt;

		struct RebuildActiveNodesDuo {		
			//std::mutex mutex;
			DataNode* activeNode = NULL;
			RebuildNode* rebuildNode = NULL;

			RebuildActiveNodesDuo(DataNode* _activeNode = NULL, RebuildNode* _rebuildNode = NULL) : activeNode(_activeNode), rebuildNode(_rebuildNode) {}
		};
		
		struct RebuildPeriodMsg {
			enum class Action { Insert, Destroy, Update };
			
			Action action;
			typename const std::list<RebuildActiveNodesDuo>::iterator nodesDuosIt;			
		};

		Node* nodesRoot = NULL;
		//std::mutex activeNodesThreadMutex;

		bool isRebuilding = false;				

		std::list<RebuildActiveNodesDuo> nodesDuos;
		//std::mutex nodesDuosMutex;		
		std::queue<RebuildPeriodMsg> rebuildPeriodMsgs;		

		VisibleDataNodesIt visibleDataNodesIt;
		TestBoundingSphereVisibility isBoundingSphereVisible;

		void insert(Node* root, typename Node::KeySphereNode* nodeToInsert);
		bool chooseSplitDim(KeySphere const& p1, KeySphere const& p2, unsigned int& chosenDim);
		void remove(typename Node::KeySphereNode* node);
		void deleteNodeRecurse(Node* node);		
		void reevaluateNodePosSYNC(DataNode* node);
		void reevaluateNodePos(typename Node::KeySphereNode* node);
		void rebuildTreeEXE();
		void rebuildTreeRecurs(std::array<std::vector<RebuildNode*>, K>& sortedDataNodes, unsigned int startIdx, unsigned int endIdx, KDTree<T, K>::Node& newNode, unsigned int rankDim);
		//std::vector<DataNode*> dataNodesMergeSort(std::vector<DataNode*>& dataNodes, unsigned int dim, unsigned int startIdx, unsigned int endIdx);

#if _DEBUG
		static const unsigned int SPACE_INIT = 10;
		void printRecurs(Node* root, unsigned int space);
#endif	
	};	

	template<class T, unsigned int K>
	class KDTree<T,K>::Node {
	public:
		friend KDTree<T,K>;

		class KeySphereNode;

		Node() {}
		Node(typename Region::Split const& _regionSplit) : regionSplit(_regionSplit) { splitNode(); }
		~Node() {}
		void assignSplit(typename Region::Split const& regionSplit);
		bool isLeftChild() const { return parent->children[0] == this; }
		bool isRightChild() const { return parent->children[1] == this; }
		void replaceLeftChild(Node* child);
		void replaceRightChild(Node* child);
		Node* getLeftChild() const { return children[0]; }
		Node* getRightChild() const { return children[1]; }
		bool isLeaf() const { return children[0] == NULL; }
		void addKeySphereNode(KeySphereNode* keySphereNode) {
			keySphereNodes.push_back(keySphereNode);
			_ASSERT(keySphereNode->owner == NULL);
			keySphereNode->owner = this;
			keySphereNode->ownerKeySphereNodesListIt = keySphereNodes.end();
			--keySphereNode->ownerKeySphereNodesListIt;
		}

#if _DEBUG
		void setName(std::string const& _name) { name = _name; }
#endif					

	private:
		Node* parent = NULL;
		Node* children[2] = { NULL, NULL };
		typename Region::Split regionSplit;
		Region containingRegion;
		std::list<KeySphereNode*> keySphereNodes;
#if _DEBUG
		std::string name = std::string("*");
#endif

		Node(Node* _parent, Region const& _containingRegion) : parent(_parent), containingRegion(_containingRegion) {}
		void splitNode();
	};

	template<class T, unsigned int K>
	class KDTree<T,K>::Node::KeySphereNode {
	public:
		friend KDTree<T, K>;
		friend Node;

		KeySphereNode(KeySphere const& _keySphere) : keySphere(_keySphere) {}
		~KeySphereNode() {}
		void updateKeySphere(KeySphere const& keySphere) { this->keySphere = keySphere; }
		KeySphere const& getKeySphere() const { return keySphere; }
		void removeFromOwner() {
			_ASSERT(owner != NULL);
			owner->keySphereNodes.erase(ownerKeySphereNodesListIt);
			owner = NULL;
		}

		bool isContainedWithinOwnerRegion() { return owner->containingRegion.doesContain(keySphere); }

	protected:
		KeySphere keySphere;
		Node* owner;
		typename std::list<KeySphereNode*>::iterator ownerKeySphereNodesListIt;
	};


	template <class T, unsigned int K>
	class KDTree<T, K>::DataNode : private KDTree<T, K>::Node::KeySphereNode {
	public:
		friend KDTree;

		T& getData() const { return data; }

#if _DEBUG
		void setName(std::string const& _name) { name = _name; }
#endif

	private:
		T& data;
		typename const std::list<RebuildActiveNodesDuo>::iterator nodesDuosIt;
#if _DEBUG
		std::string name = std::string("*");
#endif

		DataNode(KeySphere const& _keySphere, T& _data, typename std::list<RebuildActiveNodesDuo>::iterator const& _nodesDuosIt) :
			Node::KeySphereNode(_keySphere), data(_data), nodesDuosIt(_nodesDuosIt) {}
		~DataNode() {}
	};

	template<class T, unsigned int K>
	class KDTree<T,K>::RebuildNode : public KDTree<T, K>::Node::KeySphereNode {
	public:
		friend KDTree;

		RebuildNode(DataNode const& dataNode) : Node::KeySphereNode(dataNode.keySphere), nodesDuosIt(dataNode.nodesDuosIt) {}
		~RebuildNode() {}

		KeySphere const& getKeyPoint() const { return keySphere; }

	private:
		typename const std::list<RebuildActiveNodesDuo>::iterator nodesDuosIt;
	};

	template<class T, unsigned int K>
	class KDTree<T,K>::VisibleDataNodesIt {
	public:
		VisibleDataNodesIt(KDTree<T, K>& _owner, TestBoundingSphereVisibility testBoundingSphereVisibility, TestAabbVisibility testAabbVisibility) :
			owner(_owner), isBoundingSphereVisible(testBoundingSphereVisibility), isAabbVisible(testAabbVisibility) {
			init();
		}

		void init() {
			if (owner.nodesRoot) {
				nodesIt = owner.nodesRoot;
				dataNodesIt = nodesIt->keySphereNodes.begin();
			}
		}

		T* getNext() {
			while (nodesIt) {				
				while (dataNodesIt != nodesIt->keySphereNodes.end() && !isBoundingSphereVisible((*dataNodesIt)->keySphere.getCenter().getArr(), (*dataNodesIt)->keySphere.getRadius()))
					++dataNodesIt;
				
				if (dataNodesIt != nodesIt->keySphereNodes.end())
					return &static_cast<DataNode*>(*dataNodesIt++)->getData();
				else {
					bool isLeftChildVisible = false;					
					if (!nodesIt->isLeaf() && (isLeftChildVisible = isRegionVisible(nodesIt->getLeftChild()->containingRegion) || isRegionVisible(nodesIt->getRightChild()->containingRegion))) {
						if (isLeftChildVisible)
							nodesIt = nodesIt->getLeftChild();													
						else
							nodesIt = nodesIt->getRightChild();							
						dataNodesIt = nodesIt->keySphereNodes.begin();
					}
					else {					
						do {
							while (nodesIt->parent && nodesIt->isRightChild())
								nodesIt = nodesIt->parent;

							nodesIt = nodesIt->parent;
						} while (nodesIt && !isRegionVisible(nodesIt->getRightChild()->containingRegion));

						if (nodesIt) {
							nodesIt = nodesIt->getRightChild();
							dataNodesIt = nodesIt->keySphereNodes.begin();
						}
					}						
				}
			}

			return NULL;
		}

	private:
		KDTree<T, K> const& owner;
		Node* nodesIt = NULL;
		typename std::list<typename Node::KeySphereNode*>::iterator dataNodesIt;
		TestBoundingSphereVisibility isBoundingSphereVisible;
		TestAabbVisibility isAabbVisible;

		bool isRegionVisible(Region const& region) { return isAabbVisible(region.getMin().getArr(), region.getMax().getArr()); }
	};

	template<class T, unsigned int K>
	void KDTree<T, K>::Node::assignSplit(typename Region::Split const& regionSplit) {
		this->regionSplit = regionSplit;
		splitNode();
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::Node::splitNode() {
		Region leftChildRegion, rightChildRegion;
		containingRegion.genRegionsViaSplit(regionSplit, leftChildRegion, rightChildRegion);
		children[0] = new Node(this, leftChildRegion);
		children[1] = new Node(this, rightChildRegion);
	}

	template<class T, unsigned int K>
	void KDTree<T,K>::Node::replaceLeftChild(KDTree<T,K>::Node* child) {
		child->parent = this;
		child->containingRegion = children[0]->containingRegion;
		children[0] = child;
	}

	template<class T, unsigned int K>
	void KDTree<T,K>::Node::replaceRightChild(KDTree<T,K>::Node* child) {
		child->parent = this;
		child->containingRegion = children[1]->containingRegion;
		children[1] = child;
	}
	
	template<class T, unsigned int K>
	KDTree<T,K>::Region::Region() {
		min.fill(-(std::numeric_limits<float>::max)());
		max.fill((std::numeric_limits<float>::max)());
	}

	template<class T, unsigned int K>
	void KDTree<T,K>::Region::genRegionsViaSplit(KDTree<T, K>::Region::Split const& split, KDTree<T,K>::Region& minRegionOut, KDTree<T,K>::Region& maxRegionOut) const {
		minRegionOut = maxRegionOut = *this;
		minRegionOut.max[split.dim] = maxRegionOut.min[split.dim] = split.coord;		 
	}

	template<class T, unsigned int K>
	bool KDTree<T,K>::Region::doesContain(KeySphere const& keySphere) const {
		Point const& sphereC = keySphere.getCenter();
		float sphereR = keySphere.getRadius();		
		for (unsigned int dim = 0; dim < K; dim++) {
			if (sphereC[dim] + sphereR < min[dim] || max[dim] < sphereC[dim] - sphereR)
				return false;
		}

		return true;
	}

#ifdef _DEBUG
	template<class T, unsigned K>
	std::ostream& operator<<(std::ostream& out, typename KDTree<T,K>::Point const& keyPoint) {
		out << "(";
		for (unsigned int dim = 0; dim < K; dim++) {
			out << std::to_string(keyPoint[dim]);
			if (dim < K - 1)
				out << ",";
		}

		return out << ")";			
	}
#endif

	template<class T, unsigned int K>
	inline KDTree<T, K>::KDTree(TestBoundingSphereVisibility testBoundingSphereVisibility, TestAabbVisibility testAabbVisibility) :
			visibleDataNodesIt(*this, testBoundingSphereVisibility, testAabbVisibility), isBoundingSphereVisible(testBoundingSphereVisibility) {
		static_assert(0 < K, "KDTree's K must be bigger than 0"); 
	}
	

	template<class T, unsigned int K>
	KDTree<T,K>::~KDTree() {
		if (nodesRoot != NULL)
			deleteNodeRecurse(nodesRoot);
	}

	template<class T, unsigned int K>
	void KDTree<T,K>::deleteNodeRecurse(KDTree<T,K>::Node* node) {
		if (node->isLeaf()) {
			delete node;
			return;
		}

		deleteNodeRecurse(node->getLeftChild());
		deleteNodeRecurse(node->getRightChild());
		delete node;
	}

	template<class T, unsigned int K>
	typename KDTree<T,K>::DataNode* KDTree<T,K>::insert(std::array<float, K> const& boundingSphereCenter, float boundingSphereRadius, T& data) {						
		if (isRebuilding) {			
			//activeNodesThreadMutex.lock();
			if (isRebuilding) {
				//nodesDuosMutex.lock();
				nodesDuos.emplace_front();
				//nodesDuosMutex.unlock();
				rebuildPeriodMsgs.push(RebuildPeriodMsg{ RebuildPeriodMsg::Action::Insert, nodesDuos.begin() });				
			}
			else				
				nodesDuos.emplace_front();							

			//activeNodesThreadMutex.unlock();
		}
		else			
			nodesDuos.emplace_front();
		
		DataNode* newDataNode = new DataNode(KeySphere(boundingSphereCenter, boundingSphereRadius), data, nodesDuos.begin());
		nodesDuos.front().activeNode = newDataNode;
		insert(nodesRoot, newDataNode);
		

		return newDataNode;
	}

	template<class T, unsigned int K>
	void KDTree<T,K>::destroy(DataNode* node) {				
		if (isRebuilding) {
			//activeNodesThreadMutex.lock();
			if (isRebuilding) {
				//node->nodesDuosIt->mutex.lock();

				node->nodesDuosIt->activeNode = NULL;
				if (node->nodesDuosIt->rebuildNode)
					rebuildPeriodMsgs.push(RebuildPeriodMsg{ RebuildPeriodMsg::Action::Destroy, node->nodesDuosIt });

				//node->nodesDuosIt->mutex.unlock();
			}
			else
				nodesDuos.erase(node->nodesDuosIt);

			//activeNodesThreadMutex.unlock();
		}
		else			
			nodesDuos.erase(node->nodesDuosIt);		
		
		remove(node);

		delete node;
	}	

	template<class T, unsigned int K>
	void KDTree<T,K>::translateNodeBoundingSphere(DataNode* node, std::array<float, K> vec) {
		node->keySphere.getCenter() = node->getKeySphere().getCenter() + Point(vec);
		reevaluateNodePosSYNC(node);
	}	

	template<class T, unsigned int K>
	void KDTree<T, K>::setTranslationForNodeBoundingSphere(DataNode* node, std::array<float, K> vec) {
		node->keySphere.getCenter() = Point(vec);
		reevaluateNodePosSYNC(node);
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::scaleNodeBoundingSphere(DataNode* node, float scaleFactor) {
		node->keySphere.getRadius() *= scaleFactor;
		reevaluateNodePosSYNC(node);
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::setRadiusForNodeBoundingSphere(DataNode* node, float radius) {
		node->keySphere.getRadius() = radius;
		reevaluateNodePosSYNC(node);
	}

	template<class T, unsigned int K>
	bool KDTree<T, K>::isNodeVisible(DataNode* node) {
		return isBoundingSphereVisible(node->keySphere.getCenter().getArr(), node->keySphere.getRadius());		
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::initVisibleNodesIt() {
		visibleDataNodesIt.init();
	}

	template<class T, unsigned int K>
	T* KDTree<T, K>::getNextVisibleNodeData() {
		return visibleDataNodesIt.getNext();
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::reevaluateNodePosSYNC(DataNode* node) {
		if (isRebuilding) {
			//activeNodesThreadMutex.lock();
			if (isRebuilding) {
				//node->nodesDuosIt->mutex.lock();
				reevaluateNodePos(node);

				if (node->nodesDuosIt->rebuildNode)
					rebuildPeriodMsgs.push(RebuildPeriodMsg{ RebuildPeriodMsg::Action::Update, node->nodesDuosIt });

				//node->nodesDuosIt->mutex.unlock();
			}
			else
				reevaluateNodePos(node);
			
			//activeNodesThreadMutex.unlock();
		}
		else
			reevaluateNodePos(node);
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::reevaluateNodePos(typename Node::KeySphereNode* node) {		
		if (!node->isContainedWithinOwnerRegion()) {
			remove(node);
			insert(nodesRoot, node);
		}
		else if (!node->owner->isLeaf()) {
			KeySphere const& nodeKeySphere = node->getKeySphere();
			float sphereCenterSplitDimCoord = nodeKeySphere.getCenter()[node->owner->regionSplit.dim];
			if (sphereCenterSplitDimCoord + nodeKeySphere.getRadius() < node->owner->regionSplit.coord) {
				Node* nodeOwnerCurr = node->owner;
				remove(node);
				insert(nodeOwnerCurr->getLeftChild(), node);
			}
			else if (sphereCenterSplitDimCoord - nodeKeySphere.getRadius() > node->owner->regionSplit.coord) {
				Node* nodeOwnerCurr = node->owner;
				remove(node);
				insert(nodeOwnerCurr->getRightChild(), node);
			}
		}
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::rebuildTree() {
		std::thread t(&KDTree<T, K>::rebuildTreeEXE, this);
		t.detach();		
	}
	
	template<class T, unsigned int K>
	void KDTree<T, K>::rebuildTreeEXE() {
		unsigned int dataNodesNr = nodesDuos.size();
		std::array<std::vector<RebuildNode*>, K> sortedDataNodesSets;
		for (unsigned int dim = 0; dim < K; dim++)
			sortedDataNodesSets[dim].resize(dataNodesNr);

		unsigned int nodesDuoIdx = 0;	
		//nodesDuosMutex.lock();
		std::list<RebuildActiveNodesDuo>::iterator nodesDuosIt = nodesDuos.begin();
		//nodesDuosMutex.unlock();
		while(nodesDuosIt != nodesDuos.end()) {
			//nodesDuosIt->mutex.lock();
			DataNode* activeNode = nodesDuosIt->activeNode;
			if (activeNode) {
				nodesDuosIt->rebuildNode = new RebuildNode(*activeNode);
				for (unsigned int dim = 0; dim < K; dim++)
					sortedDataNodesSets[dim][nodesDuoIdx] = nodesDuosIt->rebuildNode;
				nodesDuosIt++;
			}
			else
				nodesDuosIt = nodesDuos.erase(nodesDuosIt);			
		}				

		for (unsigned int dim = 0; dim < K; dim++)
			std::sort(sortedDataNodesSets[dim].begin(), sortedDataNodesSets[dim].end(), 
					  [dim](DataNode* n1, DataNode* n2) { return n1->keySphere.getCenter()[dim] < n2->keySphere.getCenter()[dim]; });
		//sortedDataNodesSets[dim] = dataNodesMergeSort(sortedDataNodesSets[dim], dim, 0, dataNodesNr - 1);
		
		Node* nodesRootRebuilt = new Node();
		rebuildTreeRecurs(sortedDataNodesSets, 0, dataNodesNr - 1, *nodesRootRebuilt, 0);
		
		//activeNodesThreadMutex.lock();		
		std::swap(nodesRoot, nodesRootRebuilt);
		while (!rebuildPeriodMsgs.empty()) {
			RebuildPeriodMsg const& msg = rebuildPeriodMsgs.front();
			switch (msg.action) {
			case RebuildPeriodMsg::Action::Insert:
				msg.nodesDuosIt->rebuildNode = new RebuildNode(*msg.nodesDuosIt->activeNode);
				insert(nodesRoot, msg.nodesDuosIt->rebuildNode);
				break;

			case RebuildPeriodMsg::Action::Destroy:
				remove(msg.nodesDuosIt->rebuildNode);
				delete msg.nodesDuosIt->rebuildNode;
				nodesDuos.erase(msg.nodesDuosIt);	
				break;

			case RebuildPeriodMsg::Action::Update:
				msg.nodesDuosIt->rebuildNode->keySphere = msg.nodesDuosIt->activeNode->keySphere;
				reevaluateNodePos(msg.nodesDuosIt->rebuildNode);
				break;
			}
			rebuildPeriodMsgs.pop();
		}

		for (nodesDuosIt = nodesDuos.begin(); nodesDuosIt != nodesDuos.end(); nodesDuosIt++) {	
			Node* activeNodeOwner = nodesDuosIt->activeNode->owner;
			Node* rebuildNodeOwner = nodesDuosIt->rebuildNode->owner;
			nodesDuosIt->activeNode->removeFromOwner();
			nodesDuosIt->rebuildNode->removeFromOwner();
			rebuildNodeOwner->addKeySphereNode(nodesDuosIt->activeNode);
			activeNodeOwner->addKeySphereNode(nodesDuosIt->rebuildNode);
			nodesDuosIt->rebuildNode = NULL;
		}

		isRebuilding = false;

		//activeNodesThreadMutex.unlock();	
		
		deleteNodeRecurse(nodesRootRebuilt);
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::rebuildTreeRecurs(std::array<std::vector<RebuildNode*>, K>& sortedDataNodes, unsigned int startIdx, unsigned int endIdx, KDTree<T, K>::Node& newNode, unsigned int rankDim) {
		/*
		if (endIdx - startIdx == 2) {
			RebuildNode* rebuildNodes[3] = { sortedDataNodes[0][startIdx], sortedDataNodes[1][startIdx], sortedDataNodes[2][endIdx] };
			float keySphereCenterCoords[3] = { rebuildNodes[0]->keySphere.getCenter()[rankDim], rebuildNodes[1]->keySphere.getCenter()[rankDim], rebuildNodes[2]->keySphere.getCenter()[rankDim] };
			float splitCoord = 0.5f * (keySphereCenterCoords[1] + keySphereCenterCoords[2]);
			newNode.assignSplit(Region::Split{ rankDim, splitCoord });

			if (keySphereCenterCoords[0] + rebuildNodes[0]->keySphere.getRadius() < splitCoord)
				newNode.getLeftChild()->addKeySphereNode(rebuildNodes[0]);
			else
				newNode.addKeySphereNode(rebuildNodes[0]);

			if (keySphereCenterCoords[1] + rebuildNodes[1]->keySphere.getRadius() < splitCoord)
				newNode.getLeftChild()->addKeySphereNode(rebuildNodes[1]);
			else
				newNode.addKeySphereNode(rebuildNodes[1]);

			if (keySphereCenterCoords[2] - rebuildNodes[2]->keySphere.getRadius() > splitCoord)
				newNode.getRightChild()->addKeySphereNode(rebuildNodes[2]);
			else
				newNode.addKeySphereNode(rebuildNodes[2]);

			return;
		}
		else */if (endIdx - startIdx == 1) {
			RebuildNode* rebuildNodes[2] = { sortedDataNodes[0][startIdx], sortedDataNodes[0][endIdx] };
			float keySphereCenterCoords[2] = { rebuildNodes[0]->keySphere.getCenter()[rankDim], rebuildNodes[1]->keySphere.getCenter()[rankDim] };			
			float splitCoord = 0.5f * (keySphereCenterCoords[0] + keySphereCenterCoords[1]);
			newNode.assignSplit(Region::Split{ rankDim, splitCoord });

			if (keySphereCenterCoords[0] + rebuildNodes[0]->keySphere.getRadius() < splitCoord)
				newNode.getLeftChild()->addKeySphereNode(rebuildNodes[0]);
			else 
				newNode.addKeySphereNode(rebuildNodes[0]);

			if (keySphereCenterCoords[1] - rebuildNodes[1]->keySphere.getRadius() > splitCoord)
				newNode.getRightChild()->addKeySphereNode(rebuildNodes[1]);
			else
				newNode.addKeySphereNode(rebuildNodes[1]);			

			return;
		}
		else if (endIdx == startIdx) {
			newNode.addKeySphereNode(sortedDataNodes[rankDim][startIdx]);
			return;
		}
		else if (endIdx < startIdx)
			return;
				
		std::vector<RebuildNode*> dim0SortedDataNodesCpy(sortedDataNodes[0]);
		unsigned int medianNodeIdx = (startIdx + endIdx) / 2;		
		float splitCoord = 0.5f * (sortedDataNodes[0][medianNodeIdx]->keySphere.getCenter()[rankDim] + sortedDataNodes[0][medianNodeIdx + 1]->keySphere.getCenter()[rankDim]);
		unsigned int srcIdxIt = startIdx;
		unsigned int nodesToGoLeftEndIdx = startIdx - 1;
		unsigned int nodesToGoRightStartIdx = endIdx + 1;
		while (srcIdxIt <= endIdx) {
			float srcKeySphereCenterCoord = sortedDataNodes[0][srcIdxIt]->keySphere.getCenter()[rankDim];
			float srcKeySphereRadius = sortedDataNodes[0][srcIdxIt]->keySphere.getRadius();
			if (srcKeySphereCenterCoord + srcKeySphereRadius < splitCoord)
				++nodesToGoLeftEndIdx;
			else if (srcKeySphereCenterCoord - srcKeySphereRadius > splitCoord)
				--nodesToGoRightStartIdx;
			else
				newNode.addKeySphereNode(sortedDataNodes[0][srcIdxIt++]);
		}

		for (unsigned int dim = 1; dim < K; dim++) {
			srcIdxIt = startIdx;
			unsigned int dstLeftIdxIt = startIdx;
			unsigned int dstRightIdxIt = endIdx;
			while (srcIdxIt <= endIdx) {
				float srcKeySphereCenterCoord = sortedDataNodes[dim][srcIdxIt]->keySphere.getCenter()[rankDim];
				float srcKeySphereRadius = sortedDataNodes[dim][srcIdxIt]->keySphere.getRadius();
				if (srcKeySphereCenterCoord + srcKeySphereRadius < splitCoord)
					sortedDataNodes[dim - 1][dstLeftIdxIt++] = sortedDataNodes[dim][srcIdxIt++];
				else if (srcKeySphereCenterCoord - srcKeySphereRadius > splitCoord)
					sortedDataNodes[dim - 1][dstRightIdxIt--] = sortedDataNodes[dim][srcIdxIt++];				
			}									
		}

		std::copy(dim0SortedDataNodesCpy.begin(), dim0SortedDataNodesCpy.end(), sortedDataNodes[K - 1].begin());
		
		newNode.assignSplit(Region::Split{ rankDim, splitCoord });		
		rebuildTreeRecurs(sortedDataNodes, startIdx, nodesToGoLeftEndIdx, *newNode.getLeftChild(), (rankDim + 1) % K);
		rebuildTreeRecurs(sortedDataNodes, nodesToGoRightStartIdx, endIdx, *newNode.getRightChild(), (rankDim + 1) % K);		
	}

	/*
	template<class T, unsigned int K>
	std::vector<KDTree<T,K>::DataNode*> KDTree<T, K>::dataNodesMergeSort(std::vector<KDTree<T, K>::DataNode*>& dataNodes, unsigned int dim, unsigned int startIdx, unsigned int endIdx) {				
		unsigned int sortedLmntsNr = endIdx - startIdx + 1;
		std::vector<DataNode*> sortedLmnts(sortedLmntsNr);
		if (endIdx == startIdx) {
			sortedLmnts[0] = dataNodes[startIdx];
			
			return sortedLmnts;
		}		
		else if (endIdx - startIdx == 1) {			
			if (dataNodes[startIdx]->keyPoint[dim] < dataNodes[endIdx]->keyPoint[dim]) {
				sortedLmnts[0] = dataNodes[startIdx];
				sortedLmnts[1] = dataNodes[endIdx];
			}
			else {
				sortedLmnts[0] = dataNodes[endIdx];
				sortedLmnts[1] = dataNodes[startIdx];
			}

			return sortedLmnts;
		}
		
		unsigned int midIdx = (startIdx + endIdx) / 2;
		std::vector<DataNode*> left = dataNodesMergeSort(dataNodes, dim, startIdx, midIdx);
		std::vector<DataNode*> right = dataNodesMergeSort(dataNodes, dim, midIdx + 1, endIdx);

		std::vector<DataNode*>::iterator leftIt(left.begin()), rightIt(right.begin()), sortedLmntsIt(sortedLmnts.begin());		
		while (leftIt != left.end() && rightIt != right.end() ){
			if ((*leftIt)->keyPoint[dim] < (*rightIt)->keyPoint[dim]) {
				*sortedLmntsIt = (*leftIt);
				++leftIt;
			}
			else {
				*sortedLmntsIt = (*rightIt);
				++rightIt;
			}

			++sortedLmntsIt;
		}

		if (leftIt != left.end())
			std::copy(leftIt, left.end(), sortedLmntsIt);
		else
			std::copy(rightIt, right.end(), sortedLmntsIt);

		return sortedLmnts;		
	}	
	*/

#if _DEBUG
	template<class T, unsigned int K>
	void KDTree<T,K>::print() {
		std::cout << "========================================================" << std::endl;
		std::cout << "======================== KDTree ========================" << std::endl;
		std::cout << "========================================================" << std::endl;
		printRecurs(nodesRoot, 0);
		std::cout << "========================================================" << std::endl;
	}

	template<class T, unsigned int K>
	void KDTree<T, K>::printRecurs(Node* root, unsigned int space) {
		// Base case  
		if (root == NULL)
			return;

		// Increase distance between levels  
		space += SPACE_INIT;

		// Process right child first  
		printRecurs(root->getRightChild(), space);

		// Print current node after space count  
		std::cout << std::endl;
		for (int i = SPACE_INIT; i < space; i++)
			std::cout << " ";
		
		if (!root->isLeaf())
			std::cout << root->name << "-> (" << root->regionSplit.dim << "|" << root->regionSplit.coord << ")\n";			
		else
			std::cout << root->name << "-> []\n";

		// Process left child  
		printRecurs(root->getLeftChild(), space);
	}
#endif 

	template<class T, unsigned int K>
	void KDTree<T, K>::insert(Node* root, typename Node::KeySphereNode* nodeToInsert) {		
		if (root) {
			KeySphere const& nodeToInsertSphere = nodeToInsert->keySphere;
			Node* nodesIt = root;			
			while (!nodesIt->isLeaf()) {
				Region::Split const& regionSplit = nodesIt->regionSplit;
				if (nodeToInsertSphere.getCenter()[regionSplit.dim] + nodeToInsertSphere.getRadius() < nodesIt->regionSplit.coord)
					nodesIt = nodesIt->getLeftChild();
				else if (nodeToInsertSphere.getCenter()[regionSplit.dim] - nodeToInsertSphere.getRadius() > nodesIt->regionSplit.coord)
					nodesIt = nodesIt->getRightChild();
				else
					break;
			}

			if (!nodesIt->isLeaf() || nodesIt->keySphereNodes.empty())
				nodesIt->addKeySphereNode(nodeToInsert);
			else {
				Node::KeySphereNode* leafContainedNode = nodesIt->keySphereNodes.front();
				unsigned int maxDiffDim;
				bool couldFindSeparatingPlane = chooseSplitDim(nodeToInsertSphere, leafContainedNode->keySphere, maxDiffDim);
				Region::Split leafRegionSplit = { maxDiffDim, 0.5f * (nodeToInsertSphere.getCenter()[maxDiffDim] + leafContainedNode->keySphere.getCenter()[maxDiffDim]) };
				nodesIt->assignSplit(leafRegionSplit);
				if (couldFindSeparatingPlane) {				
					leafContainedNode->removeFromOwner();
					if (nodeToInsertSphere.getCenter()[maxDiffDim] < leafContainedNode->keySphere.getCenter()[maxDiffDim]) {
						nodesIt->getLeftChild()->addKeySphereNode(nodeToInsert);
						nodesIt->getRightChild()->addKeySphereNode(leafContainedNode);
					}
					else {
						nodesIt->getLeftChild()->addKeySphereNode(leafContainedNode);
						nodesIt->getRightChild()->addKeySphereNode(nodeToInsert);
					}
				}
				else
					nodesIt->addKeySphereNode(nodeToInsert);																												
			}			
		}
		else {
			nodesRoot = new Node();
			nodesRoot->addKeySphereNode(nodeToInsert);
		}
	}	

	inline float calcSegsDiff(float seg1center, float seg1HalfLen, float seg2center, float seg2HalfLen) {
		if (seg1center < seg2center)
			return seg2center - seg2HalfLen - seg1center - seg1HalfLen;
		else
			return seg1center - seg1HalfLen - seg2center - seg2HalfLen;
	}

	template<class T, unsigned int K>
	bool KDTree<T, K>::chooseSplitDim(KeySphere const& s1, KeySphere const& s2, unsigned int& chosenDim) {		
		Point const& s1c = s1.getCenter();
		Point const& s2c = s2.getCenter();
		float s1r = s1.getRadius();
		float s2r = s2.getRadius();

		chosenDim = 0;
		float maxDiff = calcSegsDiff(s1c[0], s1r, s2c[0], s2r);		
		for (unsigned int dim = 1; dim < K; ++dim) {
			float d = calcSegsDiff(s1c[dim], s1r, s2c[dim], s2r);			
			if (d > maxDiff) {
				chosenDim = dim;
				maxDiff = d;
			}
		}

		if (maxDiff > 0)
			return true;
		else
			return false;
	}	

	template<class T, unsigned int K>
	void KDTree<T, K>::remove(typename Node::KeySphereNode* node) {
		node->removeFromOwner();
	}
}



