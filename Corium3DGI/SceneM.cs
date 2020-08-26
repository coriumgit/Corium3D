using CoriumDirectX;
using System;
using System.Collections.ObjectModel;
using System.Windows.Controls;
using System.Windows.Data;

namespace Corium3DGI
{
    public class SceneM : ObservableObject
    {        
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

        public ObservableCollection<SceneModelM> SceneModelMs { get; } = new ObservableCollection<SceneModelM>();

        public SceneM(string name, DxVisualizer.IScene iDxScene) 
        {
            this.name = name;
            IDxScene = iDxScene;
        }

        public void releaseDxLmnts()
        {
            IDxScene.release();
        }

        public SceneModelM addSceneModel(ModelM model)
        {
            foreach (SceneModelM sceneModel in SceneModelMs)
            {
                if (sceneModel.ModelMRef == model)
                    return null;
            }

            SceneModelM sceneModelM = new SceneModelM(this, model);
            SceneModelMs.Add(sceneModelM);

            return sceneModelM;
        }

        public void removeSceneModel(ModelM modelMRef)
        {
            foreach (SceneModelM sceneModel in SceneModelMs)
            {
                if (sceneModel.ModelMRef == modelMRef)
                {
                    removeSceneModel(sceneModel);                    
                    return;
                }
            }            
        }

        public void removeSceneModel(SceneModelM sceneModel)
        {
            SceneModelMs.Remove(sceneModel);
            sceneModel.releaseDxLmnts();
        }

        public bool cursorSelect(float x, float y)
        {
            bool wasSceneModelInstanceSelected = IDxScene.cursorSelect(x, y);
            //if (!wasSceneModelInstanceSelected)
              //  IDxScene.dimHighlightedInstance();

            return wasSceneModelInstanceSelected;
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
