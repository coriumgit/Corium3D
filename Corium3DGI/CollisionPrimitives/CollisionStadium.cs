using CoriumDirectX;

using System.Windows;
using Corium3DGI.Utils;
using System.Windows.Media;
using System.Windows.Media.Media3D;
using System.Windows.Data;
using System.ComponentModel;
using System;
using System.Collections.Generic;

namespace Corium3DGI
{
    public class CollisionStadium : CollisionPrimitive2D
    {
        private const double EPSILON = 1E-6;
        private const string NAME_CACHE = "Stadium";        
        private static Model3DCollection avatars3DCache;
        private static uint[] dxModelIDs;

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

        private ObservableVector axisVec;
        public ObservableVector AxisVec
        {
            get { return axisVec; }

            set
            {
                if (axisVec != value)
                {
                    axisVec = value;
                    axisVec.PropertyChanged += OnAxisVecUpdated;
                    OnPropertyChanged("AxisVec");
                }
            }
        }

        private float height;
        public float Height
        {
            get { return height; }

            set
            {
                if (height != value)
                {
                    height = value;
                    OnPropertyChanged("Height");
                }
            }
        }

        private float radius;
        public float Radius
        {
            get { return radius; }

            set
            {
                if (radius != value)
                {
                    radius = value;
                    OnPropertyChanged("Radius");
                }
            }
        }

        public static void Init(DxVisualizer dxVisualizer)
        {
            List<uint> dxModelIdContainer;
            cacheAvatarsAssets(NAME_CACHE, new Color() { R = 255, G = 128, B = 0, A = 255 }, out avatars3DCache, dxVisualizer, out dxModelIdContainer);
            dxModelIDs = dxModelIdContainer.ToArray();
        }

        public CollisionStadium(Point center, Vector axisVec, float height, float radius) {
            Name = NAME_CACHE;            
            foreach (GeometryModel3D avatar3D in avatars3DCache)
                avatars3D.Add(new GeometryModel3D(avatar3D.Geometry, avatar3D.Material));            

            this.center = new ObservablePoint();
            this.axisVec = new ObservableVector();
            this.height = height;
            this.radius = radius;
            
            ScaleTransform3D scaleTransform3DShaft = new ScaleTransform3D();
            ScaleTransform3D scaleTransform3DDemiSphere1 = new ScaleTransform3D();
            TranslateTransform3D translateTransform3DDemiSphere1PerHeight = new TranslateTransform3D();
            ScaleTransform3D scaleTransform3DDemiSphere2 = new ScaleTransform3D();
            TranslateTransform3D translateTransform3DDemiSphere2PerHeight = new TranslateTransform3D();
            TranslateTransform3D translateTransform3D = new TranslateTransform3D();
            bindAvatar3DTransforms(scaleTransform3DShaft, scaleTransform3DDemiSphere1, translateTransform3DDemiSphere1PerHeight,
                                   scaleTransform3DDemiSphere2, translateTransform3DDemiSphere2PerHeight, translateTransform3D);

            RotateTransform3D rotateTransform3D = new RotateTransform3D();
            rotateTransform3D.Rotation = new QuaternionRotation3D(Quaternion.Identity);

            Transform3DGroup transform3DGroupShaft = new Transform3DGroup();
            transform3DGroupShaft.Children.Add(scaleTransform3DShaft);
            transform3DGroupShaft.Children.Add(rotateTransform3D);
            transform3DGroupShaft.Children.Add(translateTransform3D);
            Avatars3D[0].Transform = transform3DGroupShaft;

            Transform3DGroup transform3DGroupDemiSphere1 = new Transform3DGroup();
            transform3DGroupDemiSphere1.Children.Add(scaleTransform3DDemiSphere1);
            transform3DGroupDemiSphere1.Children.Add(translateTransform3DDemiSphere1PerHeight);
            transform3DGroupDemiSphere1.Children.Add(rotateTransform3D);
            transform3DGroupDemiSphere1.Children.Add(translateTransform3D);
            Avatars3D[1].Transform = transform3DGroupDemiSphere1;

            Transform3DGroup transform3DGroupDemiSphere2 = new Transform3DGroup();
            transform3DGroupDemiSphere2.Children.Add(scaleTransform3DDemiSphere2);
            transform3DGroupDemiSphere2.Children.Add(translateTransform3DDemiSphere2PerHeight);
            transform3DGroupDemiSphere2.Children.Add(rotateTransform3D);
            transform3DGroupDemiSphere2.Children.Add(translateTransform3D);
            Avatars3D[2].Transform = transform3DGroupDemiSphere2;

            Center.X = center.X; Center.Y = center.Y;
            AxisVec.X = axisVec.X; AxisVec.Y = axisVec.Y;
        }

        public CollisionStadium(CollisionStadium other)
        {
            center = new ObservablePoint(other.center);
            axisVec = new ObservableVector(other.axisVec);
            height = 1;
            radius = 1;

            bindAvatar3DTransforms((ScaleTransform3D)((Transform3DGroup)Avatars3D[0].Transform).Children[0],
                                   (ScaleTransform3D)((Transform3DGroup)Avatars3D[1].Transform).Children[0],
                                   (TranslateTransform3D)((Transform3DGroup)Avatars3D[1].Transform).Children[1],
                                   (ScaleTransform3D)((Transform3DGroup)Avatars3D[2].Transform).Children[0],
                                   (TranslateTransform3D)((Transform3DGroup)Avatars3D[2].Transform).Children[1],
                                   (TranslateTransform3D)((Transform3DGroup)Avatars3D[0].Transform).Children[2]);
        }

        public override DxVisualizer.IScene.ISceneModelInstance[] createDxInstances(SceneM sceneM, Vector3D instanceTranslate, Vector3D instanceScale, Vector3D instanceRotAx, float instanceRotAng)
        {
            // TODO: extract rotation round Z and replace instanceRotAx with it
            Quaternion quatProd = new Quaternion(instanceRotAx, instanceRotAng) * axisVecToQuat();
            Vector3D center3D = new Vector3D(center.PointCpy.X, center.PointCpy.Y, 0.0f);
            Vector3D axisVec3D = new Vector3D(axisVec.VectorCpy.X, axisVec.VectorCpy.Y, 0.0f);
            DxVisualizer.IScene.IConstrainedTransform2dInstance cylinderDxInstance =
                sceneM.createDxConstrained2dInstance(dxModelIDs[0], Color.FromArgb(50, 255, 255, 0),
                                                     new Vector3D(center.PointCpy.X, center.PointCpy.Y, 0.0f) + instanceTranslate,
                                                     new Vector3D(radius * instanceScale.X, height * instanceScale.Y, 1.0f),
                                                     quatProd.Axis, (float)quatProd.Angle, null);
            cylinderDxInstance.setScaleConstraints(DxVisualizer.IScene.TransformScaleConstraint.None, DxVisualizer.IScene.TransformScaleConstraint.None);

            DxVisualizer.IScene.IConstrainedTransform2dInstance topSemicircleDxInstance =
                sceneM.createDxConstrained2dInstance(dxModelIDs[1], Color.FromArgb(50, 255, 255, 0),
                                                     center3D + (0.5f * height * axisVec3D) + instanceTranslate,
                                                     new Vector3D(radius * instanceScale.X, radius * instanceScale.Y, 1.0f),
                                                     quatProd.Axis, (float)quatProd.Angle, null);
            topSemicircleDxInstance.setScaleConstraints(DxVisualizer.IScene.TransformScaleConstraint.MaxDimGrp, DxVisualizer.IScene.TransformScaleConstraint.MaxDimGrp);

            DxVisualizer.IScene.IConstrainedTransform2dInstance bottomSemicircleDxInstance =
                sceneM.createDxConstrained2dInstance(dxModelIDs[2], Color.FromArgb(50, 255, 255, 0),
                                                     center3D - (0.5f * height * axisVec3D) + instanceTranslate,
                                                     new Vector3D(radius * instanceScale.X, radius * instanceScale.Y, 1.0f),
                                                     quatProd.Axis, (float)quatProd.Angle, null);
            bottomSemicircleDxInstance.setScaleConstraints(DxVisualizer.IScene.TransformScaleConstraint.MaxDimGrp, DxVisualizer.IScene.TransformScaleConstraint.MaxDimGrp);
            
            return new DxVisualizer.IScene.ISceneModelInstance[] { cylinderDxInstance, topSemicircleDxInstance, bottomSemicircleDxInstance };
        }

        public override CollisionPrimitive clone()
        {
            return new CollisionStadium(this);
        }

        private void bindAvatar3DTransforms(ScaleTransform3D scaleTransform3DShaft,
                                            ScaleTransform3D scaleTransform3DDemiSphere1,
                                            TranslateTransform3D translateTransform3DDemiSphere1PerHeight,
                                            ScaleTransform3D scaleTransform3DDemiSphere2,
                                            TranslateTransform3D translateTransform3DDemiSphere2PerHeight,
                                            TranslateTransform3D translateTransform3D)
        {
            TwoWayUpdateOnChangedBinder.bindObjsProperties(this, "Radius", scaleTransform3DShaft, ScaleTransform3D.ScaleXProperty, null);
            TwoWayUpdateOnChangedBinder.bindObjsProperties(this, "Height", scaleTransform3DShaft, ScaleTransform3D.ScaleYProperty, null);

            bindRadiusToScaleTransform(scaleTransform3DDemiSphere1);
            TwoWayUpdateOnChangedBinder.bindObjsProperties(this, "Height", translateTransform3DDemiSphere1PerHeight, TranslateTransform3D.OffsetYProperty, new HalfTheDouble());

            bindRadiusToScaleTransform(scaleTransform3DDemiSphere2);
            TwoWayUpdateOnChangedBinder.bindObjsProperties(this, "Height", translateTransform3DDemiSphere2PerHeight, TranslateTransform3D.OffsetYProperty, new MinusHalfTheDouble());

            axisVec.PropertyChanged += OnAxisVecUpdated;

            bindCenterToTranslateTransform(translateTransform3D);
        }

        private void bindCenterToTranslateTransform(TranslateTransform3D translateTransform3D)
        {
            TwoWayUpdateOnChangedBinder.bindObjsProperties(center, "X", translateTransform3D, TranslateTransform3D.OffsetXProperty, null);
            TwoWayUpdateOnChangedBinder.bindObjsProperties(center, "Y", translateTransform3D, TranslateTransform3D.OffsetYProperty, null);            
        }

        private void bindRadiusToScaleTransform(ScaleTransform3D scaleTransform3D)
        {
            TwoWayUpdateOnChangedBinder.bindObjsProperties(this, "Radius", scaleTransform3D, ScaleTransform3D.ScaleXProperty, null);
            TwoWayUpdateOnChangedBinder.bindObjsProperties(this, "Radius", scaleTransform3D, ScaleTransform3D.ScaleYProperty, null);            
        }

        private void OnAxisVecUpdated(object sender, PropertyChangedEventArgs e)
        {
            const double EPSILON = 1E-6;

            Vector axisVecCpy = axisVec.VectorCpy;
            if (axisVecCpy.LengthSquared > 1)
                return;
            
            double rotationAng = System.Math.Acos((new Vector(0, 1)) * axisVecCpy) * 180.0 / System.Math.PI;
            Quaternion rotationQuat;
            if (rotationAng > EPSILON)                            
                rotationQuat = new Quaternion(new Vector3D(0, 0, 1), rotationAng);            
            else
                rotationQuat = Quaternion.Identity;

            ((QuaternionRotation3D)((RotateTransform3D)((Transform3DGroup)Avatars3D[0].Transform).Children[1]).Rotation).Quaternion = rotationQuat;
        }

        private Quaternion axisVecToQuat()
        {
            Vector3D axisVecCpy = new Vector3D(axisVec.VectorCpy.X, axisVec.VectorCpy.Y, 1.0f);
            Vector3D upVector = new Vector3D(0, 1, 0);
            Vector3D rotationAxis = Vector3D.CrossProduct(upVector, axisVecCpy);
            if (rotationAxis.LengthSquared > EPSILON)
            {
                rotationAxis.Normalize();
                double rotationAng = Math.Acos(Vector3D.DotProduct(upVector, axisVecCpy)) * 180.0 / Math.PI;
                return new Quaternion(rotationAxis, rotationAng);
            }
            else
                return Quaternion.Identity;
        }

        private class HalfTheDouble : IValueConverter
        {
            public object Convert(object value, Type targetType, object paramter, System.Globalization.CultureInfo culture)
            {
                return 0.5 * (float)value;
            }

            public object ConvertBack(object value, Type targetType, object paramter, System.Globalization.CultureInfo culture)
            {
                return 2 * (float)value;
            }
        }

        private class MinusHalfTheDouble : IValueConverter
        {
            public object Convert(object value, Type targetType, object paramter, System.Globalization.CultureInfo culture)
            {
                return -0.5 * (float)value;
            }

            public object ConvertBack(object value, Type targetType, object paramter, System.Globalization.CultureInfo culture)
            {
                return -2 * (float)value;
            }
        }
        
        private class MinusTheDouble : IValueConverter
        {
            public object Convert(object value, Type targetType, object paramter, System.Globalization.CultureInfo culture)
            {
                return -(float)value;
            }

            public object ConvertBack(object value, Type targetType, object paramter, System.Globalization.CultureInfo culture)
            {
                return -(float)value;
            }
        }
    }
}
