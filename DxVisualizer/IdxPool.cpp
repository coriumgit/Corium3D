#include "IdxPool.h"

namespace CoriumDirectX {

    unsigned int IdxPool::acquireIdx() {
        if (recycleList.size() > 0) {
            unsigned int res = recycleList.top();
            recycleList.pop();
            return res;
        }
        else
            return (++highestAcquiredIdx);
    }

    void IdxPool::releaseIdx(int idx) { recycleList.push(idx); }

}