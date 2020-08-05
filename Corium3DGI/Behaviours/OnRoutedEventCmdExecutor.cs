using System.Windows;
using System.Windows.Input;

namespace Corium3DGI.Behaviours
{
    public class OnRoutedEventCmdExecutor
    { 
        private readonly RoutedEvent routedEvent;
        //private readonly object cmdDataContainer;

        private DependencyProperty propertyHoldingCmd;

        public OnRoutedEventCmdExecutor(RoutedEvent routedEvent) //, object cmdDataContainer
        {
            this.routedEvent = routedEvent;
            //this.cmdDataContainer = cmdDataContainer;
        }

        public void OnCmdAssignedToProperty(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            propertyHoldingCmd = e.Property;
            var lmnt = sender as UIElement;
            if (lmnt != null)
            {
                if (e.OldValue != null)
                    lmnt.RemoveHandler(routedEvent, new RoutedEventHandler(handleEvent));

                if (e.NewValue != null)
                    lmnt.AddHandler(routedEvent, new RoutedEventHandler(handleEvent));
            }
        }

        private void handleEvent(object sender, RoutedEventArgs e)
        {
            var obj = sender as DependencyObject;
            if (obj != null)
            {
                var cmd = obj.GetValue(propertyHoldingCmd) as ICommand;
                if (cmd != null && cmd.CanExecute(e))
                    cmd.Execute(e);    
            }
        }
    }
}
