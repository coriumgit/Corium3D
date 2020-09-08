using System;
using System.Collections.ObjectModel;
using CoriumDirectX;
using Corium3DGI.Utils;
using System.Windows.Media.Media3D;

namespace Corium3DGI
{
    public class SceneModelM : ObservableObject, IEquatable<SceneModelM>
    {
        private IdxPool idxPool = new IdxPool();

        public DxVisualizer.IScene IDxScene { get; private set; }

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

        private uint instancesNrMax;
        public uint InstancesNrMax
        {
            get { return instancesNrMax; }

            set
            {
                Console.WriteLine("InstancesNrMax was set");
                if (instancesNrMax != value)
                {
                    instancesNrMax = value;
                    OnPropertyChanged("InstancesNrMax");
                }
            }
        }

        public ObservableCollection<SceneModelInstanceM> SceneModelInstanceMs { get; } = new ObservableCollection<SceneModelInstanceM>();

        public SceneModelM(SceneM sceneM, ModelM modelM)
        {                        
            ModelMRef = modelM;
            name = modelM.Name;            
            IDxScene = sceneM.IDxScene;
        }        

        public void releaseDxLmnts()
        {
            foreach (SceneModelInstanceM instance in SceneModelInstanceMs)
                instance.releaseDxLmnts();            
        }

        public SceneModelInstanceM addSceneModelInstance(Vector3D instanceTranslationInit, Vector3D instanceScaleFactorInit, Vector3D instanceRotAxInit, float instanceRotAngInit, SceneModelInstanceM.OnSelected selectionHandler)
        {
            SceneModelInstanceM sceneModelInstance = new SceneModelInstanceM(this, idxPool.acquireIdx(), instanceTranslationInit, instanceScaleFactorInit, instanceRotAxInit, instanceRotAngInit, selectionHandler);
            SceneModelInstanceMs.Add(sceneModelInstance);

            return sceneModelInstance;
        }

        public void removeSceneModelInstance(SceneModelInstanceM instance)
        {
            idxPool.releaseIdx(instance.getInstanceIdx());
            SceneModelInstanceMs.Remove(instance);
            instance.releaseDxLmnts();                        
        }        

        public bool Equals(SceneModelM other)
        {
            return ModelMRef == other.ModelMRef;
        }
    }
}
