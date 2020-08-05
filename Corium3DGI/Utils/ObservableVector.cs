using System;
using System.ComponentModel;
using System.Windows;

namespace Corium3DGI.Utils
{
    public class ObservableVector : ObservableObject
    {
        private Vector vector;

        public double X
        {
            get { return vector.X; }
            set
            {
                if (vector.X != value)
                {
                    vector.X = value;
                    OnPropertyChanged("X");
                }
            }
        }

        public double Y
        {
            get { return vector.Y; }
            set
            {
                if (vector.Y != value)
                {
                    vector.Y = value;
                    OnPropertyChanged("Y");
                }
            }
        }

        public Vector VectorCpy { get { return vector; } }

        public ObservableVector()
        {
            vector = new Vector();
        }

        public ObservableVector(double x, double y)
        {
            vector = new Vector(x, y);
        }

        public ObservableVector(Vector vector)
        {
            this.vector = vector;
        }

        public ObservableVector(ObservableVector other)
        {
            this.vector = other.vector;
        }

        public void Normalize()
        {
            vector.Normalize();
            OnPropertyChanged("X");
            OnPropertyChanged("Y");            
        }
    }
}
