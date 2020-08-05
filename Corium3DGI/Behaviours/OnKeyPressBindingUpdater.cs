using System.Windows;
using System.Windows.Data;
using System.Windows.Input;

namespace Corium3DGI.Behaviours
{
    public class OnKeyPressBindingUpdater
    {
        private DependencyProperty property;
        private Key bindingUpdateKey;

        public OnKeyPressBindingUpdater(Key bindingUpdateKey)
        {
            this.bindingUpdateKey = bindingUpdateKey;
        }

        public void OnBindingTargetAssignedToProperty(DependencyObject obj, DependencyPropertyChangedEventArgs e)
        {
            property = e.Property;
            UIElement lmnt = obj as UIElement;

            if (lmnt != null)
            {
                if (e.OldValue != null)
                    lmnt.PreviewKeyDown -= OnKeyPressed;

                if (e.NewValue != null)
                    lmnt.PreviewKeyDown += OnKeyPressed;
            }
        }

        public void OnKeyPressed(object sender, KeyEventArgs e)
        {
            if (e.Key == bindingUpdateKey)
            {
                UIElement uiLmnt = e.Source as UIElement;
                if (uiLmnt == null)
                    return;

                DependencyProperty dp = uiLmnt.GetValue(property) as DependencyProperty;
                if (dp != null)
                {
                    BindingExpression bindingExpression = BindingOperations.GetBindingExpression(uiLmnt, dp);
                    if (bindingExpression != null)
                    {
                        bindingExpression.UpdateSource();
                        Keyboard.ClearFocus();
                    }
                }
            }
        }         
    }
}
