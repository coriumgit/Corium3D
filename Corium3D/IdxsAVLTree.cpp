#include "IdxsAVLTree.h"
#include <limits>

using namespace Corium3DUtils;

unsigned short NULL_IDX = std::numeric_limits<unsigned short>::max();
unsigned short NULL_LMNT = std::numeric_limits<unsigned short>::max();

IdxsAVLTree::IdxsAVLTree(unsigned short _maxSz) :
		maxSz(_maxSz), nodes(new node[_maxSz]), lmntsNr(0), treeIt(NULL_IDX) {
	nodes[0].lmnt = NULL_LMNT;
	nodes[0].height = 0;
}

IdxsAVLTree::~IdxsAVLTree() { delete nodes; }

void IdxsAVLTree::clear() { 
	nodes[0].lmnt = NULL_LMNT; 
	nodes[0].height = 0;
}

void IdxsAVLTree::add(unsigned short lmnt) {
	unsigned short it = 0;
	while (nodes[it].lmnt != NULL_LMNT) {
		if (lmnt < nodes[it].lmnt)
			it = leftChild(it);
		else if (lmnt > nodes[it].lmnt)
			it = rightChild(it);
		else
			return;
	}	
	nodes[it].lmnt = lmnt;
	lmntsNr++;	
}

bool IdxsAVLTree::isFull() { return lmntsNr == maxSz; }

bool IdxsAVLTree::isEmpty() { return lmntsNr == 0; }

void IdxsAVLTree::initIt() { treeIt = 0; }

unsigned short IdxsAVLTree::itNext() { 
	//return nodes[it++].lmnt; 
}

bool IdxsAVLTree::isItDone() { return treeIt == NULL_IDX; }

unsigned short parent(unsigned short idx) { return idx / 2; }

unsigned short leftChild(unsigned short idx) { return 2*idx+ 1; }

unsigned short rightChild(unsigned short idx) { return 2*idx + 2; }

void rollRR(short idx) {}

void rollRL(short idx) {}

void rollLR(short idx) {}

void rollLL(short idx) {}