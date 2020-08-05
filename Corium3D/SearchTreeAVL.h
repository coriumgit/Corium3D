#pragma once
#include "ObjPool.h"

namespace Corium3DUtils {

	// T has to overload comparison operators (<, >, ==, !=)
	template<class T>
	class SearchTreeAVL {
	public:
		class InOrderIt {
		public:
			InOrderIt(SearchTreeAVL<T>& _tree) : tree(_tree), curr(tree.iterationHead), prev(NULL) {}

			void reset() { 
				curr = tree.iterationHead; 
				prev = NULL; 
			}

			T& next() {
#if DEBUG
				if (!hasNext())
					throw std::out_of_range("Iterator was nexted while done.");
#endif
				T& currData = curr->data;
				prev = curr;
				curr = curr->iterationNext;

				return currData;
			}

			void removeCurr() {
#if DEBUG
				if (prev == NULL)
					throw std::out_of_range("Iterator was asked to remove current elemnt before beginning.");
#endif
				tree.performNodeRemoval(prev);
				if (curr)
					prev = curr->iterationPrev;
			}

			bool hasNext() const { 
				return curr != NULL; 
			}

		private:					
			SearchTreeAVL<T>& tree;
			typename SearchTreeAVL::Node* curr;
			typename SearchTreeAVL::Node* prev;
		};

		SearchTreeAVL(unsigned int lmntsNrMax);
		~SearchTreeAVL();
		// return: if the tree contains [data] -> reference to the already contained [data]
		//		   else -> reference to the just inserted [data] 
		T& insert(T const& data);
		// return: If the tree contains [data] -> true
		//		   Else -> false
		bool remove(T const& data);		

	public:
		class Node {
		public:
			friend class SearchTreeAVL;

			Node(T const& data);			
			void linkRight(Node* rightChild);
			void linkLeft(Node* leftChild);
			void iterationPtrLinkAfter(Node* node) {
				node->iterationPrev = this;
				node->iterationNext = this->iterationNext;
				if (this->iterationNext)
					this->iterationNext->iterationPrev = node;
				this->iterationNext = node;
			}

			void iterationPtrLinkPrev(Node* node) {
				node->iterationNext = this;
				node->iterationPrev = this->iterationPrev;
				if (this->iterationPrev)
					this->iterationPrev->iterationNext = node;
				this->iterationPrev = node;
			}

			void replaceChildren(Node* leftChild, Node* rightChild);
			void resetParent() { parent = NULL; }

			T& getData() { return data; }

			bool isLeaf() const { return height == 0; }

			void rectifyHeight() {
				if (leftChild && rightChild)
					this->height = (leftChild->height > rightChild->height ? leftChild->height : rightChild->height) + 1;
				else if (leftChild)
					this->height = leftChild->height + 1;
				else if (rightChild)
					this->height = rightChild->height + 1;
				else
					this->height = 0;
			}

			int leftChildHeight() const { return (leftChild ? leftChild->height : -1); }
			int rightChildHeight() const { return (rightChild ? rightChild->height : -1); }

		private:
			T data;
			Node* leftChild = NULL;
			Node* rightChild = NULL;
			Node* parent = NULL;
			Node* iterationPrev = NULL;
			Node* iterationNext = NULL;			
			unsigned int height = 0;			
		};	
		
		ObjPool<Node>* nodesPool;
		const unsigned int lmntsNrMax;
		Node* root = NULL;
		Node* iterationHead = NULL;

		// REMINDER: assumes node has a right child
		static Node* getSuccessor(Node* node);	
		// REMINDER: Rectifies the heights of the nodes it passes
		static Node* balanceNodesUpwards(Node* startNode);
		static Node* rotateL(Node* node);
		static Node* rotateR(Node* node);	
		void performNodeRemoval(Node* node);
	};

	template <class T>
	SearchTreeAVL<T>::SearchTreeAVL(unsigned int _lmntsNrMax) : lmntsNrMax(_lmntsNrMax) {
		nodesPool = new ObjPool<Node>(lmntsNrMax);
	}

	template <class T>
	SearchTreeAVL<T>::~SearchTreeAVL() {
		delete nodesPool;
	}
	
	template <class T>
	T& SearchTreeAVL<T>::insert(T const& data) {		
		Node* newNode;
		try {
			newNode = nodesPool->acquire(Node(data));
		}
		catch (std::underflow_error) {
			throw std::overflow_error("Maximum elements number exceeded.");
		}

		bool isNewNodeSmallest = true;
		if (root != NULL) {
			Node* nodesIt = root;			
			while (true) {						
				if (data > nodesIt->data) {					
					isNewNodeSmallest = false;
					if (nodesIt->rightChild)
						nodesIt = nodesIt->rightChild;											
					else {
						nodesIt->linkRight(newNode);
						nodesIt->iterationPtrLinkAfter(newNode);						
						break;
					}					
				}
				else if (data < nodesIt->data) {					
					if (nodesIt->leftChild)
						nodesIt = nodesIt->leftChild;
					else {
						nodesIt->linkLeft(newNode);
						nodesIt->iterationPtrLinkPrev(newNode);						
						if (isNewNodeSmallest)
							iterationHead = newNode;
						break;
					}
				}
				else { // nodesIt->getData() == newNode->getData()
					nodesPool->release(newNode);
					return nodesIt->getData();
				}
			}
					
			if (nodesIt != root)
				root = balanceNodesUpwards(nodesIt->parent);							

			return newNode->data;
		}
		else {
			try {
				iterationHead = root = newNode;
			}
			catch (std::underflow_error) {
				throw std::overflow_error("Maximum elements number exceeded.");
			}
			return root->data;
		}		
	}

	template <class T>
	bool SearchTreeAVL<T>::remove(T const& data) {
		if (root != NULL) {									
			Node* nodesIt = root;
			do {
				T& nodesItData = nodesIt->getData();
				if (data > nodesItData)
					nodesIt = nodesIt->rightChild;
				else if (data < nodesItData)
					nodesIt = nodesIt->leftChild;
				else { // data == nodesIt->getData()	
					performNodeRemoval(nodesIt);
					return true;					
				}
			} while (nodesIt);

			return false;
		}
		else
			return false;
	}

	template <class T>
	typename SearchTreeAVL<T>::Node* SearchTreeAVL<T>::getSuccessor(Node* node) {
		Node* nodesIt = node->rightChild;
		while (nodesIt->leftChild)
			nodesIt = nodesIt->leftChild;	

		return nodesIt;
	}

	template <class T>
	typename SearchTreeAVL<T>::Node* SearchTreeAVL<T>::balanceNodesUpwards(Node* startNode) {
		Node* nodesIt = startNode;		
		while (true) {		
			Node* nodesItLeftChild = nodesIt->leftChild;
			Node* nodesItRightChild = nodesIt->rightChild;			
			//unsigned int  
			if ((nodesItLeftChild && !nodesItLeftChild->isLeaf()) || (nodesItRightChild && !nodesItRightChild->isLeaf())) {
				int nodesItLeftChildHeight = nodesIt->leftChildHeight();
				int nodesItRightChildHeight = nodesIt->rightChildHeight();
				if ((nodesItLeftChildHeight - nodesItRightChildHeight) == 2) {
					if (nodesItLeftChild->leftChildHeight() >= nodesItLeftChild->rightChildHeight())
						nodesIt = rotateR(nodesIt);
					else {
						nodesIt->linkLeft(rotateL(nodesItLeftChild));
						nodesIt = rotateR(nodesIt);
					}
				}
				else if ((nodesItRightChildHeight - nodesItLeftChildHeight) == 2) {
					if (nodesItRightChild->rightChildHeight() >= nodesItRightChild->leftChildHeight())
						nodesIt = rotateL(nodesIt);
					else {
						nodesIt->linkRight(rotateR(nodesItRightChild));
						nodesIt = rotateL(nodesIt);
					}
				}
				else
					nodesIt->rectifyHeight();
			}
			else
				nodesIt->rectifyHeight();

			if (nodesIt->parent) {
				nodesIt = nodesIt->parent;
			}
			else
				return nodesIt;
		}
	}

	template <class T>
	typename SearchTreeAVL<T>::Node* SearchTreeAVL<T>::rotateL(Node* node) {
		Node* nodeRightChild = node->rightChild;
		node->linkRight(nodeRightChild->leftChild);		
		if (node->parent) {
			if (node->parent->leftChild == node)
				node->parent->linkLeft(nodeRightChild);
			else
				node->parent->linkRight(nodeRightChild);
		}
		else
			nodeRightChild->resetParent();
		
		nodeRightChild->linkLeft(node);				

		return nodeRightChild;
	}

	template <class T>
	typename SearchTreeAVL<T>::Node* SearchTreeAVL<T>::rotateR(Node* node) {
		Node* nodeLeftChild = node->leftChild;
		node->linkLeft(nodeLeftChild->rightChild);
		if (node->parent) {
			if (node->parent->leftChild == node)
				node->parent->linkLeft(nodeLeftChild);
			else
				node->parent->linkRight(nodeLeftChild);
		}
		else
			nodeLeftChild->resetParent();

		nodeLeftChild->linkRight(node);		
		
		return nodeLeftChild;		
	}	

	template <class T>
	void SearchTreeAVL<T>::performNodeRemoval(Node* node) {
		Node* nodesItReplacer;
		Node* balanceStartNode;
		Node* nodesItParent = node->parent;
		if (node->rightChild && node->leftChild) {
			nodesItReplacer = node->iterationNext; // successor
			if (nodesItReplacer->parent == node) {
				balanceStartNode = nodesItReplacer;
				nodesItReplacer->linkLeft(node->leftChild);
			}
			else {
				balanceStartNode = nodesItReplacer->parent;
				balanceStartNode->linkLeft(nodesItReplacer->rightChild);
				nodesItReplacer->replaceChildren(node->leftChild, node->rightChild);
			}
		}
		else {
			if (node->leftChild)
				nodesItReplacer = node->leftChild;
			else
				nodesItReplacer = node->rightChild;

			balanceStartNode = nodesItParent;
		}

		if (nodesItParent) {
			if (nodesItParent->leftChild == node)
				nodesItParent->linkLeft(nodesItReplacer);
			else
				nodesItParent->linkRight(nodesItReplacer);
		}
		else {
			if (root = nodesItReplacer)
				root->resetParent();
		}

		if (balanceStartNode)
			root = balanceNodesUpwards(balanceStartNode);

		if (node->iterationPrev)
			node->iterationPrev->iterationNext = node->iterationNext;
		else
			iterationHead = node->iterationNext;
		if (node->iterationNext)
			node->iterationNext->iterationPrev = node->iterationPrev;

		nodesPool->release(node);
	}

	template <class T>
	SearchTreeAVL<T>::Node::Node(T const& _data) : data(_data) {}

	template <class T>
	void SearchTreeAVL<T>::Node::linkRight(Node* rightChild) {
		this->rightChild = rightChild;
		if (rightChild)
			rightChild->parent = this;
		rectifyHeight();
	}

	template <class T>
	void SearchTreeAVL<T>::Node::linkLeft(Node* leftChild) {
		this->leftChild = leftChild;
		if (leftChild)
			leftChild->parent = this;
		rectifyHeight();
	}

	template <class T>
	void SearchTreeAVL<T>::Node::replaceChildren(Node* leftChild, Node* rightChild) {
		this->rightChild = rightChild;
		this->leftChild = leftChild;
		leftChild->parent = rightChild->parent = this;
		rectifyHeight();
	}	

} // namespace Corium3DUtils
