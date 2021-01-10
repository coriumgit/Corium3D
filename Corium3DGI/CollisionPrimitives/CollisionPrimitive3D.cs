using Corium3D;
using CoriumDirectX;

using System.Windows.Media.Media3D;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class CollisionPrimitive3D : CollisionPrimitive
    {
        public virtual List<DxVisualizer.IScene.ISceneModelInstance> createDxInstances(SceneM sceneM, Vector3D instanceTranslate, Vector3D instanceScale, Vector3D instanceRotAx, float instanceRotAng)
        {
            return null;
        }

        public override void asssignPrimitiveDataInModelAssetGen(AssetsGen.IModelAssetGen modelAssetGen)
        {
            modelAssetGen.clearCollisionPrimitive3D();
        }
    }
}
