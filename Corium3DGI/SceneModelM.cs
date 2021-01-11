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

            SceneModelAssetData = sceneM.SceneAssetGen.addSceneModelData(modelM.ModelAssetGen, InstancesNrMax, IsStatic);            
        }        

        public void Dispose()
        {                        
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                idxPool.releaseIdx(instance.InstanceIdx);
                instance.Dispose();                
            }
            SceneModelInstanceMs.Clear();

            SceneModelAssetData.Dispose();
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
            instance.IDxSceneModelInstanceCollider3D = ModelMRef.CollisionPrimitive3DSelected.createDxInstances(SceneMRef,
                instance.Translate.Vector3DCpy, instance.Scale.Vector3DCpy, instance.RotQuat.Axis, (float)instance.RotQuat.Angle);
        }

        private void assignCollisionPrimitive2dDxInstances(SceneModelInstanceM instance)
        {
            instance.IDxSceneModelInstanceCollider2D = ModelMRef.CollisionPrimitive2DSelected.createDxInstances(SceneMRef,
                new Vector(instance.Translate.Vector3DCpy.X, instance.Translate.Vector3DCpy.Y),
                new Vector(instance.Scale.Vector3DCpy.X, instance.Scale.Vector3DCpy.Y),
                (float)instance.RotQuat.Angle); // TODO: Settle the 3d rotation to 2d collision primitive rotation conversion here
        }

        private void onCollisionBoxCenterChanged(Vector3D center)
        {
            addColliderDxModelsToTransformGrp();
            SceneMRef.transformGrpSetTranslation(center, DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeColliderDxModelsFromTransformGrp();
        }

        private void onCollisionBoxScaleChanged(Vector3D scale)
        {
            addColliderDxModelsToTransformGrp();
            SceneMRef.transformGrpSetScale(scale, DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeColliderDxModelsFromTransformGrp();
        }

        private void onCollisionSphereCenterChanged(Vector3D center)
        {
            addColliderDxModelsToTransformGrp();
            SceneMRef.transformGrpSetTranslation(center, DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeColliderDxModelsFromTransformGrp();
        }

        private void onCollisionSphereRadiusChanged(float radius)
        {
            addColliderDxModelsToTransformGrp();
            SceneMRef.transformGrpSetScale(new Vector3D(radius, radius, radius), DxVisualizer.IScene.TransformReferenceFrame.Local);
            removeColliderDxModelsFromTransformGrp();
        }

        private void onCollisionCapsuleCenterChanged(CollisionCapsule capsule)
        {            
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider3D[0].addToTransformGrp();
            
            SceneMRef.transformGrpSetTranslation((Vector3D)capsule.Center.Point3DCpy, DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)            
                instance.IDxSceneModelInstanceCollider3D[0].removeFromTransformGrp();

            resetPosDxCapsuleHemispheres(capsule);            
        }

        private void onCollisionCapsuleAxisVecChanged(CollisionCapsule capsule)
        {
            const double EPSILON = 1E-6;

            Vector3D axisVec = capsule.AxisVec.Vector3DCpy;
            if (axisVec.LengthSquared < EPSILON)
                return;
            
            addColliderDxModelsToTransformGrp();

            Vector3D upVector = new Vector3D(0, 1, 0);
            Vector3D rotationAxis = Vector3D.CrossProduct(upVector, axisVec);            
            if (rotationAxis.LengthSquared > EPSILON)
            {
                rotationAxis.Normalize();
                SceneMRef.transformGrpSetRotation(rotationAxis, Math.Acos(Vector3D.DotProduct(upVector, axisVec)) * 180.0 / Math.PI, DxVisualizer.IScene.TransformReferenceFrame.Local);
            }
            else
                SceneMRef.transformGrpSetRotation(upVector, 0.0f, DxVisualizer.IScene.TransformReferenceFrame.Local);

            removeColliderDxModelsFromTransformGrp();

            resetPosDxCapsuleHemispheres(capsule);
        }

        private void onCollisionCapsuleHeightChanged(CollisionCapsule capsule)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider3D[0].addToTransformGrp();
            
            SceneMRef.transformGrpSetScale(new Vector3D(capsule.Radius, capsule.Height, capsule.Radius), DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)            
                instance.IDxSceneModelInstanceCollider3D[0].removeFromTransformGrp();                
            
            resetPosDxCapsuleHemispheres(capsule);
        }

        private void onCollisionCapsuleRadiusChanged(CollisionCapsule capsule)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)            
                instance.IDxSceneModelInstanceCollider3D[0].addToTransformGrp();
            
            SceneMRef.transformGrpSetScale(new Vector3D(capsule.Radius, capsule.Height, capsule.Radius), DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                instance.IDxSceneModelInstanceCollider3D[0].removeFromTransformGrp();
                instance.IDxSceneModelInstanceCollider3D[1].addToTransformGrp();
                instance.IDxSceneModelInstanceCollider3D[2].addToTransformGrp();
            }
            
            SceneMRef.transformGrpSetScale(new Vector3D(capsule.Radius, capsule.Radius, capsule.Radius), DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                instance.IDxSceneModelInstanceCollider3D[1].removeFromTransformGrp();
                instance.IDxSceneModelInstanceCollider3D[2].removeFromTransformGrp();
            }
        }

        private void resetPosDxCapsuleHemispheres(CollisionCapsule capsule)
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)                            
                instance.IDxSceneModelInstanceCollider3D[1].addToTransformGrp();

            Vector3D center = (Vector3D)capsule.Center.Point3DCpy;
            Vector3D axisVec = capsule.AxisVec.Vector3DCpy;
            float height = capsule.Height;
            SceneMRef.transformGrpSetTranslation(center + 0.5f * height * axisVec, DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                instance.IDxSceneModelInstanceCollider3D[1].removeFromTransformGrp();
                instance.IDxSceneModelInstanceCollider3D[2].addToTransformGrp();
            }

            SceneMRef.transformGrpSetTranslation(center - 0.5f * height * axisVec, DxVisualizer.IScene.TransformReferenceFrame.Local);

            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.IDxSceneModelInstanceCollider3D[2].removeFromTransformGrp();
        }

        private void addColliderDxModelsToTransformGrp()
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                foreach (DxVisualizer.IScene.ISceneModelInstance dxColliderModel in instance.IDxSceneModelInstanceCollider3D)
                    dxColliderModel.addToTransformGrp();
            }                
        }

        private void removeColliderDxModelsFromTransformGrp()
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
            {
                foreach (DxVisualizer.IScene.ISceneModelInstance dxColliderModel in instance.IDxSceneModelInstanceCollider3D)
                    dxColliderModel.removeFromTransformGrp();
            }
        }
    }
}
