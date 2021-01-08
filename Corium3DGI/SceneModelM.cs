using Corium3D;
using CoriumDirectX;
using Corium3DGI.Utils;
using System;
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
            name = modelM.Name;

            SceneModelAssetData = sceneM.SceneAssetGen.addSceneModelData(modelM.ModelAssetGen, InstancesNrMax, IsStatic);

            IDxScene = sceneM.IDxScene;
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
            SceneModelInstanceMs.Add(sceneModelInstance);

            return sceneModelInstance;
        }

        public void removeSceneModelInstance(SceneModelInstanceM sceneModelInstance)
        {
            SceneModelInstanceMs.Remove(sceneModelInstance);
            sceneModelInstance.Dispose();                        
        }

        public bool Equals(SceneModelM other)
        {
            return ModelMRef == other.ModelMRef;
        }
    }
}
