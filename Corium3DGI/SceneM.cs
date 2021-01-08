using CoriumDirectX;
using Corium3D;
using System;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;
using System.Windows.Media.Media3D;


namespace Corium3DGI
{
    public class SceneM : ObservableObject, IDisposable
    {
        private class SceneModelMShell : SceneModelM
        {
            public SceneModelMShell(SceneM sceneM, ModelM modelM) : base(sceneM, modelM) { }
        }

        private DxVisualizer.IScene.ISceneModelInstance gridDxInstance;

        public AssetsGen.ISceneAssetGen SceneAssetGen { get; private set; }

        private DxVisualizer.IScene iDxScene;

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

        private DxVisualizer.MouseCallbacks dxVisualizerMouseNotifiers;
        public DxVisualizer.OnMouseMoveCallback DxVisualizerMouseMoveNotifier {
            get { return dxVisualizerMouseNotifiers.onMouseMoveCallback; }
        }

        public DxVisualizer.OnMouseUpCallback DxVisualizerMouseUpNotifier
        {
            get { return dxVisualizerMouseNotifiers.onMouseUpCallback; }
        }

        public ObservableCollection<SceneModelM> SceneModelMs { get; } = new ObservableCollection<SceneModelM>();

        public SceneM(string name, DxVisualizer dxVisualizer, uint gridDxModelId, DxVisualizer.IScene.TransformCallbackHandlers transformCallbackHandlers)
        {
            this.name = name;
            SceneAssetGen = AssetsGen.createSceneAssetGen();
            iDxScene = dxVisualizer.createScene(transformCallbackHandlers, out dxVisualizerMouseNotifiers);
            gridDxInstance = iDxScene.createModelInstance(gridDxModelId, Color.FromArgb(0, 0, 0, 0), new Vector3D(0, 0, 0), new Vector3D(1, 1, 1), new Vector3D(1, 0, 0), 0, null);
        }

        public void Dispose()
        {
            gridDxInstance.Dispose();
            foreach (SceneModelM sceneModel in SceneModelMs)
                sceneModel.Dispose();
            iDxScene.Dispose();

            SceneAssetGen.Dispose();
        }

        public SceneModelM addSceneModel(ModelM modelM)
        {
            foreach (SceneModelM sceneModel in SceneModelMs)
            {
                if (sceneModel.ModelMRef == modelM)
                    return null;
            }

            SceneModelM sceneModelM = new SceneModelMShell(this, modelM);
            SceneModelMs.Add(sceneModelM);

            return sceneModelM;
        }

        public void removeSceneModel(ModelM modelM)
        {
            foreach (SceneModelM sceneModel in SceneModelMs)
            {
                if (sceneModel.ModelMRef == modelM)
                {
                    sceneModel.Dispose();
                    SceneModelMs.Remove(sceneModel);
                    return;
                }
            }
        }

        public void removeSceneModel(SceneModelM sceneModelM)
        {
            foreach (SceneModelM sceneModel in SceneModelMs)
            {
                if (sceneModel == sceneModelM)
                {
                    sceneModel.Dispose();
                    SceneModelMs.Remove(sceneModel);
                    return;
                }
            }
        }

        public DxVisualizer.IScene.ISceneModelInstance createDxModelInstance(uint dxModelID, Color colorMask, Vector3D translate, Vector3D scale, Vector3D rotAx, float rotAng, DxVisualizer.IScene.ISceneModelInstance.SelectionHandler selectionHandler) {
            return iDxScene.createModelInstance(dxModelID, colorMask, translate, scale, rotAx, rotAng, selectionHandler);
        }

        public void syncDxVisualsWithModelsUpdate()
        {
            //foreach (SceneModelM sceneModel in SceneModelMs)
            //    sceneModel.flushDxVisualsUpdate();
        }        

        public Vector3D cursorPosToDxRayDirection(Point cursorPos)
        {
            return (Vector3D)iDxScene.cursorPosToRayDirection((float)cursorPos.X, (float)cursorPos.Y);
        }

        public Point3D getDxCameraPos()
        {
            return (Point3D)iDxScene.getCameraPos();
        }

        public void panDxCamera(Vector cursorMoveVec)
        {
            iDxScene.panCamera((float)cursorMoveVec.X, (float)cursorMoveVec.Y);
        }

        public void rotateDxCamera(Vector cursorMoveVec)
        {
            iDxScene.rotateCamera((float)cursorMoveVec.X, (float)cursorMoveVec.Y);
        }        

        public void zoomDxCamera(float amount)
        {
            iDxScene.zoomCamera(amount);
        }        
        
        public void activateDxScene()
        {
            iDxScene.activate();
        }

        public bool cursorSelect(float x, float y)
        {
            bool wasSceneModelInstanceSelected = iDxScene.cursorSelect(x, y);
            //if (!wasSceneModelInstanceSelected)
              //  IDxScene.dimHighlightedInstance();

            return wasSceneModelInstanceSelected;
        }

        public void transformGrpTranslate(Vector3D translation, DxVisualizer.IScene.TransformReferenceFrame referenceFrame)
        {
            iDxScene.transformGrpTranslate(translation, referenceFrame);
        }

        public void transformGrpSetTranslation(Vector3D translation, DxVisualizer.IScene.TransformReferenceFrame referenceFrame)
        {
            iDxScene.transformGrpSetTranslation(translation, referenceFrame);
        }

        public void transformGrpScale(Vector3D scaleFactorQ, DxVisualizer.IScene.TransformReferenceFrame referenceFrame)
        {
            iDxScene.transformGrpScale(scaleFactorQ, referenceFrame);
        }

        public void transformGrpSetScale(Vector3D scaleFactor, DxVisualizer.IScene.TransformReferenceFrame referenceFrame)
        {
            iDxScene.transformGrpSetScale(scaleFactor, referenceFrame);
        }

        public void transformGrpRotate(Vector3D ax, double ang, DxVisualizer.IScene.TransformReferenceFrame referenceFrame)
        {
            iDxScene.transformGrpRotate(ax, (float)ang, referenceFrame);
        }

        public void transformGrpSetRotation(Vector3D ax, double ang, DxVisualizer.IScene.TransformReferenceFrame referenceFrame)
        {
            iDxScene.transformGrpSetRotation(ax, (float)ang, referenceFrame);
        }
    }

    public class SceneMToComboBoxItemConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object param, System.Globalization.CultureInfo culture)
        {
            if (value != null)
                return ((SceneM)value).Name;
            else
                return null;
        }

        public object ConvertBack(object value, Type targetType, object param, System.Globalization.CultureInfo culture)
        {
            ComboBoxItem comboBoxItem = (ComboBoxItem)value;
            if (comboBoxItem.Content is SceneM)            
                return comboBoxItem.Content;            
            else
                return null;
        }
    }
}
