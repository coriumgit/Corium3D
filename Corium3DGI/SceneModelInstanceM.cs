using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows.Media.Media3D;
using Corium3DGI.Utils;
using CoriumDirectX;

namespace Corium3DGI
{
    public class SceneModelInstanceM : ObservableObject
    {
        private int instanceIdx;
        private DxVisualizer.IScene.SelectionHandler selectionHandler;        
        private OnSelected selectionHandlerExt;

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

        public ObservableQuaternion Rot { get; }

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
        
        public delegate void OnSelected(SceneModelInstanceM thisSelected);        
        public SceneModelInstanceM(SceneModelM sceneModel, int instanceIdx, Vector3D translate, Vector3D scale, Vector3D rotAx, float rotAng, OnSelected selectionHandlerExt)
        {
            selectionHandler = new DxVisualizer.IScene.SelectionHandler(onSelected);            
            this.selectionHandlerExt = selectionHandlerExt;
            this.instanceIdx = instanceIdx;
            SceneModelMRef = sceneModel;                        
            name = sceneModel.Name + instanceIdx.ToString();
            Translate = new ObservableVector3D(translate);
            Scale = new ObservableVector3D(scale);
            Rot = new ObservableQuaternion(rotAx, rotAng);
            
            IDxSceneModelInstance = sceneModel.IDxScene.createModelInstance(sceneModel.ModelMRef.DxModelID, translate, scale, rotAx, rotAng, selectionHandler);            
            Translate.PropertyChanged += onTranslatePropertyChanged;
            Scale.PropertyChanged += onScalePropertyChanged;
            Rot.PropertyChanged += onRotPropertyChanged;
        }

        public delegate void OnTranslationSet(Vector3D translate);
        public delegate void OnScaleSet(Vector3D scale);
        public delegate void OnRotSet(Quaternion rot);

        public event OnTranslationSet TranslationSet;
        public event OnScaleSet ScaleSet;
        public event OnRotSet RotSet;

        public void setTranslation(Vector3D translation)
        {
            Translate.PropertyChanged -= onTranslatePropertyChanged;
            Translate.X = translation.X;
            Translate.Y = translation.Y;
            Translate.Z = translation.Z;
            Translate.PropertyChanged += onTranslatePropertyChanged;
            onTranslatePropertyChanged(null, null);
            /*
            IDxSceneModelInstance.setTranslation(translation);
            if (TranslationSet != null)
            {
                foreach (OnTranslationSet handler in TranslationSet.GetInvocationList())
                    handler.Invoke(translation);
            }
            */
        }

        public void setScale(Vector3D scale)
        {
            Scale.PropertyChanged -= onScalePropertyChanged;
            Scale.X = scale.X;
            Scale.Y = scale.Y;
            Scale.Z = scale.Z;
            Scale.PropertyChanged += onScalePropertyChanged;
            onScalePropertyChanged(null, null);
            /*
            IDxSceneModelInstance.setScale(scale);
            if (ScaleSet != null)
            {
                foreach (OnScaleSet handler in ScaleSet.GetInvocationList())
                    handler.Invoke(scale);
            }
            */
        }

        public void setRotation(Quaternion rot)
        {
            Rot.PropertyChanged -= onRotPropertyChanged;
            Rot.X = rot.X;
            Rot.Y = rot.Y;
            Rot.Z = rot.Z;
            Rot.W = rot.W;
            Rot.PropertyChanged += onRotPropertyChanged;
            onRotPropertyChanged(null, null);
            /*
            IDxSceneModelInstance.setRotation(rot.Axis, (float)rot.Angle);
            if (RotSet != null)
            {
                foreach (OnRotSet handler in RotSet.GetInvocationList())
                    handler.Invoke(rot);
            }
            */
        }

        public int getInstanceIdx() { return instanceIdx; }

        public void releaseDxLmnts()
        {
            IDxSceneModelInstance.release();            
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

        private void onSelected(float x, float y)
        {            
            selectionHandlerExt(this);
        }

        private void onTranslatePropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            IDxSceneModelInstance.setTranslation(Translate.Vector3DCpy);
            if (TranslationSet != null)
            {
                foreach (OnTranslationSet handler in TranslationSet.GetInvocationList())
                    handler.Invoke(Translate.Vector3DCpy);
            }
        }

        private void onScalePropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            IDxSceneModelInstance.setScale(Scale.Vector3DCpy);
            if (ScaleSet != null)
            {
                foreach (OnScaleSet handler in ScaleSet.GetInvocationList())
                    handler.Invoke(Scale.Vector3DCpy);
            }
        }

        private void onRotPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            IDxSceneModelInstance.setRotation(Rot.Axis.Vector3DCpy, (float)Rot.Angle);
            Quaternion rot = new Quaternion(Rot.Axis.Vector3DCpy, Rot.Angle);
            if (RotSet != null)
            {
                foreach (OnRotSet handler in RotSet.GetInvocationList())
                    handler.Invoke(rot);
            }
        } 
        

    }
}
