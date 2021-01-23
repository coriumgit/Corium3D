using Corium3D;
using CoriumDirectX;

using System.Windows;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class CollisionPrimitive2D : CollisionPrimitive
    {
        public override void asssignPrimitiveDataInModelAssetGen(AssetsGen.IModelAssetGen modelAssetGen)
        {
            modelAssetGen.clearCollisionPrimitive2D();
        }
    }
}
