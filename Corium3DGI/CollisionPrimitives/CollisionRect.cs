﻿using System.Windows;
using Corium3DGI.Utils;
using System.Windows.Media;
using System.ComponentModel;
using System.Windows.Media.Media3D;
using System.Windows.Data;

namespace Corium3DGI
{
    public class CollisionRect : CollisionPrimitive
    {
        private static string nameCache;
        private static string iconPathCache;
        private static Model3DCollection avatars3DCache;

        private ObservablePoint center;
        public ObservablePoint Center
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


        private ObservablePoint scale;
        public ObservablePoint Scale
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

        static CollisionRect()
        {
            nameCache = "Rect";
            cacheAvatarsAssets(nameCache, new Color() { R = 0, G = 196, B = 80, A = 255 }, out iconPathCache, out avatars3DCache);
        }

        public CollisionRect(Point center, Point scale) {
            Name = nameCache;
            IconPath = iconPathCache;
            foreach (GeometryModel3D avatar3D in avatars3DCache)
                avatars3D.Add(new GeometryModel3D(avatar3D.Geometry, avatar3D.Material));

            this.center = new ObservablePoint();
            this.scale = new ObservablePoint();

            ScaleTransform3D scaleTransform = new ScaleTransform3D();
            bindScaleToScaleTransform(scaleTransform);
            TranslateTransform3D translateTransform = new TranslateTransform3D();
            bindCenterToTranslateTransform(translateTransform);
            Transform3DGroup transform3DGroup = new Transform3DGroup();
            transform3DGroup.Children.Add(scaleTransform);
            transform3DGroup.Children.Add(translateTransform);
            Avatars3D[0].Transform = transform3DGroup;

            Center.X = center.X; Center.Y = center.Y;
            Scale.X = scale.X; Scale.Y = scale.Y;
        }       

        private void bindScaleToScaleTransform(ScaleTransform3D scaleTransform)
        {
            bindScaleCoordToScaleTransformCoord(scaleTransform, "X", ScaleTransform3D.ScaleXProperty);
            bindScaleCoordToScaleTransformCoord(scaleTransform, "Y", ScaleTransform3D.ScaleYProperty);
        }

        private void bindScaleCoordToScaleTransformCoord(ScaleTransform3D scaleTransform3D, string coordName, DependencyProperty coordDP)
        {
            Binding coordBinding = new Binding(coordName);
            coordBinding.Source = scale;
            coordBinding.Mode = BindingMode.TwoWay;
            coordBinding.UpdateSourceTrigger = UpdateSourceTrigger.PropertyChanged;
            BindingOperations.SetBinding(scaleTransform3D, coordDP, coordBinding);
        }

        private void bindCenterToTranslateTransform(TranslateTransform3D translateTransform)
        {
            bindCenterCoordToTranslateTransformCoord(translateTransform, "X", TranslateTransform3D.OffsetXProperty);
            bindCenterCoordToTranslateTransformCoord(translateTransform, "Y", TranslateTransform3D.OffsetYProperty);            
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