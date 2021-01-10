using System.Windows;
using System.Windows.Media;

namespace Corium3DGI.CustomCtrls
{
    public class ToggledImageButton : ImageButton
    {
        private ImageSource origSource;
        private bool isToggledImgDisplayed = false;

        static ToggledImageButton()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(ToggledImageButton), new FrameworkPropertyMetadata(typeof(ToggledImageButton)));            
        }
        
        public ToggledImageButton() : base()
        {
            Loaded += onLoaded;
        }

        public static readonly DependencyProperty ToggleImgSourceProperty = DependencyProperty.Register(
            "ToggleImgSource",
            typeof(ImageSource),
            typeof(ToggledImageButton),
            new FrameworkPropertyMetadata((ImageSource)null));

        public static ImageSource GetToggleImgSource(DependencyObject obj)
        {
            return (ImageSource)obj.GetValue(ToggleImgSourceProperty);
        }

        public static void SetToggleImgSource(DependencyObject obj, ImageSource toggleImgSrc)
        {
            obj.SetValue(ToggleImgSourceProperty, toggleImgSrc);
        }

        public void onLoaded(object sender, RoutedEventArgs e)
        {
            origSource = GetSource(this);
            Click += onClick;
        }

        public void onClick(object sender, RoutedEventArgs e)
        {
            if (isToggledImgDisplayed)
                SetSource(this, origSource);
            else
                SetSource(this, GetToggleImgSource(this));

            isToggledImgDisplayed = !isToggledImgDisplayed;
        }
    }
}
