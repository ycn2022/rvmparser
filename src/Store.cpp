#include <algorithm>
#include <cassert>
#include <cstring>
#include "Store.h"
#include "StoreVisitor.h"
#include "LinAlgOps.h"



namespace {

  template<typename T>
  void insert(ListHeader<T>& list, T* item)
  {
    if (list.first == nullptr) {
      list.first = list.last = item;
    }
    else {
      list.last->next = item;
      list.last = item;
    }
  }

}


Store::Store()
{
  roots.clear();
  debugLines.clear();
  connections.clear();
  setErrorString("");

  geometryBinaryReuse = (char*)arenaGeometryBinary.alloc(geometryBinaryLength);
}

Color* Store::newColor(Node* parent)
{
  assert(parent != nullptr);
  assert(parent->kind == Node::Kind::Model);
  auto* color = arena.alloc<Color>();
  color->next = nullptr;
  insert(parent->model.colors, color);
  return color;
}

void Store::setErrorString(const char* str)
{
  auto l = strlen(str);
  error_str = strings.intern(str, str + l);
}

Node* Store::findRootGroup(const char* name)
{
  for (auto * file = roots.first; file != nullptr; file = file->next) {
    assert(file->kind == Node::Kind::File);
    for (auto * model = file->children.first; model != nullptr; model = model->next) {
      //fprintf(stderr, "model '%s'\n", model->model.name);
      assert(model->kind == Node::Kind::Model);
      for (auto * group = model->children.first; group != nullptr; group = group->next) {
        //fprintf(stderr, "group '%s' %p\n", group->group.name, (void*)group->group.name);
        if (group->group.name == name) return group;
      }
    }
  }
  return nullptr;
}


void Store::addDebugLine(float* a, float* b, uint32_t color)
{
  auto line = arena.alloc<DebugLine>();
  for (unsigned k = 0; k < 3; k++) {
    line->a[k] = a[k];
    line->b[k] = b[k];
  }
  line->color = color;
  insert(debugLines, line);
}


Connection* Store::newConnection()
{
  auto * connection = arena.alloc<Connection>();
  insert(connections, connection);
  return connection;
}

Geometry* Store::newGeometry(Node* parent)
{
  assert(parent != nullptr);
  assert(parent->kind == Node::Kind::Group);

  auto * geo =  arena.alloc<Geometry>();
  geo->next = nullptr;
  geo->id = numGeometriesAllocated++;

  insert(parent->group.geometries, geo);
  return geo;
}

Geometry* Store::cloneGeometry(Node* parent, const Geometry* src)
{
  auto * dst = newGeometry(parent);
  dst->kind = src->kind;
  dst->M_3x4 = src->M_3x4;
  dst->bboxLocal = src->bboxLocal;
  dst->bboxWorld = src->bboxWorld;
  dst->id = src->id;
  dst->sampleStartAngle = src->sampleStartAngle;
  switch (dst->kind) {
    case Geometry::Kind::Pyramid:
    case Geometry::Kind::Box:
    case Geometry::Kind::RectangularTorus:
    case Geometry::Kind::CircularTorus:
    case Geometry::Kind::EllipticalDish:
    case Geometry::Kind::SphericalDish:
    case Geometry::Kind::Snout:
    case Geometry::Kind::Cylinder:
    case Geometry::Kind::Sphere:
    case Geometry::Kind::Line:
      std::memcpy(&dst->snout, &src->snout, sizeof(src->snout));
      break;
    case Geometry::Kind::FacetGroup:
      dst->facetGroup.polygons_n = src->facetGroup.polygons_n;
      dst->facetGroup.polygons = (Polygon*)arena.alloc(sizeof(Polygon)*dst->facetGroup.polygons_n);
      for (unsigned k = 0; k < dst->facetGroup.polygons_n; k++) {
        auto & dst_poly = dst->facetGroup.polygons[k];
        const auto & src_poly = src->facetGroup.polygons[k];
        dst_poly.contours_n = src_poly.contours_n;
        dst_poly.contours = (Contour*)arena.alloc(sizeof(Contour)*dst_poly.contours_n);
        for (unsigned i = 0; i < dst_poly.contours_n; i++) {
          auto & dst_cont = dst_poly.contours[i];
          const auto & src_cont = src_poly.contours[i];
          dst_cont.vertices_n = src_cont.vertices_n;
          dst_cont.vertices = (float*)arena.dup(src_cont.vertices, 3 * sizeof(float)*dst_cont.vertices_n);
          dst_cont.normals = (float*)arena.dup(src_cont.normals, 3 * sizeof(float)*dst_cont.vertices_n);
        }
      }
      break;
    default:
      assert(false && "Geometry has invalid kind.");
      break;
  }
  
  if (src->colorName) {
    dst->colorName = strings.intern(src->colorName);
  }
  dst->color = src->color;

  if (src->triangulation) {
    dst->triangulation = arena.alloc<Triangulation>();

    const auto * stri = src->triangulation;
    auto * dtri = dst->triangulation;
    dtri->error = stri->error;
    dtri->id = stri->id;
    if (stri->vertices_n) {
      dtri->vertices_n = stri->vertices_n;
      dtri->vertices = (float*)arena.dup(stri->vertices, 3 * sizeof(float) * dtri->vertices_n);
      dtri->normals = (float*)arena.dup(stri->normals, 3 * sizeof(float) * dtri->vertices_n);
      dtri->texCoords = (float*)arena.dup(stri->texCoords, 2 * sizeof(float) * dtri->vertices_n);
    }
    if (stri->triangles_n) {
      dtri->triangles_n = stri->triangles_n;
      dtri->indices = (uint32_t*)arena.dup(stri->indices, 3 * sizeof(uint32_t) * dtri->triangles_n);
    }
  }

  return dst;
}

bool Store::serializeGeometry(const Geometry* geo, char*& binary, size_t& geosize)
{
    auto scale = getScale(geo->M_3x4);

    //int TriangleCount = geo->triangulation ? geo->triangulation->triangles_n : -1;

    //ͳһ����100
    int rvmkind = (int)geo->kind + 100;

    switch (geo->kind) {

    case Geometry::Kind::Pyramid:
    {
        geosize = sizeof(int) + sizeof(geo->pyramid) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);

        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->pyramid, sizeof(geo->pyramid));
        //pos += sizeof(geo->pyramid);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));



    }
    break;
    case Geometry::Kind::Box:
    {

        geosize = sizeof(int) + sizeof(Geometry::box) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);

        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->box, sizeof(Geometry::box));
        //pos += sizeof(Geometry::box);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));

    }
    break;
    case Geometry::Kind::RectangularTorus:
    {

        geosize = sizeof(int) + sizeof(geo->rectangularTorus) + sizeof(scale) +
            sizeof(geo->sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);

        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->rectangularTorus, sizeof(geo->rectangularTorus));
        pos += sizeof(geo->rectangularTorus);
        memcpy(geometryBinaryReuse + pos, &scale, sizeof(scale));
        pos += sizeof(scale);
        memcpy(geometryBinaryReuse + pos, &geo->sampleStartAngle, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));



    }
    break;
    case Geometry::Kind::Sphere:
    {

        geosize = sizeof(int) + sizeof(geo->sphere) + sizeof(scale) +
            sizeof(geo->sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);


        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->sphere, sizeof(geo->sphere));
        pos += sizeof(geo->sphere);
        memcpy(geometryBinaryReuse + pos, &scale, sizeof(scale));
        pos += sizeof(scale);
        memcpy(geometryBinaryReuse + pos, &geo->sampleStartAngle, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));


    }
    break;
    case Geometry::Kind::Line:
        break;
    case Geometry::Kind::FacetGroup:
    {
        geosize = sizeof(int) + sizeof(geo->facetGroup.polygons_n);
        for (size_t i = 0; i < geo->facetGroup.polygons_n; i++)
        {
            auto& polygon = geo->facetGroup.polygons[i];

            geosize += sizeof(polygon.contours_n);

            for (size_t j = 0; j < polygon.contours_n; j++)
            {
                auto& contour = polygon.contours[j];

                geosize += sizeof(contour.vertices_n) + sizeof(float) * contour.vertices_n * 6;

            }
        }
        //geosize += sizeof(TriangleCount);

        ResizeGeometryBinaryReuse(geosize);

        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->facetGroup.polygons_n, sizeof(geo->facetGroup.polygons_n));
        pos += sizeof(geo->facetGroup.polygons_n);

        for (size_t i = 0; i < geo->facetGroup.polygons_n; i++)
        {
            auto& polygon = geo->facetGroup.polygons[i];

            memcpy(geometryBinaryReuse + pos, &polygon.contours_n, sizeof(polygon.contours_n));
            pos += sizeof(polygon.contours_n);

            for (size_t j = 0; j < polygon.contours_n; j++)
            {
                auto& contour = polygon.contours[j];

                memcpy(geometryBinaryReuse + pos, &contour.vertices_n, sizeof(contour.vertices_n));
                pos += sizeof(contour.vertices_n);

                memcpy(geometryBinaryReuse + pos, contour.vertices, sizeof(float) * contour.vertices_n * 3);
                pos += sizeof(float) * contour.vertices_n * 3;
                memcpy(geometryBinaryReuse + pos, contour.normals, sizeof(float) * contour.vertices_n * 3);
                pos += sizeof(float) * contour.vertices_n * 3;

                //for (size_t k = 0; k < contour.vertices_n; k++)
                //{
                //    memcpy(geometryBinaryReuse + pos, contour.vertices, sizeof(float) * contour.vertices_n * 3);
                //    pos += sizeof(float) * contour.vertices_n * 3;
                //    memcpy(geometryBinaryReuse + pos, contour.normals, sizeof(float) * contour.vertices_n * 3);
                //    pos += sizeof(float) * contour.vertices_n * 3;
                //}

            }
        }

        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));

    }
    break;

    case Geometry::Kind::Snout:
    {

        geosize = sizeof(int) + sizeof(geo->snout) + sizeof(scale) +
            sizeof(geo->sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);


        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->snout, sizeof(geo->snout));
        pos += sizeof(geo->snout);
        memcpy(geometryBinaryReuse + pos, &scale, sizeof(scale));
        pos += sizeof(scale);
        memcpy(geometryBinaryReuse + pos, &geo->sampleStartAngle, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));



    }
    break;
    case Geometry::Kind::EllipticalDish:
    {

        geosize = sizeof(int) + sizeof(geo->ellipticalDish) + sizeof(scale) +
            sizeof(geo->sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);


        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->ellipticalDish, sizeof(geo->ellipticalDish));
        pos += sizeof(geo->ellipticalDish);
        memcpy(geometryBinaryReuse + pos, &scale, sizeof(scale));
        pos += sizeof(scale);
        memcpy(geometryBinaryReuse + pos, &geo->sampleStartAngle, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));


    }
    break;
    case Geometry::Kind::SphericalDish:
    {

        geosize = sizeof(int) + sizeof(geo->sphericalDish) + sizeof(scale) +
            sizeof(geo->sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);

        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->sphericalDish, sizeof(geo->sphericalDish));
        pos += sizeof(geo->sphericalDish);
        memcpy(geometryBinaryReuse + pos, &scale, sizeof(scale));
        pos += sizeof(scale);
        memcpy(geometryBinaryReuse + pos, &geo->sampleStartAngle, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));

    }
    break;
    case Geometry::Kind::Cylinder:
    {
        geosize = sizeof(int) + sizeof(geo->cylinder) + sizeof(scale) +
            sizeof(geo->sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);

        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->cylinder, sizeof(geo->cylinder));
        pos += sizeof(geo->cylinder);
        memcpy(geometryBinaryReuse + pos, &scale, sizeof(scale));
        pos += sizeof(scale);
        memcpy(geometryBinaryReuse + pos, &geo->sampleStartAngle, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));

    }
    break;

    case Geometry::Kind::CircularTorus:
    {
        geosize = sizeof(int) + sizeof(geo->circularTorus) + sizeof(scale) +
            sizeof(geo->sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        ResizeGeometryBinaryReuse(geosize);

        size_t pos = 0;
        memcpy(geometryBinaryReuse, &rvmkind, sizeof(int));
        pos += sizeof(int);
        memcpy(geometryBinaryReuse + pos, &geo->circularTorus, sizeof(geo->circularTorus));
        pos += sizeof(geo->circularTorus);
        memcpy(geometryBinaryReuse + pos, &scale, sizeof(scale));
        pos += sizeof(scale);
        memcpy(geometryBinaryReuse + pos, &geo->sampleStartAngle, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(geometryBinaryReuse + pos, &TriangleCount, sizeof(TriangleCount));


    }
    break;

    default:
        assert(false && "Illegal kind");
        break;
    }

    if (geosize > 0)
    {
        binary = geometryBinaryReuse;
        return true;
    }

    return false;
}

bool Store::deserializeGeometry(const char* buffer, const size_t& bufsize, Geometry* geo, float& scale)
{
    Geometry::Kind geometrytype = Geometry::Kind::Line;

    if (bufsize < sizeof(int))
        return false;

    size_t pos = 0;

    int geosize = 0;

    //int TriangleCount = 0;

    //float scale = 1.f;

    float sampleStartAngle = 0.f;

    int rvmkind = 0;

    memcpy(&rvmkind, buffer, sizeof(int));

    //-100 ��ԭ
    rvmkind = rvmkind - 100;

    if (rvmkind >= 0 && rvmkind <= 10)
    {
        geometrytype = (Geometry::Kind)rvmkind;
    }
    else
    {
        return false;
    }

    pos += sizeof(geometrytype);

    switch (geometrytype)
    {
    case Geometry::Kind::Box:
    {
        geosize = sizeof(int) + sizeof(Geometry::box) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();

        geo->kind = Geometry::Kind::Box;
        memcpy(&geo->box, buffer + pos, sizeof(Geometry::box));
        //pos += sizeof(Geometry::box);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    case Geometry::Kind::CircularTorus:
    {
        geosize = sizeof(int) + sizeof(Geometry::circularTorus) + sizeof(scale) +
            sizeof(sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();

        geo->kind = Geometry::Kind::CircularTorus;
        memcpy(&geo->circularTorus, buffer + pos, sizeof(geo->circularTorus));
        pos += sizeof(geo->circularTorus);
        memcpy(&scale, buffer + pos, sizeof(scale));
        pos += sizeof(scale);
        memcpy(&geo->sampleStartAngle, buffer + pos, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    case Geometry::Kind::Cylinder:
    {
        geosize = sizeof(int) + sizeof(Geometry::cylinder) + sizeof(scale) +
            sizeof(sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();
        geo->kind = Geometry::Kind::Cylinder;
        memcpy(&geo->cylinder, buffer + pos, sizeof(geo->cylinder));
        pos += sizeof(geo->cylinder);
        memcpy(&scale, buffer + pos, sizeof(scale));
        pos += sizeof(scale);
        memcpy(&geo->sampleStartAngle, buffer + pos, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;

    }
    break;
    case Geometry::Kind::EllipticalDish:
    {
        geosize = sizeof(int) + sizeof(Geometry::ellipticalDish) + sizeof(scale) +
            sizeof(sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();
        geo->kind = Geometry::Kind::EllipticalDish;
        memcpy(&geo->ellipticalDish, buffer + pos, sizeof(geo->ellipticalDish));
        pos += sizeof(geo->ellipticalDish);
        memcpy(&scale, buffer + pos, sizeof(scale));
        pos += sizeof(scale);
        memcpy(&geo->sampleStartAngle, buffer + pos, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    case Geometry::Kind::FacetGroup:
    {
        geosize = sizeof(int) + sizeof(uint32_t);

        if (bufsize < geosize)
            return false;

        //Geometry* geo = new Geometry();

        memcpy(&geo->facetGroup.polygons_n, buffer + pos, sizeof(uint32_t));
        pos += sizeof(uint32_t);

        geo->facetGroup.polygons = new Polygon[geo->facetGroup.polygons_n];

        for (size_t i = 0; i < geo->facetGroup.polygons_n; i++)
        {
            geosize += sizeof(uint32_t);

            if (bufsize < geosize)
            {
                DeleteFacetGroup(geo);
                return false;
            }

            auto& polygon = geo->facetGroup.polygons[i];

            memcpy(&polygon.contours_n, buffer + pos, sizeof(uint32_t));
            pos += sizeof(uint32_t);

            polygon.contours = new Contour[polygon.contours_n];

            for (size_t j = 0; j < polygon.contours_n; j++)
            {
                geosize += sizeof(uint32_t);

                if (bufsize < geosize)
                {
                    DeleteFacetGroup(geo);
                    return false;
                }

                auto& contour = polygon.contours[j];

                memcpy(&contour.vertices_n, buffer + pos, sizeof(uint32_t));
                pos += sizeof(uint32_t);

                geosize += sizeof(float) * contour.vertices_n * 6;

                if (bufsize < geosize)
                {
                    DeleteFacetGroup(geo);
                    return false;
                }

                contour.vertices = new float[contour.vertices_n * 3];
                contour.normals = new float[contour.vertices_n * 3];

                memcpy(contour.vertices, buffer + pos, sizeof(float) * contour.vertices_n * 3);
                pos += sizeof(float) * contour.vertices_n * 3;
                memcpy(contour.normals, buffer + pos, sizeof(float) * contour.vertices_n * 3);
                pos += sizeof(float) * contour.vertices_n * 3;

                //for (size_t k = 0; k < contour.vertices_n; k++)
                //{
                //    geosize += sizeof(float) * 6;

                //    if (bufsize < geosize)
                //    {
                //        DeleteFacetGroup(geo);
                //        return nullptr;
                //    }


                //}
            }

        }
        //geosize += sizeof(TriangleCount);
        //if (bufsize < geosize)
        //{
        //    DeleteFacetGroup(geo);
        //    return false;
        //}
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        geo->kind = Geometry::Kind::FacetGroup;

        return true;

    }
    break;
    case Geometry::Kind::Pyramid:
    {
        geosize = sizeof(int) + sizeof(Geometry::pyramid) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();
        geo->kind = Geometry::Kind::Pyramid;
        memcpy(&geo->pyramid, buffer + pos, sizeof(geo->pyramid));
        //pos += sizeof(geo->pyramid);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    case Geometry::Kind::RectangularTorus:
    {
        geosize = sizeof(int) + sizeof(Geometry::rectangularTorus) + sizeof(scale) +
            sizeof(sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();
        geo->kind = Geometry::Kind::RectangularTorus;
        memcpy(&geo->rectangularTorus, buffer + pos, sizeof(geo->rectangularTorus));
        pos += sizeof(geo->rectangularTorus);
        memcpy(&scale, buffer + pos, sizeof(scale));
        pos += sizeof(scale);
        memcpy(&geo->sampleStartAngle, buffer + pos, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    case Geometry::Kind::Snout:
    {
        geosize = sizeof(int) + sizeof(Geometry::snout) + sizeof(scale) +
            sizeof(sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();
        geo->kind = Geometry::Kind::Snout;
        memcpy(&geo->snout, buffer + pos, sizeof(geo->snout));
        pos += sizeof(geo->snout);
        memcpy(&scale, buffer + pos, sizeof(scale));
        pos += sizeof(scale);
        memcpy(&geo->sampleStartAngle, buffer + pos, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    case Geometry::Kind::SphericalDish:
    {
        geosize = sizeof(int) + sizeof(Geometry::sphericalDish) + sizeof(scale) +
            sizeof(sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();
        geo->kind = Geometry::Kind::SphericalDish;
        memcpy(&geo->sphericalDish, buffer + pos, sizeof(geo->sphericalDish));
        pos += sizeof(geo->sphericalDish);
        memcpy(&scale, buffer + pos, sizeof(scale));
        pos += sizeof(scale);
        memcpy(&geo->sampleStartAngle, buffer + pos, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    case Geometry::Kind::Sphere:
    {
        geosize = sizeof(int) + sizeof(Geometry::sphere) + sizeof(scale) +
            sizeof(sampleStartAngle) /*+ sizeof(TriangleCount)*/;

        if (geosize != bufsize)
            return false;

        //Geometry* geo = new Geometry();
        geo->kind = Geometry::Kind::Sphere;
        memcpy(&geo->sphere, buffer + pos, sizeof(geo->sphere));
        pos += sizeof(geo->sphere);
        memcpy(&scale, buffer + pos, sizeof(scale));
        pos += sizeof(scale);
        memcpy(&geo->sampleStartAngle, buffer + pos, sizeof(geo->sampleStartAngle));
        //pos += sizeof(geo->sampleStartAngle);
        //memcpy(&TriangleCount, buffer + pos, sizeof(TriangleCount));

        return true;
    }
    break;
    default:
        break;
    }

    return false;
}



Node* Store::newNode(Node* parent, Node::Kind kind)
{
  auto grp = arena.alloc<Node>();
  std::memset(grp, 0, sizeof(Node));

  if (parent == nullptr) {
    insert(roots, grp);
  }
  else {
    insert(parent->children, grp);
  }

  grp->kind = kind;
  numGroupsAllocated++;
  return grp;
}

Attribute* Store::getAttribute(Node* group, const char* key)
{
  for (auto * attribute = group->attributes.first; attribute != nullptr; attribute = attribute->next) {
    if (attribute->key == key) return attribute;
  }
  return nullptr;
}

Attribute* Store::newAttribute(Node* group, const char* key)
{
  auto * attribute = arena.alloc<Attribute>();
  attribute->key = key;
  insert(group->attributes, attribute);
  return attribute;
}


Node* Store::getDefaultModel()
{
  auto * file = roots.first;
  if (file == nullptr) {
    file = newNode(nullptr, Node::Kind::File);
    file->file.info = strings.intern("");
    file->file.note = strings.intern("");
    file->file.date = strings.intern("");
    file->file.user = strings.intern("");
    file->file.encoding = strings.intern("");
  }
  auto * model = file->children.first;
  if (model == nullptr) {
    model = newNode(file, Node::Kind::Model);
    model->model.project = strings.intern("");
    model->model.name = strings.intern("");
  }
  return model;
}


Node* Store::cloneNode(Node* parent, const Node* src)
{
  auto * dst = newNode(parent, src->kind);
  switch (src->kind) {
  case Node::Kind::File:
    dst->file.info = strings.intern(src->file.info);
    dst->file.note = strings.intern(src->file.note);
    dst->file.date = strings.intern(src->file.date);
    dst->file.user = strings.intern(src->file.user);
    dst->file.encoding = strings.intern(src->file.encoding);
    break;
  case Node::Kind::Model:
    dst->model.project = strings.intern(src->model.project);
    dst->model.name = strings.intern(src->model.name);
    break;
  case Node::Kind::Group:
    dst->group.name = strings.intern(src->group.name);
    dst->group.bboxWorld = src->group.bboxWorld;
    dst->group.material = src->group.material;
    dst->group.id = src->group.id;
    for (unsigned k = 0; k < 3; k++) dst->group.translation[k] = src->group.translation[k];
    break;
  default:
    assert(false && "Group has invalid kind.");
    break;
  }

  for (auto * src_att = src->attributes.first; src_att != nullptr; src_att = src_att->next) {
    auto * dst_att = newAttribute(dst, strings.intern(src_att->key));
    dst_att->val = strings.intern(src_att->val);
  }

  return dst;
}


void Store::apply(StoreVisitor* visitor, Node* group)
{
  assert(group->kind == Node::Kind::Group);
  visitor->beginGroup(group);

  if (group->attributes.first) {
    visitor->beginAttributes(group);
    for (auto * a = group->attributes.first; a != nullptr; a = a->next) {
      visitor->attribute(a->key, a->val);
    }
    visitor->endAttributes(group);
  }

  if (group->kind == Node::Kind::Group && group->group.geometries.first != nullptr) {
    visitor->beginGeometries(group);
    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      visitor->geometry(geo);
    }
    visitor->endGeometries();
  }

  visitor->doneGroupContents(group);

  if (group->children.first != nullptr) {
    visitor->beginChildren(group);
    for (auto * g = group->children.first; g != nullptr; g = g->next) {
      apply(visitor, g);
    }
    visitor->endChildren();
  }

  visitor->EndGroup();
}

void Store::ResizeGeometryBinaryReuse(const int& RequestLen)
{
    if (geometryBinaryLength < RequestLen)
    {
        geometryBinaryLength = RequestLen;
        arenaGeometryBinary.clear();
        geometryBinaryReuse = nullptr;
    }

    if (geometryBinaryReuse == nullptr)
        geometryBinaryReuse = (char*)arenaGeometryBinary.alloc(geometryBinaryLength);
}

void Store::DeleteFacetGroup(Geometry* geo)
{
    if (geo == nullptr || geo->kind != Geometry::Kind::FacetGroup)
        return;

    if (geo->facetGroup.polygons)
    {
        for (size_t i = 0; i < geo->facetGroup.polygons_n; i++)
        {
            auto& polygon = geo->facetGroup.polygons[i];
            if (polygon.contours)
            {
                for (size_t j = 0; j < polygon.contours_n; j++)
                {
                    auto& contour = polygon.contours[j];
                    if (contour.vertices)
                    {
                        delete[] contour.vertices;
                        contour.vertices = nullptr;
                    }
                    if (contour.normals)
                    {
                        delete[] contour.normals;
                        contour.normals = nullptr;
                    }
                    contour.vertices_n = 0;
                }
                delete[] polygon.contours;
                polygon.contours = nullptr;
                polygon.contours_n = 0;
            }
        }

        delete[] geo->facetGroup.polygons;
        geo->facetGroup.polygons = nullptr;
        geo->facetGroup.polygons_n = 0;
    }
}

void Store::apply(StoreVisitor* visitor)
{
  visitor->init(*this);
  do {
    for (auto * file = roots.first; file != nullptr; file = file->next) {
      assert(file->kind == Node::Kind::File);
      visitor->beginFile(file);

      for (auto * model = file->children.first; model != nullptr; model = model->next) {
        assert(model->kind == Node::Kind::Model);
        visitor->beginModel(model);

        for (auto * group = model->children.first; group != nullptr; group = group->next) {
          apply(visitor, group);
        }
        visitor->endModel();
      }

      visitor->endFile();
    }
  } while (visitor->done() == false);
}

void Store::updateCountsRecurse(Node* group)
{

  for (auto * child = group->children.first; child != nullptr; child = child->next) {
    updateCountsRecurse(child);
  }

  numGroups++;
  if (group->children.first == nullptr) {
    numLeaves++;
  }

  if (group->kind == Node::Kind::Group) {
    if (group->children.first == nullptr && group->group.geometries.first == nullptr) {
      numEmptyLeaves++;
    }

    if (group->children.first != nullptr && group->group.geometries.first != nullptr) {
      numNonEmptyNonLeaves++;
    }

    for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
      numGeometries++;
    }
  }

}

void Store::updateCounts()
{
  numGroups = 0;
  numLeaves = 0;
  numEmptyLeaves = 0;
  numNonEmptyNonLeaves = 0;
  numGeometries = 0;
  for (auto * root = roots.first; root != nullptr; root = root->next) {
    updateCountsRecurse(root);
  }

}

namespace {

  void storeGroupIndexInGeometriesRecurse(Node* group)
  {
    for (auto * child = group->children.first; child != nullptr; child = child->next) {
      storeGroupIndexInGeometriesRecurse(child);
    }
    if (group->kind == Node::Kind::Group) {
      for (auto * geo = group->group.geometries.first; geo != nullptr; geo = geo->next) {
        geo->id = group->group.id;
        if (geo->triangulation) {
          geo->triangulation->id = group->group.id;
        }
      }
    }
  }
}


void Store::forwardGroupIdToGeometries()
{
  for (auto * root = roots.first; root != nullptr; root = root->next) {
    storeGroupIndexInGeometriesRecurse(root);
  }
}

