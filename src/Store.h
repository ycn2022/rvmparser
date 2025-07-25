#pragma once
#include <cstdint>
#include "Common.h"
#include "LinAlg.h"

struct Node;
struct Geometry;

struct Contour
{
  float* vertices;
  float* normals;
  uint32_t vertices_n;
};

struct Polygon
{
  Contour* contours;
  uint32_t contours_n;
};

struct Triangulation {
  float* vertices = nullptr;
  float* normals = nullptr;
  float* texCoords = nullptr;
  uint32_t* indices = 0;
  uint32_t vertices_n = 0;
  uint32_t triangles_n = 0;
  int32_t id = 0;
  float error = 0.f;
};

struct Color
{
  Color* next = nullptr;
  uint32_t colorKind;
  uint32_t colorIndex;
  uint8_t rgb[3];
};

struct Connection
{
  enum struct Flags : uint8_t {
    None = 0,
    HasCircularSide =     1<<0,
    HasRectangularSide =  1<<1
  };

  Connection* next = nullptr;
  Geometry* geo[2] = { nullptr, nullptr };
  unsigned offset[2];
  Vec3f p;
  Vec3f d;
  unsigned temp;
  Flags flags = Flags::None;

  void setFlag(Flags flag) { flags = (Flags)((uint8_t)flags | (uint8_t)flag); }
  bool hasFlag(Flags flag) { return (uint8_t)flags & (uint8_t)flag; }

};


struct Geometry
{
  enum struct Kind
  {
    Pyramid,
    Box,
    RectangularTorus,
    CircularTorus,
    EllipticalDish,
    SphericalDish,
    Snout,
    Cylinder,
    Sphere,
    Line,
    FacetGroup
  };
  enum struct Type
  {
    Primitive,
    Obstruction,
    Insulation
  };
  Geometry* next = nullptr;                 // Next geometry in the list of geometries in group.
  Triangulation* triangulation = nullptr;
  Geometry* next_comp = nullptr;            // Next geometry in list of geometries of this composite

  Connection* connections[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
  const char* colorName = nullptr;
  void * clientData = nullptr;
  uint32_t color = 0x202020u;

  Kind kind;
  Type type = Geometry::Type::Primitive;
  uint32_t transparency = 0;                 ///< Transparency of primitive, a percentage in [0,100].

  unsigned id;

  Mat3x4f M_3x4;
  BBox3f bboxLocal;
  BBox3f bboxWorld;
  float sampleStartAngle = 0.f;
  union {
    struct {
      float bottom[2];
      float top[2];
      float offset[2];
      float height;
    } pyramid;
    struct {
      float lengths[3];
    } box;
    struct {
      float inner_radius;
      float outer_radius;
      float height;
      float angle;
    } rectangularTorus;
    struct {
      float offset;
      float radius;
      float angle;
    } circularTorus;
    struct {
      float baseRadius;
      float height;
    } ellipticalDish;
    struct {
      float baseRadius;
      float height;
    } sphericalDish;
    struct {
      float offset[2];
      float bshear[2];
      float tshear[2];
      float radius_b;
      float radius_t;
      float height;
    } snout;
    struct {
      float radius;
      float height;
    } cylinder;
    struct {
      float diameter;
    } sphere;
    struct {
      float a, b;
    } line;
    struct {
      struct Polygon* polygons;
      uint32_t polygons_n;
    } facetGroup;
  };
};

template<typename T>
struct ListHeader
{
  T* first;
  T* last;

  void clear()
  {
    first = last = nullptr;
  }

  void insert(T* item)
  {
    if (first == nullptr) {
      first = last = item;
    }
    else {
      last->next = item;
      last = item;
    }
  }

  T* popFront()
  {
    T* rv = first;
    if (rv) {
      first = rv->next;
      rv->next = nullptr;
    }
    return rv;
  }

};

struct Attribute
{
  Attribute* next = nullptr;
  const char* key = nullptr;
  const char* val = nullptr;
};


struct Node
{
  Node() {}

  enum struct Kind
  {
    File,
    Model,
    Group
  };

  enum struct Flags
  {
    None = 0,
    ClientFlagStart = 1
  };

  Node* next = nullptr;
  ListHeader<Node> children;
  ListHeader<Attribute> attributes;

  Kind kind = Kind::Group;
  Flags flags = Flags::None;

  BBox3f bboxWorld;

  void setFlag(Flags flag) { flags = (Flags)((unsigned)flags | (unsigned)flag); }
  void unsetFlag(Flags flag) { flags = (Flags)((unsigned)flags & (~(unsigned)flag)); }
  bool hasFlag(Flags flag) const { return ((unsigned)flags & (unsigned)flag) != 0; }

  union {
    struct {
      const char* info;
      const char* note;
      const char* date;
      const char* user;
      const char* encoding;
      const char* path;
    } file;
    struct {
      ListHeader<Color> colors;
      const char* project;
      const char* name;
    } model;
    struct {
      ListHeader<Geometry> geometries;
      const char* name;
      BBox3f bboxWorld;
      uint32_t material;
      uint32_t transparency;
      int32_t id = 0;
      float translation[3];
      uint32_t clientTag;     // For use by passes to stuff temporary info
    } group;
  };

};


struct DebugLine
{
  DebugLine* next = nullptr;
  float a[3];
  float b[3];
  uint32_t color = 0xff0000u;
};


class StoreVisitor;

class Store
{
public:
  Store();

  Color* newColor(Node* parent);

  Geometry* newGeometry(Node* parent);

  Geometry* cloneGeometry(Node* parent, const Geometry* src);


  //binary生命周期 由Store管理，无需释放
  bool serializeGeometry(const Geometry* geo, char*& binary, size_t& geosize);

  //binary由调用者管理，Geometry*也由调用者释放
  static bool deserializeGeometry(const char* buffer, const size_t& bufsize, Geometry* geo, float& scale);


  Node* getDefaultModel();

  Node* newNode(Node * parent, Node::Kind kind);

  Node* cloneNode(Node* parent, const Node* src);

  Node* findRootGroup(const char* name);

  Attribute* getAttribute(Node* group, const char* key);

  Attribute* newAttribute(Node* group, const char* key);

  void addDebugLine(float* a, float* b, uint32_t color);

  Connection* newConnection();

  void apply(StoreVisitor* visitor);

  unsigned groupCount_() const { return numGroups; }
  unsigned groupCountAllocated() const { return numGroupsAllocated; }
  unsigned leafCount() const { return numLeaves; }
  unsigned emptyLeafCount() const { return numEmptyLeaves; }
  unsigned nonEmptyNonLeafCount() const { return numNonEmptyNonLeaves; }
  unsigned geometryCount_() const { return numGeometries; }
  unsigned geometryCountAllocated() const { return numGeometriesAllocated; }

  const char* errorString() const { return error_str; }
  void setErrorString(const char* str);

  Node* getFirstRoot() { return roots.first; }
  Connection* getFirstConnection() { return connections.first; }
  DebugLine* getFirstDebugLine() { return debugLines.first; }

  Arena arena;
  Arena arenaTriangulation;

  Arena arenaGeometryBinary;

  struct Stats* stats = nullptr;
  struct Connectivity* conn = nullptr;


  StringInterning strings;

  void updateCounts();

  void forwardGroupIdToGeometries();

private:
  unsigned numGroups = 0;
  unsigned numGroupsAllocated = 0;
  unsigned numLeaves = 0;
  unsigned numEmptyLeaves = 0;
  unsigned numNonEmptyNonLeaves = 0;
  unsigned numGeometries = 0;
  unsigned numGeometriesAllocated = 0;

  const char* error_str = nullptr;

  void updateCountsRecurse(Node* group);

  void apply(StoreVisitor* visitor, Node* group);

  ListHeader<Node> roots;
  ListHeader<DebugLine> debugLines;
  ListHeader<Connection> connections;

protected:
  char* geometryBinaryReuse = nullptr;
  int geometryBinaryLength = 16 * 1024 * 1024; //预申请16M内存，重复使用

  void ResizeGeometryBinaryReuse(const int& RequestLen);

public:
  static void DeleteFacetGroup(Geometry* geo);
  
};