using System.Windows;
using System.Windows.Input;

namespace Corium3DGI.Behaviours
{
    public class InputBehaviours
    {
        public static readonly DependencyProperty OnMouseDownProperty = DependencyProperty.RegisterAttached(
            "OnMouseDown",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.MouseDownEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnMouseDown(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnMouseDownProperty);
        }

        public static void SetOnMouseDown(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnMouseDownProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewDragEnterProperty = DependencyProperty.RegisterAttached(
            "OnPreviewDragEnter",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewDragEnterEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewDragEnter(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewDragEnterProperty);
        }

        public static void SetOnPreviewDragEnter(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewDragEnterProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewDragOverProperty = DependencyProperty.RegisterAttached(
            "OnPreviewDragOver",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewDragOverEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewDragOver(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewDragOverProperty);
        }

        public static void SetOnPreviewDragOver(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewDragOverProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewDragLeaveProperty = DependencyProperty.RegisterAttached(
            "OnPreviewDragLeave",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewDragLeaveEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewDragLeave(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewDragLeaveProperty);
        }

        public static void SetOnPreviewDragLeave(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewDragLeaveProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewDropProperty = DependencyProperty.RegisterAttached(
            "OnPreviewDrop",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewDropEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewDrop(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewDropProperty);
        }

        public static void SetOnPreviewDrop(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewDropProperty, cmd);
        }

        public static readonly DependencyProperty OnDragEnterProperty = DependencyProperty.RegisterAttached(
            "OnDragEnter",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.DragEnterEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnDragEnter(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnDragEnterProperty);
        }

        public static void SetOnDragEnter(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnDragEnterProperty, cmd);
        }

        public static readonly DependencyProperty OnDragOverProperty = DependencyProperty.RegisterAttached(
            "OnDragOver",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.DragOverEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnDragOver(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnDragOverProperty);
        }

        public static void SetOnDragOver(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnDragOverProperty, cmd);
        }

        public static readonly DependencyProperty OnDropProperty = DependencyProperty.RegisterAttached(
            "OnDrop",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.DropEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnDropProperty(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnDropProperty);
        }

        public static void SetOnDropProperty(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnDropProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewMouseDownProperty = DependencyProperty.RegisterAttached(
            "OnPreviewMouseDown", 
            typeof(ICommand), 
            typeof(InputBehaviours), 
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewMouseDownEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewMouseDown(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewMouseDownProperty);
        }

        public static void SetOnPreviewMouseDown(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewMouseDownProperty, cmd);
        }

        public static readonly DependencyProperty OnDragLeaveProperty = DependencyProperty.RegisterAttached(
            "OnDragLeave",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.DragLeaveEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnDragLeave(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnDragLeaveProperty);
        }

        public static void SetOnDragLeave(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnDragLeaveProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewMouseUpProperty = DependencyProperty.RegisterAttached(
            "OnPreviewMouseUp",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewMouseUpEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewMouseUp(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewMouseUpProperty);
        }

        public static void SetOnPreviewMouseUp(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewMouseUpProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewMouseMoveProperty = DependencyProperty.RegisterAttached(
            "OnPreviewMouseMove",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewMouseMoveEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewMouseMove(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewMouseMoveProperty);
        }

        public static void SetOnPreviewMouseMove(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewMouseMoveProperty, cmd);
        }

        public static readonly DependencyProperty OnPreviewMouseWheelProperty = DependencyProperty.RegisterAttached(
            "OnPreviewMouseWheel",
            typeof(ICommand),
            typeof(InputBehaviours),
            new FrameworkPropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.PreviewMouseWheelEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnPreviewMouseWheel(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnPreviewMouseWheelProperty);
        }

        public static void SetOnPreviewMouseWheel(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnPreviewMouseWheelProperty, cmd);
        }

        public static readonly DependencyProperty UpdateSourcePropertyOnEnterPressProperty = DependencyProperty.RegisterAttached(
            "UpdateSourcePropertyOnEnterPress",
            typeof(DependencyProperty),
            typeof(InputBehaviours),
            new PropertyMetadata(new OnKeyPressBindingUpdater(Key.Enter).OnBindingTargetAssignedToProperty));

        public static DependencyProperty GetUpdateSourcePropertyOnEnterPress(DependencyObject lmnt)
        {
            return (DependencyProperty)lmnt.GetValue(UpdateSourcePropertyOnEnterPressProperty);
        }

        public static void SetUpdateSourcePropertyOnEnterPress(DependencyObject lmnt, DependencyProperty dp)
        {
            lmnt.SetValue(UpdateSourcePropertyOnEnterPressProperty, dp);
        }

        public static readonly DependencyProperty OnLostFocusProperty = DependencyProperty.RegisterAttached(
            "OnLostFocus",
            typeof(ICommand),
            typeof(InputBehaviours),
            new PropertyMetadata(new OnRoutedEventCmdExecutor(UIElement.LostKeyboardFocusEvent).OnCmdAssignedToProperty));

        public static ICommand GetOnLostFocus(DependencyObject obj)
        {
            return (ICommand)obj.GetValue(OnLostFocusProperty);
        }

        public static void SetOnLostFocus(DependencyObject obj, ICommand cmd)
        {
            obj.SetValue(OnLostFocusProperty, cmd);
        }
    }
}
