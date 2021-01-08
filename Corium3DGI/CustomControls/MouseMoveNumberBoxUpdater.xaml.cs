using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;

namespace Corium3DGI.CustomCtrls
{
    /// <summary>
    /// Interaction logic for MouseMoveNumberBoxUpdater.xaml
    /// </summary>
    public partial class MouseMoveNumberBoxUpdater : UserControl
    {
        private BindingExpression txtBoxBinding;
        private double prevMouseXPos;
        private bool isUpdateViaMouseMoveActivated = false;

        public static readonly DependencyProperty UpdaterTextProperty = DependencyProperty.Register(
            "UpdaterText",
            typeof(string),
            typeof(MouseMoveNumberBoxUpdater));

        public string UpdaterText
        {
            get { return (string)GetValue(UpdaterTextProperty); }

            set { SetValue(UpdaterTextProperty, value); }
        }

        public static readonly DependencyProperty NumberProperty = DependencyProperty.Register(
            "NumberBoxText",
            typeof(double),
            typeof(MouseMoveNumberBoxUpdater),
            new FrameworkPropertyMetadata(0.0));

        public double NumberBoxText
        {
            get { return (double)GetValue(NumberProperty); }

            set { SetValue(NumberProperty, value); }
        }

        public static readonly DependencyProperty ValueIncrementPerMouseMoveProperty = DependencyProperty.Register(
            "ValueIncrementPerMouseMove",
            typeof(double),
            typeof(MouseMoveNumberBoxUpdater),
            new FrameworkPropertyMetadata(0.1));

        public double ValueIncrementPerMouseMove
        {
            get { return (double)GetValue(ValueIncrementPerMouseMoveProperty); }

            set { SetValue(ValueIncrementPerMouseMoveProperty, value); }
        }
        
        public MouseMoveNumberBoxUpdater() : base()
        {            
            InitializeComponent();
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();
            txtBoxBinding = numberBox.GetBindingExpression(TextBox.TextProperty);
        }

        public void OnLabelPressed(object sender, MouseButtonEventArgs e)
        {
            if (numberBox != null)
            {
                isUpdateViaMouseMoveActivated = true;
                prevMouseXPos = e.GetPosition(this).X;                                
                ((Label)sender).CaptureMouse();                
            }
        }

        public void OnPreviewMouseMove(object sender, MouseEventArgs e)
        {
            if (isUpdateViaMouseMoveActivated)
            {
                double mousePosX = e.GetPosition(this).X;
                double mouseMove = mousePosX - prevMouseXPos;

                double parsedDouble;
                if (double.TryParse(numberBox.Text, out parsedDouble))
                {
                    numberBox.Text = (parsedDouble + mouseMove * ValueIncrementPerMouseMove).ToString("F3");
                    txtBoxBinding?.UpdateSource();
                    Keyboard.Focus(numberBox);
                    numberBox.SelectAll();
                }

                prevMouseXPos = mousePosX;
            }
        }

        public void OnPreviewMouseUp(object sender, MouseButtonEventArgs e)
        {
            if (isUpdateViaMouseMoveActivated)
            {
                isUpdateViaMouseMoveActivated = false;
                ((Label)sender).ReleaseMouseCapture();                
            }
        }

        public void OnNumberBoxPressed(object sender, MouseButtonEventArgs e)
        {
            TextBox numberBox = (TextBox)sender;
            if (!numberBox.IsKeyboardFocusWithin && e.OriginalSource.GetType().Name == "TextBoxView")
            {
                e.Handled = true;
                numberBox.Focus();                
            }
        }

        /*
        public void OnPreviewDrop(object sender, DragEventArgs e) { }

        public void OnPreviewDragEnter(object sender, DragEventArgs e) { }

        public void OnPreviewDragLeave(object sender, DragEventArgs e) { }
        */
    }
}
