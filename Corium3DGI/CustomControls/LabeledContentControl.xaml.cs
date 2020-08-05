using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Corium3DGI.CustomCtrls
{
    /// <summary>
    /// Interaction logic for LabeledContentControl.xaml
    /// </summary>
    public partial class LabeledContentControl : UserControl
    {
        public static readonly DependencyProperty KeyProperty = DependencyProperty.Register(
            "Key",
            typeof(string),
            typeof(LabeledContentControl),
            new PropertyMetadata(default(string)));

        public string Key
        {
            get { return (string)GetValue(KeyProperty); }

            set { SetValue(KeyProperty, value); }
        }

        public static readonly DependencyProperty ContentProperty = DependencyProperty.Register(
            "Content",
            typeof(object),
            typeof(LabeledContentControl),
            new PropertyMetadata(default(object)));

        public object Content
        {
            get { return (object)GetValue(ContentProperty); }

            set { SetValue(ContentProperty, value); }
        }

        public static readonly DependencyProperty ContentTemplateProperty = DependencyProperty.Register(
            "ContentTemplate",
            typeof(DataTemplate),
            typeof(LabeledContentControl),
            new PropertyMetadata(default(object)));

        public DataTemplate ContentTemplate
        {
            get { return (DataTemplate)GetValue(ContentTemplateProperty); }

            set { SetValue(ContentTemplateProperty, value); }
        }        

        public LabeledContentControl()
        {
            InitializeComponent();
        }
    }
}
