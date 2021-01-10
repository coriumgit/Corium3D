using CoriumDirectX;

namespace Corium3DGI
{
    public class CollisionRegionModelM : ModelM
    {        
        private const string COLLISION_REGION_NAME_BASE = "Collision Region";
        private static Utils.IdxPool collisionRegionsIdxPool = new Utils.IdxPool();        

        private int collisionRegionIdx = -1;

        public CollisionRegionModelM(DxVisualizer dxVisualizer) : base(string.Empty, dxVisualizer)
        {
            collisionRegionIdx = collisionRegionsIdxPool.acquireIdx();
            name = COLLISION_REGION_NAME_BASE + " " + collisionRegionIdx.ToString("D2");
        }

        public override void Dispose()
        {
            if (collisionRegionIdx > -1)
                collisionRegionsIdxPool.releaseIdx(collisionRegionIdx);

            base.Dispose();
        }
    }
}
