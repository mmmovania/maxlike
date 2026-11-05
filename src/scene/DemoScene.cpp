#include "scene/DemoScene.h"
#include "scene/Primitives.h"

namespace scene {

static Node* Add(Scene& s, const std::string& base, std::shared_ptr<Object> object, uint32_t colour,
                 const math::Vec3& position) {
    Node* n = s.CreateNode(s.UniqueName(base));
    n->SetObject(std::move(object));
    n->SetWireColor(colour);
    n->Transform().position = position;
    return n;
}

bool BuildDemoScene(Scene& s, const std::string& which) {
    struct Sample { const char* name; const char* base; uint32_t colour; double allX;
                    std::shared_ptr<Object> (*make)(); };
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
    };
    const bool all = which == "all";
    bool found = all;
    for (const Sample& sm : kSamples) {
        if (!all && which != sm.name) continue;
        Add(s, sm.base, sm.make(), sm.colour, {all ? sm.allX : 0.0, 0, 0});
        found = true;
    }
    return found;
}

} // namespace scene
