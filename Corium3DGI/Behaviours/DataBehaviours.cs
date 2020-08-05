using System.Windows;
using System.Windows.Input;

namespace Corium3DGI.Behaviours
{
    public class DataBehaviours
    {
        public static readonly DependencyProperty OnLoadProperty = DependencyProperty.RegisterAttached(
            "OnLoad",
            typeof(ICommand),
            typeof(DataBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(FrameworkElement.LoadedEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnLoad(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnLoadProperty);
        }

        public static void SetOnLoad(DependencyObject obj, ICommand value)
        {
            obj.SetValue(OnLoadProperty, value);
        }

        public static readonly DependencyProperty OnSizeChangedProperty = DependencyProperty.RegisterAttached(
            "OnSizeChanged",
            typeof(ICommand),
            typeof(DataBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(FrameworkElement.SizeChangedEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnSizeChanged(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnSizeChangedProperty);
        }

        public static void SetOnSizeChanged(DependencyObject obj, ICommand value)
        {
            obj.SetValue(OnSizeChangedProperty, value);
        }

        public static readonly DependencyProperty IsDragObjProperty = DependencyProperty.RegisterAttached(
            "IsDragObj",
            typeof(bool),
            typeof(DataBehaviours),
            new FrameworkPropertyMetadata(true, isDragObjAssigned));

        public static bool GetIsDragObject(DependencyObject obj)
        {
            return (bool)obj.GetValue(IsDragObjProperty);
        }

        public static void SetIsDragObject(DependencyObject obj, bool value)
        {
            obj.SetValue(IsDragObjProperty, value);
        }

        public static void isDragObjAssigned(DependencyObject obj, DependencyPropertyChangedEventArgs e)
        {
            DataObject.AddCopyingHandler(obj, (object sender, DataObjectCopyingEventArgs f) => 
            { 
                if (f.IsDragDrop)
                    f.CancelCommand(); 
            });
        }
    }
}
