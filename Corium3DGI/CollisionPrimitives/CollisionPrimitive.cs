﻿using System.Linq;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using System.Xml.Linq;
using Corium3DGI.Utils;

namespace Corium3DGI
{
    public class CollisionPrimitive : ObservableObject
    {
        private static readonly string COLLISION_PRIMITIVES_AVATARS_PATHS = System.IO.Path.Combine("Resources", "CollisionPrimitivesAvatarsPaths.xml");

        private static string nameCache;
        private static string iconPathCache;
        private static Model3DCollection avatars3DCache;

        public string Name { get; protected set; }

        public string IconPath { get; protected set; }

        protected Model3DCollection avatars3D;
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

        static CollisionPrimitive()
        {
            nameCache = "None";
            cacheAvatarsAssets(nameCache, new Color() { R = 0, G = 0, B = 0, A = 0 }, out iconPathCache, out avatars3DCache);
        }

        public CollisionPrimitive() {
            Name = nameCache;
            IconPath = iconPathCache;
            avatars3D = new Model3DCollection();           
        }        

        public CollisionPrimitive(CollisionPrimitive other)
        {
            Name = other.Name;
            IconPath = other.IconPath;
            avatars3D = other.avatars3D;
        }

        public virtual CollisionPrimitive clone()
        {
            return (CollisionPrimitive)this.MemberwiseClone();
        }

        protected static void cacheAvatarsAssets(string primitiveName, Color primitiveColor, out string iconPath, out Model3DCollection avatars3D)
        {
            XElement root = XDocument.Load(COLLISION_PRIMITIVES_AVATARS_PATHS).Root;
            XElement avatarsPathsNode = root.Elements().Where(e => e.Attribute("Primitive").Value == primitiveName).FirstOrDefault();
            iconPath = avatarsPathsNode.Descendants().Where(e => e.Name == "IconPath").FirstOrDefault().Value;
            avatars3D = new Model3DCollection();
            foreach (XElement modelPathLmnt in avatarsPathsNode.Descendants().Where(e => e.Name == "ModelPath"))
            {
                AssetsImporter.NamedModelData namedModelData = AssetsImporter.Instance.importModel(modelPathLmnt.Value);
                AssetsImporter.Instance.removeModel(namedModelData.name);                               
                avatars3D.Add(new GeometryModel3D(namedModelData.importData.meshesGeometries[0],
                                                  new DiffuseMaterial(new SolidColorBrush(primitiveColor))));
            }
        }
    }
}