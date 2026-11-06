#include "ui/ParametersSpec.h"
#include "scene/Primitives.h"

namespace ui::params {

namespace {

using scene::BoxObject; using scene::ConeObject; using scene::SphereObject; using scene::GeoSphereObject;
using scene::CylinderObject; using scene::TubeObject; using scene::TorusObject; using scene::PyramidObject;
using scene::TeapotObject; using scene::PlaneObject;

constexpr double kBig = 1e6;

struct Builder {
    Spec spec;
    int y = 24;
    void Size(const wchar_t* label, int index, double min = 0) {
        spec.rows.push_back({label, index, Kind::Float, min, kBig, 0.1, y});
        y += 21;
    }
    void Angle(const wchar_t* label, int index) {
        spec.rows.push_back({label, index, Kind::Float, -kBig, kBig, 1.0, y});
        y += 21;
    }
    void Count(const wchar_t* label, int index, int min, int max) {
        spec.rows.push_back({label, index, Kind::Int, static_cast<double>(min), static_cast<double>(max), 1.0, y});
        y += 21;
    }
    void Gap() { y += 7; }
    void Check(const wchar_t* label, int index) {
        if (!spec.rows.empty() && spec.rows.back().kind != Kind::Check) y += 5;   // 136 -> 162 for Box
        spec.rows.push_back({label, index, Kind::Check, 0, 1, 1, y});
        y += 21;
    }
    Spec Done() {
        spec.contentHeight = y - 15;   // frame bottom = title + last row + 21; title is 16 rows
        return spec;
    }
};

} // namespace

Spec ParametersFor(int type) {
    Builder b;
    switch (type) {
    case 0:  // Box (measured)
        b.Size(L"Length:", BoxObject::kLength); b.Size(L"Width:", BoxObject::kWidth);
        b.Size(L"Height:", BoxObject::kHeight, -kBig);
        b.Gap();
        b.Count(L"Length Segs:", BoxObject::kLengthSegs, 1, 200); b.Count(L"Width Segs:", BoxObject::kWidthSegs, 1, 200);
        b.Count(L"Height Segs:", BoxObject::kHeightSegs, 1, 200);
        b.Check(L"Generate Mapping Coords.", -1);
        break;
    case 1:  // Cone
        b.Size(L"Radius 1:", ConeObject::kRadius1); b.Size(L"Radius 2:", ConeObject::kRadius2);
        b.Size(L"Height:", ConeObject::kHeight, -kBig);
        b.Gap();
        b.Count(L"Height Segments:", ConeObject::kHeightSegs, 1, 200); b.Count(L"Cap Segments:", ConeObject::kCapSegs, 1, 100);
        b.Count(L"Sides:", ConeObject::kSides, 3, 200);
        b.Check(L"Smooth", ConeObject::kSmooth);
        break;
    case 2:  // Sphere
        b.Size(L"Radius:", SphereObject::kRadius); b.Count(L"Segments:", SphereObject::kSegments, 4, 200);
        b.Check(L"Smooth", SphereObject::kSmooth); b.Check(L"Base To Pivot", SphereObject::kBaseToPivot);
        break;
    case 3:  // GeoSphere
        b.Size(L"Radius:", GeoSphereObject::kRadius); b.Count(L"Segments:", GeoSphereObject::kSegments, 1, 200);
        b.Check(L"Smooth", GeoSphereObject::kSmooth); b.Check(L"Base To Pivot", GeoSphereObject::kBaseToPivot);
        break;
    case 4:  // Cylinder
        b.Size(L"Radius:", CylinderObject::kRadius); b.Size(L"Height:", CylinderObject::kHeight, -kBig);
        b.Gap();
        b.Count(L"Height Segments:", CylinderObject::kHeightSegs, 1, 200); b.Count(L"Cap Segments:", CylinderObject::kCapSegs, 1, 100);
        b.Count(L"Sides:", CylinderObject::kSides, 3, 200);
        b.Check(L"Smooth", CylinderObject::kSmooth);
        break;
    case 5:  // Tube
        b.Size(L"Radius 1:", TubeObject::kRadius1); b.Size(L"Radius 2:", TubeObject::kRadius2);
        b.Size(L"Height:", TubeObject::kHeight, -kBig);
        b.Gap();
        b.Count(L"Height Segments:", TubeObject::kHeightSegs, 1, 200); b.Count(L"Cap Segments:", TubeObject::kCapSegs, 1, 100);
        b.Count(L"Sides:", TubeObject::kSides, 3, 200);
        b.Check(L"Smooth", TubeObject::kSmooth);
        break;
    case 6:  // Torus
        b.Size(L"Radius 1:", TorusObject::kRadius1); b.Size(L"Radius 2:", TorusObject::kRadius2);
        b.Angle(L"Rotation:", TorusObject::kRotation); b.Angle(L"Twist:", TorusObject::kTwist);
        b.Gap();
        b.Count(L"Segments:", TorusObject::kSegments, 3, 200); b.Count(L"Sides:", TorusObject::kSides, 3, 200);
        break;
    case 7:  // Pyramid
        b.Size(L"Width:", PyramidObject::kWidth); b.Size(L"Depth:", PyramidObject::kDepth);
        b.Size(L"Height:", PyramidObject::kHeight, -kBig);
        b.Gap();
        b.Count(L"Width Segs:", PyramidObject::kWidthSegs, 1, 200); b.Count(L"Depth Segs:", PyramidObject::kDepthSegs, 1, 200);
        b.Count(L"Height Segs:", PyramidObject::kHeightSegs, 1, 200);
        break;
    case 8:  // Teapot
        b.Size(L"Radius:", TeapotObject::kRadius); b.Count(L"Segments:", TeapotObject::kSegments, 1, 64);
        b.Check(L"Smooth", TeapotObject::kSmooth);
        b.Check(L"Body", TeapotObject::kBody); b.Check(L"Handle", TeapotObject::kHandle);
        b.Check(L"Spout", TeapotObject::kSpout); b.Check(L"Lid", TeapotObject::kLid);
        break;
    case 9:  // Plane
        b.Size(L"Length:", PlaneObject::kLength); b.Size(L"Width:", PlaneObject::kWidth);
        b.Gap();
        b.Count(L"Length Segs:", PlaneObject::kLengthSegs, 1, 200); b.Count(L"Width Segs:", PlaneObject::kWidthSegs, 1, 200);
        break;
    default:
        return {{}, 0};
    }
    return b.Done();
}

Methods CreationMethodsFor(int type) {
    switch (type) {
    case 0:  return {L"Cube", L"Box", 1};                 // measured
    case 7:  return {L"Base/Apex", L"Center", 0};
    case 9:  return {L"Rectangle", L"Square", 0};
    default: return {L"Edge", L"Center", 1};
    }
}

} // namespace ui::params
