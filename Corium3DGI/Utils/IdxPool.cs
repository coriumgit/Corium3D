using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Corium3DGI.Utils
{
    public class IdxPool
    {
        Stack<int> recycleList = new Stack<int>();
        int highestAcquiredIdx = -1;

        public int acquireIdx()
        {
            if (recycleList.Count > 0)
                return recycleList.Pop();
            else
                return (++highestAcquiredIdx);
            
        } 

        public void releaseIdx(int idx)
        {
            verifyReleasedIdx(idx);

            recycleList.Push(idx);
        }

        [Conditional("DEBUG")]
        private void verifyReleasedIdx(int idx)
        {
            if (recycleList.Contains(idx) || idx > highestAcquiredIdx)
                throw new Exception("Invalid index for release : " + idx);
        }
    }
}
