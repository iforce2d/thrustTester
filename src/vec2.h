#ifndef VEC2_H
#define VEC2_H

#include <math.h>
#include <cfloat>
#include <limits>

inline bool isValid(float x)
{
    if (x != x)
    {
        // NaN.
        return false;
    }

    float infinity = std::numeric_limits<float>::infinity();
    return -infinity < x && x < infinity;
}

struct vec2
{
    /// Default constructor does nothing (for performance).
    vec2() {}

    /// Construct using coordinates.
    vec2(float x, float y) : x(x), y(y) {}

    /// Set this vector to all zeros.
    void SetZero() { x = 0.0f; y = 0.0f; }

    /// Set this vector to some specified coordinates.
    void Set(float x_, float y_) { x = x_; y = y_; }

    /// Negate this vector.
    vec2 operator -() const { vec2 v; v.Set(-x, -y); return v; }

    /// Read from and indexed element.
    float operator () (int i) const
    {
        return (&x)[i];
    }

    /// Write to an indexed element.
    float& operator () (int i)
    {
        return (&x)[i];
    }

    /// Add a vector to this vector.
    void operator += (const vec2& v)
    {
        x += v.x; y += v.y;
    }

    /// Subtract a vector from this vector.
    void operator -= (const vec2& v)
    {
        x -= v.x; y -= v.y;
    }

    /// Multiply this vector by a scalar.
    void operator *= (float a)
    {
        x *= a; y *= a;
    }

    /// Get the length of this vector (the norm).
    float Length() const
    {
        return sqrt(x * x + y * y);
    }

    /// Get the length squared. For performance, use this instead of
    /// vec2::Length (if possible).
    float LengthSquared() const
    {
        return x * x + y * y;
    }

    /// Convert this vector into a unit vector. Returns the length.
    float Normalize()
    {
        float length = Length();
        if (length < FLT_EPSILON)
        {
            return 0.0f;
        }
        float invLength = 1.0f / length;
        x *= invLength;
        y *= invLength;

        return length;
    }

    /// Does this vector contain finite coordinates?
    bool IsValid() const
    {
        return isValid(x) && isValid(y);
    }

    /// Get the skew vector such that dot(skew_vec, other) == cross(vec, other)
    vec2 Skew() const
    {
        return vec2(-y, x);
    }

    float x, y;
};



/// Add two vectors component-wise.
inline vec2 operator + (const vec2& a, const vec2& b)
{
    return vec2(a.x + b.x, a.y + b.y);
}

/// Subtract two vectors component-wise.
inline vec2 operator - (const vec2& a, const vec2& b)
{
    return vec2(a.x - b.x, a.y - b.y);
}

inline vec2 operator * (float s, const vec2& a)
{
    return vec2(s * a.x, s * a.y);
}

inline bool operator == (const vec2& a, const vec2& b)
{
    return a.x == b.x && a.y == b.y;
}

#endif // VEC2_H
