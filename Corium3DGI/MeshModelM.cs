using CoriumDirectX;
using System.IO;

namespace Corium3DGI
{
    public class MeshModelM : ModelM
    {
        public MeshModelM(string model3dDatalFilePath, DxVisualizer dxVisualizer) : base(model3dDatalFilePath, dxVisualizer)
        {
            name = Path.GetFileNameWithoutExtension(model3dDatalFilePath);
        }
    }
}
