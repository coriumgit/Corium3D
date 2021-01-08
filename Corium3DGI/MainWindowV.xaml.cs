using CoriumDirectX;

using System;
using System.Windows;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Corium3DGI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindowV : Window, ICorium3DGIView
    {
        private bool isSceneViewportVisible = false;
        private TimeSpan prevRenderTime;        

        // TODO: remove this when the SceneViewPort is a custom control
        public DxVisualizer DxVisualizerRef { get; private set; }
        public UIElement ModelViewportContainer { get; private set; }
        public UIElement SceneViewportContainer { get; private set; }

        public event RoutedEventHandler UIElementsLoaded;

        // TODO: Move to view model when the SceneViewPort is a custom control
        public float CameraFieldOfView { get; } = 60;
        public float CameraNearPlaneDist { get; } = 1; //0.125;
        public float CameraFarPlaneDist { get; } = 1000;
        
        public MainWindowV()
        {
            DxVisualizerRef = new DxVisualizer(CameraFieldOfView, CameraNearPlaneDist, CameraFarPlaneDist);
            InitializeComponent();
        }

        private void OnWindowLoaded(object sender, RoutedEventArgs e)
        {
            ModelViewportContainer = modelViewportContainer;
            SceneViewportContainer = sceneViewportContainer;
            UIElementsLoaded.Invoke(this, e);
        }

        private void OnWindowClosing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            CompositionTarget.Rendering -= OnCompositionTargetRendering;
        }

        private void OnSceneViewportLoaded(object sender, RoutedEventArgs e)
        {            
            sceneViewport.WindowOwner = (new System.Windows.Interop.WindowInteropHelper(this)).Handle;
            sceneViewport.OnRender = RenderDxScene;
            sceneViewport.RequestRender();                        
        }        

        private void RenderDxScene(IntPtr surface, bool isSurfaceNew)
        {
            if (isSurfaceNew)
                DxVisualizerRef.initRenderer(surface);

            DxVisualizerRef.render();                   
        }

        private void OnViewportSizeChanged(object sender, RoutedEventArgs e)
        {
            double dpiScale = 1.0; // default value for 96 dpi

            // determine DPI
            // (as of .NET 4.6.1, this returns the DPI of the primary monitor, if you have several different DPIs)
            var hwndTarget = PresentationSource.FromVisual(this).CompositionTarget as HwndTarget;
            if (hwndTarget != null)           
                dpiScale = hwndTarget.TransformToDevice.M11;            

            //int surfWidth = (int)(sceneViewportContainer.ActualWidth < 0 ? 0 : Math.Ceiling(sceneViewportContainer.ActualWidth * dpiScale));
            //int surfHeight = (int)(sceneViewportContainer.ActualHeight < 0 ? 0 : Math.Ceiling(sceneViewportContainer.ActualHeight * dpiScale));
            int surfWidth = (int)(sceneViewportContainer.ActualWidth < 0 ? 0 : sceneViewportContainer.ActualWidth);
            int surfHeight = (int)(sceneViewportContainer.ActualHeight < 0 ? 0 : sceneViewportContainer.ActualHeight);

            // Notify the D3D11Image of the pixel size desired for the DirectX rendering.
            // The D3DRendering component will determine the size of the new surface it is given, at that point.
            sceneViewport.SetPixelSize(surfWidth, surfHeight);

            // Stop rendering if the D3DImage isn't visible - currently just if width or height is 0
            // TODO: more optimizations possible (scrolled off screen, etc...)
            bool isVisible = surfWidth != 0 && surfHeight != 0;
            if (isSceneViewportVisible != isVisible)
            {
                isSceneViewportVisible = isVisible;
                if (isVisible)                
                    CompositionTarget.Rendering += OnCompositionTargetRendering;                
                else                
                    CompositionTarget.Rendering -= OnCompositionTargetRendering;                
            }
        }

        private void OnCompositionTargetRendering(object sender, EventArgs e)
        {
            RenderingEventArgs args = (RenderingEventArgs)e;

            // It's possible for Rendering to call back twice in the same frame 
            // so only render when we haven't already rendered in this frame.
            if (prevRenderTime != args.RenderingTime)
            {
                sceneViewport.RequestRender();
                prevRenderTime = args.RenderingTime;
            }
        }

        private void SetScenesComboBoxEditable(object sender, RoutedEventArgs e)
        {
            sceneComboBox.IsEditable = true;            
        }

        private void OnSceneModelIsStaticGotFocus(object sender, RoutedEventArgs e)
        {/*
            DataGridCell cell = sender as DataGridCell;
            if (cell != null)
            {
                CheckBox checkBox = cell.Content as CheckBox;
                if (checkBox != null)
                    checkBox.IsChecked = !checkBox.IsChecked;
            }
            */
        }        

        public void ModelsDataTable_DeleteModel(object sender, RoutedEventArgs e)
        {            
            ModelsTable.Items.Refresh();
        }
    }

    public class IsShownToVisibilityBtnImageSource : IValueConverter
    {
        public object Convert(object value, Type targetType, object param, System.Globalization.CultureInfo culture)
        {
            if ((bool)value)
                return new BitmapImage(new Uri("pack://application:,,,/Corium3DGI;" + System.IO.Path.Combine("component", "Resources", "Imgs", "eye.png")));
            else
                return new BitmapImage(new Uri("pack://application:,,,/Corium3DGI;" + System.IO.Path.Combine("component", "Resources", "Imgs", "eye_crossed.png")));
        }

        public object ConvertBack(object value, Type targetType, object param, System.Globalization.CultureInfo culture)
        {
            if (((BitmapImage)value).UriSource == new Uri(System.IO.Path.Combine("Resources", "Imgs", "eye.png")))
                return true;
            else
                return false;
        }
    }
}
