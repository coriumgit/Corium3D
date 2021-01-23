using CoriumDirectX;

using System.Windows;
using System.Windows.Data;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using Corium3DGI.Utils;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class CollisionBox : CollisionPrimitive3D
    {
        private const string NAME_CACHE = "Box";
        private static Model3DCollection avatars3DCache;        
        private static uint dxModelID;        

        private ObservablePoint3D center;
        public ObservablePoint3D Center
        {
            get { return center; }

            set
            {
                if (center != value)
                {
                    center = value;
                    OnPropertyChanged("Center");                    
                }                
            }
        }

        private ObservablePoint3D scale;
        public ObservablePoint3D Scale
        {
            get { return scale; }

            set
            {
                if (scale != value)
                {                    
                    scale = value;
                    OnPropertyChanged("Scale");
                }          
            }
        }

        public delegate void OnTransform(Point3D center, Point3D scale);

        public static void Init(DxVisualizer dxVisualizer)
        {
            List<uint> dxModelIdContainer;
            cacheAvatarsAssets(NAME_CACHE, new Color() { R = 0, G = 255, B = 0, A = 255 }, out avatars3DCache, dxVisualizer, out dxModelIdContainer);
            dxModelID = dxModelIdContainer[0];
        }

        public CollisionBox(Point3D center, Point3D scale) {
            Name = NAME_CACHE;          
            foreach (GeometryModel3D avatar3D in avatars3DCache)
                avatars3D.Add(new GeometryModel3D(avatar3D.Geometry, avatar3D.Material));                        

            this.center = new ObservablePoint3D();
            this.scale = new ObservablePoint3D();

            ScaleTransform3D scaleTransform3D = new ScaleTransform3D();
            bindScaleToScaleTransform(scaleTransform3D);
            TranslateTransform3D translateTransform3D = new TranslateTransform3D();
            bindCenterToTranslateTransform(translateTransform3D);            
            Transform3DGroup transform3DGroup = new Transform3DGroup();
            transform3DGroup.Children.Add(scaleTransform3D);
            transform3DGroup.Children.Add(translateTransform3D);                                    
            Avatars3D[0].Transform = transform3DGroup;

            Center.X = center.X; Center.Y = center.Y; Center.Z = center.Z;
            Scale.X = scale.X; Scale.Y = scale.Y; Scale.Z = scale.Z;
        }

        public override DxVisualizer.IScene.ISceneModelInstance[] createDxInstances(SceneM sceneM, Vector3D instanceTranslate, Vector3D instanceScale, Vector3D instanceRotAx, float instanceRotAng)
        {
            return new DxVisualizer.IScene.ISceneModelInstance[] {
                sceneM.createDxModelInstance(dxModelID, Color.FromArgb(50, 0, 255, 0),
                                             (Vector3D)center.Point3DCpy + instanceTranslate,
                                             new Vector3D(scale.Point3DCpy.X * instanceScale.X, scale.Point3DCpy.Y * instanceScale.Y, scale.Point3DCpy.Z * instanceScale.Z),
                                             instanceRotAx, instanceRotAng, null)
            };            
        }

        private void bindScaleToScaleTransform(ScaleTransform3D scaleTransform3D)
        {
            bindScaleCoordToScaleTransformCoord(scaleTransform3D, "X", ScaleTransform3D.ScaleXProperty);
            bindScaleCoordToScaleTransformCoord(scaleTransform3D, "Y", ScaleTransform3D.ScaleYProperty);
            bindScaleCoordToScaleTransformCoord(scaleTransform3D, "Z", ScaleTransform3D.ScaleZProperty);
        }

        private void bindScaleCoordToScaleTransformCoord(ScaleTransform3D scaleTransform3D, string coordName, DependencyProperty coordDP)
        {
            Binding coordBinding = new Binding(coordName);
            coordBinding.Source = scale;
            coordBinding.Mode = BindingMode.TwoWay;
            coordBinding.UpdateSourceTrigger = UpdateSourceTrigger.PropertyChanged;
            BindingOperations.SetBinding(scaleTransform3D, coordDP, coordBinding);
        }

        private void bindCenterToTranslateTransform(TranslateTransform3D translateTransform3D)
        {
            bindCenterCoordToTranslateTransformCoord(translateTransform3D, "X", TranslateTransform3D.OffsetXProperty);
            bindCenterCoordToTranslateTransformCoord(translateTransform3D, "Y", TranslateTransform3D.OffsetYProperty);
            bindCenterCoordToTranslateTransformCoord(translateTransform3D, "Z", TranslateTransform3D.OffsetZProperty);
        }

        private void bindCenterCoordToTranslateTransformCoord(TranslateTransform3D translateTransform3D, string coordName, DependencyProperty coordDP)
        {
            Binding coordBinding = new Binding(coordName);
            coordBinding.Source = center;
            coordBinding.Mode = BindingMode.TwoWay;
            coordBinding.UpdateSourceTrigger = UpdateSourceTrigger.PropertyChanged;
            BindingOperations.SetBinding(translateTransform3D, coordDP, coordBinding);
        }  
    }
}
