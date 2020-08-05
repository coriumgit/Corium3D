using System;
using System.Windows;
using System.Windows.Media;

namespace Corium3DGI.Utils
{
    public class ObservablePoint : ObservableObject
    {
        private Point point;

        public double X
        {
            get { return point.X; }
            set
            {
                if (point.X != value)
                {
                    point.X = value;
                    OnPropertyChanged("X");
                }
            }
        }

        public double Y
        {
            get { return point.Y; }
            set
            {
                if (point.Y != value)
                {
                    point.Y = value;
                    OnPropertyChanged("Y");
                }
            }
        }

        public Point PointCpy { get { return point; } }

        public ObservablePoint()
        {
            point = new Point();
        }

        public ObservablePoint(double x, double y)
        {
            point = new Point(x, y);
        }

        public ObservablePoint(Point point)
        {
            this.point = point;
        }

        public ObservablePoint(ObservablePoint other)
        {
            this.point = other.point;
        }
    }    
}
