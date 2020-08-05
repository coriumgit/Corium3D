using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace Corium3DGI.CustomCtrls
{
    public class ToggableIsEditableComboBox : ComboBox
    {
        public static readonly DependencyProperty OnTextBoxLostFocusProperty = DependencyProperty.Register(
            "OnTextBoxLostFocus",
            typeof(ICommand),
            typeof(ToggableIsEditableComboBox));

        public ICommand OnTextBoxLostFocus
        {
            get { return (ICommand)GetValue(OnTextBoxLostFocusProperty); }

            set { SetValue(OnTextBoxLostFocusProperty, value); }
        }

        public static readonly DependencyProperty DefaultTextBoxTextProperty = DependencyProperty.Register(
            "DefaultTextBoxText",
            typeof(string),
            typeof(ToggableIsEditableComboBox),
            new FrameworkPropertyMetadata(""));

        public string DefaultTextBoxText
        {
            get { return (string)GetValue(DefaultTextBoxTextProperty); }

            set { SetValue(DefaultTextBoxTextProperty, value); }
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            TextBox textBox = Template.FindName("PART_EditableTextBox", this) as TextBox;
            if (textBox != null)
            {                
                textBox.PreviewKeyDown += OnKeyPressed;
                Corium3DGI.Behaviours.InputBehaviours.SetOnLostFocus(textBox, OnTextBoxLostFocus);                
                textBox.Text = DefaultTextBoxText;
                textBox.Focus();
            }
        }

        public void OnKeyPressed(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                IsEditable = false;
                Keyboard.ClearFocus();
            }
        }
    }
}
