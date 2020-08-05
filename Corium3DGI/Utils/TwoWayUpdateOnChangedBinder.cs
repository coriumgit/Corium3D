using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media.Media3D;

namespace Corium3DGI.Utils
{
    public static class TwoWayUpdateOnChangedBinder
    {
        public static void bindPoint3DToTranslateTransform(ObservablePoint3D point3D, TranslateTransform3D translateTransform3D)
        {
            bindObjsProperties(point3D, "X", translateTransform3D, TranslateTransform3D.OffsetXProperty, null);
            bindObjsProperties(point3D, "Y", translateTransform3D, TranslateTransform3D.OffsetYProperty, null);
            bindObjsProperties(point3D, "Z", translateTransform3D, TranslateTransform3D.OffsetZProperty, null);
        }

        public static void bindVector3DToScaleTransform(ObservableVector3D vector3D, ScaleTransform3D scaleTransform3D)
        {
            bindObjsProperties(vector3D, "X", scaleTransform3D, ScaleTransform3D.ScaleXProperty, null);
            bindObjsProperties(vector3D, "Y", scaleTransform3D, ScaleTransform3D.ScaleYProperty, null);
            bindObjsProperties(vector3D, "Z", scaleTransform3D, ScaleTransform3D.ScaleZProperty, null);
        }

        public static void bindQuaternionToAxisAngleRotation(ObservableQuaternion quaternion, AxisAngleRotation3D axisAngleRotation)
        {
            bindObjsProperties(quaternion, "Angle", axisAngleRotation, AxisAngleRotation3D.AngleProperty, null);
            bindObjsProperties(quaternion, "Axis", axisAngleRotation, AxisAngleRotation3D.AxisProperty, new Vector3DObservableVector3DConverter());                     
        }

        public static void bindObjsProperties(object sourceObj, string propertyName, DependencyObject targetObj, DependencyProperty transform3DDP, IValueConverter converter)
        {
            Binding objPropertyBinding = new Binding(propertyName)
            {
                Source = sourceObj,
                Mode = BindingMode.TwoWay,
                UpdateSourceTrigger = UpdateSourceTrigger.PropertyChanged,
                Converter = converter
            };
            BindingOperations.SetBinding(targetObj, transform3DDP, objPropertyBinding);
        }

        private class Vector3DObservableVector3DConverter : IValueConverter
        {
            public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
            {
                return ((ObservableVector3D)value).Vector3DCpy;
            }

            public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
            {
                return new ObservableVector3D((Vector3D)value);
            }
        }
    }
}
