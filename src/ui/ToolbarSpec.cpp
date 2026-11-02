#include "ui/ToolbarSpec.h"
#include "ui/Metrics.h"

namespace ui::toolbar {

namespace {
Item Btn(const char* cmd, const wchar_t* tip, int icon) {
    Item i; i.kind = Kind::Button; i.command = cmd; i.tooltip = tip; i.icon = icon; return i;
}
// A flyout button; the first alternative is its initial face.
Item Fly(std::vector<Alternative> alts) {
    Item i = Btn(alts[0].command.c_str(), alts[0].tooltip.c_str(), alts[0].icon);
    i.flyout = std::move(alts);
    return i;
}
Item Radio(Item i, const char* group, bool checked = false) {
    i.group = group; i.checked = checked; return i;
}
Item Sep() { Item i; i.kind = Kind::Separator; return i; }
Item Gap(int w) { Item i; i.kind = Kind::Gap; i.width = w; return i; }
Item Combo(const char* cmd, const wchar_t* tip, int w, const wchar_t* text,
           std::vector<std::wstring> choices) {
    Item i; i.kind = Kind::Combo; i.command = cmd; i.tooltip = tip; i.width = w; i.text = text;
    i.choices = std::move(choices);
    return i;
}
Item Editable(Item i) { i.editable = true; return i; }
} // namespace

// Order and geometry measured from the reference screenshot. Tooltip names, combo
// entries and flyout alternatives are from memory of R3.1 and still to be verified
// (docs/spec/01-ui-look.md).
const std::vector<Item>& MainToolbarSpec() {
    static const std::vector<Item> items = {
        Btn("help.whats_this",   L"Help Mode",                   kIconHelp),
        Sep(),
        Btn("edit.undo",         L"Undo",                        kIconUndo),
        Btn("edit.redo",         L"Redo",                        kIconRedo),
        Sep(),
        Btn("tool.link",         L"Select and Link",             kIconLink),
        Btn("tool.unlink",       L"Unlink Selection",            kIconUnlink),
        Sep(),
        Btn("tool.bind_warp",    L"Bind to Space Warp",          kIconBindWarp),
        Sep(),
        Radio(Btn("tool.select", L"Select Object",               kIconSelect), "mode", true),
        Fly({{"tool.region_rect",   L"Rectangular Selection Region", kIconRegionRect},
             {"tool.region_circle", L"Circular Selection Region",    kIconRegionCircle},
             {"tool.region_fence",  L"Fence Selection Region",       kIconRegionFence}}),
        Gap(2),
        Combo("tool.selection_filter", L"Selection Filter", 70, L"All",
              {L"All", L"Geometry", L"Shapes", L"Lights", L"Cameras", L"Helpers", L"Warps"}),
        Sep(),
        Btn("edit.select_by_name", L"Select by Name",            kIconSelectByName),
        Sep(),
        Radio(Btn("tool.move",   L"Select and Move",             kIconMove), "mode"),
        Radio(Btn("tool.rotate", L"Select and Rotate",           kIconRotate), "mode"),
        Radio(Fly({{"tool.scale_uniform",    L"Select and Uniform Scale",     kIconScale},
                   {"tool.scale_nonuniform", L"Select and Non-uniform Scale", kIconScaleNonUniform},
                   {"tool.scale_squash",     L"Select and Squash",            kIconScaleSquash}}), "mode"),
        Sep(),
        Combo("tool.ref_coord", L"Reference Coordinate System", 70, L"View",
              {L"View", L"Screen", L"World", L"Parent", L"Local", L"Grid", L"Pick"}),
        Fly({{"tool.pivot_point",     L"Use Pivot Point Center",                kIconPivotCenter},
             {"tool.pivot_selection", L"Use Selection Center",                  kIconPivotSelection},
             {"tool.pivot_transform", L"Use Transform Coordinate Center",       kIconPivotTransform}}),
        Sep(),
        Radio(Btn("constraint.x",  L"Restrict to X",             kIconX), "axis"),
        Radio(Btn("constraint.y",  L"Restrict to Y",             kIconY), "axis"),
        Radio(Btn("constraint.z",  L"Restrict to Z",             kIconZ), "axis"),
        Radio(Fly({{"constraint.xy", L"Restrict to XY Plane", kIconXY},
                   {"constraint.yz", L"Restrict to YZ Plane", kIconYZ},
                   {"constraint.zx", L"Restrict to ZX Plane", kIconZX}}), "axis", true),
        Btn("tool.ik",           L"Inverse Kinematics Toggle",   kIconIK),
        Sep(),
        Btn("tools.mirror",      L"Mirror Selected Objects",     kIconMirror),
        Fly({{"tools.array",    L"Array",        kIconArray},
             {"tools.snapshot", L"Snapshot",     kIconSnapshot},
             {"tools.spacing",  L"Spacing Tool", kIconSpacing}}),
        Fly({{"tools.align",           L"Align",           kIconAlign},
             {"tools.align_normals",   L"Normal Align",    kIconNormalAlign},
             {"tools.place_highlight", L"Place Highlight", kIconPlaceHighlight},
             {"tools.align_camera",    L"Align Camera",    kIconAlignCamera},
             {"tools.align_to_view",   L"Align to View",   kIconAlignToView}}),
        Editable(Combo("edit.named_selection", L"Named Selection Sets", 120, L"", {})),
        Sep(),
        Btn("trackview.open",    L"Open Track View",             kIconTrackView),
        Btn("schematic.open",    L"Open Schematic View",         kIconSchematic),
        Sep(),
        Btn("tools.material_editor", L"Material Editor",         kIconMaterialEditor),
        Btn("render.render",     L"Render Scene",                kIconRenderScene),
        Fly({{"render.quick",       L"Quick Render (Production)", kIconQuickRender},
             {"render.quick_draft", L"Quick Render (Draft)",      kIconQuickRenderDraft}}),
        Gap(2),
        Combo("render.type", L"Render Type", 70, L"View",
              {L"View", L"Selected", L"Region", L"Crop", L"Blowup"}),
        Btn("render.last",       L"Render Last",                 kIconRenderLast),
    };
    return items;
}

std::vector<Rect> LayoutToolbar(const std::vector<Item>& items, int h, unsigned dpi) {
    auto s = [dpi](int v) { return metrics::Scale(v, dpi); };
    std::vector<Rect> rects;
    rects.reserve(items.size());
    int x = kLeft; // running position at 100%; scaled per edge so items never drift apart
    for (const Item& it : items) {
        int w = 0;
        switch (it.kind) {
        case Kind::Button:    w = kButtonWidth; break;
        case Kind::Separator: w = kSeparatorWidth; break;
        case Kind::Gap:       w = it.width; break;
        case Kind::Combo:     w = it.width; break;
        }
        Rect r{s(x), 0, s(x + w), h};
        if (it.kind == Kind::Button) { r.top = s(kTop); r.bottom = s(kTop + kButtonHeight); }
        if (it.kind == Kind::Combo)  { r.top = s(kTop); r.bottom = s(kTop + kComboHeight); }
        rects.push_back(r);
        x += w;
    }
    return rects;
}

} // namespace ui::toolbar
