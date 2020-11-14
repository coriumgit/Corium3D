using Corium3D;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Media3D;

namespace Corium3DGI
{
    public class ModelM : ObservableObject
    {
        private ModelAssetGen modelAssetGen;

        public uint DxModelID { get; private set; }        

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

        private List<CollisionPrimitive> collisionPrimitives3DCache = new List<CollisionPrimitive>();
        public List<CollisionPrimitive> CollisionPrimitives3DCache
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

        private CollisionPrimitive collisionPrimitive3DSelected;
        public CollisionPrimitive CollisionPrimitive3DSelected
        {
            get { return collisionPrimitive3DSelected; }

            set
            {
                if (collisionPrimitive3DSelected != value)
                {
                    collisionPrimitive3DSelected = value;
                    switch (value)
                    {
                        case CollisionBox cb:
                            modelAssetGen.assignCollisionBox(cb.Center.Point3DCpy, cb.Scale.Point3DCpy);
                            break;
                        case CollisionSphere cs:
                            modelAssetGen.assignCollisionSphere(cs.Center.Point3DCpy, cs.Radius);
                            break;
                        case CollisionCapsule cc:
                            Vector3D axisVec = cc.Height * cc.AxisVec.Vector3DCpy;
                            modelAssetGen.assignCollisionCapsule(cc.Center.Point3DCpy - 0.5 * axisVec, axisVec, cc.Radius);
                            break;
                        default:
                            modelAssetGen.clearCollisionPrimitive3D();
                            break;
                    }
                    OnPropertyChanged("CollisionPrimitive3DSelected");
                }
            } 
        }

        private List<CollisionPrimitive> collisionPrimitives2DCache = new List<CollisionPrimitive>();
        public List<CollisionPrimitive> CollisionPrimitives2DCache
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

        private CollisionPrimitive collisionPrimitive2DSelected;
        public CollisionPrimitive CollisionPrimitive2DSelected
        {
            get { return collisionPrimitive2DSelected; }

            set
            {
                if (collisionPrimitive2DSelected != value)
                {
                    collisionPrimitive2DSelected = value;
                    switch (value)
                    {
                        case CollisionRect cr:
                            modelAssetGen.assignCollisionRect(cr.Center.PointCpy, cr.Scale.PointCpy);
                            break;
                        case CollisionCircle cc:
                            modelAssetGen.assignCollisionCircle(cc.Center.PointCpy, cc.Radius);
                            break;
                        case CollisionStadium cs:
                            Vector axisVec = cs.Height * cs.AxisVec.VectorCpy;
                            modelAssetGen.assignCollisionStadium(cs.Center.PointCpy - 0.5 * axisVec, axisVec, cs.Radius);
                            break;
                        default:
                            modelAssetGen.clearCollisionPrimitive2D();
                            break;
                    }
                    OnPropertyChanged("CollisionPrimitive2DSelected");
                }
            }
        }

        public ModelM(string name, ModelAssetGen modelAssetGen, uint dxModelID)
        {
            this.name = name;
            this.modelAssetGen = modelAssetGen;

            ModelAssetGen.ManagedImportedData modelImportData = modelAssetGen.ManagedImportedDataRef;
            avatars3D = new Model3DCollection();
            foreach (MeshGeometry3D meshGeometry in modelImportData.meshesGeometries)
                avatars3D.Add(new GeometryModel3D(meshGeometry, new DiffuseMaterial(new SolidColorBrush(Colors.WhiteSmoke))));

            collisionPrimitives3DCache.Add(new CollisionPrimitive());
            Point3D aabb3DMinVertex = modelImportData.aabb3DMinVertex;
            Point3D aabb3DMaxVertex = modelImportData.aabb3DMaxVertex;
            Point3D boxCenter = new Point3D(0.5 * (aabb3DMinVertex.X + aabb3DMaxVertex.X), 
                                            0.5 * (aabb3DMinVertex.Y + aabb3DMaxVertex.Y), 
                                            0.5 * (aabb3DMinVertex.Z + aabb3DMaxVertex.Z));
            Point3D boxScale = new Point3D(0.5 * (aabb3DMaxVertex.X - aabb3DMinVertex.X),
                                           0.5 * (aabb3DMaxVertex.Y - aabb3DMinVertex.Y),
                                           0.5 * (aabb3DMaxVertex.Z - aabb3DMinVertex.Z));
            collisionPrimitives3DCache.Add(new CollisionBox(boxCenter, boxScale));           
            collisionPrimitives3DCache.Add(new CollisionSphere(modelImportData.boundingSphereCenter, modelImportData.boundingSphereRadius));
            collisionPrimitives3DCache.Add(new CollisionCapsule(modelImportData.boundingCapsuleCenter, 
                                                                modelImportData.boundingCapsuleAxisVec,
                                                                modelImportData.boundingCapsuleHeight,
                                                                modelImportData.boundingCapsuleRadius));
            collisionPrimitive3DSelected = collisionPrimitives3DCache[0];

            collisionPrimitives2DCache.Add(new CollisionPrimitive());            
            Point rectCenter = new Point(0.5 * (aabb3DMinVertex.X + aabb3DMaxVertex.X),
                                         0.5 * (aabb3DMinVertex.Y + aabb3DMaxVertex.Y));
            Point rectScale = new Point(0.5 * (aabb3DMaxVertex.X - aabb3DMinVertex.X),
                                        0.5 * (aabb3DMaxVertex.Y - aabb3DMinVertex.Y));
            collisionPrimitives2DCache.Add(new CollisionRect(rectCenter, rectScale));
            collisionPrimitives2DCache.Add(new CollisionCircle(new Point(modelImportData.boundingSphereCenter.X, modelImportData.boundingSphereCenter.Y), modelImportData.boundingSphereRadius));
            collisionPrimitives2DCache.Add(new CollisionStadium(new Point(modelImportData.boundingCapsuleCenter.X, modelImportData.boundingCapsuleCenter.Y),
                                                                new Vector(modelImportData.boundingCapsuleAxisVec.X, modelImportData.boundingCapsuleAxisVec.Y),
                                                                modelImportData.boundingCapsuleHeight,
                                                                modelImportData.boundingCapsuleRadius));
            collisionPrimitive2DSelected = collisionPrimitives2DCache[0];

            DxModelID = dxModelID;
        } 
        
        public void genCorium3dAssetFile()
        {
            modelAssetGen.genCorium3dAsset(name);
        }
    }
}
