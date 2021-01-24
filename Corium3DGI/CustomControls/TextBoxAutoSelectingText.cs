using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Corium3DGI.CustomCtrls
{
    public class TextBoxAutoSelectingText : TextBox
    {  
        protected override void OnMouseDown(MouseButtonEventArgs e)
        {
            bool wasPreviouslyFocused = IsFocused;
            base.OnMouseDown(e);

            if (!wasPreviouslyFocused)
                SelectAll();
        }
    }
}
