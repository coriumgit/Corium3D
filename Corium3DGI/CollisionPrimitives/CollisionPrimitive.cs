using Corium3D;
using CoriumDirectX;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using System.Xml.Linq;

namespace Corium3DGI
{
    public abstract class CollisionPrimitive : ObservableObject
    {
        private static readonly string RESOURCES_PATHS_XML_PATH = System.IO.Path.Combine("Resources", "ResourcesPaths.xml");        

        private const string NAME_CACHE = "None";
        private static string iconPathCache;                

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

        public static void Init()
        {            
            cacheAvatarsAssets(NAME_CACHE, out iconPathCache);
        }

        public CollisionPrimitive() {
            Name = NAME_CACHE;
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

        public abstract void asssignPrimitiveDataInModelAssetGen(AssetsGen.IModelAssetGen modelAssetGen);        

        protected static void cacheAvatarsAssets(string primitiveName, out string iconPath)
        {
            XElement root = XDocument.Load(RESOURCES_PATHS_XML_PATH).Root.Element("CollisionPrimitivesAvatars");
            XElement avatarsPathsNode = root.Elements().Where(e => e.Attribute("Primitive").Value == primitiveName).FirstOrDefault();
            iconPath = avatarsPathsNode.Descendants().Where(e => e.Name == "Icon").FirstOrDefault().Value;            
        }

        protected static void cacheAvatarsAssets(string primitiveName, Color primitiveColor, out string iconPath, out Model3DCollection avatars3D, DxVisualizer dxVisualizer, out List<uint> dxModelIDs)
        {
            XElement root = XDocument.Load(RESOURCES_PATHS_XML_PATH).Root.Element("CollisionPrimitivesAvatars");
            XElement avatarsPathsNode = root.Elements().Where(e => e.Attribute("Primitive").Value == primitiveName).FirstOrDefault();
            iconPath = avatarsPathsNode.Descendants().Where(e => e.Name == "Icon").FirstOrDefault().Value;
            avatars3D = new Model3DCollection();
            dxModelIDs = new List<uint>();                
            foreach (XElement modelPathLmnt in avatarsPathsNode.Descendants().Where(e => e.Name == "Model"))
            {
                AssetsGen.IModelAssetGen modelAssetGen = AssetsGen.createModelAssetGen(modelPathLmnt.Value);
                avatars3D.Add(new GeometryModel3D(modelAssetGen.ManagedImportedDataRef.meshesGeometries[0],
                                                  new DiffuseMaterial(new SolidColorBrush(primitiveColor))));
                modelAssetGen.Dispose();

                uint dxModelID;
                dxVisualizer.addModel(modelAssetGen.ManagedImportedDataRef.meshesVertices[0],
                    modelAssetGen.ManagedImportedDataRef.meshesVertexIndices[0],
                    Color.FromArgb(255, 175, 175, 175),
                    modelAssetGen.ManagedImportedDataRef.boundingSphereCenter,
                    modelAssetGen.ManagedImportedDataRef.boundingSphereRadius,
                    DxVisualizer.PrimitiveTopology.TRIANGLELIST, out dxModelID);
                dxModelIDs.Add(dxModelID);
            }
        }
    }
}
