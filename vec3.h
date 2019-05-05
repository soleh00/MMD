#ifndef VEC3_H
#define VEC3_H
#include <iostream>

class Vec3
{
private:
    double X, Y, Z;
public:
    Vec3(double a = 0.0, double b = 0.0, double c = 0.0) : X(a), Y(b), Z(c) {}

    Vec3 operator + (Vec3 & a);
    Vec3 operator - (Vec3 & a);
    Vec3 operator / (Vec3 & a);
    Vec3 operator / (double a);
    Vec3 operator * (double a);
    Vec3 & operator += (const Vec3 & a);
    Vec3 & operator -= (const Vec3 & a);
    double getX();
    double getY();
    double getZ();
    void setX(double _X);
    void setY(double _Y);
    void setZ(double _Z);
    void set(double a, double b, double c);
    double abs();
    double abs2();
    void normalize();
    friend std::ostream & operator << (std::ostream &, const Vec3 &);
};



#endif // VEC3_H
