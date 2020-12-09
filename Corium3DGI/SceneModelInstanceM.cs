using Corium3D;
using System;
using System.ComponentModel;
using System.Windows.Media.Media3D;
using Corium3DGI.Utils;
using CoriumDirectX;
using System.Windows.Media;

namespace Corium3DGI
{
    public class SceneModelInstanceM : ObservableObject, IDisposable
    {
        private int instanceIdx;
        private IdxPool sceneModelIdxPool;
        private EventHandlers eventHandlers;
        private DxVisualizer.IScene.ISceneModelInstance.SelectionHandler selectionHandler;

        public AssetsGen.ISceneAssetGen.ISceneModelData.ISceneModelInstanceData SceneModelInstanceAssetData { get; private set; }

        public DxVisualizer.IScene.ISceneModelInstance IDxSceneModelInstance { get; private set; }
        
        public SceneModelM SceneModelMRef { get; }

        public ModelVisual3D ModelVisual3DRef { get; }

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
            public PropertyChangedEventHandler transformPanelTranslationEditHander;
            public PropertyChangedEventHandler transformPanelScaleEditHander;
            public PropertyChangedEventHandler transformPanelRotEditHander;
        }

        protected SceneModelInstanceM(SceneModelM sceneModel, IdxPool sceneModelIdxPool, Vector3D translate, Vector3D scale, Vector3D rotAx, float rotAng, EventHandlers eventHandlers)
        {                        
            instanceIdx = sceneModelIdxPool.acquireIdx();
            this.sceneModelIdxPool = sceneModelIdxPool;
            this.eventHandlers = eventHandlers;
            SceneModelMRef = sceneModel;
            name = sceneModel.Name + instanceIdx.ToString();
            Translate = new ObservableVector3D(translate);
            Translate.PropertyChanged += this.eventHandlers.transformPanelTranslationEditHander;
            Translate.PropertyChanged += updateAssetDataTranslation;
            Scale = new ObservableVector3D(scale);
            Scale.PropertyChanged += this.eventHandlers.transformPanelScaleEditHander;
            Scale.PropertyChanged += updateAssetDataScale;
            RotQuat = new Quaternion(rotAx, rotAng);
            RotEueler = new ObservableVector3D(RotQuat.toEuler());
            RotEueler.PropertyChanged += onInstanceRotated;
            RotEueler.PropertyChanged += updateAssetDataRot;

            SceneModelInstanceAssetData = sceneModel.SceneModelAssetData.addSceneModelInstanceData(translate, scale, RotQuat);

            selectionHandler = new DxVisualizer.IScene.ISceneModelInstance.SelectionHandler(onInstanceSelected);
            IDxSceneModelInstance = sceneModel.IDxScene.createModelInstance(sceneModel.ModelMRef.DxModelID, Color.FromArgb(0, 0, 0, 0), translate, scale, rotAx, rotAng, selectionHandler);
        }        

        public void setDisplayedTranslation(Vector3D translation)
        {
            Translate.PropertyChanged -= eventHandlers.transformPanelTranslationEditHander;
            Translate.X = translation.X;
            Translate.Y = translation.Y;
            Translate.Z = translation.Z;
            Translate.PropertyChanged += eventHandlers.transformPanelTranslationEditHander;
            //onTranslatePropertyChanged(null, null);
            /*
            IDxSceneModelInstance.setTranslation(translation);
            if (TranslationSet != null)
            {
                foreach (OnTranslationSet handler in TranslationSet.GetInvocationList())
                    handler.Invoke(translation);
            }
            */
        }

        public void translateDisplayedTranslation(float x, float y, float z)
        {
            setDisplayedTranslation(Translate.Vector3DCpy + new Vector3D(x, y, z));
        }

        public void setDisplayedScale(Vector3D scale)
        {
            Scale.PropertyChanged -= eventHandlers.transformPanelScaleEditHander;
            Scale.X = scale.X;
            Scale.Y = scale.Y;
            Scale.Z = scale.Z;
            Scale.PropertyChanged += eventHandlers.transformPanelScaleEditHander;
            //onScalePropertyChanged(null, null);
            /*
            IDxSceneModelInstance.setScale(scale);
            if (ScaleSet != null)
            {
                foreach (OnScaleSet handler in ScaleSet.GetInvocationList())
                    handler.Invoke(scale);
            }
            */
        }

        public void scaleDisplayedScale(float x, float y, float z)
        {
            setDisplayedScale(Scale.Vector3DCpy + new Vector3D(x, y, z));            
        }

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
            SceneModelInstanceAssetData.setRotInit(rot);            

            //onRotPropertyChanged(null, null);
            /*
            IDxSceneModelInstance.setRotation(rot.Axis, (float)rot.Angle);
            if (RotSet != null)
            {
                foreach (OnRotSet handler in RotSet.GetInvocationList())
                    handler.Invoke(rot);
            }
            */
        }

        public void rotateDisplayedRotation(float axX, float axY, float axZ, float ang)
        {
            setDisplayedRotation(new Quaternion(new Vector3D(axX, axY, axZ), ang) * RotQuat);
        }
            
        public void addToTransformGrp()
        {
            IDxSceneModelInstance.addToTransformGrp();
        }

        public void removeFromTransformGrp()
        {
            IDxSceneModelInstance.removeFromTransformGrp();
        }        

        public void Dispose()
        {
            sceneModelIdxPool.releaseIdx(instanceIdx);
            SceneModelMRef.SceneModelInstanceMs.Remove(this);

            SceneModelInstanceAssetData.Dispose();

            IDxSceneModelInstance.Dispose();            
        }

        public void highlight()
        {
            IDxSceneModelInstance.highlight();
        }

        public void dim()
        {
            IDxSceneModelInstance.dim();
        }

        public void toggleVisibility()
        {
            IsShown = !IsShown;

            if (IsShown)
                IDxSceneModelInstance.show();
            else
                IDxSceneModelInstance.hide();            
        }       
        
        private void onInstanceSelected(float x, float y)
        {
            eventHandlers.onThisInstanceSelection(this);
        }

        private void onInstanceRotated(object sender, PropertyChangedEventArgs e)
        {
            RotQuat = RotEueler.Vector3DCpy.asEulerToQuaternion();            
            eventHandlers.transformPanelRotEditHander(sender, e);
        }

        private void updateAssetDataTranslation(object sender, PropertyChangedEventArgs e)
        {
            SceneModelInstanceAssetData.setTranslationInit(Translate.Vector3DCpy);
        }

        private void updateAssetDataScale(object sender, PropertyChangedEventArgs e)
        {
            SceneModelInstanceAssetData.setScaleInit(Scale.Vector3DCpy);
        }

        private void updateAssetDataRot(object sender, PropertyChangedEventArgs e)
        {
            SceneModelInstanceAssetData.setRotInit(RotQuat);
        }
    }
}
