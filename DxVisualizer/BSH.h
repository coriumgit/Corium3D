#pragma once

#include "BoundingSphere.h"

#include <vector>
#include <array>
#include <string>

namespace CoriumDirectX {
	
	const float FATTENING_FACTOR = 1.1f;

	template <class T>
	class BSH {
	public:						
		class Node {
		public:			
			friend BSH;

			Node* getParent() const { return parent; }
			Node* getChild(unsigned int childIdx) const { return children[childIdx]; }
			Node* getEscapeNode() const { return escapeNode; }
			bool isLeaf() const { return children[0] == NULL; }
			BoundingSphere const& getBoundingSphere() const { return boundingSphere; }
#if _DEBUG
			void setName(std::string const& _name) { name = _name; }
#endif
		protected:			
			Node(BoundingSphere const& boundingSphere);
			Node(Node* leftChild, Node* rightChild);			
			~Node() {}
			bool isLeftChild() const { return parent->children[0] == this; }
			bool isRightChild() const { return parent->children[1] == this; }
			void replaceChild(Node* child, unsigned int childIdx);
			void refitBS();			

		private:			
#if _DEBUG
			std::string name = std::string("*");
#endif
			Node* parent = NULL;
			Node* escapeNode = NULL;
			BoundingSphere boundingSphere;
			BoundingSphere boundingSphereFattened;
			Node* children[2] = { NULL, NULL };			
		};
		
		class DataNode : public Node {
		public:
			friend BSH;			

			T& getData() const { return data; }

		private:
			T& data;

			DataNode(BoundingSphere const& boundingSphere, T& data);			
			~DataNode() {}
		};		

		BSH() {}
		BSH(BSH const&) = delete;
		~BSH();		

		DataNode* insert(BoundingSphere const& boundingSphere, T& data);
		void destroy(DataNode* node);
		void translateNodeBS(DataNode* node, DirectX::XMVECTOR const& translate);
		void setTranslationForNodeBS(DataNode* node, DirectX::XMVECTOR const& translation);
		void scaleNodeBS(DataNode* node, float scaleFactor);
		void setRadiusForNodeBS(DataNode* node, float radius);
		Node* getNodesRoot() const { return nodesRoot; }						

	private:				
		Node* nodesRoot = NULL;			

		void insert(DataNode* node);
		void remove(DataNode* node);
		void testBoundingSphereContainmentAndUpdate(DataNode* node);

		static Node* findNewNodeSibling(Node* root, Node* newNode);	
		static void refitBoundingSpheresUpTree(Node* node);
		static void deleteNodeRecurse(Node* node);		
	};

	template <class T>
	BSH<T>::Node::Node(BoundingSphere const& _boundingSphere) :
		boundingSphere(_boundingSphere), boundingSphereFattened(BoundingSphere::calcFattenedBoundingSphere(boundingSphere, FATTENING_FACTOR)) {}

	template <class T>
	BSH<T>::Node::Node(Node* leftChild, Node* rightChild) :
		Node(BoundingSphere::calcCombinedBoundingSphere(leftChild->boundingSphere, rightChild->boundingSphere)) {
		leftChild->parent = rightChild->parent = this;
		children[0] = leftChild;
		children[1] = rightChild;
		rightChild->escapeNode = this->escapeNode;
		Node* nodesIt = leftChild;
		do {
			nodesIt->escapeNode = rightChild;
			nodesIt = nodesIt->children[1];
		} while (nodesIt);
	}

	template <class T>
	inline void BSH<T>::Node::replaceChild(Node* child, unsigned int childIdx) {
		child->parent = this;

		Node* nodesIt;
		if (childIdx == 0)
			child->escapeNode = children[1];
		else { // childIdx == 1
			child->escapeNode = this->escapeNode;
			nodesIt = children[0];
			do {
				nodesIt->escapeNode = child;
				nodesIt = nodesIt->children[1];
			} while (nodesIt);
		}

		nodesIt = child->children[1];
		while (nodesIt) {
			nodesIt->escapeNode = child->escapeNode;
			nodesIt = nodesIt->children[1];
		}

		children[childIdx] = child;
		refitBS();
	}

	template <class T>
	inline void BSH<T>::Node::refitBS() {
		if (!isLeaf()) {
			boundingSphere = BoundingSphere::calcCombinedBoundingSphere(children[0]->boundingSphere, children[1]->boundingSphere);
			boundingSphereFattened = BoundingSphere::calcFattenedBoundingSphere(boundingSphere, FATTENING_FACTOR);;
		}
	}
	 
	template <class T>
	BSH<T>::DataNode::DataNode(BoundingSphere const& boundingSphere, T& _data) : Node(boundingSphere), data(_data) {}

	template <class T>
	BSH<T>::~BSH() {
		if (nodesRoot != NULL)
			deleteNodeRecurse(nodesRoot);
	}

	template <class T>
	void BSH<T>::deleteNodeRecurse(Node* node) {
		if (node->isLeaf()) {
			delete node;
			return;
		}

		deleteNodeRecurse(node->getChild(0));
		deleteNodeRecurse(node->getChild(1));
		delete node;
	}

	template <class T>
	typename BSH<T>::DataNode* BSH<T>::insert(BoundingSphere const& boundingSphere, T& data) {
		DataNode* newNode = new DataNode(boundingSphere, data);
		insert(newNode);

		return newNode;
	}

	template<class T>
	inline void BSH<T>::destroy(DataNode* node) {
		remove(node);
		delete node;
	}

	template <class T>
	void BSH<T>::insert(DataNode* node) {
		if (nodesRoot != NULL) {
			if (!nodesRoot->isLeaf()) {
				Node* newNodeSibling = findNewNodeSibling(nodesRoot, node);
				Node* newNodeSiblingParent = newNodeSibling->parent;
				Node* newBranch = new Node(newNodeSibling, node);
				if (newNodeSiblingParent != NULL) {
					if (newNodeSiblingParent->children[0] == newNodeSibling)
						newNodeSiblingParent->replaceChild(newBranch, 0);
					else
						newNodeSiblingParent->replaceChild(newBranch, 1);

					refitBoundingSpheresUpTree(newNodeSiblingParent->parent);
				}
				else
					nodesRoot = newBranch;
			}
			else
				nodesRoot = new Node(nodesRoot, node);
		}
		else
			nodesRoot = node;
	}

	template <class T>
	void BSH<T>::remove(DataNode* nodeToRemove) {
		if (nodesRoot != nodeToRemove) {
			Node* nodeParent = nodeToRemove->parent;
			Node* nodeSibling;
			if (nodeToRemove->isLeftChild())
				nodeSibling = nodeParent->children[1];
			else
				nodeSibling = nodeParent->children[0];

			if (nodesRoot != nodeParent) {
				Node* nodeGrandParent = nodeParent->parent;
				if (nodeParent->isLeftChild())
					nodeGrandParent->replaceChild(nodeSibling, 0);
				else
					nodeGrandParent->replaceChild(nodeSibling, 1);

				refitBoundingSpheresUpTree(nodeGrandParent->parent);
			}
			else {
				nodesRoot = nodeSibling;
				nodeSibling->parent = NULL;
			}

			delete nodeParent;
		}
		else
			nodesRoot = NULL;		
	}

	template <class T>
	void BSH<T>::translateNodeBS(DataNode* node, DirectX::XMVECTOR const& translate) {
		node->boundingSphere.translate(translate);
		testBoundingSphereContainmentAndUpdate(node);
	}

	template <class T>
	void BSH<T>::setTranslationForNodeBS(DataNode* node, DirectX::XMVECTOR const& translation) {
		node->boundingSphere.setTranslation(translation);
		testBoundingSphereContainmentAndUpdate(node);
	}

	template <class T>
	void BSH<T>::scaleNodeBS(DataNode* node, float factor) {
		node->boundingSphere.scale(factor);
		testBoundingSphereContainmentAndUpdate(node);
	}

	template <class T>
	void BSH<T>::setRadiusForNodeBS(DataNode* node, float radius) {
		node->boundingSphere.setRadius(radius);
		testBoundingSphereContainmentAndUpdate(node);
	}

	template <class T>
	void BSH<T>::testBoundingSphereContainmentAndUpdate(DataNode* node) {
		if (!node->boundingSphereFattened.doesContain(node->boundingSphere)) {
			remove(node);
			insert(node);
			node->boundingSphereFattened = BoundingSphere::calcFattenedBoundingSphere(node->boundingSphere, FATTENING_FACTOR);
		}
	}

	template <class T>
	typename BSH<T>::Node* BSH<T>::findNewNodeSibling(Node* root, Node* newNode) {
		Node* nodesIt = root;
		while (!nodesIt->isLeaf()) {
			float nodeAndLeafCombinedRadius = (nodesIt->boundingSphere).calcCombinedSphereRadius(newNode->boundingSphere);
			float costInsertingAsSibling = 2 * nodeAndLeafCombinedRadius;
			float inheritanceCost = 2 * (nodeAndLeafCombinedRadius - nodesIt->boundingSphere.getRadius());
			Node* child0 = nodesIt->children[0];
			Node* child1 = nodesIt->children[1];
			float child0DescentCost = inheritanceCost;
			float child1DescentCost = inheritanceCost;
			if (child0->isLeaf())
				child0DescentCost += 2 * (child0->boundingSphere).calcCombinedSphereRadius(newNode->boundingSphere);
			else
				child0DescentCost += 2 * ((child0->boundingSphere).calcCombinedSphereRadius(newNode->boundingSphere) - (child0->boundingSphere).getRadius());
			if (child1->isLeaf())
				child1DescentCost += 2 * (child1->boundingSphere).calcCombinedSphereRadius(newNode->boundingSphere);
			else
				child1DescentCost += 2 * ((child1->boundingSphere).calcCombinedSphereRadius(newNode->boundingSphere) - (child1->boundingSphere).getRadius());

			if (costInsertingAsSibling < child0DescentCost && costInsertingAsSibling < child1DescentCost)
				return nodesIt;
			else if (child0DescentCost < child1DescentCost)
				nodesIt = child0;
			else
				nodesIt = child1;
		}

		return nodesIt;
	}

	template <class T>
	void BSH<T>::refitBoundingSpheresUpTree(Node* node) {
		Node* nodesIt = node;
		while (nodesIt != NULL) {
			nodesIt->refitBS();
			nodesIt = nodesIt->parent;
		}
	}
} // namespace CoriumDirectX