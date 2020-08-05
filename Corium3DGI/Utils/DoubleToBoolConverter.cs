using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace Corium3DGI.Utils
{
    public class DoubleToBoolConverter : IValueConverter
    {
        private const double DELTA = 1E-6;

        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (System.Math.Abs((double)value) < DELTA)
                return false;
            else
                return true;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if ((bool)value)
                return 1.0;
            else
                return 0.0;
        }
    }
}
