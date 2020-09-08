using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace Corium3DGI.Utils
{
    public class RadToDegConverter : IValueConverter
    {
        public object Convert(object val, Type targetType, object arg, System.Globalization.CultureInfo culture)
        {
            return (double)val / Math.PI * 180.0f;
        }

        public object ConvertBack(object val, Type targetType, object arg, System.Globalization.CultureInfo culture)
        {
            return (double)val / 180.0f * Math.PI;
        }
    }
}
