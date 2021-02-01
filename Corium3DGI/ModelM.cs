using Corium3D;
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
        private bool isDisposed = false;

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
                    collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
                    CollisionPrimitive2dChanged?.Invoke();              
                    OnPropertyChanged("CollisionPrimitive2DSelected");
                }
            }
        }

        public delegate void OnCollisionPrimitive3dChanged();
        public event OnCollisionPrimitive3dChanged CollisionPrimitive3dChanged;

        public delegate void OnCollisionBoxCenterChanged(Vector3D center);
        public event OnCollisionBoxCenterChanged CollisionBoxCenterChanged;
        public delegate void OnCollisionBoxScaleChanged(Vector3D ceneter);
        public event OnCollisionBoxScaleChanged CollisionBoxScaleChanged;

        public delegate void OnCollisionSphereCenterChanged(Vector3D center);
        public event OnCollisionSphereCenterChanged CollisionSphereCenterChanged;
        public delegate void OnCollisionSphereRadiusChanged(float radius);
        public event OnCollisionSphereRadiusChanged CollisionSphereRadiusChanged;

        public delegate void OnCollisionCapsuleCenterChanged(CollisionCapsule capsule);
        public event OnCollisionCapsuleCenterChanged CollisionCapsuleCenterChanged;
        public delegate void OnCollisionCapsuleAxisVecChanged(CollisionCapsule capsule);
        public event OnCollisionCapsuleAxisVecChanged CollisionCapsuleAxisVecChanged;
        public delegate void OnCollisionCapsuleHeightChanged(CollisionCapsule capsule);
        public event OnCollisionCapsuleHeightChanged CollisionCapsuleHeightChanged;
        public delegate void OnCollisionCapsuleRadiusChanged(CollisionCapsule capsule);
        public event OnCollisionCapsuleRadiusChanged CollisionCapsuleRadiusChanged;

        public delegate void OnCollisionPrimitive2dChanged();        
        public event OnCollisionPrimitive2dChanged CollisionPrimitive2dChanged;

        public delegate void OnCollisionRectCenterChanged(Vector center);
        public event OnCollisionRectCenterChanged CollisionRectCenterChanged;
        public delegate void OnCollisionRectScaleChanged(Vector ceneter);
        public event OnCollisionRectScaleChanged CollisionRectScaleChanged;
        
        public delegate void OnCollisionCircleCenterChanged(Vector center);
        public event OnCollisionCircleCenterChanged CollisionCircleCenterChanged;
        public delegate void OnCollisionCircleRadiusChanged(float radius);
        public event OnCollisionCircleRadiusChanged CollisionCircleRadiusChanged;
        
        public delegate void OnCollisionStadiumCenterChanged(CollisionStadium stadium);
        public event OnCollisionStadiumCenterChanged CollisionStadiumCenterChanged;
        public delegate void OnCollisionStadiumAxisVecChanged(CollisionStadium stadium);
        public event OnCollisionStadiumAxisVecChanged CollisionStadiumAxisVecChanged;
        public delegate void OnCollisionStadiumHeightChanged(CollisionStadium stadium);
        public event OnCollisionStadiumHeightChanged CollisionStadiumHeightChanged;
        public delegate void OnCollisionStadiumRadiusChanged(CollisionStadium stadium);
        public event OnCollisionStadiumRadiusChanged CollisionStadiumRadiusChanged;
        
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
            collisionBox.Center.PropertyChanged += onCollisionCubeCenterChanged;
            collisionBox.Scale.PropertyChanged += onCollisionCubeScaleChanged;
            collisionPrimitives3DCache.Add(collisionBox);

            CollisionSphere collisionSphere = new CollisionSphere(modelData.boundingSphereCenter, modelData.boundingSphereRadius);
            collisionSphere.Center.PropertyChanged += onCollisionSphereCenterChanged;
            collisionSphere.PropertyChanged += onCollisionSphereChanged;
            collisionPrimitives3DCache.Add(collisionSphere);

            CollisionCapsule collisionCapsule = new CollisionCapsule(modelData.boundingCapsuleCenter, modelData.boundingCapsuleAxisVec, modelData.boundingCapsuleHeight, modelData.boundingCapsuleRadius);
            collisionCapsule.Center.PropertyChanged += onCollisionCapsuleCenterChanged;
            collisionCapsule.AxisVec.PropertyChanged += onCollisionCapsuleAxisVecChanged;
            collisionCapsule.PropertyChanged += onCollisionCapsuleChanged;
            collisionPrimitives3DCache.Add(collisionCapsule);            

            collisionPrimitive3DSelected = collisionPrimitives3DCache[0];
            
            collisionPrimitives2DCache.Add(new CollisionPrimitive2D());

            CollisionRect collisionRect = new CollisionRect(new Point(modelData.boundingBoxCenter.X, modelData.boundingBoxCenter.Y), new Point(modelData.boundingBoxScale.X, modelData.boundingBoxScale.Y));
            collisionRect.Center.PropertyChanged += onCollisionRectCenterChanged;
            collisionRect.Scale.PropertyChanged += onCollisionRectScaleChanged;
            collisionPrimitives2DCache.Add(collisionRect);

            CollisionCircle collisionCircle = new CollisionCircle(new Point(modelData.boundingSphereCenter.X, modelData.boundingSphereCenter.Y), modelData.boundingSphereRadius);
            collisionCircle.Center.PropertyChanged += onCollisionCircleCenterChanged;
            collisionCircle.PropertyChanged += onCollisionCircleChanged;
            collisionPrimitives2DCache.Add(collisionCircle);

            CollisionStadium collisionStadium = new CollisionStadium(new Point(modelData.boundingCapsuleCenter.X, modelData.boundingCapsuleCenter.Y),
                                                                     new Vector(0.0f, 1.0f),
                                                                     modelData.boundingCapsuleHeight,
                                                                     modelData.boundingCapsuleRadius);
            collisionStadium.Center.PropertyChanged += onCollisionStadiumCenterChanged;
            collisionStadium.AxisVec.PropertyChanged += onCollisionStadiumAxisVecChanged;
            collisionStadium.PropertyChanged += onCollisionStadiumChanged;            
            collisionPrimitives2DCache.Add(collisionStadium);

            collisionPrimitive2DSelected = collisionPrimitives2DCache[0];
        }         

        private void onCollisionCubeCenterChanged(object center, PropertyChangedEventArgs e)
        {
            CollisionBoxCenterChanged?.Invoke((Vector3D)((Utils.ObservablePoint3D)center).Point3DCpy);
            collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionCubeScaleChanged(object scale, PropertyChangedEventArgs e)
        {
            CollisionBoxScaleChanged?.Invoke((Vector3D)((Utils.ObservablePoint3D)scale).Point3DCpy);
            collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionSphereCenterChanged(object center, PropertyChangedEventArgs e)
        {
            CollisionSphereCenterChanged?.Invoke((Vector3D)((Utils.ObservablePoint3D)center).Point3DCpy);
            collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionSphereChanged(object collisionSphere, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Radius")
            {
                CollisionSphereRadiusChanged?.Invoke(((CollisionSphere)collisionSphere).Radius);
                collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
            }
        }

        private void onCollisionCapsuleCenterChanged(object center, PropertyChangedEventArgs e)
        {
            CollisionCapsuleCenterChanged?.Invoke((CollisionCapsule)collisionPrimitive3DSelected);
            collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionCapsuleAxisVecChanged(object axisVec, PropertyChangedEventArgs e)
        {
            CollisionCapsuleAxisVecChanged?.Invoke((CollisionCapsule)collisionPrimitive3DSelected);
            collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }
        
        private void onCollisionCapsuleChanged(object collisionCapsule, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Height")
            {
                CollisionCapsuleHeightChanged?.Invoke((CollisionCapsule)collisionCapsule);
                collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
            }
            else if (e.PropertyName == "Radius")
            {
                CollisionCapsuleRadiusChanged?.Invoke((CollisionCapsule)collisionCapsule);
                collisionPrimitive3DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
            }
        }

        private void onCollisionRectCenterChanged(object center, PropertyChangedEventArgs e)
        {
            CollisionRectCenterChanged?.Invoke((Vector)((Utils.ObservablePoint)center).PointCpy);
            collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionRectScaleChanged(object scale, PropertyChangedEventArgs e)
        {
            CollisionRectScaleChanged?.Invoke((Vector)((Utils.ObservablePoint)scale).PointCpy);
            collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionCircleCenterChanged(object center, PropertyChangedEventArgs e)
        {
            CollisionCircleCenterChanged?.Invoke((Vector)((Utils.ObservablePoint)center).PointCpy);
            collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionCircleChanged(object collisionCircle, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Radius")
            {
                CollisionCircleRadiusChanged?.Invoke(((CollisionCircle)collisionCircle).Radius);
                collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
            }
        }

        private void onCollisionStadiumCenterChanged(object center, PropertyChangedEventArgs e)
        {
            CollisionStadiumCenterChanged?.Invoke((CollisionStadium)collisionPrimitive2DSelected);
            collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionStadiumAxisVecChanged(object axisVec, PropertyChangedEventArgs e)
        {
            CollisionStadiumAxisVecChanged?.Invoke((CollisionStadium)collisionPrimitive2DSelected);
            collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
        }

        private void onCollisionStadiumChanged(object collisionStadium, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "Height")
            {
                CollisionStadiumHeightChanged?.Invoke((CollisionStadium)collisionStadium);
                collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
            }
            else if (e.PropertyName == "Radius")
            {
                CollisionStadiumRadiusChanged?.Invoke((CollisionStadium)collisionStadium);
                collisionPrimitive2DSelected.asssignPrimitiveDataInModelAssetGen(ModelAssetGen);
            }
        }

        virtual public void Dispose()
        {
            if (isDisposed)
                return;

            dxVisualizer.removeModel(DxModelID);
            ModelAssetGen.Dispose();

            isDisposed = true;
        }
    }
}
