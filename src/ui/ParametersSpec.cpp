#include "ui/ParametersSpec.h"
#include "scene/Primitives.h"
#include "scene/Modifier.h"

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
        return Spec{};
    }
    return b.Done();
}

KeyboardSpec KeyboardEntryFor(int type) {
    const Spec params = ParametersFor(type);
    KeyboardSpec k;
    if (params.rows.empty()) return k;
    int y = 24;
    for (auto [label, index] : {std::pair<const wchar_t*, int>{L"X:", kPosX}, {L"Y:", kPosY}, {L"Z:", kPosZ}}) {
        k.fields.rows.push_back({label, index, Kind::Float, -kBig, kBig, 0.1, y});
        y += 21;
    }
    y += 7;
    // The sizes: Parameters' first group of float rows (lengths and radii, not angles).
    for (const Row& r : params.rows) {
        if (r.kind != Kind::Float || r.step != 0.1) break;
        Row copy = r;
        copy.top = y;
        k.fields.rows.push_back(copy);
        y += 21;
    }
    k.buttonTop = y + 5;
    k.fields.contentHeight = k.buttonTop + kCreateButtonHeight + 8 - 15;
    return k;
}

Spec ModifierParametersFor(const std::string& name) {
    using scene::BendModifier; using scene::TaperModifier; using scene::TwistModifier;
    auto spinner = [](const wchar_t* label, int index, int top, int fieldLeft, double min, double max, double step) {
        Row r{label, index, Kind::Float, min, max, step, top};
        r.fieldLeft = fieldLeft;
        r.decimals = 1;                                    // measured: "30.5", "0.0"
        return r;
    };
    auto radios = [](int index, int top, std::vector<std::wstring> labels, std::vector<int> lefts) {
        Row r{L"", index, Kind::Radio, 0, static_cast<double>(labels.size() - 1), 1, top};
        r.options = std::move(labels);
        r.optionLefts = std::move(lefts);
        return r;
    };
    auto check = [](const wchar_t* label, int index, int top, int left) {
        Row r{label, index, Kind::Check, 0, 1, 1, top};
        r.checkLeft = left;
        return r;
    };
    // The measured Bend layout, shared: a first group with two spinners, an axis group
    // with X / Y / Z, a Limits group with Limit Effect and two limits.
    auto standard = [&](const wchar_t* first, const wchar_t* a, int ia, double aMin, double aMax, double aStep,
                        const wchar_t* b, int ib, double bMin, double bMax, double bStep,
                        const wchar_t* axisGroup, int iAxis, int iLimit, int iUpper, int iLower, int firstGap, int axisGap) {
        Spec s;
        s.groups = {{25, 84, first, firstGap}, {97, 135, axisGroup, axisGap}, {149, 227, L"Limits", 53}};
        s.rows = {spinner(a, ia, 36, 81, aMin, aMax, aStep), spinner(b, ib, 57, 81, bMin, bMax, bStep),
                  radios(iAxis, 113, {L"X", L"Y", L"Z"}, {40, 81, 120}),
                  check(L"Limit Effect", iLimit, 160, 87),
                  spinner(L"Upper Limit:", iUpper, 180, 87, 0, kBig, 0.1),
                  spinner(L"Lower Limit:", iLower, 201, 87, -kBig, 0, 0.1)};
        s.contentHeight = 223;                             // measured: frame's shadow line at title + 237
        return s;
    };
    if (name == "Bend")
        return standard(L"Bend:", L"Angle:", BendModifier::kAngle, -kBig, kBig, 0.5, L"Direction:", BendModifier::kDirection,
                        -kBig, kBig, 0.5, L"Bend Axis:", BendModifier::kAxis, BendModifier::kLimit, BendModifier::kUpper,
                        BendModifier::kLower, 55, 77);
    if (name == "Twist")
        return standard(L"Twist:", L"Angle:", TwistModifier::kAngle, -kBig, kBig, 0.5, L"Bias:", TwistModifier::kBias,
                        -100, 100, 0.5, L"Twist Axis:", TwistModifier::kAxis, TwistModifier::kLimit, TwistModifier::kUpper,
                        TwistModifier::kLower, 56, 80);
    if (name == "Taper") {
        // Taper's axis group holds Primary (X / Y / Z), Effect (X / Y / XY) and Symmetry:
        // two more rows than Bend's, so it and Limits are 42 rows taller / lower.
        Spec s = standard(L"Taper:", L"Amount:", TaperModifier::kAmount, -10, 10, 0.01, L"Curve:", TaperModifier::kCurve,
                          -10, 10, 0.01, L"Taper Axis:", TaperModifier::kAxis, TaperModifier::kLimit, TaperModifier::kUpper,
                          TaperModifier::kLower, 57, 79);
        s.rows[0].decimals = s.rows[1].decimals = 2;
        s.groups[1].bottom += 42;
        s.groups[2].top += 42;
        s.groups[2].bottom += 42;
        for (size_t i = 3; i < s.rows.size(); ++i) s.rows[i].top += 42;
        s.rows.insert(s.rows.begin() + 3, radios(TaperModifier::kEffect, 134, {L"X", L"Y", L"XY"}, {40, 81, 120}));
        s.rows.insert(s.rows.begin() + 4, check(L"Symmetry", TaperModifier::kSymmetry, 155, 40));
        s.contentHeight += 42;
        return s;
    }
    return {};
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
