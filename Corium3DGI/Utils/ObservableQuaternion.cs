using System.ComponentModel;
using System.Security.Cryptography;
using System.Windows;
using System.Windows.Media.Media3D;

namespace Corium3DGI.Utils
{
    public class ObservableQuaternion : ObservableObject
    {
        private Quaternion quat;

        public double X
        {
            get { return quat.X; }

            set
            {
                if (quat.X != value)
                {
                    quat.X = value;
                    OnPropertyChanged("X");
                    SignalQuadFormChanged();
                }
            }
        }

        public double Y
        {
            get { return quat.Y; }

            set
            {
                if (quat.Y != value)
                {
                    quat.Y = value;
                    OnPropertyChanged("Y");
                    SignalQuadFormChanged();
                }
            }
        }

        public double Z
        {
            get { return quat.Z; }

            set
            {
                if (quat.Z != value)
                {
                    quat.Z = value;
                    OnPropertyChanged("Z");
                    SignalQuadFormChanged();
                }
            }
        }

        public double W
        {
            get { return quat.W; }

            set
            {
                if (quat.W != value)
                {
                    quat.W = value;
                    OnPropertyChanged("W");
                    SignalQuadFormChanged();
                }
            }
        }

        private double angle = 0;
        public double Angle
        {
            get { return angle; }

            set
            {
                if (angle != value)
                {
                    angle = value;
                    OnPropertyChanged("Angle");
                    SignalAxisAngleFormChanged();
                }
            }
        }

        private ObservableVector3D axis = new ObservableVector3D(0, 1, 0);
        public ObservableVector3D Axis
        {
            get { return axis; }

            set
            {
                if (axis != value)
                {
                    axis = new ObservableVector3D(value);
                    axis.PropertyChanged += OnAxisChanged;
                    OnPropertyChanged("Axis");
                    SignalAxisAngleFormChanged();
                }
            }
        }

        public Quaternion QuaternionCpy { get { return quat; } }

        public ObservableQuaternion()
        {
            quat = new Quaternion(axis.Vector3DCpy, angle);            
            axis.PropertyChanged += OnAxisChanged;
        }

        public ObservableQuaternion(double x, double y, double z, double w)
        {
            quat = new Quaternion(x, y, z, w);
            angle = quat.Angle;
            axis = new ObservableVector3D(quat.Axis);
            axis.PropertyChanged += OnAxisChanged;
        }

        public ObservableQuaternion(Vector3D axis, double angle)
        {
            this.angle = angle;
            this.axis = new ObservableVector3D(axis);
            this.axis.PropertyChanged += OnAxisChanged;
            quat = new Quaternion(axis, angle);
        }
        
        private void OnAxisChanged(object sender, PropertyChangedEventArgs e)
        {
            OnPropertyChanged("Axis");
            SignalAxisAngleFormChanged();
        }

        private void SignalQuadFormChanged()
        {
            angle = quat.Angle;            
            OnPropertyChanged("Angle");
            axis = new ObservableVector3D(quat.Axis);
            axis.PropertyChanged += OnAxisChanged;
            OnPropertyChanged("Axis");            
        }

        private void SignalAxisAngleFormChanged()
        {
            quat = new Quaternion(axis.Vector3DCpy, angle);
            OnPropertyChanged("X");
            OnPropertyChanged("Y");
            OnPropertyChanged("Z");
            OnPropertyChanged("W");
        }
    }
}
