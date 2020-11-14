using Corium3D;

using System.Linq;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using System.Xml.Linq;

namespace Corium3DGI
{
    public class CollisionPrimitive : ObservableObject
    {
        private static readonly string RESOURCES_PATHS_XML_PATH = System.IO.Path.Combine("Resources", "ResourcesPaths.xml");        

        private const string NAME_CACHE = "None";

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
            cacheAvatarsAssets(NAME_CACHE, new Color() { R = 0, G = 0, B = 0, A = 0 }, out iconPathCache, out avatars3DCache);
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

        protected static void cacheAvatarsAssets(string primitiveName, Color primitiveColor, out string iconPath, out Model3DCollection avatars3D)
        {
            XElement root = XDocument.Load(RESOURCES_PATHS_XML_PATH).Root.Element("CollisionPrimitivesAvatars");
            XElement avatarsPathsNode = root.Elements().Where(e => e.Attribute("Primitive").Value == primitiveName).FirstOrDefault();
            iconPath = avatarsPathsNode.Descendants().Where(e => e.Name == "Icon").FirstOrDefault().Value;
            avatars3D = new Model3DCollection();
            foreach (XElement modelPathLmnt in avatarsPathsNode.Descendants().Where(e => e.Name == "Model"))
            {
                ModelAssetGen modelAssetGen = new ModelAssetGen(modelPathLmnt.Value);                                               
                avatars3D.Add(new GeometryModel3D(modelAssetGen.ManagedImportedDataRef.meshesGeometries[0],
                                                  new DiffuseMaterial(new SolidColorBrush(primitiveColor))));
            }
        }
    }
}
