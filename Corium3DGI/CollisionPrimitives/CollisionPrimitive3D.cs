using Corium3D;
using CoriumDirectX;

using System.Windows.Media.Media3D;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class CollisionPrimitive3D : CollisionPrimitive
    {
        public override void asssignPrimitiveDataInModelAssetGen(AssetsGen.IModelAssetGen modelAssetGen)
        {
            modelAssetGen.clearCollisionPrimitive3D();
        }
    }
}
