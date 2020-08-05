using System;
using System.ComponentModel;
using System.Reflection;
using System.Windows;
using System.Windows.Media.Media3D;

namespace Corium3DGI.Utils
{
    public class ObservablePoint3D : ObservableObject
    {
        private Point3D point3D;

        public double X { 
            get { return point3D.X; } 
            set
            {
                if (point3D.X != value)
                {
                    point3D.X = value;
                    OnPropertyChanged("X");
                }
            }
        }

        public double Y
        {
            get { return point3D.Y; }
            set
            {
                if (point3D.Y != value)
                {
                    point3D.Y = value;
                    OnPropertyChanged("Y");
                }
            }
        }

        public double Z
        {
            get { return point3D.Z; }
            set
            {
                if (point3D.Z != value)
                {
                    point3D.Z = value;
                    OnPropertyChanged("Z");
                }
            }
        }

        public Point3D Point3DCpy { get { return point3D; } }

        public ObservablePoint3D()
        {
            point3D = new Point3D();
        }

        public ObservablePoint3D(double x, double y, double z)
        {
            point3D = new Point3D(x, y, z);
        }

        public ObservablePoint3D(Point3D point3D)
        {
            this.point3D = point3D;
        }

        public ObservablePoint3D(ObservablePoint3D other)
        {            
            this.point3D = other.point3D;
        }
    }
}
