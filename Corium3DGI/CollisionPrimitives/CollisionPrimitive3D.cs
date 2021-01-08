using Corium3D;
using CoriumDirectX;

using System.Windows.Media.Media3D;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class CollisionPrimitive3D : CollisionPrimitive
    {
        public virtual Vector3D Translation { get { return new Vector3D(0.0f, 0.0f, 0.0f); } }
        public virtual Vector3D Scale { get { return new Vector3D(1.0f, 1.0f, 1.0f); } }
        public virtual Quaternion Rot { get { return new Quaternion(new Vector3D(1.0f, 0.0f, 0.0f), 0.0f); } }

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
