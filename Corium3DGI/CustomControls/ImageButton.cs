using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Media;

namespace Corium3DGI.CustomCtrls
{
    public class ImageButton : Button
    {
        static ImageButton()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(ImageButton), new FrameworkPropertyMetadata(typeof(ImageButton)));
        }

        public static readonly DependencyProperty SourceProperty = DependencyProperty.Register(
            "Source",
            typeof(ImageSource),
            typeof(ImageButton),
            new FrameworkPropertyMetadata((ImageSource)null));

        public static ImageSource GetSource(DependencyObject obj)
        {
            return (ImageSource)obj.GetValue(SourceProperty);
        }

        public static void SetSource(DependencyObject obj, ImageSource imgSrc)
        {
            obj.SetValue(SourceProperty, imgSrc);
        }

        public new static readonly DependencyProperty WidthProperty = DependencyProperty.Register(
            "Width",
            typeof(double),
            typeof(ImageButton),
            new FrameworkPropertyMetadata((double)0.0));

        public static double GetWidth(DependencyObject obj)
        {
            return (double)obj.GetValue(WidthProperty);
        }

        public static void SetWidth(DependencyObject obj, double width)
        {
            obj.SetValue(WidthProperty, width);
        }

        public new static readonly DependencyProperty HeightProperty = DependencyProperty.Register(
            "Height",
            typeof(double),
            typeof(ImageButton),
            new FrameworkPropertyMetadata((double)0.0));

        public static double GetHeight(DependencyObject obj)
        {
            return (double)obj.GetValue(HeightProperty);
        }

        public static void SetHeight(DependencyObject obj, double height)
        {
            obj.SetValue(HeightProperty, height);
        }

        public static readonly DependencyProperty RotationProperty = DependencyProperty.Register(
            "Rotation",
            typeof(double),
            typeof(ImageButton),
            new FrameworkPropertyMetadata((double)0.0));

        public static double GetRotation(DependencyObject obj)
        {
            return (double)obj.GetValue(RotationProperty);
        }

        public static void SetRotation(DependencyObject obj, double value)
        {
            obj.SetValue(RotationProperty, value);
        }
    }

    public class DoubleHalfer : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return ((double)value) / 2;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
