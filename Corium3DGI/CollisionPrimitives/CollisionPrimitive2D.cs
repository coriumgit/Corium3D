using Corium3D;
using CoriumDirectX;

using System.Windows;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class CollisionPrimitive2D : CollisionPrimitive
    {
        public virtual Vector Translation { get { return new Vector(0.0f, 0.0f); } }
        public virtual Vector Scale { get { return new Vector(1.0f, 1.0f); } }
        public virtual float Rot { get { return 0.0f; } }

        public virtual List<DxVisualizer.IScene.ISceneModelInstance> createDxInstances(SceneM sceneM, Vector instanceTranslate, Vector instanceScale, float instanceRotAng)
        {
            return null;
        }

        public override void asssignPrimitiveDataInModelAssetGen(AssetsGen.IModelAssetGen modelAssetGen)
        {
            modelAssetGen.clearCollisionPrimitive2D();
        }
    }
}
