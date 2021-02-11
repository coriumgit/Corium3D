using Corium3D;
using CoriumDirectX;
using Corium3DGI.Utils;
using System;
using System.Windows;
using System.Collections.ObjectModel;
using System.Windows.Media.Media3D;

namespace Corium3DGI
{
    public class SceneModelM : ObservableObject, IEquatable<SceneModelM>, IDisposable
    {
        private class SceneModelInstanceMShell : SceneModelInstanceM
        {
            public SceneModelInstanceMShell(SceneModelM sceneModel, int instanceIdx, Vector3D translate, Vector3D scale, Vector3D rotAx, float rotAng, EventHandlers eventHandlers) : 
                base(sceneModel, instanceIdx, translate, scale, rotAx, rotAng, eventHandlers) { }
        }
        
        private IdxPool idxPool = new IdxPool();
        private bool isDisposed = false;

        public AssetsGen.ISceneAssetGen.ISceneModelData SceneModelAssetData { get; private set; }

        public SceneM SceneMRef { get; private set; }
        
        public ModelM ModelMRef { get; }

        private string name;
        public string Name
        {
            get { return name; }

            set
            {
                if (name != value)
                {
                    name = value;
                    OnPropertyChanged("Name");
                }
            }
        }

        private uint instancesNrMax = 50;
        public uint InstancesNrMax
        {
            get { return instancesNrMax; }

            set
            {                
                if (instancesNrMax != value)
                {
                    instancesNrMax = value;
                    SceneModelAssetData.setInstancesNrMax(value);                    
                    OnPropertyChanged("InstancesNrMax");
                }
            }
        }

        private bool isStatic = false;
        public bool IsStatic
        {
            get { return isStatic; }

            set
            {                
                if (isStatic != value)
                {
                    isStatic = value;
                    SceneModelAssetData.setIsStatic(value);
                    OnPropertyChanged("IsStatic");
                }
            }
        }

        public ObservableCollection<SceneModelInstanceM> SceneModelInstanceMs { get; } = new ObservableCollection<SceneModelInstanceM>();

        protected SceneModelM(SceneM sceneM, ModelM modelM)
        {                        
            ModelMRef = modelM;
            SceneMRef = sceneM;
            name = modelM.Name;

            modelM.CollisionPrimitive3dChanged += onModelCollisionPrimitive3DChanged;
            modelM.CollisionPrimitive2dChanged += onModelCollisionPrimitive2DChanged;
            modelM.CollisionBoxCenterChanged += onCollisionBoxCenterChanged;
            modelM.CollisionBoxScaleChanged += onCollisionBoxScaleChanged;
            modelM.CollisionSphereCenterChanged += onCollisionSphereCenterChanged;
            modelM.CollisionSphereRadiusChanged += onCollisionSphereRadiusChanged;
            modelM.CollisionCapsuleCenterChanged += onCollisionCapsuleCenterChanged;
            modelM.CollisionCapsuleAxisVecChanged += onCollisionCapsuleAxisVecChanged;
            modelM.CollisionCapsuleHeightChanged += onCollisionCapsuleHeightChanged;
            modelM.CollisionCapsuleRadiusChanged += onCollisionCapsuleRadiusChanged;
            modelM.CollisionRectCenterChanged += onCollisionRectCenterChanged;
            modelM.CollisionRectScaleChanged += onCollisionRectScaleChanged;
            modelM.CollisionCircleCenterChanged += onCollisionCircleCenterChanged;
            modelM.CollisionCircleRadiusChanged += onCollisionCircleRadiusChanged;
            modelM.CollisionStadiumCenterChanged += onCollisionStadiumCenterChanged;
            modelM.CollisionStadiumAxisVecChanged += onCollisionStadiumAxisVecChanged;
            modelM.CollisionStadiumHeightChanged += onCollisionStadiumHeightChanged;
            modelM.CollisionStadiumRadiusChanged += onCollisionStadiumRadiusChanged;

            SceneModelAssetData = sceneM.SceneAssetGen.addSceneModelData(modelM.ModelAssetGen, InstancesNrMax, IsStatic);            
        }        

        public void Dispose()
        {
            if (isDisposed)
                return;

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                idxPool.releaseIdx(instance.InstanceIdx);
                instance.Dispose();                
            }            

            SceneModelAssetData.Dispose();

            ModelMRef.CollisionPrimitive3dChanged -= onModelCollisionPrimitive3DChanged;
            ModelMRef.CollisionPrimitive2dChanged -= onModelCollisionPrimitive2DChanged;
            ModelMRef.CollisionBoxCenterChanged -= onCollisionBoxCenterChanged;
            ModelMRef.CollisionBoxScaleChanged -= onCollisionBoxScaleChanged;
            ModelMRef.CollisionSphereCenterChanged -= onCollisionSphereCenterChanged;
            ModelMRef.CollisionSphereRadiusChanged -= onCollisionSphereRadiusChanged;
            ModelMRef.CollisionCapsuleCenterChanged -= onCollisionCapsuleCenterChanged;
            ModelMRef.CollisionCapsuleAxisVecChanged -= onCollisionCapsuleAxisVecChanged;
            ModelMRef.CollisionCapsuleHeightChanged -= onCollisionCapsuleHeightChanged;
            ModelMRef.CollisionCapsuleRadiusChanged -= onCollisionCapsuleRadiusChanged;
            ModelMRef.CollisionRectCenterChanged -= onCollisionRectCenterChanged;
            ModelMRef.CollisionRectScaleChanged -= onCollisionRectScaleChanged;
            ModelMRef.CollisionCircleCenterChanged -= onCollisionCircleCenterChanged;
            ModelMRef.CollisionCircleRadiusChanged -= onCollisionCircleRadiusChanged;
            ModelMRef.CollisionStadiumCenterChanged -= onCollisionStadiumCenterChanged;
            ModelMRef.CollisionStadiumAxisVecChanged -= onCollisionStadiumAxisVecChanged;
            ModelMRef.CollisionStadiumHeightChanged -= onCollisionStadiumHeightChanged;
            ModelMRef.CollisionStadiumRadiusChanged -= onCollisionStadiumRadiusChanged;

            isDisposed = true;
        }

        public SceneModelInstanceM addSceneModelInstance(Vector3D instanceTranslationInit, Vector3D instanceScaleFactorInit, Vector3D instanceRotAxInit, float instanceRotAngInit, SceneModelInstanceM.EventHandlers eventHandlers)
        {
            SceneModelInstanceM sceneModelInstance = new SceneModelInstanceMShell(this, idxPool.acquireIdx(), instanceTranslationInit, instanceScaleFactorInit, instanceRotAxInit, instanceRotAngInit, eventHandlers);            
            assignCollisionPrimitive3dDxInstances(sceneModelInstance);            
            assignCollisionPrimitive2dDxInstances(sceneModelInstance);

            SceneModelInstanceMs.Add(sceneModelInstance);

            return sceneModelInstance;
        }

        public void removeSceneModelInstance(SceneModelInstanceM sceneModelInstance)
        {
            SceneModelInstanceMs.Remove(sceneModelInstance);
            idxPool.releaseIdx(sceneModelInstance.InstanceIdx);
            sceneModelInstance.Dispose();                        
        }

        public bool Equals(SceneModelM other)
        {
            return ModelMRef == other.ModelMRef;
        }

        public void onModelCollisionPrimitive3DChanged()
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                assignCollisionPrimitive3dDxInstances(instance);
        }

        public void onModelCollisionPrimitive2DChanged()
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)            
                assignCollisionPrimitive2dDxInstances(instance);            
        }

        private void assignCollisionPrimitive3dDxInstances(SceneModelInstanceM instance)
        {
            instance.IDxSceneModelInstanceCollider3D = ModelMRef.CollisionPrimitive3DSelected.createDxInstances(SceneMRef);
        }

        private void assignCollisionPrimitive2dDxInstances(SceneModelInstanceM instance)
        {
            instance.IDxSceneModelInstanceCollider2D = ModelMRef.CollisionPrimitive2DSelected.createDxInstances(SceneMRef);
        }

        private void onCollisionBoxCenterChanged(Vector3D center)
        {
            addCollider3dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetTranslation(center, DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider3dDxModelFromTransformGrp(0);
        }

        private void onCollisionBoxScaleChanged(Vector3D scale)
        {
            addCollider3dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(scale, DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider3dDxModelFromTransformGrp(0);
        }

        private void onCollisionSphereCenterChanged(Vector3D center)
        {
            addCollider3dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetTranslation(center, DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider3dDxModelFromTransformGrp(0);
        }

        private void onCollisionSphereRadiusChanged(float radius)
        {
            addCollider3dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(new Vector3D(radius, radius, radius), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider3dDxModelFromTransformGrp(0);
        }

        private void onCollisionCapsuleCenterChanged(CollisionCapsule capsule)
        {
            addCollider3dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetTranslation((Vector3D)capsule.Center.Point3DCpy, DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider3dDxModelFromTransformGrp(0);
        }

        private void onCollisionCapsuleAxisVecChanged(CollisionCapsule capsule)
        {
            const double EPSILON = 1E-6;

            Vector3D axisVec = capsule.AxisVec.Vector3DCpy;
            if (axisVec.LengthSquared < EPSILON)
                return;

            addCollider3dDxModelToTransformGrp(0);

            Vector3D upVector = new Vector3D(0, 1, 0);
            Vector3D rotationAxis = Vector3D.CrossProduct(upVector, axisVec);            
            if (rotationAxis.LengthSquared > EPSILON)
            {
                rotationAxis.Normalize();
                SceneMRef.transformGrpSetRotation(rotationAxis, Math.Acos(Vector3D.DotProduct(upVector, axisVec)) * 180.0 / Math.PI, DxVisualizer.IScene.TransformReferenceFrame.Local);
            }
            else
                SceneMRef.transformGrpSetRotation(upVector, 0.0f, DxVisualizer.IScene.TransformReferenceFrame.Local);

            removeCollider3dDxModelFromTransformGrp(0);
        }

        private void onCollisionCapsuleHeightChanged(CollisionCapsule capsule)
        {
            updateCollisionCapsuleConstraints(capsule);

            addCollider3dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(new Vector3D(capsule.Radius, capsule.Height, capsule.Radius), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider3dDxModelFromTransformGrp(0);
        }

        private void onCollisionCapsuleRadiusChanged(CollisionCapsule capsule)
        {
            updateCollisionCapsuleConstraints(capsule);

            addCollider3dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(new Vector3D(capsule.Radius, capsule.Height, capsule.Radius), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider3dDxModelFromTransformGrp(0);           
        }        

        private void onCollisionRectCenterChanged(Vector center)
        {
            addCollider2dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetTranslation(new Vector3D(center.X, center.Y, 0.0f), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider2dDxModelFromTransformGrp(0);
        }

        private void onCollisionRectScaleChanged(Vector scale)
        {
            addCollider2dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(new Vector3D(scale.X, scale.Y, 1.0f), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider2dDxModelFromTransformGrp(0);
        }

        private void onCollisionCircleCenterChanged(Vector center)
        {
            addCollider2dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetTranslation(new Vector3D(center.X, center.Y, 0.0f), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider2dDxModelFromTransformGrp(0);
        }

        private void onCollisionCircleRadiusChanged(float radius)
        {
            addCollider2dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(new Vector3D(radius, radius, 1.0f), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider2dDxModelFromTransformGrp(0);
        }
        
        private void onCollisionStadiumCenterChanged(CollisionStadium stadium)
        {
            addCollider2dDxModelToTransformGrp(0);
            Point stadiumCenter = stadium.Center.PointCpy;
            SceneMRef.transformGrpSetTranslation(new Vector3D(stadiumCenter.X, stadiumCenter.Y, 0.0f), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider2dDxModelFromTransformGrp(0);
        }

        private void onCollisionStadiumAxisVecChanged(CollisionStadium stadium)
        {
            const double EPSILON = 1E-6;

            Vector3D axisVec = new Vector3D(stadium.AxisVec.VectorCpy.X, stadium.AxisVec.VectorCpy.Y, 0.0f);
            if (axisVec.LengthSquared < EPSILON)
                return;

            addCollider2dDxModelToTransformGrp(0);

            Vector3D upVector = new Vector3D(0, 1, 0);
            Vector3D rotationAxis = Vector3D.CrossProduct(upVector, axisVec);
            if (rotationAxis.LengthSquared > EPSILON)
            {
                rotationAxis.Normalize();
                SceneMRef.transformGrpSetRotation(rotationAxis, Math.Acos(Vector3D.DotProduct(upVector, axisVec)) * 180.0 / Math.PI, DxVisualizer.IScene.TransformReferenceFrame.Local);
            }
            else
                SceneMRef.transformGrpSetRotation(upVector, 0.0f, DxVisualizer.IScene.TransformReferenceFrame.Local);

            removeCollider2dDxModelFromTransformGrp(0);
        }

        private void onCollisionStadiumHeightChanged(CollisionStadium stadium)
        {
            updateCollisionStadiumCylinderStateConstraints(stadium);

            addCollider2dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(new Vector3D(stadium.Radius, stadium.Height, 1.0f), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider2dDxModelFromTransformGrp(0);            
        }

        private void onCollisionStadiumRadiusChanged(CollisionStadium stadium)
        {
            updateCollisionStadiumCylinderStateConstraints(stadium);

            addCollider2dDxModelToTransformGrp(0);
            SceneMRef.transformGrpSetScale(new Vector3D(stadium.Radius, stadium.Height, 1.0f), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeCollider2dDxModelFromTransformGrp(0);
        }

        private void resetPosDxStadiumSemicircles(CollisionStadium stadium)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider2D[1].addToTransformGrp();

            Vector3D center = new Vector3D(stadium.Center.PointCpy.X, stadium.Center.PointCpy.Y, 0.0f);
            Vector3D axisVec = new Vector3D(stadium.AxisVec.VectorCpy.X, stadium.AxisVec.VectorCpy.Y, 0.0f);
            float height = stadium.Height;
            SceneMRef.transformGrpSetTranslation(center + 0.5f * height * axisVec, DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                instance.IDxSceneModelInstanceCollider2D[1].removeFromTransformGrp();
                instance.IDxSceneModelInstanceCollider2D[2].addToTransformGrp();
            }

            SceneMRef.transformGrpSetTranslation(center - 0.5f * height * axisVec, DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider2D[2].removeFromTransformGrp();
        }

        private void addCollider3dDxModelToTransformGrp(int modelIdx)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)            
                instance.IDxSceneModelInstanceCollider3D[modelIdx].addToTransformGrp();                            
        }

        private void removeCollider3dDxModelFromTransformGrp(int modelIdx)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider3D[modelIdx].removeFromTransformGrp();
        }

        private void updateCollisionCapsuleConstraints(CollisionCapsule capsule)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                ((DxVisualizer.IScene.IConstrainedTransformInstance)instance.IDxSceneModelInstanceCollider3D[0]).setScaleConstraints(
                    DxVisualizer.IScene.TransformScaleConstraint.MaxDimGrp, 0.0f,
                    DxVisualizer.IScene.TransformScaleConstraint.Factor, capsule.Height / capsule.Radius,
                    DxVisualizer.IScene.TransformScaleConstraint.MaxDimGrp, 0.0f);
            }
        }

        private void addCollider2dDxModelToTransformGrp(int modelIdx)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider2D[modelIdx].addToTransformGrp();
        }

        private void removeCollider2dDxModelFromTransformGrp(int modelIdx)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider2D[modelIdx].removeFromTransformGrp();
        }

        private void updateCollisionStadiumCylinderStateConstraints(CollisionStadium stadium)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                ((DxVisualizer.IScene.IConstrainedTransform2dInstance)instance.IDxSceneModelInstanceCollider2D[0]).setScaleConstraints(
                    DxVisualizer.IScene.TransformScaleConstraint.MaxDimGrp, 0.0f,
                    DxVisualizer.IScene.TransformScaleConstraint.Factor, stadium.Height / stadium.Radius);
            }
        }
    }
}
