﻿using Corium3D;
using CoriumDirectX;
using System.IO;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Media3D;

namespace Corium3DGI
{
    public class ModelM : ObservableObject, System.IDisposable
    {                
        private DxVisualizer dxVisualizer;
        
        public AssetsGen.IModelAssetGen ModelAssetGen { get; private set; }

        private uint dxModelID;
        public uint DxModelID { get { return dxModelID; } }

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
                            ModelAssetGen.assignCollisionBox(cb.Center.Point3DCpy, cb.Scale.Point3DCpy);
                            break;
                        case CollisionSphere cs:
                            ModelAssetGen.assignCollisionSphere(cs.Center.Point3DCpy, cs.Radius);
                            break;
                        case CollisionCapsule cc:
                            Vector3D axisVec = cc.Height * cc.AxisVec.Vector3DCpy;
                            ModelAssetGen.assignCollisionCapsule(cc.Center.Point3DCpy - 0.5 * axisVec, axisVec, cc.Radius);
                            break;
                        default:
                            ModelAssetGen.clearCollisionPrimitive3D();
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
                            ModelAssetGen.assignCollisionRect(cr.Center.PointCpy, cr.Scale.PointCpy);
                            break;
                        case CollisionCircle cc:
                            ModelAssetGen.assignCollisionCircle(cc.Center.PointCpy, cc.Radius);
                            break;
                        case CollisionStadium cs:
                            Vector axisVec = cs.Height * cs.AxisVec.VectorCpy;
                            ModelAssetGen.assignCollisionStadium(cs.Center.PointCpy - 0.5 * axisVec, axisVec, cs.Radius);
                            break;
                        default:
                            ModelAssetGen.clearCollisionPrimitive2D();
                            break;
                    }
                    OnPropertyChanged("CollisionPrimitive2DSelected");
                }
            }
        }

        public ModelM(string model3dDatalFilePath, DxVisualizer dxVisualizer)
        {
            name = Path.GetFileNameWithoutExtension(model3dDatalFilePath);
            
            ModelAssetGen = AssetsGen.createModelAssetGen(model3dDatalFilePath);
            AssetsGen.ManagedImportedData modelImportData = ModelAssetGen.ManagedImportedDataRef;

            this.dxVisualizer = dxVisualizer;
            dxVisualizer.addModel(modelImportData.meshesVertices[0],
                modelImportData.meshesVertexIndices[0],
                Color.FromArgb(255, 175, 175, 175),
                modelImportData.boundingSphereCenter,
                modelImportData.boundingSphereRadius,
                PrimitiveTopology.TRIANGLELIST, out dxModelID);
            
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
        }         

        public void Dispose()
        {
            dxVisualizer.removeModel(DxModelID);
            ModelAssetGen.Dispose();            
        }
    }
}
