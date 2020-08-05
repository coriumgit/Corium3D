using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace Corium3DGI.Utils
{
    public class StringAttachment
    {
        public static readonly DependencyProperty AttachedStrProperty = DependencyProperty.RegisterAttached(
            "AttachedStr",
            typeof(string),
            typeof(StringAttachment),
            new FrameworkPropertyMetadata(null));

        public static string GetAttachedStr(DependencyObject obj)
        {
            return (string)obj.GetValue(AttachedStrProperty);
        }

        public static void SetAttachedStr(DependencyObject obj, string value)
        {
            obj.SetValue(AttachedStrProperty, value);
        }
    }
}
