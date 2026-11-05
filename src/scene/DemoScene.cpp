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
    const bool all = which == "all";
    if (!all && which != "box" && which != "plane" && which != "pyramid") return false;
    if (all || which == "box")
        Add(s, "Box", std::make_shared<BoxObject>(60, 60, 60), 0x1AB194, {all ? -80.0 : 0.0, 0, 0});
    if (all || which == "pyramid")
        Add(s, "Pyramid", std::make_shared<PyramidObject>(20, 20, 10), 0x390888, {0, 0, 0});
    if (all || which == "plane") {
        auto plane = std::make_shared<PlaneObject>(42, 40);   // length (Y) 42, width (X) 40
        Add(s, "Plane", plane, 0xB1581A, {all ? 60.0 : 0.0, 0, 0});
    }
    return true;
}

} // namespace scene
