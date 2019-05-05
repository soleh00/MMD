#include "vec3.h"
#include <cmath>

Vec3 Vec3::operator + (Vec3 & a)
{
    Vec3 tmp(0.0, 0.0, 0.0);
    tmp.X = X + a.X;
    tmp.Y = Y + a.Y;
    tmp.Z = Z + a.Z;
    return tmp;
}

Vec3 Vec3::operator - (Vec3 & a)
{
    Vec3 tmp(0.0, 0.0, 0.0);
    tmp.X = X - a.X;
    tmp.Y = Y - a.Y;
    tmp.Z = Z - a.Z;
    return tmp;
}

Vec3 Vec3::operator / (Vec3 & a)
{
    Vec3 tmp(0.0, 0.0, 0.0);
    tmp.X = X / a.X;
    tmp.Y = Y / a.Y;
    tmp.Z = Z / a.Z;
    return tmp;
}

Vec3 Vec3::operator / (double a)
{
    Vec3 tmp(0.0, 0.0, 0.0);
    tmp.X = X / a;
    tmp.Y = Y / a;
    tmp.Z = Z / a;
    return tmp;
}

Vec3 Vec3::operator * (double a)
{
    Vec3 tmp(0.0, 0.0, 0.0);
    tmp.X = X * a;
    tmp.Y = Y * a;
    tmp.Z = Z * a;
    return tmp;
}

Vec3 & Vec3::operator += (const Vec3 & a)
{
        X += a.X;
        Y += a.Y;
        Z += a.Z;
        return *this;
}

Vec3 & Vec3::operator -= (const Vec3 & a)
{
        X -= a.X;
        Y -= a.Y;
        Z -= a.Z;
        return *this;
}

double Vec3::getX()
{
    return X;
}

double Vec3::getY()
{
    return Y;
}

double Vec3::getZ()
{
    return Z;
}

void Vec3::setX(double _X)
{
    X = _X;
}

void Vec3::setY(double _Y)
{
    Y = _Y;
}

void Vec3::setZ(double _Z)
{
    Z = _Z;
}

void Vec3::set(double a, double b, double c)
{
    X = a;
    Y = b;
    Z = c;
}

double Vec3::abs()
{
    return sqrt(X*X + Y*Y + Z*Z);
}

double Vec3::abs2()
{
    return (X*X + Y*Y + Z*Z);
}

void Vec3::normalize()
{
    double magnitude = sqrt(X*X + Y*Y + Z*Z);
    X /= magnitude;
    Y /= magnitude;
    Z /= magnitude;
}

std::ostream & operator << (std::ostream & out, const Vec3 & a)
{
    out << a.X << " " << a.Y << " " << a.Z << " ";
    return  out;
}
