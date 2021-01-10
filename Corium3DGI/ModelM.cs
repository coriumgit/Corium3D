﻿using Corium3D;
using CoriumDirectX;
using System.IO;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using System.Xml.Linq;
using System.ComponentModel;

namespace Corium3DGI
{
    public class ModelM : ObservableObject, System.IDisposable
    {
        private static readonly string RESOURCES_PATHS_XML_PATH = Path.Combine("Resources", "ResourcesPaths.xml");
        private static AssetsGen.ManagedImportedData nullModelData;

        private DxVisualizer dxVisualizer;                                

        public AssetsGen.IModelAssetGen ModelAssetGen { get; private set; }

        protected string name;
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

        private uint dxModelID;
        public uint DxModelID { get { return dxModelID; } }
        
        private Model3DCollection avatars3D;
        public Model3DCollection Avatars3D
        {
            get { return avatars3D; }

            set
            {
                if (avatars3D != value)
                {
                    avatars3D = value;
                    OnPropertyChanged("Avatars3D");
                }
            }
        }

        private Quaternion rotation;
        public Quaternion Rotation
        {
            get { return rotation; }

            set
            {
                if (rotation != value)
                {
                    rotation = value;
                    OnPropertyChanged("Rotation");
                }
            }
        }

        private List<CollisionPrimitive3D> collisionPrimitives3DCache = new List<CollisionPrimitive3D>();
        public List<CollisionPrimitive3D> CollisionPrimitives3DCache
        {
            get { return collisionPrimitives3DCache; }

            set
            {
                if (collisionPrimitives3DCache != value)
                {
                    collisionPrimitives3DCache = value;
                    OnPropertyChanged("CollisionPrimitives3DCache");
                }
            }
        }

        private CollisionPrimitive3D collisionPrimitive3DSelected;
        public CollisionPrimitive3D CollisionPrimitive3DSelected
        {
            get { return collisionPrimitive3DSelected; }

            set
            {
                if (collisionPrimitive3DSelected != value)
                {                    
                    collisionPrimitive3DSelected = value;
                    collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
                    CollisionPrimitive3dChanged?.Invoke();
                    OnPropertyChanged("CollisionPrimitive3DSelected");
                }
            } 
        }

        private List<CollisionPrimitive2D> collisionPrimitives2DCache = new List<CollisionPrimitive2D>();
        public List<CollisionPrimitive2D> CollisionPrimitives2DCache
        {
            get { return collisionPrimitives2DCache; }

            set
            {
                if (collisionPrimitives2DCache != value)
                {
                    collisionPrimitives2DCache = value;
                    OnPropertyChanged("CollisionPrimitives2DCache");
                }
            }
        }

        private CollisionPrimitive2D collisionPrimitive2DSelected;
        public CollisionPrimitive2D CollisionPrimitive2DSelected
        {
            get { return collisionPrimitive2DSelected; }

            set
            {
                if (collisionPrimitive2DSelected != value)
                {
                    collisionPrimitive2DSelected = value;
                    collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
                    CollisionPrimitive2dChanged?.Invoke();              
                    OnPropertyChanged("CollisionPrimitive2DSelected");
                }
            }
        }

        public delegate void OnCollisionPrimitive3dChanged();
        public event OnCollisionPrimitive3dChanged CollisionPrimitive3dChanged;
        public delegate void OnCollisionBoxCenterChanged(Vector3D ceneter);
        public event OnCollisionBoxCenterChanged CollisionBoxCenterChanged;
        public delegate void OnCollisionBoxScaleChanged(Vector3D ceneter);
        public event OnCollisionBoxScaleChanged CollisionBoxScaleChanged;

        public delegate void OnCollisionPrimitive2dChanged();        
        public event OnCollisionPrimitive2dChanged CollisionPrimitive2dChanged;

        static ModelM()
        {
            string nullModelAvatarPath = XDocument.Load(RESOURCES_PATHS_XML_PATH).Root.Element("NullModelAvatar").Value;
            AssetsGen.IModelAssetGen modelAssetGen = AssetsGen.createModelAssetGen(nullModelAvatarPath);
            nullModelData = modelAssetGen.ManagedImportedDataRef;
            modelAssetGen.Dispose();
        }

        public ModelM(string model3dDatalFilePath, DxVisualizer dxVisualizer)
        {            
            ModelAssetGen = AssetsGen.createModelAssetGen(model3dDatalFilePath);                        
            AssetsGen.ManagedImportedData modelData;
            if (model3dDatalFilePath != string.Empty)
                modelData = ModelAssetGen.ManagedImportedDataRef;
            else
                modelData = nullModelData;

            this.dxVisualizer = dxVisualizer;
            dxVisualizer.addModel(modelData.meshesVertices[0],
                    modelData.meshesVertexIndices[0],
                    Color.FromArgb(255, 175, 175, 175),
                    modelData.boundingSphereCenter,
                    modelData.boundingSphereRadius,
                    DxVisualizer.PrimitiveTopology.TRIANGLELIST, out dxModelID);

            avatars3D = new Model3DCollection();
            foreach (MeshGeometry3D meshGeometry in modelData.meshesGeometries)
                avatars3D.Add(new GeometryModel3D(meshGeometry, new DiffuseMaterial(new SolidColorBrush(Colors.WhiteSmoke))));

            collisionPrimitives3DCache.Add(new CollisionPrimitive3D());
            CollisionBox collisionBox = new CollisionBox(modelData.boundingBoxCenter, modelData.boundingBoxScale);
            collisionBox.Center.PropertyChanged += onCubeColliderCenterChanged;
            collisionBox.Scale.PropertyChanged += onCubeColliderScaleChanged;
            collisionPrimitives3DCache.Add(collisionBox);

            collisionPrimitives3DCache.Add(new CollisionSphere(modelData.boundingSphereCenter, modelData.boundingSphereRadius));
            collisionPrimitives3DCache.Add(new CollisionCapsule(modelData.boundingCapsuleCenter,
                                                                modelData.boundingCapsuleAxisVec,
                                                                modelData.boundingCapsuleHeight,
                                                                modelData.boundingCapsuleRadius));
            // TODO: <<continue here>>




            collisionPrimitive3DSelected = collisionPrimitives3DCache[0];

            collisionPrimitives2DCache.Add(new CollisionPrimitive2D());
            collisionPrimitives2DCache.Add(new CollisionRect(new Point(modelData.boundingBoxCenter.X, modelData.boundingBoxCenter.Y), new Point(modelData.boundingBoxScale.X, modelData.boundingBoxScale.Y)));
            collisionPrimitives2DCache.Add(new CollisionCircle(new Point(modelData.boundingSphereCenter.X, modelData.boundingSphereCenter.Y), modelData.boundingSphereRadius));
            collisionPrimitives2DCache.Add(new CollisionStadium(new Point(modelData.boundingCapsuleCenter.X, modelData.boundingCapsuleCenter.Y),
                                                                new Vector(modelData.boundingCapsuleAxisVec.X, modelData.boundingCapsuleAxisVec.Y),
                                                                modelData.boundingCapsuleHeight,
                                                                modelData.boundingCapsuleRadius));
            collisionPrimitive2DSelected = collisionPrimitives2DCache[0];
        }         

        private void onCubeColliderCenterChanged(object center, PropertyChangedEventArgs e)
        {
            CollisionBoxCenterChanged?.Invoke((Vector3D)((Utils.ObservablePoint3D)center).Point3DCpy);
        }

        private void onCubeColliderScaleChanged(object scale, PropertyChangedEventArgs e)
        {
            CollisionBoxScaleChanged?.Invoke((Vector3D)((Utils.ObservablePoint3D)scale).Point3DCpy);
        }

        private void OnSphereTransform(Point3D center, float radius)
        {

        }

        private void OnCapsuleTransform(Point3D center, Point3D axisVec, float height, float radius)
        {

        }

        virtual public void Dispose()
        {
            dxVisualizer.removeModel(DxModelID);
            ModelAssetGen.Dispose();            
        }
    }
}
