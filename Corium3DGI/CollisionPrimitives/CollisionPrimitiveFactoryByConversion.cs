using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace Corium3DGI
{
    public class CollisionPrimitive3DFactoryByConversion : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            string valueName = ((CollisionPrimitive)value).ToString();
            return MainWindowVM.CollisionPrimitive3DPrimals.FirstOrDefault(p => p.ToString() == valueName);
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return ((CollisionPrimitive)value).clone();
        }
    }
    
    public class CollisionPrimitive2DFactoryByConversion : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            string valueName = ((CollisionPrimitive)value).ToString();
            return MainWindowVM.CollisionPrimitive2DPrimals.FirstOrDefault(p => p.ToString() == valueName);
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return ((CollisionPrimitive)value).clone();
        }
    }
}
