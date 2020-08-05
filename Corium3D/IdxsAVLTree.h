#pragma once

namespace Corium3DUtils {	
	class IdxsAVLTree
	{
	public:
		IdxsAVLTree(unsigned short _maxSz);
		~IdxsAVLTree();
		void clear();
		void add(unsigned short lmnt);
		void remove(unsigned short lmnt);
		bool isFull();
		bool isEmpty();		
		void initIt();
		unsigned short itNext();
		bool isItDone();

	private:
		struct node {
			unsigned short lmnt;
			unsigned char height;
		};

		unsigned short maxSz;
		node* nodes;
		unsigned short lmntsNr;		

		unsigned short treeIt;
		
		unsigned short parent(unsigned short idx);
		unsigned short leftChild(unsigned short idx);
		unsigned short rightChild(unsigned short idx);
		void rollRR(short idx);
		void rollRL(short idx);
		void rollLR(short idx);
		void rollLL(short idx);
	};
}

