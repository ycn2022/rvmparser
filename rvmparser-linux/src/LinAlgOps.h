#pragma once
#include <cmath>
#include <cfloat>
#include "LinAlg.h"

inline Vec2f operator*(const float a, const Vec2f& b) { return makeVec2f(a*b.x, a*b.y); }

inline Vec2f operator-(const Vec2f& a, const Vec2f& b) { return makeVec2f(a.x - b.x, a.y - b.y); }

inline Vec2f operator+(const Vec2f& a, const Vec2f& b) { return makeVec2f(a.x + b.x, a.y + b.y); }

inline Vec3f cross(const Vec3f& a, const Vec3f& b)
{
  return makeVec3f(a.y * b.z - a.z * b.y,
                   a.z * b.x - a.x * b.z,
                   a.x * b.y - a.y * b.x);
}

inline float dot(const Vec3f& a, const Vec3f& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3f operator+(const Vec3f& a, const Vec3f& b) { return makeVec3f(a.x + b.x, a.y + b.y, a.z + b.z); }

inline Vec3f operator-(const Vec3f& a, const Vec3f& b) { return makeVec3f(a.x - b.x, a.y - b.y, a.z - b.z); }

inline Vec3f operator*(const float a, const Vec3f& b) { return makeVec3f(a*b.x, a*b.y, a*b.z); }

inline float lengthSquared(const Vec3f& a) { return dot(a, a); }

inline float length(const Vec3f& a) { return std::sqrt(dot(a, a)); }

inline float distanceSquared(const Vec3f& a, const Vec3f&b) { return lengthSquared(a - b); }

inline float distance(const Vec3f& a, const Vec3f&b) { return length(a - b); }

inline Vec3f normalize(const Vec3f& a) { return (1.f / length(a))*a; }

inline void write(float* dst, const Vec3f& a) { *dst++ = a.data[0]; *dst++ = a.data[1]; *dst++ = a.data[2]; }

inline Vec3f max(const Vec3f& a, const Vec3f& b)
{
  return makeVec3f(a.x > b.x ? a.x : b.x,
                   a.y > b.y ? a.y : b.y,
                   a.z > b.z ? a.z : b.z);
}

inline Vec3f min(const Vec3f& a, const Vec3f& b)
{
  return makeVec3f(a.x < b.x ? a.x : b.x,
                   a.y < b.y ? a.y : b.y,
                   a.z < b.z ? a.z : b.z);
}

inline Vec3d operator+(const Vec3d& a, const Vec3d& b) { return makeVec3d(a.x + b.x, a.y + b.y, a.z + b.z); }

inline Vec3d operator*(const double a, const Vec3d& b) { return makeVec3d(a * b.x, a * b.y, a * b.z); }


Mat3f inverse(const Mat3f& M);

Mat3f mul(const Mat3f& A, const Mat3f& B);

float getScale(const Mat3f& M);

inline float getScale(const Mat3x4f& M) { return getScale(makeMat3f(M.data)); }

inline Vec3f mul(const Mat3f& A, const Vec3f& x)
{
  Vec3f r;
  for (size_t k = 0; k < 3; k++) {
    r.data[k] = A.data[k] * x.data[0] + A.data[3 + k] * x.data[1] + A.data[6 + k] * x.data[2];
  }
  return r;
}


inline Vec3f mul(const Mat3x4f& A, const Vec3f& x)
{
  Vec3f r;
  for (size_t k = 0; k < 3; k++) {
    r.data[k] = A.data[k] * x.data[0] + A.data[3 + k] * x.data[1] + A.data[6 + k] * x.data[2] + A.data[9 + k];
  }
  return r;
}

inline Vec3d mul(const Mat3x4d& A, const Vec3d& x)
{
  Vec3d r;
  for (size_t k = 0; k < 3; k++) {
    r.data[k] = A.data[k] * x.data[0] + A.data[3 + k] * x.data[1] + A.data[6 + k] * x.data[2] + A.data[9 + k];
  }
  return r;
}

inline BBox3f createEmptyBBox3f()
{
    return makeBBox3f(makeVec3f(FLT_MAX, FLT_MAX, FLT_MAX), makeVec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX), false);
}

inline BBox3f makeBBox3f(const BBox3f& bbox, float margin)
{
  return makeBBox3f(bbox.min - makeVec3f(margin),
                    bbox.max + makeVec3f(margin));
}


inline void engulf(BBox3f& target, const Vec3f& p)
{
  target.min = min(target.min, p);
  target.max = max(target.max, p);
}

inline void engulf(BBox3f& target, const BBox3f& other)
{
  target.min = min(target.min, other.min);
  target.max = max(target.max, other.max);
}

BBox3f transform(const Mat3x4f& M, const BBox3f& bbox);

inline float diagonal(const BBox3f& b) { return distance(b.min, b.max); }

inline bool isEmpty(const BBox3f& b) { return b.max.x < b.min.x; }

inline bool isNotEmpty(const BBox3f& b) { return b.min.x <= b.max.x; }

inline float maxSideLength(const BBox3f& b)
{
  Vec3f l = b.max - b.min;
  float t = l.x > l.y ? l.x : l.y;
  return l.z > t ? l.z : t;
}

inline bool isStrictlyInside(const BBox3f& a, const BBox3f& b)
{
  bool lx = a.min.x <= b.min.x;
  bool ly = a.min.y <= b.min.y;
  bool lz = a.min.z <= b.min.z;
  bool ux = b.max.x <= a.max.x;
  bool uy = b.max.y <= a.max.y;
  bool uz = b.max.z <= a.max.z;
  return lx && ly && lz && ux && uy && uz;
}

inline bool isNotOverlapping(const BBox3f& a, const BBox3f& b)
{
  bool lx = b.max.x < a.min.x;
  bool ly = b.max.y < a.min.y;
  bool lz = b.max.z < a.min.z;
  bool ux = a.max.x < b.min.x;
  bool uy = a.max.y < b.min.y;
  bool uz = a.max.z < b.min.z;
  return lx || ly || lz || ux || uy || uz;
}

inline bool isOverlapping(const BBox3f& a, const BBox3f& b) { return !isNotOverlapping(a, b); }

