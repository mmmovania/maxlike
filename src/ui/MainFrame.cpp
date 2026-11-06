#include "ui/MainFrame.h"
#include "ui/FrameLayout.h"
#include "ui/Bevel.h"
#include "scene/DemoScene.h"
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
void MainFrame::UpdateCreatePanel() {
    panelObject_ = creation_.TargetObject();
    if (!panelObject_) panelObject_ = template_;
    commandPanel_.SetParameterSource(panelObject_ ? &panelObject_->Params() : nullptr);
    const std::string name = creation_.TargetName();
    commandPanel_.SetObjectName(std::wstring(name.begin(), name.end()));
    commandPanel_.SetObjectColor(creation_.Color());
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

std::array<win::Window*, 7> MainFrame::Regions() {
    return {&tabPanel_, &mainToolbar_, &viewports_, &timeSlider_,
            &trackBar_, &commandPanel_, &statusArea_};
}

bool MainFrame::CreateRegions() {
    if (!tabPanel_.Create(Hwnd()) || !mainToolbar_.Create(Hwnd(), toolbar::MainToolbarConfig()) ||
        !timeSlider_.Create(Hwnd()) || !trackBar_.Create(Hwnd()) || !statusArea_.Create(Hwnd()) ||
        !viewports_.Create(Hwnd()) || !commandPanel_.Create(Hwnd()))
        return false;

    viewports_.SetScene(&scene_);

    // Creation: the checked Object Type button picks the primitive; viewports feed the
    // mouse; the swatch gives the colour, and shows the next one after each object.
    // NOTE: R3's prompt wording is unverified.
    creation_.SetColor(commandPanel_.ObjectColor());
    commandPanel_.onObjectColorChanged = [this](unsigned rgb) { creation_.SetColor(rgb); };
    commandPanel_.onCreateTypeChanged = [this](int type) {
        if (creation_.Cancel()) viewports_.Redraw();
        creation_.SetType(type);
        template_ = type >= 0 ? scene::MakePrimitive(type) : nullptr;
        UpdateCreatePanel();
        // Prompt verified against R3.1 (screenshot r3.1-box-created.png).
        statusArea_.SetPrompt(1, type >= 0 ? L"Click and drag to begin creation process"
                                           : L"Click or click-and-drag to select objects");
    };
    // Keyboard Entry's Create: the typed object, oriented by the active viewport.
    commandPanel_.onKeyboardCreate = [this](double x, double y, double z, const std::vector<std::pair<int, double>>& sizes) {
        std::vector<std::pair<size_t, double>> params;
        for (const auto& [index, value] : sizes) params.push_back({static_cast<size_t>(index), value});
        if (creation_.CreateDirect(params, viewports_.ActivePlane(), {x, y, z}).empty()) return;
        UpdateCreatePanel();
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
        viewports_.Redraw();
        return true;
    };
    viewports_.onCreateCancel = [this] {
        if (!creation_.Cancel()) return false;
        UpdateCreatePanel();
        viewports_.Redraw();
        return true;
    };

    tabPanel_.onTabChanged = [this](int index) {
        const auto& tab = tabstrip::Tabs()[static_cast<size_t>(index)];
        mainToolbar_.ShowStrip(index, tab.label);
    };
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

    if (command == "edit.undo" || command == "edit.redo") {
        const bool ok = command == "edit.undo" ? undo_.Undo(scene_) : undo_.Redo(scene_);
        if (ok) { viewports_.Redraw(); UpdateCreatePanel(); }
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
