using CoriumDirectX;
using System.Windows;

namespace Corium3DGI
{
    public interface ICorium3DGIView
    {        
        DxVisualizer DxVisualizerRef { get; }
        UIElement ModelViewportContainer { get; }
        UIElement SceneViewportContainer { get; }

        event RoutedEventHandler UIElementsLoaded;
    }
}
