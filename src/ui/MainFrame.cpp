#include "ui/MainFrame.h"
#include <cwchar>
#include "ui/FrameLayout.h"
#include "ui/Bevel.h"
#include "scene/DemoScene.h"
#include "scene/MoveMath.h"
#include "ui/MenuBuilder.h"
#include "ui/MenuSpec.h"
#include "ui/TabStrip.h"
#include "ui/Theme.h"

namespace ui {

static constexpr const wchar_t* kAppName = L"maxlike";

MainFrame::~MainFrame() {
    if (uiFont_) DeleteObject(uiFont_);
}

bool MainFrame::Create() {
    std::wstring title = docName_ + L" - " + kAppName;
    CreateParams p;
    p.title  = title.c_str();
    p.style  = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    p.width  = 1280;
    p.height = 800;
    // A top-level window's menu is passed in the "menu or ID" slot. Once the
    // window owns it, DestroyWindow destroys the menu too.
    p.menuOrId = BuildMenuBar(MainMenuSpec(), commands_);
    if (win::Window::Create(p))
        return true;
    DestroyMenu(p.menuOrId); // creation failed: the menu was never attached
    return false;
}

// The Create panel follows the creation tool: the object being built or just created
// (else a template with the defaults), its name, and its colour (R3: the swatch shows
// the new object's colour until the next one is started).
// Verified wording: "None Selected" (reference) and "1 Object Selected" (Box screenshot);
// the plural form is assumed.
void MainFrame::UpdateSelectionStatus() {
    const size_t n = scene_.Selection().size();
    std::wstring text = n == 0 ? L"None Selected"
                      : n == 1 ? L"1 Object Selected"
                               : std::to_wstring(n) + L" Objects Selected";
    statusArea_.SetPrompt(0, text);
}

// The figure at the selection's pivot and the prompt follow the tool and constraint.
// Prompts: Select and Move verified (r3.1-move-selected.png); Rotate and Scale follow
// the same pattern (NOTE: unverified). Move, Rotate and Scale all show the Move gizmo
// (R3.1, confirmed by the project owner); Select and creation show the tripod.
void MainFrame::UpdateTransformDisplay() {
    viewports_.SetAxesDisplay(tool_ != Tool::Select ? view::AxesStyle::MoveGizmo : view::AxesStyle::Tripod, constraint_);
    viewports_.SetTransformTool(tool_ != Tool::Select);
    if (commandPanel_.CreateType() >= 0) return;   // creation keeps its own prompt
    const wchar_t* prompt = L"Click or click-and-drag to select objects";
    if (tool_ == Tool::Move) prompt = L"Click and drag to select and move objects";
    if (tool_ == Tool::Rotate) prompt = L"Click and drag to select and rotate objects";
    if (tool_ == Tool::Scale) prompt = L"Click and drag to select and scale objects";
    statusArea_.SetPrompt(1, prompt);
}

void MainFrame::ShowCoordinates(const math::Vec3& v) {
    auto f = [](double d) {
        wchar_t buf[32];
        std::swprintf(buf, 32, L"%.3f", std::fabs(d) < 0.0005 ? 0.0 : d);   // no "-0.000"
        return std::wstring(buf);
    };
    statusArea_.SetCoordinates(f(v.x), f(v.y), f(v.z));
}

void MainFrame::DeleteSelection() {
    if (scene_.Selection().empty()) return;
    undo_.Execute(std::make_unique<scene::DeleteSelectionCommand>(), scene_);
    UpdateSelectionStatus();
    UpdateCreatePanel();
    viewports_.Redraw();
}

// Node names are UTF-8; the panel works in UTF-16.
static std::wstring FromUtf8(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

static std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

// While a primitive is active: the object being built or just created. Otherwise the
// one selected object (none with zero or several selected; NOTE: R3's look with several
// selected is unverified).
scene::Node* MainFrame::NameColorTarget() const {
    if (commandPanel_.CreateType() >= 0) {
        const std::string name = creation_.TargetName();
        return name.empty() ? nullptr : scene_.Find(name);
    }
    const auto& sel = scene_.Selection();
    return sel.size() == 1 ? scene_.FindById(sel[0]) : nullptr;
}

void MainFrame::UpdateCreatePanel() {
    panelObject_ = creation_.TargetObject();
    if (!panelObject_) panelObject_ = template_;
    commandPanel_.SetParameterSource(panelObject_ ? &panelObject_->Params() : nullptr);
    const scene::Node* target = NameColorTarget();
    if (creation_.Busy()) {
        commandPanel_.SetObjectName(FromUtf8(creation_.TargetName()));   // the live node, not yet recorded
        commandPanel_.SetObjectColor(creation_.Color());
    } else {
        commandPanel_.SetObjectName(target ? FromUtf8(target->Name()) : std::wstring());
        commandPanel_.SetObjectColor(target ? target->WireColor() : creation_.Color());
    }
    commandPanel_.Refresh();
}

bool MainFrame::LoadDemoScene(const std::string& which) {
    if (!scene::BuildDemoScene(scene_, which)) return false;
    undo_.Clear();      // like opening a file: nothing to undo
    undo_.MarkClean();
    viewports_.SetScene(&scene_);
    return true;
}

void MainFrame::SetDocumentName(const std::wstring& name) {
    docName_ = name;
    if (Hwnd()) {
        std::wstring title = docName_ + L" - " + kAppName;
        SetWindowTextW(Hwnd(), title.c_str());
    }
}

std::array<win::Window*, 8> MainFrame::Regions() {
    return {&tabPanel_, &mainToolbar_, &objectsToolbar_, &viewports_, &timeSlider_,
            &trackBar_, &commandPanel_, &statusArea_};
}

bool MainFrame::CreateRegions() {
    if (!tabPanel_.Create(Hwnd()) || !mainToolbar_.Create(Hwnd(), toolbar::MainToolbarConfig()) ||
        !timeSlider_.Create(Hwnd()) || !trackBar_.Create(Hwnd()) || !statusArea_.Create(Hwnd()) ||
        !viewports_.Create(Hwnd()) || !commandPanel_.Create(Hwnd()) ||
        !objectsToolbar_.Create(Hwnd(), toolbar::ObjectsToolbarConfig()))
        return false;
    ShowWindow(objectsToolbar_.Hwnd(), SW_HIDE);   // shown with the Objects tab

    viewports_.SetScene(&scene_);

    // Creation: the checked Object Type button picks the primitive; viewports feed the
    // mouse; the swatch gives the colour, and shows the next one after each object.
    // NOTE: R3's prompt wording is unverified.
    creation_.SetColor(commandPanel_.ObjectColor());
    // The swatch recolours the selected objects (one undo step); with nothing selected it
    // sets the next new object's colour.
    commandPanel_.onObjectColorChanged = [this](unsigned rgb) {
        if (creation_.Busy() || scene_.Selection().empty()) {
            creation_.SetColor(rgb);
        } else {
            auto macro = std::make_unique<scene::MacroCommand>("Color Change");
            for (scene::Node::Id id : scene_.Selection()) macro->Add(std::make_unique<scene::WireColorCommand>(id, rgb));
            undo_.Execute(std::move(macro), scene_);
            viewports_.Redraw();
        }
        UpdateCreatePanel();
    };
    // Renaming the object Name and Color shows (one undo step).
    commandPanel_.onNameChanged = [this](const std::wstring& name) {
        scene::Node* target = creation_.Busy() ? nullptr : NameColorTarget();
        if (!target) return;
        undo_.Execute(std::make_unique<scene::RenameCommand>(target->GetId(), ToUtf8(name)), scene_);
        UpdateCreatePanel();
    };
    commandPanel_.onCreateTypeChanged = [this](int type) {
        if (creation_.Cancel()) viewports_.Redraw();
        creation_.SetType(type);
        template_ = type >= 0 ? scene::MakePrimitive(type) : nullptr;
        // The Objects strip's button for the active primitive looks pressed too.
        // NOTE: whether R3 shows it pressed is unverified.
        for (int i = 0; i < 10; ++i)
            objectsToolbar_.SetChecked(toolbar::ObjectsToolbarSpec()[static_cast<size_t>(i)].command, i == type);
        UpdateCreatePanel();
        // Prompt verified against R3.1 (screenshot r3.1-box-created.png).
        if (type >= 0) statusArea_.SetPrompt(1, L"Click and drag to begin creation process");
        else UpdateTransformDisplay();
    };
    // Keyboard Entry's Create: the typed object, oriented by the active viewport.
    commandPanel_.onKeyboardCreate = [this](double x, double y, double z, const std::vector<std::pair<int, double>>& sizes) {
        std::vector<std::pair<size_t, double>> params;
        for (const auto& [index, value] : sizes) params.push_back({static_cast<size_t>(index), value});
        if (creation_.CreateDirect(params, viewports_.ActivePlane(), {x, y, z}).empty()) return;
        UpdateCreatePanel();
        UpdateSelectionStatus();
        viewports_.Redraw();
    };
    commandPanel_.onCreationMethodChanged = [this](int method) {
        if (method != ui::params::CreationMethodsFor(commandPanel_.CreateType()).selected)
            statusArea_.SetPrompt(1, L"This creation method is not implemented yet");
    };
    // Parameters: while building, changes go straight to the object; afterwards a spinner
    // drag previews live and becomes one undo step when released.
    commandPanel_.onParamChanged = [this](int index, double value, bool final) {
        std::shared_ptr<scene::Object> obj = creation_.TargetObject();
        const auto i = static_cast<size_t>(index);
        if (!obj || creation_.Busy()) {
            if (panelObject_) panelObject_->Params().Set(i, value);
        } else if (!final) {
            if (!paramDragging_) { paramDragging_ = true; paramDragStart_ = obj->Params().Float(i); }
            obj->Params().Set(i, value);
        } else {
            if (paramDragging_) obj->Params().Set(i, paramDragStart_);   // so undo returns here
            paramDragging_ = false;
            if (obj->Params().Float(i) != value)
                undo_.Execute(std::make_unique<scene::SetParamCommand>(obj, i, value), scene_);
        }
        viewports_.Redraw();
        commandPanel_.Refresh();
    };
    viewports_.creationActive = [this] { return creation_.Type() >= 0; };
    viewports_.onCreateEvent = [this](scene::Creator::Event e, int owner, const scene::ConstructionPlane& p,
                                      double u, double v, int y, double upp) {
        if (!creation_.Feed(e, owner, p, u, v, y, upp)) return false;
        UpdateCreatePanel();
        UpdateSelectionStatus();
        viewports_.Redraw();
        return true;
    };
    viewports_.onCreateCancel = [this] {
        if (!creation_.Cancel()) return false;
        UpdateCreatePanel();
        viewports_.Redraw();
        return true;
    };

    // Selection: a click replaces the selection (Ctrl adds, Alt removes; empty space
    // clears); a region does the same with everything it touches. Each change is one
    // undoable "Select". NOTE: R3's exact modifier rules are from memory.
    viewports_.onSelect = [this](const std::vector<scene::Node::Id>& ids, bool, bool ctrl, bool alt) {
        std::vector<scene::Node::Id> next = scene_.Selection();
        if (ctrl) {
            for (auto id : ids) if (!scene_.IsSelected(id)) next.push_back(id);
        } else if (alt) {
            next.erase(std::remove_if(next.begin(), next.end(), [&](scene::Node::Id id) {
                return std::find(ids.begin(), ids.end(), id) != ids.end(); }), next.end());
        } else {
            next = ids;
        }
        if (next == scene_.Selection()) return;
        undo_.Execute(std::make_unique<scene::SelectCommand>(next), scene_);
        UpdateSelectionStatus();
        UpdateCreatePanel();
        viewports_.Redraw();
    };
    viewports_.onDeleteKey = [this] { DeleteSelection(); };
    // Moving: a live preview from the starting positions, then one "Move" undo step.
    viewports_.onMoveBegin = [this] {
        moving_.clear();
        for (scene::Node::Id id : scene_.Selection()) {
            scene::Node* n = scene_.FindById(id);
            if (!n) continue;
            bool ancestorSelected = false;
            for (const scene::Node* p = n->Parent(); p && !ancestorSelected; p = p->Parent())
                ancestorSelected = scene_.IsSelected(p->GetId());
            if (ancestorSelected) continue;
            Moving m{id, n->Transform(), math::TransformPoint(n->WorldMatrix(), {0, 0, 0}), n->Parent() != nullptr, {}, {}};
            if (n->Parent()) m.parentWorld = n->Parent()->WorldMatrix();
            for (const scene::Node* p = n->Parent(); p; p = p->Parent())     // the parents' combined rotation
                m.parentRotation = p->Transform().rotation * m.parentRotation;
            moving_.push_back(m);
        }
    };
    // Move follows the mouse in the world; Rotate and Scale follow its vertical movement,
    // each object about its own pivot (NOTE: R3's pivot options are not implemented yet).
    viewports_.onMoveDrag = [this](const math::Vec3& delta, int up) {
        const int axis = scene::move::RotationAxis(constraint_);
        const math::Quat turn = scene::move::DragRotation(axis, up);
        const math::Vec3 factors = scene::move::DragScale(scaleMode_, constraint_, up);
        if (tool_ == Tool::Move) ShowCoordinates(delta);
        if (tool_ == Tool::Rotate) ShowCoordinates(scene::move::RotationDegrees(axis, up));
        if (tool_ == Tool::Scale) ShowCoordinates(scene::move::ScalePercent(factors));
        for (const Moving& m : moving_) {
            scene::Node* n = scene_.FindById(m.id);
            if (!n) continue;
            if (tool_ == Tool::Move) {
                const math::Vec3 world = m.startWorld + delta;
                math::Vec3 local = world;
                if (m.hasParent && !scene::move::InverseTransformPoint(m.parentWorld, world, local)) continue;
                n->Transform().position = local;
            } else if (tool_ == Tool::Rotate) {
                n->Transform().rotation = scene::move::RotateInParent(m.parentRotation, turn, m.start.rotation);
            } else if (tool_ == Tool::Scale) {
                n->Transform().scale = {m.start.scale.x * factors.x, m.start.scale.y * factors.y, m.start.scale.z * factors.z};
            }
        }
        viewports_.Redraw();
    };
    // The status bar's X/Y/Z: the cursor's world position, or (below) the amount of the
    // transform being dragged. NOTE: R3's display rules and format are unverified.
    viewports_.onCursorWorld = [this](bool ok, const math::Vec3& p) {
        if (!moving_.empty()) return;
        if (ok) ShowCoordinates(p);
        else statusArea_.SetCoordinates(L"", L"", L"");
    };
    viewports_.onMoveEnd = [this](bool commit) {
        const char* name = tool_ == Tool::Rotate ? "Rotate" : tool_ == Tool::Scale ? "Scale" : "Move";
        auto macro = std::make_unique<scene::MacroCommand>(name);
        bool changed = false;
        for (const Moving& m : moving_) {
            scene::Node* n = scene_.FindById(m.id);
            if (!n) continue;
            const scene::PRS after = n->Transform();
            n->Transform() = m.start;                   // so the command records the real "before"
            const bool same = math::Length(after.position - m.start.position) < 1e-12 &&
                              math::Length(after.scale - m.start.scale) < 1e-12 &&
                              std::fabs(after.rotation.x - m.start.rotation.x) + std::fabs(after.rotation.y - m.start.rotation.y) +
                                  std::fabs(after.rotation.z - m.start.rotation.z) + std::fabs(after.rotation.w - m.start.rotation.w) < 1e-12;
            if (commit && !same) {
                macro->Add(std::make_unique<scene::TransformCommand>(m.id, m.start, after, name));
                changed = true;
            }
        }
        moving_.clear();
        if (changed) undo_.Execute(std::move(macro), scene_);
        viewports_.Redraw();
    };

    tabPanel_.onTabChanged = [this](int index) {
        const auto& tab = tabstrip::Tabs()[static_cast<size_t>(index)];
        // The Objects tab (index 1) has its own toolbar in the main toolbar's place; the
        // other strips are still drawn by the main toolbar as placeholders.
        const bool objects = index == 1;
        ShowWindow(objectsToolbar_.Hwnd(), objects ? SW_SHOW : SW_HIDE);
        ShowWindow(mainToolbar_.Hwnd(), objects ? SW_HIDE : SW_SHOW);
        if (!objects) mainToolbar_.ShowStrip(index, tab.label);
    };
    objectsToolbar_.onCommand = [this](const std::string& command) { OnCommand(command); };
    // The track bar follows the time slider. Later the current frame will live in
    // the scene's animation state, and both will just display it.
    timeSlider_.onFrameChanged = [this](int frame) { ShowFrame(frame); };
    statusArea_.onFrameEntered = [this](int frame) {
        timeSlider_.SetFrame(frame);
        ShowFrame(timeSlider_.Frame()); // clamped to the range
    };
    statusArea_.onCommand = [this](const std::string& command) { OnCommand(command); };

    // The active navigation mode: its button stays pressed and the prompt explains it.
    // NOTE: R3's prompt texts for these modes are not verified.
    viewports_.onNavModeChanged = [this](NavMode mode) {
        struct ModeInfo { NavMode mode; const char* command; const wchar_t* prompt; };
        static const ModeInfo kModes[] = {
            {NavMode::Zoom,       "nav.zoom",        L"Click and drag to zoom in or out"},
            {NavMode::ZoomAll,    "nav.zoom_all",    L"Click and drag to zoom all viewports"},
            {NavMode::RegionZoom, "nav.region_zoom", L"Drag a rectangle to zoom into"},
            {NavMode::Pan,        "nav.pan",         L"Click and drag to pan"},
            {NavMode::ArcRotate,  "nav.arc_rotate",  L"Click and drag to rotate the view"},
        };
        std::wstring prompt = L"Click or click-and-drag to select objects";
        for (const ModeInfo& m : kModes) {
            statusArea_.SetCommandChecked(m.command, m.mode == mode);
            if (m.mode == mode) prompt = std::wstring(m.prompt) + L"  (right-click to finish)";
        }
        statusArea_.SetPrompt(1, prompt);
    };

    statusArea_.onListenerInput = [this](const std::wstring& text) {
        statusArea_.SetListenerOutput(text + L"  -- MAXScript is not implemented yet");
    };
    statusArea_.onLockChanged = [this](bool locked) {
        statusArea_.SetPrompt(1, locked ? L"Selection locked" : L"Selection unlocked");
    };

    // Toolbar buttons and menu items share one command handler.
    mainToolbar_.onCommand = [this](const std::string& command) { OnCommand(command); };
    mainToolbar_.onComboChanged = [this](const std::string& command, const std::wstring& text) {
        const std::wstring name(command.begin(), command.end()); // command names are ASCII
        statusArea_.SetPrompt(1, name + L" = " + text + L"  (not implemented yet)");
    };
    return true;
}

void MainFrame::LayoutRegions(int w, int h) {
    const FrameLayout L = ComputeFrameLayout(w, h, Dpi());

    struct Item { win::Window* wnd; const Rect* r; };
    const Item items[] = {
        {&tabPanel_, &L.tabPanel},     {&mainToolbar_, &L.mainToolbar},
        {&objectsToolbar_, &L.mainToolbar},
        {&viewports_, &L.viewports},   {&timeSlider_, &L.timeSlider},
        {&trackBar_, &L.trackBar},     {&commandPanel_, &L.commandPanel},
        {&statusArea_, &L.statusArea},
    };

    // Move all children in one batch so the window repaints once, not seven times.
    HDWP dwp = BeginDeferWindowPos(static_cast<int>(std::size(items)));
    for (const Item& it : items) {
        if (!dwp) break;
        dwp = DeferWindowPos(dwp, it.wnd->Hwnd(), nullptr,
                             it.r->left, it.r->top, it.r->Width(), it.r->Height(),
                             SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (dwp) EndDeferWindowPos(dwp);
}

void MainFrame::RecreateFonts() {
    HFONT old = uiFont_;
    uiFont_ = theme::CreateUiFont(Dpi());
    // Hand the new font to every child before deleting the old one, so no
    // child is ever left holding a deleted font.
    for (win::Window* r : Regions())
        if (r->Hwnd())
            SendMessageW(r->Hwnd(), WM_SETFONT, reinterpret_cast<WPARAM>(uiFont_), TRUE);
    if (old) DeleteObject(old);
}

// Until animation exists, the time slider holds the current frame; the other
// controls only display it.
void MainFrame::ShowFrame(int frame) {
    trackBar_.SetFrame(frame);
    statusArea_.SetFrame(frame);
}

void MainFrame::OnCommand(const std::string& command) {
    // Playback buttons that already make sense without animation: move the frame.
    const int frame = timeSlider_.Frame();
    int target = frame;
    if (command == "anim.go_start")   target = 0;
    if (command == "anim.go_end")     target = 100;
    if (command == "anim.prev_frame") target = frame - 1;
    if (command == "anim.next_frame") target = frame + 1;
    if (command == "anim.go_start" || command == "anim.go_end" ||
        command == "anim.prev_frame" || command == "anim.next_frame") {
        timeSlider_.SetFrame(target);
        ShowFrame(timeSlider_.Frame());
        return;
    }

    // The Objects strip's Standard Primitives: the same as their Create panel buttons
    // (clicking the active one again turns it off).
    for (int i = 0; i < 10; ++i)
        if (command == toolbar::ObjectsToolbarSpec()[static_cast<size_t>(i)].command) {
            commandPanel_.SetCreateType(commandPanel_.CreateType() == i ? -1 : i);
            return;
        }

    if (command == "edit.undo" || command == "edit.redo") {
        const bool ok = command == "edit.undo" ? undo_.Undo(scene_) : undo_.Redo(scene_);
        if (ok) { viewports_.Redraw(); UpdateCreatePanel(); UpdateSelectionStatus(); }
        return;
    }

    // Transform tools and axis constraints (radio groups in the toolbar).
    {
        struct T { const char* cmd; Tool tool; };
        static const T kTools[] = {{"tool.select", Tool::Select}, {"tool.move", Tool::Move}, {"tool.rotate", Tool::Rotate},
                                   {"tool.scale_uniform", Tool::Scale}, {"tool.scale_nonuniform", Tool::Scale},
                                   {"tool.scale_squash", Tool::Scale}};
        for (const T& t : kTools)
            if (command == t.cmd) {
                tool_ = t.tool;
                if (command == "tool.scale_uniform") scaleMode_ = scene::move::ScaleMode::Uniform;
                if (command == "tool.scale_nonuniform") scaleMode_ = scene::move::ScaleMode::NonUniform;
                if (command == "tool.scale_squash") scaleMode_ = scene::move::ScaleMode::Squash;
                UpdateTransformDisplay();
                return;
            }
        struct C { const char* cmd; bool x, y, z; };
        static const C kAxes[] = {{"constraint.x", 1, 0, 0}, {"constraint.y", 0, 1, 0}, {"constraint.z", 0, 0, 1},
                                  {"constraint.xy", 1, 1, 0}, {"constraint.yz", 0, 1, 1}, {"constraint.zx", 1, 0, 1}};
        for (const C& c : kAxes)
            if (command == c.cmd) {
                constraint_[0] = c.x; constraint_[1] = c.y; constraint_[2] = c.z;
                UpdateTransformDisplay();
                return;
            }
    }

    if (command == "edit.delete") {
        DeleteSelection();
        return;
    }

    if (command == "nav.min_max") {
        viewports_.ToggleMaximized();
        return;
    }
    // Navigation modes: clicking the active mode's button again leaves it.
    auto toggleMode = [this](NavMode m) { viewports_.SetNavMode(viewports_.Mode() == m ? NavMode::None : m); };
    if (command == "nav.zoom")        { toggleMode(NavMode::Zoom); return; }
    if (command == "nav.zoom_all")    { toggleMode(NavMode::ZoomAll); return; }
    if (command == "nav.region_zoom") { toggleMode(NavMode::RegionZoom); return; }
    if (command == "nav.pan")         { toggleMode(NavMode::Pan); return; }
    if (command.rfind("nav.arc_rotate", 0) == 0) { toggleMode(NavMode::ArcRotate); return; } // all three variants
    if (command == "nav.zoom_extents" || command == "nav.zoom_extents_sel") { viewports_.ZoomExtents(false); return; }
    if (command == "nav.zoom_extents_all" || command == "nav.zoom_extents_all_sel") { viewports_.ZoomExtents(true); return; }

    if (command == "file.exit") {
        // Same path as the close button, so a future "save changes?" prompt
        // in WM_CLOSE covers both.
        PostMessageW(Hwnd(), WM_CLOSE, 0, 0);
    } else if (command == "help.about") {
        const std::string version = MAXLIKE_VERSION;
        std::wstring text = L"maxlike " + std::wstring(version.begin(), version.end()) +
                            L"\n\nA 3D Studio MAX R3.1 look-alike for Windows.";
        MessageBoxW(Hwnd(), text.c_str(), L"About maxlike", MB_OK | MB_ICONINFORMATION);
    } else {
        // Not implemented yet: show which command fired on the prompt line.
        std::wstring name(command.begin(), command.end()); // command names are ASCII
        statusArea_.SetPrompt(1, name + L"  (not implemented yet)");
    }
}

// Before the Edit menu opens: show what Undo/Redo would do, and grey them when there
// is nothing to undo or redo. NOTE: R3's exact wording ("Undo Create"?) is not verified.
void MainFrame::UpdateEditMenu(HMENU menu) {
    auto update = [&](const char* command, const wchar_t* label, bool can, const std::string& what) {
        const UINT id = commands_.Find(command);
        if (!id || GetMenuState(menu, id, MF_BYCOMMAND) == static_cast<UINT>(-1)) return;
        std::wstring text = label;
        if (can) text += L" " + std::wstring(what.begin(), what.end());
        ModifyMenuW(menu, id, MF_BYCOMMAND | MF_STRING | (can ? MF_ENABLED : MF_GRAYED), id, text.c_str());
    };
    update("edit.undo", L"&Undo", undo_.CanUndo(), undo_.UndoName());
    update("edit.redo", L"&Redo", undo_.CanRedo(), undo_.RedoName());
}

void MainFrame::OnPaint() {
    // Only the gaps between regions are visible here; WS_CLIPCHILDREN keeps
    // this fill from painting over the child windows.
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(Hwnd(), &ps);
    FillRect(dc, &ps.rcPaint, theme::FaceBrush());

    // The frame around the tab panel and toolbar starts in the gap above the tab
    // panel (measured, rows 1..3): a white top-left corner and the two edge columns.
    RECT rc;
    GetClientRect(Hwnd(), &rc);
    const int gap = Scale(metrics::TopGap);
    if (rc.right > 4 && gap > 1) {
        const COLORREF hi = theme::kButtonHighlight.Ref();
        FillSolid(dc, {0, 1, 4, 2}, hi);
        FillSolid(dc, {0, 1, 1, gap}, hi);
        FillSolid(dc, {rc.right - 2, 1, rc.right - 1, gap}, theme::kButtonShadow.Ref());
    }
    EndPaint(Hwnd(), &ps);
}

LRESULT MainFrame::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        if (!CreateRegions())
            return -1; // cancels creation; CreateWindowExW returns null
        RecreateFonts();
        return 0;

    case WM_COMMAND:
        // HIWORD(wp): 0 = menu, 1 = accelerator, otherwise a control notification
        // (then lp is the control's HWND). Menus and accelerators carry the ID.
        if (HIWORD(wp) <= 1 && lp == 0) {
            const std::string& name = commands_.Name(LOWORD(wp));
            if (!name.empty()) {
                OnCommand(name);
                return 0;
            }
        }
        break;

    case WM_INITMENUPOPUP:
        // Sent just before a menu opens: the moment to update its items.
        UpdateEditMenu(reinterpret_cast<HMENU>(wp));
        break;

    case WM_SIZE:
        if (wp != SIZE_MINIMIZED)
            LayoutRegions(LOWORD(lp), HIWORD(lp));
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_GETMINMAXINFO: {
        auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize.x = Scale(metrics::MinWindowWidth);
        mmi->ptMinTrackSize.y = Scale(metrics::MinWindowHeight);
        return 0;
    }

    case WM_DPICHANGED: {
        RecreateFonts();
        const RECT* r = reinterpret_cast<const RECT*>(lp);
        // Resizing sends WM_SIZE, which re-lays out the regions at the new DPI.
        SetWindowPos(Hwnd(), nullptr, r->left, r->top,
                     r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(Hwnd(), nullptr, TRUE);
        return 0;
    }

    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        RedrawWindow(Hwnd(), nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return win::Window::HandleMessage(msg, wp, lp);
}

} // namespace ui
