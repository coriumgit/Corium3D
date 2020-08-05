#pragma once

#include <stack>

namespace CoriumDirectX {
    
    class IdxPool {
    public:
        unsigned int acquireIdx();
        void releaseIdx(int idx);
        unsigned int acquiredIdxsNr() { return highestAcquiredIdx + 1 - recycleList.size(); }

    private:
        std::stack<unsigned int> recycleList = std::stack<unsigned int>();
        int highestAcquiredIdx = -1;                   
    };
}