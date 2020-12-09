using CoriumDirectX;
using Corium3D;
using System;
using System.Collections.ObjectModel;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media.Media3D;

namespace Corium3DGI
{
    public class SceneM : ObservableObject, IDisposable
    {
        private class SceneModelMShell : SceneModelM
        {
            public SceneModelMShell(SceneM sceneM, ModelM modelM) : base(sceneM, modelM) { }
        }

        public AssetsGen.ISceneAssetGen SceneAssetGen { get; private set; }

        public DxVisualizer.IScene IDxScene { get; private set; }

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

        public SceneM(string name, DxVisualizer dxVisualizer, DxVisualizer.IScene.TransformCallbackHandlers transformCallbackHandlers) 
        {
            this.name = name;

            SceneAssetGen = AssetsGen.createSceneAssetGen();
            
            IDxScene = dxVisualizer.createScene(transformCallbackHandlers, out dxVisualizerMouseNotifiers);
        }

        public void Dispose()
        {
            IDxScene.Dispose();

            foreach (SceneModelM sceneModel in SceneModelMs)                                            
                sceneModel.Dispose();            
            SceneAssetGen.Dispose();
        }

        public SceneModelM addSceneModel(ModelM model)
        {            
            foreach (SceneModelM sceneModel in SceneModelMs)
            {
                if (sceneModel.ModelMRef == model)
                    return null;
            }

            SceneModelM sceneModelM = new SceneModelMShell(this, model);
            SceneModelMs.Add(sceneModelM);

            return sceneModelM;
        }

        public void removeSceneModel(ModelM modelMRef)
        {
            foreach (SceneModelM sceneModel in SceneModelMs)
            {
                if (sceneModel.ModelMRef == modelMRef)
                {
                    sceneModel.Dispose();
                    return;
                }
            }            
        }

        public bool cursorSelect(float x, float y)
        {
            bool wasSceneModelInstanceSelected = IDxScene.cursorSelect(x, y);
            //if (!wasSceneModelInstanceSelected)
              //  IDxScene.dimHighlightedInstance();

            return wasSceneModelInstanceSelected;
        }

        public void transformGrpTranslate(Vector3D translation)
        {
            IDxScene.transformGrpTranslate(translation);
        }

        public void transformGrpSetTranslation(Vector3D translation)
        {
            IDxScene.transformGrpSetTranslation(translation);
        }

        public void transformGrpScale(Vector3D scaleFactorQ)
        {
            IDxScene.transformGrpScale(scaleFactorQ);
        }

        public void transformGrpSetScale(Vector3D scaleFactor)
        {
            IDxScene.transformGrpSetScale(scaleFactor);
        }

        public void transformGrpRotate(Vector3D ax, double ang)
        {
            IDxScene.transformGrpRotate(ax, (float)ang);
        }

        public void transformGrpSetRotation(Vector3D ax, double ang)
        {
            IDxScene.transformGrpSetRotation(ax, (float)ang);
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
