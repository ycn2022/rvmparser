#pragma once

struct Vec2f
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // Nonstandard extension of nameless struct/union
#endif
  union {
    struct {
      float x;
      float y;
    };
    float data[2];
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  float& operator[](size_t i) { return data[i]; }
  const float& operator[](size_t i) const { return data[i]; }
};

inline Vec2f makeVec2f(float x) { Vec2f r; r.x = x; r.y = x; return r; }
inline Vec2f makeVec2f(const float* ptr) { Vec2f r; r.x = ptr[0]; r.y = ptr[1]; return r; }
inline Vec2f makeVec2f(float x, float y) { Vec2f r; r.x = x; r.y = y; return r; }


struct Vec3f
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // Nonstandard extension of nameless struct/union
#endif
  union {
    struct {
      float x;
      float y;
      float z;
    };
    float data[3];
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  float& operator[](size_t i) { return data[i]; }
  const float& operator[](size_t i) const { return data[i]; }
};

inline Vec3f makeVec3f(const Vec3f& q) { Vec3f r; r.x = q.x; r.y = q.y; r.z = q.z; return r; }
inline Vec3f makeVec3f(float x) { Vec3f r; r.x = x; r.y = x; r.z = x; return r; }
inline Vec3f makeVec3f(const float* ptr) { Vec3f r; r.x = ptr[0]; r.y = ptr[1]; r.z = ptr[2]; return r; }
inline Vec3f makeVec3f(float x, float y, float z) { Vec3f r; r.x = x; r.y = y; r.z = z; return r; }
inline Vec3f makeVec3f(const Vec2f& a, float z) { Vec3f r; r.x = a.x; r.y = a.y; r.z = z; return r; }

struct Vec3d
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // Nonstandard extension of nameless struct/union
#endif
  union {
    struct {
      double x;
      double y;
      double z;
    };
    double data[3];
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  double& operator[](size_t i) { return data[i]; }
  const double& operator[](size_t i) const { return data[i]; }
};
inline Vec3d makeVec3d(const float* ptr) { Vec3d r; r.x = ptr[0]; r.y = ptr[1]; r.z = ptr[2]; return r; }
inline Vec3d makeVec3d(double x, double y, double z) { Vec3d r; r.x = x; r.y = y; r.z = z; return r; }
inline Vec3f makeVec3f(const Vec3d& q) { Vec3f r; r.x = static_cast<float>(q.x); r.y = static_cast<float>(q.y); r.z = static_cast<float>(q.z); return r; }


struct BBox3f
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // Nonstandard extension of nameless struct/union
#endif
  union {
    struct {
      Vec3f min;
      Vec3f max;
    };
    float data[6];
    bool isValid = false;
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

inline BBox3f makeBBox3f(const BBox3f& a) { BBox3f r; r.min = a.min; r.max = a.max; r.isValid = a.isValid; return r; }
inline BBox3f makeBBox3f(const Vec3f& min, const Vec3f& max, const bool& isvalid = true) { BBox3f r; r.min = min; r.max = max; r.isValid = isvalid; return r; }


struct Mat3f
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // Nonstandard extension of nameless struct/union
#endif
  union {
    struct {
      float m00;
      float m10;
      float m20;
      float m01;
      float m11;
      float m21;
      float m02;
      float m12;
      float m22;
    };
    Vec3f cols[3];
    float data[3 * 3];
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

inline Mat3f makeMat3f(const Mat3f& a) { Mat3f r; for (size_t i = 0; i < 3 * 3; i++) { r.data[i] = a.data[i]; } return r; }
inline Mat3f makeMat3f(const float* ptr) { Mat3f r; for (size_t i = 0; i < 3 * 3; i++) { r.data[i] = ptr[i]; } return r; }
inline Mat3f makeMat3f(float m00, float m01, float m02,
                       float m10, float m11, float m12,
                       float m20, float m21, float m22)
{
  Mat3f r;
  r.m00 = m00; r.m10 = m10; r.m20 = m20;
  r.m01 = m01; r.m11 = m11; r.m21 = m21;
  r.m02 = m02; r.m12 = m12; r.m22 = m22;
  return r;
}


struct Mat3x4f
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // Nonstandard extension of nameless struct/union
#endif
  union {
    struct {
      float m00;
      float m10;
      float m20;

      float m01;
      float m11;
      float m21;

      float m02;
      float m12;
      float m22;

      float m03;
      float m13;
      float m23;
    };
    Vec3f cols[4];
    float data[4 * 3];
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

inline Mat3x4f makeMat3x4f(const Mat3x4f& a) { Mat3x4f r; for (size_t i = 0; i < 4 * 3; i++) { r.data[i] = a.data[i]; } return r; }
inline Mat3x4f makeMat3x4f(const float* ptr) { Mat3x4f r; for (size_t i = 0; i < 4 * 3; i++) { r.data[i] = ptr[i]; } return r; }

struct Mat3x4d
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)  // Nonstandard extension of nameless struct/union
#endif
  union {
    struct {
      double m00;
      double m10;
      double m20;

      double m01;
      double m11;
      double m21;

      double m02;
      double m12;
      double m22;

      double m03;
      double m13;
      double m23;
    };
    Vec3d cols[4];
    double data[4 * 3];
  };
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
inline Mat3x4d makeMat3x4d(const float* ptr) { Mat3x4d r; for (size_t i = 0; i < 4 * 3; i++) { r.data[i] = ptr[i]; } return r; }
