using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Media.Media3D;

namespace Corium3DGI.Utils
{
    public class ObservableVector3D : ObservableObject
    {
        private Vector3D vector3D;

        public double X
        {
            get { return vector3D.X; }
            set
            {
                if (vector3D.X != value)
                {
                    vector3D.X = value;
                    OnPropertyChanged("X");
                }
            }
        }

        public double Y
        {
            get { return vector3D.Y; }
            set
            {
                if (vector3D.Y != value)
                {
                    vector3D.Y = value;
                    OnPropertyChanged("Y");
                }
            }
        }

        public double Z
        {
            get { return vector3D.Z; }
            set
            {
                if (vector3D.Z != value)
                {
                    vector3D.Z = value;
                    OnPropertyChanged("Z");
                }
            }
        }

        public Vector3D Vector3DCpy { get { return vector3D; } }

        public ObservableVector3D()
        {
            vector3D = new Vector3D();
        }

        public ObservableVector3D(double x, double y, double z)
        {
            vector3D = new Vector3D(x, y, z);
        }

        public ObservableVector3D(Vector3D vector3D)
        {
            this.vector3D = vector3D;
        }

        public ObservableVector3D(ObservableVector3D other)
        {
            this.vector3D = other.vector3D;
        }

        public void Normalize()
        {
            vector3D.Normalize();
            OnPropertyChanged("X");
            OnPropertyChanged("Y");
            OnPropertyChanged("Z");
        }
    }
}
