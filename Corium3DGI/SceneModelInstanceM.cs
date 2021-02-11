using Corium3D;

using System;
using System.ComponentModel;
using System.Windows.Media.Media3D;
using Corium3DGI.Utils;
using CoriumDirectX;
using System.Windows.Media;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class SceneModelInstanceM : ObservableObject, IDisposable
    {                
        private EventHandlers eventHandlers;
        private DxVisualizer.IScene.ISceneModelInstance.SelectionHandler selectionHandler;
        private AssetsGen.ISceneAssetGen.ISceneModelData.ISceneModelInstanceData sceneModelInstanceAssetData;
        private DxVisualizer.IScene.ISceneModelInstance iDxSceneModelInstance;
        private bool isDisposed = false;

        private DxVisualizer.IScene.ISceneModelInstance[] iDxSceneModelInstanceCollider3D = null;
        public DxVisualizer.IScene.ISceneModelInstance[] IDxSceneModelInstanceCollider3D
        {
            get { return iDxSceneModelInstanceCollider3D; }
            set
            {
                disposeDxSceneModelInstanceCollider3D();

                iDxSceneModelInstanceCollider3D = value;

                if (iDxSceneModelInstanceCollider3D != null)
                {
                    iDxSceneModelInstanceCollider3D[0].assignParent(iDxSceneModelInstance, false);
                    foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider3D)
                    {                        
                        if (!isShown)
                            instance.hide();
                    }
                }
            }
        }

        private DxVisualizer.IScene.ISceneModelInstance[] iDxSceneModelInstanceCollider2D = null;
        public DxVisualizer.IScene.ISceneModelInstance[] IDxSceneModelInstanceCollider2D
        {
            get { return iDxSceneModelInstanceCollider2D; }
            set
            {
                disposeDxSceneModelInstanceCollider2D();

                iDxSceneModelInstanceCollider2D = value;

                if (iDxSceneModelInstanceCollider2D != null)
                {
                    iDxSceneModelInstanceCollider2D[0].assignParent(iDxSceneModelInstance, false);
                    foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider2D)
                    {                        
                        if (!isShown)
                            instance.hide();
                    }
                }
            }
        }

        public SceneModelM SceneModelMRef { get; }

        public int InstanceIdx { get; }        

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

        public ObservableVector3D Translate { get; }

        public ObservableVector3D Scale { get; }

        public Quaternion RotQuat { get; private set; }

        public ObservableVector3D RotEueler { get; }

        private bool isShown = true;
        public bool IsShown
        {
            get { return isShown; }

            set
            {
                if (isShown != value)
                {
                    isShown = value;
                    OnPropertyChanged("IsShown");
                }
            }
        }

        public delegate void OnThisInstanceSelection(SceneModelInstanceM thisInstance);

        public struct EventHandlers
        {
            public OnThisInstanceSelection onThisInstanceSelection;
            public PropertyChangedEventHandler transformPanelTranslationEditHandler;
            public PropertyChangedEventHandler transformPanelScaleEditHandler;
            public PropertyChangedEventHandler transformPanelRotEditHandler;
        }

        protected SceneModelInstanceM(SceneModelM sceneModel, int instanceIdx, Vector3D translate, Vector3D scale, Vector3D rotAx, float rotAng, EventHandlers eventHandlers)
        {
            SceneModelMRef = sceneModel;
            name = sceneModel.Name + instanceIdx.ToString();            
            InstanceIdx = instanceIdx;            
            this.eventHandlers = eventHandlers;                        
            Translate = new ObservableVector3D(translate);
            Translate.PropertyChanged += this.eventHandlers.transformPanelTranslationEditHandler;
            Translate.PropertyChanged += updateAssetDataTranslation;
            Scale = new ObservableVector3D(scale);
            Scale.PropertyChanged += this.eventHandlers.transformPanelScaleEditHandler;
            Scale.PropertyChanged += updateAssetDataScale;
            RotQuat = new Quaternion(rotAx, rotAng);
            RotEueler = new ObservableVector3D(RotQuat.toEuler());
            RotEueler.PropertyChanged += onInstanceRotated;
            RotEueler.PropertyChanged += updateAssetDataRot;

            sceneModelInstanceAssetData = sceneModel.SceneModelAssetData.addSceneModelInstanceData(translate, scale, RotQuat);

            selectionHandler = new DxVisualizer.IScene.ISceneModelInstance.SelectionHandler(onInstanceSelected);
            iDxSceneModelInstance = sceneModel.SceneMRef.createDxModelInstance(sceneModel.ModelMRef.DxModelID, Color.FromArgb(0, 0, 0, 0), translate, scale, rotAx, rotAng, selectionHandler);
        }                        

        // dx -> wpf
        public void setDisplayedTranslation(Vector3D translation)
        {
            Translate.PropertyChanged -= eventHandlers.transformPanelTranslationEditHandler;
            Translate.X = translation.X;
            Translate.Y = translation.Y;
            Translate.Z = translation.Z;
            Translate.PropertyChanged += eventHandlers.transformPanelTranslationEditHandler;
        }

        public void translateDisplayedTranslation(float x, float y, float z)
        {
            setDisplayedTranslation(Translate.Vector3DCpy + new Vector3D(x, y, z));
        }

        // dx -> wpf
        public void setDisplayedScale(Vector3D scale)
        {
            Scale.PropertyChanged -= eventHandlers.transformPanelScaleEditHandler;
            Scale.X = scale.X;
            Scale.Y = scale.Y;
            Scale.Z = scale.Z;
            Scale.PropertyChanged += eventHandlers.transformPanelScaleEditHandler;
        }

        public void scaleDisplayedScale(float x, float y, float z)
        {
            setDisplayedScale(Scale.Vector3DCpy + new Vector3D(x, y, z));            
        }

        // dx -> wpf
        public void setDisplayedRotation(Quaternion rot)
        {                        
            Vector3D rotEuler = rot.toEuler();
            RotEueler.PropertyChanged -= updateAssetDataRot;
            RotEueler.PropertyChanged -= onInstanceRotated;            
            RotEueler.X = rotEuler.X;
            RotEueler.Y = rotEuler.Y;
            RotEueler.Z = rotEuler.Z;
            RotEueler.PropertyChanged += onInstanceRotated;
            RotEueler.PropertyChanged += updateAssetDataRot;

            RotQuat = rot;
            sceneModelInstanceAssetData.setRotInit(rot);            
        }

        public void rotateDisplayedRotation(float axX, float axY, float axZ, float ang)
        {
            setDisplayedRotation(new Quaternion(new Vector3D(axX, axY, axZ), ang) * RotQuat);
        }
            
        public void addToTransformGrp()
        {
            iDxSceneModelInstance.addToTransformGrp();            
        }

        public void removeFromTransformGrp()
        {
            iDxSceneModelInstance.removeFromTransformGrp();            
        }        

        public void Dispose()
        {
            if (isDisposed)
                return;

            sceneModelInstanceAssetData.Dispose();
            iDxSceneModelInstance.Dispose();
            disposeDxSceneModelInstanceCollider3D();
            disposeDxSceneModelInstanceCollider2D();

            isDisposed = true;
        }

        public void disposeDxSceneModelInstanceCollider3D()
        {
            if (iDxSceneModelInstanceCollider3D == null)
                return;

            foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider3D)
                instance.Dispose();
        }

        public void disposeDxSceneModelInstanceCollider2D()
        {
            if (iDxSceneModelInstanceCollider2D == null)
                return;

            foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider2D)
                instance.Dispose();
        }

        public void highlight()
        {
            iDxSceneModelInstance.highlight();
        }

        public void dim()
        {
            iDxSceneModelInstance.dim();
        }

        public void toggleVisibility()
        {
            IsShown = !IsShown;

            if (IsShown)
            {
                iDxSceneModelInstance.show();
                if (iDxSceneModelInstanceCollider3D != null)
                {
                    foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider3D)
                        instance.show();
                }
                if (iDxSceneModelInstanceCollider2D != null)
                {
                    foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider2D)
                        instance.show();
                }
            }
            else
            {
                iDxSceneModelInstance.hide();
                if (iDxSceneModelInstanceCollider3D != null)
                {
                    foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider3D)
                        instance.hide();
                }
                if (iDxSceneModelInstanceCollider2D != null)
                {
                    foreach (DxVisualizer.IScene.ISceneModelInstance instance in iDxSceneModelInstanceCollider2D)
                        instance.hide();
                }
            }
        }       
        
        private void onInstanceSelected(float x, float y)
        {
            eventHandlers.onThisInstanceSelection(this);
        }

        // wpf -> dx (translate and scale go directly to event handlers given in SceneModelInstanceM ctor
        private void onInstanceRotated(object sender, PropertyChangedEventArgs e)
        {
            RotQuat = RotEueler.Vector3DCpy.asEulerToQuaternion();            
            eventHandlers.transformPanelRotEditHandler(sender, e);
        }

        private void updateAssetDataTranslation(object sender, PropertyChangedEventArgs e)
        {
            sceneModelInstanceAssetData.setTranslationInit(Translate.Vector3DCpy);
        }

        private void updateAssetDataScale(object sender, PropertyChangedEventArgs e)
        {
            sceneModelInstanceAssetData.setScaleInit(Scale.Vector3DCpy);
        }

        private void updateAssetDataRot(object sender, PropertyChangedEventArgs e)
        {
            sceneModelInstanceAssetData.setRotInit(RotQuat);
        }
    }
}
