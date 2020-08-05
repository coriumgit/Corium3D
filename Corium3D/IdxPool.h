#pragma once

#include <stdexcept>

namespace Corium3DUtils {
	class IdxPool {
	public:
		IdxPool(unsigned int _poolSz);
		IdxPool(IdxPool const& idxPool);
		~IdxPool();
		unsigned int acquire();
		void release(unsigned int idx);
#if DEBUG
		bool isAcquired(unsigned int idx) const { return acquiredIdxsLogicalVec[idx]; }
#endif
		unsigned int getAcquiredIdxsNr() { return acquiredIdxsNr; }

	private:
#if DEBUG
		bool* acquiredIdxsLogicalVec;
#endif
		unsigned int* nextAvailableIdxs;
		unsigned int poolSz;
		unsigned int availableIdx = 0;
		unsigned int acquiredIdxsNr = 0;
	};
}