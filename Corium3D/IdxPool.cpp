#include "IdxPool.h"

namespace Corium3DUtils {

	IdxPool::IdxPool(unsigned int _poolSz) : poolSz(_poolSz) {
#if DEBUG
		acquiredIdxsLogicalVec = new bool[_poolSz];
#endif 
		nextAvailableIdxs = new unsigned int[_poolSz]; 
		for (unsigned int idxIdx = 0; idxIdx < poolSz; idxIdx++)
			nextAvailableIdxs[idxIdx] = idxIdx + 1;
	}

	IdxPool::IdxPool(IdxPool const& idxPool) : 
		poolSz(idxPool.poolSz), availableIdx(idxPool.availableIdx), acquiredIdxsNr(idxPool.acquiredIdxsNr) {
#if DEBUG
		acquiredIdxsLogicalVec = new bool[idxPool.poolSz];
		memcpy(acquiredIdxsLogicalVec, idxPool.acquiredIdxsLogicalVec, idxPool.poolSz * sizeof(bool));
#endif
		nextAvailableIdxs = new unsigned int[idxPool.poolSz];
		memcpy(nextAvailableIdxs, idxPool.nextAvailableIdxs, idxPool.poolSz * sizeof(unsigned int));		
	}

	IdxPool::~IdxPool() {
		delete[] nextAvailableIdxs;
#if DEBUG
		delete[] acquiredIdxsLogicalVec;
#endif
	}

	unsigned int IdxPool::acquire() {
		if (acquiredIdxsNr == poolSz)
			throw std::underflow_error("No more Indexes.");

		unsigned int returnedIdx = availableIdx;
		availableIdx = nextAvailableIdxs[availableIdx];
#if DEBUG
		acquiredIdxsLogicalVec[returnedIdx] = true;
#endif
		acquiredIdxsNr++;
		return returnedIdx;
	}

	void IdxPool::release(unsigned int idx) {
#if DEBUG
		if (!acquiredIdxsLogicalVec[idx])
			throw std::invalid_argument("Index was not previously acquired.");
		if (idx >= poolSz)
			throw std::out_of_range("Index out of range");
#endif
		nextAvailableIdxs[idx] = availableIdx;
		availableIdx = idx;
		acquiredIdxsNr--;
#if DEBUG
		acquiredIdxsLogicalVec[idx] = false;
#endif		
	}

} //namespace Corium3DUtils