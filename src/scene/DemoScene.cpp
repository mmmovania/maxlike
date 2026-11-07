#include "scene/DemoScene.h"
#include "scene/Primitives.h"
#include "scene/Modifier.h"
#include "math/Quat.h"

namespace scene {

static Node* Add(Scene& s, const std::string& base, std::shared_ptr<Object> object, uint32_t colour,
                 const math::Vec3& position) {
    Node* n = s.CreateNode(s.UniqueName(base));
    n->SetObject(std::move(object));
    n->SetWireColor(colour);
    n->Transform().position = position;
    return n;
}

// Illustrative version of instances.max: seven nodes sharing one Derived Object (a Box
// with an Edit Mesh that turns the top four vertices by +25 degrees), each node turned
// about Z from -41.5 (Box01) to +47.0 degrees (Box07). The box size and the node
// positions are not from the sample.
static void BuildInstances(Scene& s) {
    constexpr double kPi = 3.14159265358979323846;
    auto box = std::make_shared<BoxObject>(20, 20, 40);
    const Mesh base = box->Evaluate();
    const math::Quat twist = math::Quat::FromAxisAngle({0, 0, 1}, 25 * kPi / 180);
    std::vector<EditMeshModifier::Move> moves;
    for (uint32_t v = 4; v < 8; ++v) moves.push_back({v, twist.Rotate(base.verts[v]) - base.verts[v]});
    auto derived = std::make_shared<DerivedObject>(box);
    derived->AddModifier(std::make_shared<EditMeshModifier>(base.verts.size(), moves));
    for (int i = 0; i < 7; ++i) {
        const double deg = -41.5 + (47.0 + 41.5) * i / 6;
        Node* n = Add(s, "Box", derived, 0x1AB194, {-150.0 + 50.0 * i, 0, 0});
        n->Transform().rotation = math::Quat::FromAxisAngle({0, 0, 1}, deg * kPi / 180);
    }
}

// Bend, Twist and Taper (Phase 7) on three tall boxes with enough segments to show the
// curves: Bend 90 degrees, Twist 90 degrees, Taper inwards with a slight curve.
static void BuildDeform(Scene& s) {
    auto tall = [] {
        auto b = std::make_shared<BoxObject>(20, 20, 100);
        b->Params().Set(BoxObject::kHeightSegs, 20);
        b->Params().Set(BoxObject::kLengthSegs, 4);
        b->Params().Set(BoxObject::kWidthSegs, 4);
        return b;
    };
    auto bend = std::make_shared<BendModifier>();
    bend->Params().Set(BendModifier::kAngle, 90);
    auto twist = std::make_shared<TwistModifier>();
    twist->Params().Set(TwistModifier::kAngle, 90);
    auto taper = std::make_shared<TaperModifier>();
    taper->Params().Set(TaperModifier::kAmount, -0.6);
    taper->Params().Set(TaperModifier::kCurve, 0.3);
    const std::shared_ptr<Modifier> mods[3] = {bend, twist, taper};
    const char* names[3] = {"Bend", "Twist", "Taper"};
    const uint32_t colours[3] = {0x1AB194, 0xB1581A, 0x8A1AB1};
    for (int i = 0; i < 3; ++i) {
        auto d = std::make_shared<DerivedObject>(tall());
        d->AddModifier(mods[i]);
        Add(s, names[i], d, colours[i], {-90.0 + 90.0 * i, 0, 0});
    }
}

bool BuildDemoScene(Scene& s, const std::string& which) {
    if (which == "instances") { BuildInstances(s); return true; }
    if (which == "deform") { BuildDeform(s); return true; }
    struct Sample { const char* name; const char* base; uint32_t colour; double allX;
                    std::shared_ptr<Object> (*make)(); double allY = 0; };
    static const Sample kSamples[] = {
        {"box", "Box", 0x1AB194, -160, [] { return std::shared_ptr<Object>(std::make_shared<BoxObject>(60, 60, 60)); }},
        {"pyramid", "Pyramid", 0x390888, -110, [] { return std::shared_ptr<Object>(std::make_shared<PyramidObject>(20, 20, 10)); }},
        {"plane", "Plane", 0xB1581A, -65, [] { return std::shared_ptr<Object>(std::make_shared<PlaneObject>(42, 40)); }},
        {"cylinder", "Cylinder", 0x8A1AB1, -15, [] { return std::shared_ptr<Object>(std::make_shared<CylinderObject>(15, 25)); }},
        {"cone", "Cone", 0x57E0C6, 30, [] {
             auto c = std::make_shared<ConeObject>(10, -20, 20);
             c->Params().Set(ConeObject::kSides, 24);
             c->Params().Set(ConeObject::kHeightSegs, 5);
             return std::shared_ptr<Object>(c); }},
        {"tube", "Tube", 0x1AB158, 70, [] {
             auto t = std::make_shared<TubeObject>(10, 6.185, 10);
             t->Params().Set(TubeObject::kSides, 18);
             t->Params().Set(TubeObject::kCapSegs, 1);
             t->Params().Set(TubeObject::kHeightSegs, 5);
             return std::shared_ptr<Object>(t); }},
        {"torus", "Torus", 0xE0C657, 140, [] { return std::shared_ptr<Object>(std::make_shared<TorusObject>(25, 5)); }},
        {"sphere", "Sphere", 0x860606, -110, [] { return std::shared_ptr<Object>(std::make_shared<SphereObject>(10)); }, 70},
        {"geosphere", "GeoSphere", 0x99E4B8, -110, [] { return std::shared_ptr<Object>(std::make_shared<GeoSphereObject>(10)); }, -70},
        {"teapot", "Teapot", 0xC6E057, 40, [] { return std::shared_ptr<Object>(std::make_shared<TeapotObject>(30)); }, -65},
    };
    const bool all = which == "all";
    bool found = all;
    for (const Sample& sm : kSamples) {
        if (!all && which != sm.name) continue;
        Add(s, sm.base, sm.make(), sm.colour, {all ? sm.allX : 0.0, all ? sm.allY : 0.0, 0});
        found = true;
    }
    return found;
}

} // namespace scene
