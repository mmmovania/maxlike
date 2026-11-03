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

// Explicitly placed items (status area). Rectangles are at 100%, relative to the
// group's top-left, measured from the reference screenshot.
Item At(Item i, int l, int t, int r, int b) { i.place = {l, t, r, b}; return i; }
Item Toggle(Item i, bool checked = false) { i.toggle = true; i.checked = checked; return i; }
Item Label(const char* cmd, const wchar_t* tip, const wchar_t* text) {
    Item i; i.kind = Kind::Button; i.command = cmd; i.tooltip = tip; i.text = text; return i;
}
Item Field(const char* cmd, const wchar_t* tip, const wchar_t* text) {
    Item i; i.kind = Kind::Edit; i.command = cmd; i.tooltip = tip; i.text = text; return i;
}
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

Config MainToolbarConfig() {
    Config c;
    c.items = MainToolbarSpec();
    c.hasStrips = true;
    return c;
}

// Names and tooltips of the status-area buttons are from memory of R3.1 and still
// to be verified; positions are measured.
Config StatusSnapConfig() {
    Config c;
    c.explicitLayout = true;
    c.icons = IconStrip::Status;
    c.iconSize = 16; c.iconOffsetX = 4; c.iconOffsetY = 3;
    c.frameEdges = false;
    c.flyoutMarkerDx = 1;
    c.items = {
        At(Toggle(Btn("view.degradation_override", L"Degradation Override", kSIconDegradeOverride)), 3, 2, 26, 24),
        At(Toggle(Btn("select.window_crossing", L"Window/Crossing Selection", kSIconWindowCrossing)), 32, 2, 55, 24),
        At(Toggle(Btn("view.adaptive_degradation", L"Adaptive Degradation Toggle", kSIconAdaptiveDegrade), true), 62, 2, 85, 24),
        At(Sep(), 85, 5, 91, 21),
        At(Toggle(Fly({{"snap.3d",  L"3D Snap Toggle",   kSIconSnap3D},
                       {"snap.25d", L"2.5D Snap Toggle", kSIconSnap25D},
                       {"snap.2d",  L"2D Snap Toggle",   kSIconSnap2D}})), 90, 2, 113, 24),
        At(Toggle(Btn("snap.angle",   L"Angle Snap Toggle",   kSIconAngleSnap)), 113, 2, 136, 24),
        At(Toggle(Btn("snap.percent", L"Percent Snap",        kSIconPercentSnap)), 136, 2, 159, 24),
        At(Sep(), 160, 5, 166, 21),
        At(Toggle(Btn("snap.spinner", L"Spinner Snap Toggle", kSIconSpinnerSnap)), 165, 2, 188, 24),
    };
    return c;
}

Config StatusAnimConfig() {
    Config c;
    c.explicitLayout = true;
    c.icons = IconStrip::Status;
    c.iconSize = 16; c.iconOffsetX = 4; c.iconOffsetY = 3;
    c.frameEdges = false;
    c.flyoutMarkerDx = 1;
    Item animate = Toggle(Label("anim.toggle", L"Animate Mode Toggle", L"Animate"));
    animate.alarm = true;
    c.items = {
        At(animate, 4, 3, 52, 47),
        // top row: playback
        At(Btn("anim.go_start",   L"Go to Start",    kSIconGoStart), 54, 3, 77, 25),
        At(Sep(), 76, 6, 82, 22),
        At(Btn("anim.prev_frame", L"Previous Frame", kSIconPrevFrame), 80, 3, 103, 25),
        At(Fly({{"anim.play",          L"Play Animation", kSIconPlay},
                {"anim.play_selected", L"Play Selected",  kSIconPlaySelected}}), 103, 3, 126, 25),
        At(Btn("anim.next_frame", L"Next Frame",     kSIconNextFrame), 126, 3, 149, 25),
        At(Sep(), 148, 6, 154, 22),
        At(Btn("anim.go_end",     L"Go to End",      kSIconGoEnd), 152, 3, 175, 25),
        // top row: navigation
        At(Btn("nav.zoom",        L"Zoom",           kSIconZoom), 175, 3, 198, 25),
        At(Btn("nav.zoom_all",    L"Zoom All",       kSIconZoomAll), 198, 3, 221, 25),
        At(Fly({{"nav.zoom_extents",     L"Zoom Extents",          kSIconZoomExtents},
                {"nav.zoom_extents_sel", L"Zoom Extents Selected", kSIconZoomExtentsSel}}), 221, 3, 244, 25),
        At(Fly({{"nav.zoom_extents_all",     L"Zoom Extents All",          kSIconZoomExtentsAll},
                {"nav.zoom_extents_all_sel", L"Zoom Extents All Selected", kSIconZoomExtentsAllSel}}), 244, 3, 267, 25),
        // bottom row
        At(Toggle(Btn("anim.key_mode", L"Key Mode Toggle", kSIconKeyMode)), 54, 25, 77, 47),
        At(Sep(), 76, 28, 82, 44),
        At(Field("anim.frame", L"Current Frame", L"0"), 81, 26, 150, 47),
        At(Sep(), 149, 28, 155, 44),
        At(Btn("anim.time_config", L"Time Configuration", kSIconTimeConfig), 152, 25, 175, 47),
        At(Btn("nav.region_zoom", L"Region Zoom",     kSIconRegionZoom), 175, 25, 198, 47),
        At(Btn("nav.pan",         L"Pan",             kSIconPan), 198, 25, 221, 47),
        At(Fly({{"nav.arc_rotate",     L"Arc Rotate",           kSIconArcRotate},
                {"nav.arc_rotate_sel", L"Arc Rotate Selected",  kSIconArcRotateSel},
                {"nav.arc_rotate_sub", L"Arc Rotate SubObject", kSIconArcRotateSub}}), 221, 25, 244, 47),
        At(Btn("nav.min_max",     L"Min/Max Toggle",  kSIconMinMax), 244, 25, 267, 47),
    };
    return c;
}

std::vector<Rect> LayoutItems(const Config& config, int w, int h, unsigned dpi) {
    if (!config.explicitLayout)
        return LayoutToolbar(config.items, h, dpi);
    (void)w;
    auto s = [dpi](int v) { return metrics::Scale(v, dpi); };
    std::vector<Rect> rects;
    rects.reserve(config.items.size());
    for (const Item& it : config.items)
        rects.push_back({s(it.place.left), s(it.place.top), s(it.place.right), s(it.place.bottom)});
    return rects;
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
        case Kind::Edit:      w = it.width; break;
        }
        Rect r{s(x), 0, s(x + w), h};
        if (it.kind == Kind::Button) { r.top = s(kTop); r.bottom = s(kTop + kButtonHeight); }
        if (it.kind == Kind::Combo)  { r.top = s(kTop); r.bottom = s(kTop + kComboHeight); }
        if (it.kind == Kind::Separator) { r.top = s(kSeparatorTop); r.bottom = s(kSeparatorTop + kSeparatorHeight); }
        rects.push_back(r);
        x += w;
    }
    return rects;
}

} // namespace ui::toolbar
