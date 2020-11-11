using System;
using System.Windows.Media.Media3D;

namespace Corium3DGI.Utils
{
    public static class Extensions
    {
        private const double Deg2Rad = Math.PI / 180.0;
        private const double Rad2Deg = 1 / Deg2Rad;

        public static Vector3D toEuler(this Quaternion quat)
        {
            return new Vector3D(Math.Atan2(2 * (quat.W * quat.X + quat.Y * quat.Z), 1 - 2 * (quat.X * quat.X + quat.Y * quat.Y)),
                                Math.Asin(2 * (quat.W * quat.Y - quat.X * quat.Z)),
                                Math.Atan2(2 * (quat.W * quat.Z + quat.X * quat.Y), 1 - 2 * (quat.Y * quat.Y + quat.Z * quat.Z))) * Rad2Deg;
        }

        public static Quaternion asEulerToQuaternion(this Vector3D euler)
        {
            euler *= Deg2Rad;
            double cz = Math.Cos(0.5f * euler.Z);
            double sz = Math.Sin(0.5f * euler.Z);
            double cy = Math.Cos(0.5f * euler.Y);
            double sy = Math.Sin(0.5f * euler.Y);
            double cx = Math.Cos(0.5f * euler.X);
            double sx = Math.Sin(0.5f * euler.X);

            Quaternion quat = new Quaternion();
            quat.W = cx * cy * cz + sx * sy * sz;
            quat.X = sx * cy * cz - cx * sy * sz;
            quat.Y = cx * sy * cz + sx * cy * sz;
            quat.Z = cx * cy * sz - sx * sy * cz;

            return quat;
        }
    }
}
