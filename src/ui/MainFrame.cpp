#include "ui/MainFrame.h"
#include "scene/SoftSelection.h"
#include "scene/Groups.h"
#include "scene/Arrange.h"
#include "scene/TypeIn.h"
#include "scene/MeshTopology.h"
#include "scene/Summary.h"
#include "scene/Clone.h"
#include "ui/Dialogs.h"
#include "win/App.h"
#include "ui/RecentFiles.h"
#include <cctype>
#include <shellapi.h>
#include <commdlg.h>
#include "io/MaxImport.h"
#include "io/Merge.h"
#include "io/NativeFile.h"
#include "io/MaxViewports.h"
#include "ui/ModifiersDialog.h"
#include <cwchar>
#include <algorithm>
#include "ui/FrameLayout.h"
#include "ui/Bevel.h"
#include "scene/DemoScene.h"
#include "scene/MoveMath.h"
#include "ui/MenuBuilder.h"
#include "ui/MenuSpec.h"
#include "ui/TabStrip.h"
#include "ui/Theme.h"

namespace ui {

// The recently opened or saved files, most recent first, in the registry like other
// Windows programs (R3 keeps them in 3dsmax.ini).
static const wchar_t kRecentKey[] = L"Software\\maxlike\\Recent File List";


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
    for (int i = 1; i <= 9; ++i) commands_.Register("file.recent." + std::to_string(i));
    if (win::Window::Create(p)) {
        undo_.MarkClean();       // the startup scene counts as saved: closing it asks nothing
        InstallShortcuts();
        HKEY key;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, kRecentKey, 0, KEY_READ, &key) == ERROR_SUCCESS) {
            for (int i = 1; i <= 9; ++i) {
                wchar_t value[MAX_PATH * 2] = L"";
                DWORD size = sizeof(value), type = 0;
                const std::wstring name = L"File" + std::to_wstring(i);
                if (RegQueryValueExW(key, name.c_str(), nullptr, &type, reinterpret_cast<BYTE*>(value), &size) == ERROR_SUCCESS && type == REG_SZ && value[0])
                    recentFiles_.push_back(value);
            }
            RegCloseKey(key);
        }
        return true;
    }
    DestroyMenu(p.menuOrId); // creation failed: the menu was never attached
    return false;
}

// The Create panel follows the creation tool: the object being built or just created
// (else a template with the defaults), its name, and its colour (R3: the swatch shows
// the new object's colour until the next one is started).
// Verified wording: "None Selected" (reference) and "1 Object Selected" (Box screenshot);
// the plural form is assumed.
// The Transform Type-In follows the tool and the selection: Absolute shows the one selected
// object's values; with the Select tool nothing can be typed.
void MainFrame::UpdateTypeIn() {
    if (!typeIn_.IsOpen()) return;
    const bool transform = tool_ != Tool::Select;
    const auto tool = tool_ == Tool::Rotate ? scene::typein::Tool::Rotate : tool_ == Tool::Scale ? scene::typein::Tool::Scale : scene::typein::Tool::Move;
    const wchar_t* name = tool_ == Tool::Rotate ? L"Rotate" : tool_ == Tool::Scale ? L"Scale" : L"Move";
    const auto& sel = scene_.Selection();
    const scene::Node* one = sel.size() == 1 ? scene_.FindById(sel[0]) : nullptr;
    typeIn_.Update(name, transform && !sel.empty(), one != nullptr, one ? scene::typein::Absolute(tool, *one) : math::Vec3{0, 0, 0},
                   tool == scene::typein::Tool::Scale ? 100.0 : 0.0);
}

void MainFrame::UpdateSelectionStatus() {
    UpdateTypeIn();
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
    UpdateTypeIn();
    viewports_.SetAxesDisplay(tool_ != Tool::Select && showGizmo_ ? view::AxesStyle::MoveGizmo : view::AxesStyle::Tripod, constraint_);
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
    // In Edit Mesh at Vertex or Face level, Delete removes the selected vertices (with their
    // faces) or faces, as one undo step.
    if (const auto mod = VertexEditingMod()) {
        scene::EditMeshModifier::State after;
        if (subLevel_ >= 1) {
            // Edges delete the faces on both sides; faces, polygons and elements themselves.
            if ((subLevel_ == 1 ? mod->SelectedEdges() : mod->SelectedFaces()).empty()) return;
            // NOTE: R3's question and its wording are unverified.
            const int answer = MessageBoxW(Hwnd(), L"Delete isolated vertices?", kAppName, MB_YESNOCANCEL | MB_ICONQUESTION);
            if (answer == IDCANCEL) return;
            const scene::Mesh input = EditMeshInput(mod);
            after = subLevel_ == 1 ? mod->AfterDeletingEdges(input, answer == IDYES) : mod->AfterDeletingFaces(input, answer == IDYES);
        } else {
            if (mod->SelectedVertices().empty()) return;
            after = mod->AfterDeletingVertices();
        }
        undo_.Execute(std::make_unique<scene::EditMeshStateCommand>("Delete", mod, after), scene_);
        UpdateModifyPanel();
        viewports_.Redraw();
        return;
    }
    if (scene_.Selection().empty()) return;
    // A selected group goes with its members: select them all, then delete (one step).
    const auto all = scene::groups::WithMembers(scene_, scene_.Selection());
    auto macro = std::make_unique<scene::MacroCommand>("Delete");
    if (all.size() != scene_.Selection().size()) macro->Add(std::make_unique<scene::SelectCommand>(all));
    macro->Add(std::make_unique<scene::DeleteSelectionCommand>());
    undo_.Execute(std::move(macro), scene_);
    UpdateSelectionStatus();
    UpdateModifyPanel();
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
    if (commandPanel_.SelectedTab() == cmdpanel::kCreate && commandPanel_.CreateType() >= 0) {
        const std::string name = creation_.TargetName();
        return name.empty() ? nullptr : scene_.Find(name);
    }
    const auto& sel = scene_.Selection();
    return sel.size() == 1 ? scene_.FindById(sel[0]) : nullptr;
}

std::shared_ptr<scene::DerivedObject> MainFrame::SelectedStack(scene::Node** nodeOut) const {
    const auto& sel = scene_.Selection();
    scene::Node* node = sel.size() == 1 ? scene_.FindById(sel[0]) : nullptr;
    if (nodeOut) *nodeOut = node;
    return node ? std::dynamic_pointer_cast<scene::DerivedObject>(node->GetObject()) : nullptr;
}

std::shared_ptr<scene::EditMeshModifier> MainFrame::VertexEditingMod() const {
    if (!subObject_ || subLevel_ < 0 || subLevel_ > 4 || !commandPanel_.InModify()) return nullptr;
    return std::dynamic_pointer_cast<scene::EditMeshModifier>(CurrentModifier());
}

std::shared_ptr<scene::DeformModifier> MainFrame::GizmoEditingMod() const {
    if (!subObject_ || !commandPanel_.InModify()) return nullptr;
    return std::dynamic_pointer_cast<scene::DeformModifier>(CurrentModifier());
}

std::vector<uint32_t> MainFrame::SubObjectVertices(const std::shared_ptr<scene::EditMeshModifier>& mod) const {
    std::vector<uint32_t> verts = mod->SelectedVertices();
    if (subLevel_ >= 1) {
        verts.clear();
        const scene::Mesh ext = mod->Extended(EditMeshInput(mod));          // added geometry too
        if (subLevel_ == 1)
            for (uint32_t e : mod->SelectedEdges()) {
                uint32_t a, b;
                if (scene::EdgeVertices(ext, e, a, b)) { verts.push_back(a); verts.push_back(b); }
            }
        if (subLevel_ >= 2)
            for (uint32_t f : mod->SelectedFaces())
                if (f < ext.faces.size()) verts.insert(verts.end(), ext.faces[f].v, ext.faces[f].v + 3);
    }
    std::sort(verts.begin(), verts.end());
    verts.erase(std::unique(verts.begin(), verts.end()), verts.end());
    return verts;
}

scene::Mesh MainFrame::EditMeshInput(const std::shared_ptr<scene::EditMeshModifier>& mod) const {
    scene::Node* node = nullptr;
    const auto stack = const_cast<MainFrame*>(this)->SelectedStack(&node);
    if (!stack) return {};
    const auto& mods = stack->Modifiers();
    for (size_t i = 0; i < mods.size(); ++i)
        if (mods[i] == mod) return stack->EvaluateUpTo(i);
    return {};
}

std::shared_ptr<scene::Modifier> MainFrame::CurrentModifier() const {
    const auto stack = SelectedStack();
    if (!stack) return nullptr;
    const auto& mods = stack->Modifiers();
    if (modifyIndex_ < 0 || static_cast<size_t>(modifyIndex_) >= mods.size()) return nullptr;
    return mods[mods.size() - 1 - static_cast<size_t>(modifyIndex_)];   // the list shows the top first
}

// The Modify tab shows the one selected object: its stack (modifiers top first, then
// the base object), the entry chosen in the list, and that entry's parameters.
void MainFrame::UpdateModifyPanel() {
    ui::CommandPanel::ModifyInfo info;
    scene::Node* node = nullptr;
    const auto stack = SelectedStack(&node);
    if (node && node->GetObject()) {
        info.valid = true;
        const std::shared_ptr<scene::Object> base = stack ? stack->Base() : node->GetObject();
        if (stack)
            for (auto it = stack->Modifiers().rbegin(); it != stack->Modifiers().rend(); ++it)
                info.stack.push_back(FromUtf8((*it)->ClassName()));
        info.stack.push_back(FromUtf8(base ? base->ClassName() : std::string("Object")));
        modifyIndex_ = std::clamp(modifyIndex_, 0, static_cast<int>(info.stack.size()) - 1);
        info.current = modifyIndex_;
        const auto mod = CurrentModifier();
        info.currentIsModifier = mod != nullptr;
        if (mod) info.currentClass = mod->ClassName();
        info.currentEnabled = mod ? mod->Enabled() : true;
        info.params = mod ? &mod->Params() : (base ? &base->Params() : nullptr);
        for (int t = 0; t < 10 && base; ++t)
            if (base->ClassName() == scene::PrimitiveBaseName(t)) info.baseType = t;
        if (std::dynamic_pointer_cast<scene::EditMeshModifier>(mod))
            info.subObjectLevels = {L"Vertex", L"Edge", L"Face", L"Polygon", L"Element"};
        if (std::dynamic_pointer_cast<scene::DeformModifier>(mod)) info.subObjectLevels = {L"Gizmo", L"Center"};
        if (info.subObjectLevels.empty()) subObject_ = false;
        info.subObjectOn = subObject_;
        info.subObjectLevel = subLevel_;
        if (const auto em = std::dynamic_pointer_cast<scene::EditMeshModifier>(mod)) {
            // NOTE: R3's wording for one vertex or none is unverified.
            // NOTE: R3's wording at Face level is unverified.
            // (Polygon and Element count faces here; NOTE: R3's wording at those levels too.)
            info.selectionText = subLevel_ == 0 ? std::to_wstring(em->SelectedVertices().size()) + L" Vertices Selected"
                               : subLevel_ == 1 ? std::to_wstring(em->SelectedEdges().size()) + L" Edges Selected"
                                                : std::to_wstring(em->SelectedFaces().size()) + L" Faces Selected";
        }
    }
    commandPanel_.SetModifyInfo(info);
    // A deformer shown in the Modify panel draws its gizmo; at sub-object level it is edited.
    {
        ui::Viewport::GizmoEditing gizmo;
        scene::Node* gnode = nullptr;
        const auto gstack = commandPanel_.InModify() ? SelectedStack(&gnode) : nullptr;
        if (const auto dm = std::dynamic_pointer_cast<scene::DeformModifier>(commandPanel_.InModify() ? CurrentModifier() : nullptr); dm && gstack && gnode) {
            const auto& mods = gstack->Modifiers();
            for (size_t i = 0; i < mods.size(); ++i)
                if (mods[i] == dm) gizmo.index = i;
            gizmo.on = true;
            gizmo.node = gnode->GetId();
            gizmo.stack = gstack;
            gizmo.mod = dm;
            gizmo.level = subObject_ ? subLevel_ : -1;
        }
        viewports_.SetGizmoEditing(gizmo);
    }
    // The views draw and pick vertices while an Edit Mesh is at Vertex level.
    if (!VertexEditingMod() || subLevel_ != 0) weldTargetMode_ = false;   // Weld > Target ends with the level
    ui::Viewport::VertexEditing editing;
    if (const auto em = VertexEditingMod()) {
        editing.on = true;
        editing.node = node->GetId();
        editing.stack = stack;
        editing.mod = em;
        editing.level = subLevel_;
        editing.weldTarget = weldTargetMode_ && subLevel_ == 0;
        const auto& mods = stack->Modifiers();
        for (size_t i = 0; i < mods.size(); ++i)
            if (mods[i] == em) editing.upTo = i + 1;
    }
    viewports_.SetVertexEditing(editing);
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

// Read with Win32 (wide paths on every compiler); a message if it fails.
bool MainFrame::ReadWholeFile(const std::wstring& path, std::vector<uint8_t>& bytes) {
    bytes.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER size{};
        if (GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < (1ll << 31)) {
            bytes.resize(static_cast<size_t>(size.QuadPart));
            DWORD read = 0;
            if (!ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) || read != bytes.size()) bytes.clear();
        }
        CloseHandle(file);
    }
    if (bytes.empty()) {
        const std::wstring text = L"Could not read " + path + L".";
        MessageBoxW(Hwnd(), text.c_str(), kAppName, MB_OK | MB_ICONWARNING);
        return false;
    }
    return true;
}

bool MainFrame::OpenFile(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring fileName = slash == std::wstring::npos ? path : path.substr(slash + 1);
    std::vector<uint8_t> bytes;
    if (!ReadWholeFile(path, bytes)) return false;
    // Import into a new scene first: a file that cannot be read leaves the current one.
    // maxlike's own files are recognised by their signature, whatever their name.
    const bool native = io::IsNativeFile(bytes);
    scene::Scene imported;
    io::ImportReport report;
    std::string error;
    io::NativeViews savedViews;                       // .mlk files keep the views (10.4)
    if (native ? !io::LoadNative(bytes, imported, error, &savedViews) : !io::ImportMax(bytes, imported, report, error)) {
        const std::wstring text = L"Could not open " + fileName + L":\n" + std::wstring(error.begin(), error.end());
        MessageBoxW(Hwnd(), text.c_str(), kAppName, MB_OK | MB_ICONWARNING);
        return false;
    }
    creation_.Cancel();
    scene_ = std::move(imported);
    undo_.Clear();                                  // like a new document: nothing to undo
    undo_.MarkClean();
    modifyIndex_ = 0;
    subObject_ = false;
    vertexMove_ = {};
    viewports_.SetScene(&scene_);
    UpdateSelectionStatus();
    UpdateModifyPanel();
    UpdateCreatePanel();
    viewports_.Redraw();
    if (native) report.nodes = scene_.Nodes().size();
    docPath_ = native ? path : std::wstring();       // Save writes .mlk files only
    // R3 restores the viewports saved in the file; so do maxlike's own files.
    if (native && savedViews.present) {
        for (int i = 0; i < 4; ++i)
            viewports_.ApplySavedView(i, static_cast<ViewType>(savedViews.type[i]), savedViews.ortho[i], savedViews.perspective[i]);
        viewports_.SetActive(savedViews.active);
    }
    if (!native) {
        std::vector<io::MaxViewport> views;
        std::string ignored;
        io::ReadViewports(bytes, views, ignored);
        for (const io::MaxViewport& v : views) {
            static const int kTypes[8] = {static_cast<int>(ViewType::Left), static_cast<int>(ViewType::Right),
                                          static_cast<int>(ViewType::Top), static_cast<int>(ViewType::Bottom),
                                          static_cast<int>(ViewType::Front), static_cast<int>(ViewType::Back), -1,
                                          static_cast<int>(ViewType::Perspective)};
            const int type = v.type >= 0 && v.type < 8 ? kTypes[v.type] : -1;
            if (type < 0) {
                report.notes.push_back("viewport " + std::to_string(v.slot + 1) + ": a User or Camera view is not supported; left as it was");
                continue;
            }
            const auto viewType = static_cast<ViewType>(type);
            viewports_.ApplySavedView(v.slot, viewType,
                                      viewType == ViewType::Perspective ? view::OrthoView{} : io::ToOrthoView(v),
                                      viewType == ViewType::Perspective ? io::ToPerspectiveView(v) : view::PerspectiveView{});
        }
    }
    SetDocumentName(fileName);
    AddRecentFile(path);
    std::wstring prompt = L"Opened " + fileName + L": " + std::to_wstring(report.nodes) +
                          (report.nodes == 1 ? L" object" : L" objects");
    if (!report.notes.empty()) prompt += L" (" + std::to_wstring(report.notes.size()) + L" import notes)";
    statusArea_.SetPrompt(1, prompt);
    if (!report.notes.empty()) {
        // What could not be imported, as R3 reports missing plug-ins on opening.
        std::wstring text = L"Some parts of " + fileName + L" could not be imported:\n\n";
        for (size_t i = 0; i < report.notes.size() && i < 20; ++i) {
            const std::string& n = report.notes[i];
            text += L"\x2022 " + std::wstring(n.begin(), n.end()) + L"\n";
        }
        if (report.notes.size() > 20) text += L"\x2026 and " + std::to_wstring(report.notes.size() - 20) + L" more\n";
        MessageBoxW(Hwnd(), text.c_str(), kAppName, MB_OK | MB_ICONINFORMATION);
    }
    return true;
}

bool MainFrame::SaveFile(const std::wstring& path) {
    // The scene with its selection, and the four views (Phase 10.4).
    io::NativeViews views;
    views.present = true;
    views.active = viewports_.Active();
    for (int i = 0; i < 4; ++i) {
        const ui::Viewport& v = viewports_.ViewAt(i);
        views.type[i] = static_cast<int>(v.Type());
        views.ortho[i] = v.OrthoState();
        views.perspective[i] = v.PerspectiveState();
    }
    std::vector<scene::Node::Id> all;
    for (const auto& n : scene_.Nodes()) all.push_back(n->GetId());
    const std::vector<uint8_t> bytes = io::SaveNative(scene_, all, &views);
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written = 0;
    const bool ok = file != INVALID_HANDLE_VALUE &&
                    WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (!ok) {
        const std::wstring text = L"Could not save " + path + L".";
        MessageBoxW(Hwnd(), text.c_str(), kAppName, MB_OK | MB_ICONWARNING);
        return false;
    }
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring fileName = slash == std::wstring::npos ? path : path.substr(slash + 1);
    docPath_ = path;
    undo_.MarkClean();
    SetDocumentName(fileName);
    AddRecentFile(path);
    statusArea_.SetPrompt(1, L"Saved " + fileName);
    return true;
}

bool MainFrame::SaveAs() {
    // The suggested name: the document's, with maxlike's extension.
    std::wstring suggested = docName_;
    const size_t dot = suggested.find_last_of(L'.');
    if (dot != std::wstring::npos) suggested.resize(dot);
    suggested += L".mlk";
    wchar_t path[MAX_PATH] = L"";
    wcsncpy_s(path, suggested.c_str(), _TRUNCATE);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = Hwnd();
    ofn.lpstrFilter = L"maxlike (*.mlk)\0*.mlk\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Save File As";
    ofn.lpstrDefExt = L"mlk";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    return GetSaveFileNameW(&ofn) && SaveFile(path);
}

bool MainFrame::ConfirmDiscard() {
    if (undo_.IsClean()) return true;
    // NOTE: R3's wording for this question is unverified.
    const std::wstring text = L"The scene " + docName_ + L" has been modified.\nDo you want to save your changes?";
    switch (MessageBoxW(Hwnd(), text.c_str(), kAppName, MB_YESNOCANCEL | MB_ICONQUESTION)) {
    case IDYES: return docPath_.empty() ? SaveAs() : SaveFile(docPath_);
    case IDNO: return true;
    default: return false;
    }
}

void MainFrame::ResetScene() {
    creation_.Cancel();
    scene_ = scene::Scene();
    undo_.Clear();
    undo_.MarkClean();
    modifyIndex_ = 0;
    subObject_ = false;
    vertexMove_ = {};
    docPath_.clear();
    viewports_.SetScene(&scene_);
    UpdateSelectionStatus();
    UpdateModifyPanel();
    UpdateCreatePanel();
    viewports_.Redraw();
    SetDocumentName(L"Untitled");
}

std::wstring MainFrame::AskForSceneFile(const wchar_t* title) {
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = Hwnd();
    ofn.lpstrFilter = L"Scenes (*.max, *.mlk)\0*.max;*.mlk\0" L"3D Studio MAX (*.max)\0*.max\0" L"maxlike (*.mlk)\0*.mlk\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = L"max";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    return GetOpenFileNameW(&ofn) ? std::wstring(path) : std::wstring();
}

void MainFrame::SelectNodes(std::vector<scene::Node::Id> ids) {
    undo_.Execute(std::make_unique<scene::SelectCommand>(std::move(ids)), scene_);
    modifyIndex_ = 0;
    subObject_ = false;
    UpdateSelectionStatus();
    UpdateModifyPanel();
    viewports_.Redraw();
}

// File > Merge: another file's objects added to this scene, as one undoable step. Names
// already in the scene get the next free number (R3 asks in a dialog; NOTE: not built).
bool MainFrame::MergeFile(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring fileName = slash == std::wstring::npos ? path : path.substr(slash + 1);
    std::vector<uint8_t> bytes;
    if (!ReadWholeFile(path, bytes)) return false;
    io::ImportReport report;
    std::vector<scene::Node::Id> ids;
    std::string error;
    if (!io::MergeScene(bytes, scene_, report, ids, error)) {
        const std::wstring text = L"Could not merge " + fileName + L":\n" + std::wstring(error.begin(), error.end());
        MessageBoxW(Hwnd(), text.c_str(), kAppName, MB_OK | MB_ICONWARNING);
        return false;
    }
    undo_.Execute(std::make_unique<scene::AddedNodesCommand>("Merge", ids), scene_);
    viewports_.Redraw();
    statusArea_.SetPrompt(1, L"Merged " + fileName + L": " + std::to_wstring(ids.size()) + (ids.size() == 1 ? L" object" : L" objects"));
    if (!report.notes.empty()) {
        std::wstring text = L"Merging " + fileName + L":\n\n";
        for (size_t i = 0; i < report.notes.size() && i < 20; ++i) text += L"\x2022 " + std::wstring(report.notes[i].begin(), report.notes[i].end()) + L"\n";
        MessageBoxW(Hwnd(), text.c_str(), kAppName, MB_OK | MB_ICONINFORMATION);
    }
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
        const auto i = static_cast<size_t>(index);
        if (commandPanel_.InModify()) {
            // The Modify tab: the shown stack entry's parameters, one undo step per change
            // (a drag previews live and is recorded once, from where it started).
            const auto mod = CurrentModifier();
            scene::Node* node = nullptr;
            const auto stack = SelectedStack(&node);
            std::shared_ptr<scene::Object> base = stack ? stack->Base() : (node ? node->GetObject() : nullptr);
            scene::ParamBlock* block = mod ? &mod->Params() : (base ? &base->Params() : nullptr);
            if (!block) return;
            if (!final) {
                if (!paramDragging_) { paramDragging_ = true; paramDragStart_ = block->Float(i); }
                block->Set(i, value);
            } else {
                if (paramDragging_) block->Set(i, paramDragStart_);
                paramDragging_ = false;
                if (block->Float(i) != value) {
                    if (mod) undo_.Execute(std::make_unique<scene::SetParamCommand>(mod, i, value), scene_);
                    else undo_.Execute(std::make_unique<scene::SetParamCommand>(base, i, value), scene_);
                }
            }
            viewports_.Redraw();
            commandPanel_.Refresh();
            return;
        }
        std::shared_ptr<scene::Object> obj = creation_.TargetObject();
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
    viewports_.onSelect = [this](const std::vector<scene::Node::Id>& picked, bool, bool ctrl, bool alt) {
        if (attachPick_) {
            // Group > Attach: the clicked object's group receives the selection.
            attachPick_ = false;
            scene::Node* target = picked.empty() ? nullptr : scene_.FindById(picked.front());
            while (target && !target->IsGroupHead()) target = target->Parent();
            auto macro = target ? scene::groups::MakeAttach(scene_, scene_.Selection(), target->GetId()) : nullptr;
            if (!macro) { statusArea_.SetPrompt(1, L"Attach: that is not a group"); return; }   // NOTE: R3's wording unverified
            undo_.Execute(std::move(macro), scene_);
            UpdateSelectionStatus();
            UpdateModifyPanel();
            viewports_.Redraw();
            return;
        }
        if (alignPick_) {
            // Tools > Align (10.2): the clicked object (its group, if closed) is the target.
            alignPick_ = false;
            scene::Node* target = picked.empty() ? nullptr : scene::groups::SelectableFor(scene_.FindById(picked.front()));
            if (!target || scene_.IsSelected(target->GetId())) { statusArea_.SetPrompt(1, L"Align: pick an object that is not selected"); return; }
            const std::wstring name(target->Name().begin(), target->Name().end());
            const ui::AlignChoice c = ui::ShowAlignDialog(Hwnd(), name);
            if (!c.ok) return;
            scene::tools::AlignOptions o;
            for (int i = 0; i < 3; ++i) { o.position[i] = c.position[i]; o.orientation[i] = c.orientation[i]; o.scale[i] = c.scale[i]; }
            o.current = static_cast<scene::tools::AlignPoint>(c.current);
            o.target = static_cast<scene::tools::AlignPoint>(c.target);
            auto macro = std::make_unique<scene::MacroCommand>("Align");
            const auto& sel = scene_.Selection();
            for (scene::Node::Id id : sel) {
                scene::Node* n = scene_.FindById(id);
                bool under = false;                   // a selected child moves with its parent
                for (const scene::Node* p = n ? n->Parent() : nullptr; p && !under; p = p->Parent()) under = scene_.IsSelected(p->GetId());
                if (!n || under) continue;
                macro->Add(std::make_unique<scene::TransformCommand>(id, n->Transform(), scene::tools::Aligned(*n, *target, o), "Align"));
            }
            undo_.Execute(std::move(macro), scene_);
            viewports_.Redraw();
            return;
        }
        // Clicking a member of a closed group selects the group (Phase 10).
        const std::vector<scene::Node::Id> ids = scene::groups::Selectable(scene_, picked);
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
        modifyIndex_ = 0;
        subObject_ = false;
        UpdateModifyPanel();
        UpdateCreatePanel();
        viewports_.Redraw();
    };
    viewports_.onDeleteKey = [this] { DeleteSelection(); };

    // The Modify tab (Phase 7).
    commandPanel_.onTabSelected = [this](int tab) {
        if (tab == cmdpanel::kModify && commandPanel_.CreateType() >= 0) commandPanel_.SetCreateType(-1);  // R3 leaves creation
        modifyIndex_ = 0;
        subObject_ = false;
        UpdateModifyPanel();
        UpdateCreatePanel();
    };
    commandPanel_.onModifierButton = [this](const std::wstring& button) {
        scene::Node* node = nullptr;
        SelectedStack(&node);
        std::shared_ptr<scene::Modifier> m;
        std::wstring name = button;
        if (button == L"More...") {
            name = ui::ShowModifiersDialog(Hwnd());
            if (name.empty()) return;                           // cancelled
        }
        if (name == L"Edit Mesh" && node) {
            // Edit Mesh records the vertex count below it: its edits only apply while
            // the mesh keeps that count (as in R3 files).
            const auto stack = SelectedStack();
            const size_t count = stack ? stack->Evaluate().verts.size() : node->GetObject()->Evaluate().verts.size();
            m = std::make_shared<scene::EditMeshModifier>(count, std::vector<scene::EditMeshModifier::Move>{});
        }
        if (button == L"Bend") m = std::make_shared<scene::BendModifier>();
        if (button == L"Taper") m = std::make_shared<scene::TaperModifier>();
        if (button == L"Twist") m = std::make_shared<scene::TwistModifier>();
        if (!m || !node) {
            statusArea_.SetPrompt(1, name + L" (not implemented yet)");
            return;
        }
        undo_.Execute(std::make_unique<scene::AddModifierCommand>(node->GetId(), m), scene_);
        modifyIndex_ = 0;                                       // the new modifier is shown
        subObject_ = false;
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    commandPanel_.onStackSelect = [this](int index) {
        modifyIndex_ = index;
        subObject_ = false;                                     // R3 leaves sub-object mode
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    commandPanel_.onSubObject = [this](bool on) {
        subObject_ = on;
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    commandPanel_.onRolloutButton = [this](const std::wstring& label) {
        if (label == L"Delete" && VertexEditingMod()) { DeleteSelection(); return; }   // Edit Geometry > Delete
        // Weld > Target (9.7): a mode at Vertex level, until pressed again.
        if (label == L"Target" && VertexEditingMod()) {
            if (subLevel_ != 0) { statusArea_.SetPrompt(1, L"Weld works at Vertex level"); return; }
            weldTargetMode_ = !weldTargetMode_;
            statusArea_.SetPrompt(1, weldTargetMode_ ? L"Drag a vertex onto another vertex to weld it" : L"");   // NOTE: R3's prompt unverified
            UpdateModifyPanel();
            return;
        }
        // Detach (9.7): the selected faces become a new Editable Mesh in the same place.
        if (label == L"Detach") {
            const auto mod = VertexEditingMod();
            if (!mod || subLevel_ < 2) { statusArea_.SetPrompt(1, L"Detach works on faces"); return; }
            if (mod->SelectedFaces().empty()) return;
            scene::Node* node = nullptr;
            SelectedStack(&node);
            if (!node) return;
            const std::string suggested = scene_.UniqueName("Object");
            const ui::DetachChoice choice = ui::ShowDetachDialog(Hwnd(), std::wstring(suggested.begin(), suggested.end()));
            if (!choice.ok) return;
            std::string name;
            for (wchar_t c : choice.name) name += static_cast<char>(c < 0x80 ? c : '?');
            if (name.empty()) name = suggested;
            const scene::Mesh input = EditMeshInput(mod);
            scene::Node* part = scene_.CreateNode(name);
            part->SetObject(std::make_shared<scene::MeshObject>(mod->Extract(input, mod->SelectedFaces())));
            part->SetWireColor(node->WireColor());
            scene::Scene::SetWorldMatrix(part, node->WorldMatrix());   // top level, exactly where the faces were
            part->ObjectOffset() = node->ObjectOffset();
            auto macro = std::make_unique<scene::MacroCommand>("Detach");
            macro->Add(std::make_unique<scene::AddedNodesCommand>("Detach", std::vector<scene::Node::Id>{part->GetId()}));
            if (!choice.asClone) macro->Add(std::make_unique<scene::EditMeshStateCommand>("Detach", mod, mod->AfterDeletingFaces(input, true)));
            undo_.Execute(std::move(macro), scene_);
            UpdateModifyPanel();
            viewports_.Redraw();
            return;
        }
        // Edit Geometry (10.7): Remove Isolated Vertices, Select Open Edges, Collapse, Make
        // Planar, View Align, Grid Align; one undo step each.
        if (const auto mod = VertexEditingMod(); mod && (label == L"Remove Isolated Vertices" || label == L"Select Open Edges" ||
                                                          label == L"Collapse" || label == L"Make Planar" || label == L"View Align" ||
                                                          label == L"Grid Align")) {
            const scene::Mesh input = EditMeshInput(mod);
            const std::vector<uint32_t> verts = SubObjectVertices(mod);
            scene::Node* node = nullptr;
            SelectedStack(&node);
            const math::Mat4 toObject = node ? math::AffineInverse(node->ObjectMatrix()) : math::Translation({0, 0, 0});
            auto direction = [&](const math::Vec3& world) {           // a world direction in object space
                return math::TransformPoint(toObject, world) - math::TransformPoint(toObject, {0, 0, 0});
            };
            scene::EditMeshModifier::State after;
            if (label == L"Remove Isolated Vertices") after = mod->AfterRemovingIsolated(input);
            else if (label == L"Select Open Edges") after = mod->AfterSelectingOpenEdges(input);
            else {
                if (verts.empty()) return;
                if (label == L"Collapse") after = mod->AfterCollapsing(input, verts);
                else if (label == L"Make Planar") after = mod->AfterMakingPlanar(input, verts);
                else if (label == L"View Align") after = mod->AfterAligning(input, verts, direction(viewports_.ActiveView().OutOfScreen()), nullptr);
                else {
                    const scene::ConstructionPlane plane = viewports_.ActivePlane();
                    const math::Vec3 at = math::TransformPoint(toObject, {0, 0, 0});   // the plane passes through the world origin
                    after = mod->AfterAligning(input, verts, direction(plane.normal), &at);
                }
            }
            std::string name;
            for (wchar_t c : label) name += static_cast<char>(c);
            undo_.Execute(std::make_unique<scene::EditMeshStateCommand>(name, mod, after), scene_);
            UpdateModifyPanel();
            viewports_.Redraw();
            return;
        }
        // Selection > Hide / Unhide All, and Edit Geometry > Weld > Selected (Phase 9.6).
        if (const auto mod = VertexEditingMod(); mod && (label == L"Hide" || label == L"Unhide All" || label == L"Selected")) {
            const bool faces = subLevel_ >= 1;
            scene::EditMeshModifier::State after;
            std::string name;
            if (label == L"Hide") {
                if (subLevel_ == 1) { statusArea_.SetPrompt(1, L"Hide works on vertices and faces"); return; }   // NOTE: R3 at Edge level unverified
                if ((faces ? mod->SelectedFaces() : mod->SelectedVertices()).empty()) return;
                after = mod->AfterHiding(faces);
                name = "Hide";
            } else if (label == L"Unhide All") {
                if ((faces ? mod->HiddenFaces() : mod->HiddenVertices()).empty()) return;
                after = mod->AfterUnhidingAll(faces);
                name = "Unhide All";
            } else {
                if (subLevel_ != 0) { statusArea_.SetPrompt(1, L"Weld works at Vertex level"); return; }
                size_t merged = 0;
                after = mod->AfterWelding(EditMeshInput(mod), mod->Params().Float(scene::EditMeshModifier::kWeldThreshold, 0.1), &merged);
                if (merged == 0) { statusArea_.SetPrompt(1, L"No vertices within weld threshold"); return; }   // NOTE: R3's wording unverified
                name = "Weld";
            }
            undo_.Execute(std::make_unique<scene::EditMeshStateCommand>(name, mod, after), scene_);
            UpdateModifyPanel();
            viewports_.Redraw();
            return;
        }
        statusArea_.SetPrompt(1, label + L" (not implemented yet)");
    };
    commandPanel_.onSubObjectLevel = [this](int level) {
        subLevel_ = level;
        subObject_ = true;                                      // a level icon or the combo enters sub-object mode
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    // Vertex selection: a click replaces it (Ctrl adds, Alt removes); one "Select" step.
    viewports_.onWeldTarget = [this](uint32_t from, uint32_t to) {
        const auto mod = VertexEditingMod();
        if (!mod) return;
        const auto after = mod->AfterWeldingTarget(from, to);
        if (after.welds.size() == mod->Welds().size()) return;      // nothing to weld
        undo_.Execute(std::make_unique<scene::EditMeshStateCommand>("Weld", mod, after), scene_);
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    viewports_.onVertexSelect = [this](const std::vector<uint32_t>& verts, bool, bool ctrl, bool alt) {
        const auto mod = VertexEditingMod();
        if (!mod) return;
        // Vertices, edges or faces (Edit Mesh's input indices), by the level shown; Polygon
        // and Element select faces.
        const int kind = subLevel_ == 0 ? 0 : subLevel_ == 1 ? 1 : 2;
        const std::vector<uint32_t>& current = kind == 0 ? mod->SelectedVertices() : kind == 1 ? mod->SelectedEdges() : mod->SelectedFaces();
        std::vector<uint32_t> next = current;
        if (ctrl) next.insert(next.end(), verts.begin(), verts.end());
        else if (alt) next.erase(std::remove_if(next.begin(), next.end(), [&](uint32_t v) {
                                     return std::find(verts.begin(), verts.end(), v) != verts.end(); }), next.end());
        else next = verts;
        std::sort(next.begin(), next.end());
        next.erase(std::unique(next.begin(), next.end()), next.end());
        if (next == current) return;
        auto state = mod->GetState();
        (kind == 0 ? state.selectedVertices : kind == 1 ? state.selectedEdges : state.selectedFaces) = next;
        undo_.Execute(std::make_unique<scene::EditMeshStateCommand>("Select", mod, state), scene_);
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    commandPanel_.onStackButton = [this](int button) {
        scene::Node* node = nullptr;
        const auto stack = SelectedStack(&node);
        const auto mod = CurrentModifier();
        if (button == 0 && mod) {
            undo_.Execute(std::make_unique<scene::EnableModifierCommand>(mod, !mod->Enabled()), scene_);
        } else if (button == 3 && mod && stack && node) {
            const size_t bottomIndex = stack->Modifiers().size() - 1 - static_cast<size_t>(modifyIndex_);
            undo_.Execute(std::make_unique<scene::RemoveModifierCommand>(node->GetId(), bottomIndex), scene_);
            modifyIndex_ = 0;
        } else if (button != 0 && button != 3) {
            static const wchar_t* kNames[5] = {L"", L"Show End Result", L"Make Unique", L"", L"Edit Stack"};
            statusArea_.SetPrompt(1, std::wstring(kNames[button]) + L" (not implemented yet)");
            return;
        } else {
            return;
        }
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    // Moving: a live preview from the starting positions, then one "Move" undo step.
    viewports_.onMoveBegin = [this] {
        moving_.clear();
        if (const auto dm = GizmoEditingMod()) {
            scene::Node* node = nullptr;
            SelectedStack(&node);
            if (!node) return;
            gizmoMove_ = {true, dm, dm->GetGizmo(), node->ObjectMatrix(),
                          scene::typein::WorldRotation(*node) * node->ObjectOffset().rotation};
            dragFrame_ = viewports_.ActiveView().CurrentFrame();
            return;
        }
        if (const auto mod = VertexEditingMod()) {
            scene::Node* node = nullptr;
            SelectedStack(&node);
            // The selection's vertices (edges' and faces' too, added geometry included).
            const std::vector<uint32_t> verts = SubObjectVertices(mod);
            vertexMove_ = {true, mod, mod->Moves(), verts, math::AffineInverse(node->ObjectMatrix()), {}, node->ObjectMatrix(), {}, {}};
            // Positions now: the input with the current moves; the centre is the vertices'
            // average, or the reference system's origin (Use Transform Coordinate Center).
            // NOTE: R3's sub-object centre rules are unverified.
            const scene::Mesh at = mod->Positions(EditMeshInput(mod));    // added geometry too (10.7)
            math::Vec3 sum{0, 0, 0};
            for (uint32_t v : verts) {
                const math::Vec3 p = v < at.verts.size() ? at.verts[v] : math::Vec3{0, 0, 0};
                vertexMove_.points.push_back(p);
                sum = sum + math::TransformPoint(vertexMove_.objectToWorld, p);
            }
            dragFrame_ = viewports_.ActiveView().CurrentFrame();
            vertexMove_.centre = centre_ == 2 ? dragFrame_.origin : verts.empty() ? sum : sum * (1.0 / static_cast<double>(verts.size()));
            // Soft Selection (10.6): every vertex the falloff reaches joins, with its weight;
            // the centre stays the selection's.
            const auto& sp = mod->Params();
            if (sp.Int(scene::EditMeshModifier::kUseSoftSelection, 0) != 0) {
                const int edges = sp.Int(scene::EditMeshModifier::kEdgeDistance, 0) != 0 ? sp.Int(scene::EditMeshModifier::kEdgeIterations, 1) : -1;
                const auto w = scene::soft::Weights(at, verts, sp.Float(scene::EditMeshModifier::kFalloff, 20),
                                                    sp.Float(scene::EditMeshModifier::kPinch, 0), sp.Float(scene::EditMeshModifier::kBubble, 0), edges);
                vertexMove_.vertices.clear();
                vertexMove_.points.clear();
                for (uint32_t v = 0; v < w.size(); ++v)
                    if (w[v] > 0) {
                        vertexMove_.vertices.push_back(v);
                        vertexMove_.points.push_back(at.verts[v]);
                        vertexMove_.weights.push_back(w[v]);
                    }
            }
            return;
        }
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
        // The frame and centre of the view being dragged in, fixed for the whole drag.
        dragFrame_ = viewports_.ActiveView().CurrentFrame();
        dragCentre_ = viewports_.ActiveView().TransformCenter();
    };
    // Move follows the mouse in the world; Rotate and Scale follow its vertical movement,
    // each object about its own pivot (NOTE: R3's pivot options are not implemented yet).
    viewports_.onMoveDrag = [this](const math::Vec3& delta, int up) {
        if (gizmoMove_.active) {
            // In the object's space: Move shifts the gizmo (Gizmo) or its centre (Center);
            // Rotate turns the gizmo about its centre; Scale scales it (Gizmo level only).
            const math::Mat4 inv = math::AffineInverse(gizmoMove_.objectWorld);
            const math::Vec3 d = math::TransformPoint(inv, delta) - math::TransformPoint(inv, {0, 0, 0});
            scene::DeformModifier::Gizmo g = gizmoMove_.start;
            if (tool_ == Tool::Move) {
                if (subLevel_ == 0) g.transform.position = g.transform.position + d;
                else {
                    const math::Mat4 gi = math::AffineInverse(g.transform.Matrix());
                    g.center = g.center + (math::TransformPoint(gi, d) - math::TransformPoint(gi, {0, 0, 0}));
                }
                ShowCoordinates(delta);
            } else if (subLevel_ == 0 && tool_ == Tool::Rotate) {
                const int axis = scene::move::RotationAxis(constraint_);
                const math::Quat turn = scene::move::DragRotationInFrame(axis, up, dragFrame_);
                const math::Quat& o = gizmoMove_.objectRotation;
                const math::Quat q = math::Quat{-o.x, -o.y, -o.z, o.w} * turn * o;        // the turn in object space
                const math::Vec3 pc = gizmoMove_.mod->CenterPoint();
                g.transform.position = pc + q.Rotate(g.transform.position - pc);
                g.transform.rotation = (q * g.transform.rotation).Normalized();
                ShowCoordinates(scene::move::RotationDegrees(axis, up));
            } else if (subLevel_ == 0 && tool_ == Tool::Scale) {
                const math::Vec3 f = scene::move::DragScale(scaleMode_, constraint_, up);
                g.transform.scale = {g.transform.scale.x * f.x, g.transform.scale.y * f.y, g.transform.scale.z * f.z};
                ShowCoordinates(scene::move::ScalePercent(f));
            }
            gizmoMove_.mod->SetGizmo(g);
            viewports_.Redraw();
            return;
        }
        if (vertexMove_.active) {
            // Move: one offset in the object's space. Rotate and Scale (10.5): a world-space
            // turn or scale about the centre, in the reference system's frame, applied to each
            // vertex in world space and taken back to object space.
            const auto& wts = vertexMove_.weights;
            if (tool_ == Tool::Move) {
                const math::Vec3 d = math::TransformPoint(vertexMove_.worldToObject, delta) -
                                     math::TransformPoint(vertexMove_.worldToObject, {0, 0, 0});
                if (wts.empty()) {
                    vertexMove_.mod->SetMoves(scene::EditMeshModifier::AddToMoves(vertexMove_.before, vertexMove_.vertices, d));
                } else {
                    std::vector<math::Vec3> deltas;
                    for (double w : wts) deltas.push_back(d * w);
                    vertexMove_.mod->SetMoves(scene::EditMeshModifier::AddToMoves(vertexMove_.before, vertexMove_.vertices, deltas));
                }
                ShowCoordinates(delta);
            } else {
                const math::Vec3& c = vertexMove_.centre;
                math::Mat4 change;
                if (tool_ == Tool::Rotate) {
                    const int axis = scene::move::RotationAxis(constraint_);
                    change = math::Translation(c) * scene::move::DragRotationInFrame(axis, up, dragFrame_).ToMat4() * math::Translation(c * -1.0);
                    ShowCoordinates(scene::move::RotationDegrees(axis, up));
                } else {
                    const math::Vec3 f = scene::move::DragScale(scaleMode_, constraint_, up);
                    const double s[3] = {f.x, f.y, f.z};
                    change = math::Translation(c) * scene::tools::FrameScale(dragFrame_, s) * math::Translation(c * -1.0);
                    ShowCoordinates(scene::move::ScalePercent(f));
                }
                auto deltas = scene::tools::SubObjectDeltas(vertexMove_.points, vertexMove_.objectToWorld, change);
                for (size_t i = 0; i < wts.size() && i < deltas.size(); ++i) deltas[i] = deltas[i] * wts[i];   // Soft Selection
                vertexMove_.mod->SetMoves(scene::EditMeshModifier::AddToMoves(vertexMove_.before, vertexMove_.vertices, deltas));
            }
            viewports_.Redraw();
            return;
        }
        const int axis = scene::move::RotationAxis(constraint_);
        const math::Quat turn = scene::move::DragRotationInFrame(axis, up, dragFrame_);   // about the frame's axis (9.9)
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
            } else if (tool_ == Tool::Rotate && centre_ != 0) {
                // About the selection's centre or the system's origin: each object orbits it.
                n->Transform() = m.start;
                const math::Mat4 world = math::Translation(dragCentre_) * turn.ToMat4() * math::Translation(dragCentre_ * -1.0) * n->WorldMatrix();
                scene::Scene::SetWorldMatrix(n, world);
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
        if (!moving_.empty() || vertexMove_.active) return;
        if (ok) ShowCoordinates(p);
        else statusArea_.SetCoordinates(L"", L"", L"");
    };
    typeIn_.onApply = [this](bool absolute, const math::Vec3& value) {
        if (tool_ == Tool::Select || scene_.Selection().empty()) return;
        const auto tool = tool_ == Tool::Rotate ? scene::typein::Tool::Rotate : tool_ == Tool::Scale ? scene::typein::Tool::Scale : scene::typein::Tool::Move;
        const std::string name = tool_ == Tool::Rotate ? "Rotate" : tool_ == Tool::Scale ? "Scale" : "Move";
        // Rotate offsets turn the selection about its centre (the pivots' average).
        math::Vec3 pivot{0, 0, 0};
        for (scene::Node::Id id : scene_.Selection())
            if (const scene::Node* n = scene_.FindById(id)) pivot = pivot + math::TransformPoint(n->WorldMatrix(), {0, 0, 0});
        pivot = pivot * (1.0 / static_cast<double>(scene_.Selection().size()));
        auto macro = std::make_unique<scene::MacroCommand>(name);
        for (scene::Node::Id id : scene_.Selection()) {
            scene::Node* n = scene_.FindById(id);
            if (!n || (absolute && scene_.Selection().size() != 1)) continue;
            const scene::PRS after = absolute ? scene::typein::WithAbsolute(tool, *n, value) : scene::typein::WithOffset(tool, *n, value, pivot);
            macro->Add(std::make_unique<scene::TransformCommand>(id, n->Transform(), after, name));
        }
        undo_.Execute(std::move(macro), scene_);
        viewports_.Redraw();
        UpdateTypeIn();                                 // Absolute shows the result; Offset returns to neutral
    };
    viewports_.onMoveEnd = [this](bool commit) {
        if (gizmoMove_.active) {
            gizmoMove_.active = false;
            const auto after = gizmoMove_.mod->GetGizmo();
            gizmoMove_.mod->SetGizmo(gizmoMove_.start);         // so the command records the real "before"
            if (commit) undo_.Execute(std::make_unique<scene::GizmoCommand>(gizmoMove_.mod, gizmoMove_.start, after), scene_);
            viewports_.Redraw();
            return;
        }
        if (vertexMove_.active) {
            vertexMove_.active = false;
            const auto after = vertexMove_.mod->Moves();
            vertexMove_.mod->SetMoves(vertexMove_.before);      // so the command records the real "before"
            bool changed = after.size() != vertexMove_.before.size();
            for (size_t i = 0; !changed && i < after.size(); ++i)
                changed = math::Length(after[i].delta - vertexMove_.before[i].delta) > 1e-12;
            if (commit && changed) undo_.Execute(std::make_unique<scene::MoveVerticesCommand>(vertexMove_.mod, after), scene_);
            viewports_.Redraw();
            return;
        }
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
        if (command == "tool.ref_coord") {
            using RC = scene::move::RefCoord;
            static const std::pair<const wchar_t*, RC> kSystems[] = {{L"View", RC::View}, {L"Screen", RC::Screen}, {L"World", RC::World},
                                                                     {L"Parent", RC::Parent}, {L"Local", RC::Local}, {L"Grid", RC::Grid}, {L"Pick", RC::Pick}};
            for (const auto& [label, system] : kSystems)
                if (text == label) refCoord_ = system;
            if (refCoord_ == RC::Pick) statusArea_.SetPrompt(1, L"Pick (not implemented yet): World is used");
            viewports_.SetReferenceSystem(refCoord_, centre_);
            return;
        }
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
        if (ok) { viewports_.Redraw(); UpdateModifyPanel(); UpdateCreatePanel(); UpdateSelectionStatus(); }
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

    if (command == "file.save") {
        if (docPath_.empty()) SaveAs();
        else SaveFile(docPath_);
        return;
    }
    if (command == "file.save_as") { SaveAs(); return; }
    if (command == "file.new") {
        // NOTE: R3 first asks Keep Objects and Hierarchy / Keep Objects / New All (dialog
        // not built); this is New All, keeping the views.
        if (ConfirmDiscard()) { ResetScene(); statusArea_.SetPrompt(1, L"New scene"); }
        return;
    }
    if (command == "file.reset") {
        if (!ConfirmDiscard()) return;
        // NOTE: R3's wording unverified.
        if (MessageBoxW(Hwnd(), L"Do you really want to reset?", kAppName, MB_YESNO | MB_ICONQUESTION) != IDYES) return;
        ResetScene();
        viewports_.ResetAllViews();
        return;
    }
    if (command == "file.merge") {
        const std::wstring path = AskForSceneFile(L"Merge File");
        if (!path.empty()) MergeFile(path);
        return;
    }
    if (command == "edit.select_all" || command == "edit.select_none" || command == "edit.select_invert") {
        std::vector<scene::Node::Id> ids;
        if (command != "edit.select_none")
            for (scene::Node::Id id : scene::groups::AllSelectable(scene_)) {     // groups, not their members
                const auto& sel = scene_.Selection();
                const bool selected = std::find(sel.begin(), sel.end(), id) != sel.end();
                if (command == "edit.select_all" || !selected) ids.push_back(id);
            }
        SelectNodes(ids);
        return;
    }
    if (command == "views.redraw_all") { viewports_.Redraw(); return; }
    if (command.rfind("group.", 0) == 0) {
        std::unique_ptr<scene::Command> cmd;
        const auto sel = scene_.Selection();
        if (command == "group.group") {
            if (sel.empty()) return;
            const std::string suggested = scene_.UniqueName("Group");
            const std::wstring name = ui::AskName(Hwnd(), L"Group", L"Group name:", std::wstring(suggested.begin(), suggested.end()));
            if (name.empty()) return;
            std::string n8;
            for (wchar_t c : name) n8 += static_cast<char>(c < 0x80 ? c : '?');
            cmd = scene::groups::MakeGroup(scene_, sel, n8);
        } else if (command == "group.ungroup" || command == "group.explode") {
            std::vector<scene::Node::Id> heads;
            for (scene::Node::Id id : sel) if (const scene::Node* n = scene_.FindById(id); n && n->IsGroupHead()) heads.push_back(id);
            cmd = scene::groups::MakeUngroup(scene_, heads, command == "group.explode");
        } else if (command == "group.open" || command == "group.close") {
            // Open: the selected closed groups. Close: the open group of each selected
            // object (or the selected open group itself); the closed group is selected.
            auto macro = std::make_unique<scene::MacroCommand>(command == "group.open" ? "Group Open" : "Group Close");
            std::vector<scene::Node::Id> closedHeads;
            for (scene::Node::Id id : sel) {
                scene::Node* n = scene_.FindById(id);
                if (command == "group.open") {
                    if (n && n->IsGroupHead() && !n->GroupOpen()) macro->Add(std::make_unique<scene::GroupOpenCommand>(id, true));
                } else {
                    for (scene::Node* p = n; p; p = p->Parent())
                        if (p->IsGroupHead() && p->GroupOpen()) {
                            if (std::find(closedHeads.begin(), closedHeads.end(), p->GetId()) == closedHeads.end()) {
                                macro->Add(std::make_unique<scene::GroupOpenCommand>(p->GetId(), false));
                                closedHeads.push_back(p->GetId());
                            }
                            break;
                        }
                }
            }
            if (!closedHeads.empty()) macro->Add(std::make_unique<scene::SelectCommand>(closedHeads));
            cmd = std::move(macro);
        } else if (command == "group.detach") {
            cmd = scene::groups::MakeDetach(scene_, sel);
        } else if (command == "group.attach") {
            if (sel.empty()) return;
            attachPick_ = true;
            statusArea_.SetPrompt(1, L"Pick group to attach to");               // NOTE: R3's prompt unverified
            return;
        }
        if (!cmd) return;
        undo_.Execute(std::move(cmd), scene_);
        UpdateSelectionStatus();
        UpdateModifyPanel();
        viewports_.Redraw();
        return;
    }
    if (command == "tool.pivot_point" || command == "tool.pivot_selection" || command == "tool.pivot_transform") {
        centre_ = command == "tool.pivot_point" ? 0 : command == "tool.pivot_selection" ? 1 : 2;
        viewports_.SetReferenceSystem(refCoord_, centre_);
        return;
    }
    if (command == "tools.transform_type_in") { typeIn_.Show(Hwnd()); UpdateTypeIn(); return; }
    if (command == "tools.align") {
        if (scene_.Selection().empty()) { statusArea_.SetPrompt(1, L"Select the objects first"); return; }
        alignPick_ = true;
        statusArea_.SetPrompt(1, L"Pick Align Target Object");        // NOTE: R3's prompt unverified
        return;
    }
    if (command == "tools.mirror" || command == "tools.array") {
        if (scene_.Selection().empty()) { statusArea_.SetPrompt(1, L"Select the objects first"); return; }
        // The topmost selected objects only: a selected child moves with its parent.
        auto topmost = [&](const std::vector<scene::Node::Id>& ids) {
            std::vector<scene::Node*> out;
            for (scene::Node::Id id : ids) {
                scene::Node* n = scene_.FindById(id);
                if (!n) continue;
                bool under = false;
                for (const scene::Node* p = n->Parent(); p && !under; p = p->Parent())
                    under = std::find(ids.begin(), ids.end(), p->GetId()) != ids.end();
                if (!under) out.push_back(n);
            }
            return out;
        };
        const scene::move::Frame frame = viewports_.ActiveView().CurrentFrame();
        const math::Vec3 centre = viewports_.ActiveView().TransformCenter();
        const std::vector<scene::Node::Id> selection = scene::groups::WithMembers(scene_, scene_.Selection());
        if (command == "tools.mirror") {
            static const wchar_t* kNames[] = {L"View", L"Screen", L"World", L"Parent", L"Local", L"Grid", L"Pick"};
            const ui::MirrorChoice c = ui::ShowMirrorDialog(Hwnd(), kNames[static_cast<int>(refCoord_)]);
            if (!c.ok) return;
            const math::Mat4 m = scene::tools::MirrorMatrix(frame, static_cast<scene::tools::MirrorAxis>(c.axis), centre, c.offset);
            auto macro = std::make_unique<scene::MacroCommand>("Mirror");
            if (c.clone == 0) {
                for (scene::Node* n : topmost(selection))
                    macro->Add(std::make_unique<scene::TransformCommand>(n->GetId(), n->Transform(), scene::tools::Changed(*n, m), "Mirror"));
            } else {
                const auto ids = scene::CloneNodes(scene_, selection, c.clone == 2);
                for (scene::Node* n : topmost(ids)) n->Transform() = scene::tools::Changed(*n, m);
                macro->Add(std::make_unique<scene::AddedNodesCommand>("Mirror", ids));
                macro->Add(std::make_unique<scene::SelectCommand>(ids));
            }
            undo_.Execute(std::move(macro), scene_);
        } else {
            const ui::ArrayChoice c = ui::ShowArrayDialog(Hwnd());
            if (!c.ok) return;
            scene::tools::ArrayStep step;
            step.move = c.move;
            step.rotateDegrees = c.rotate;
            step.scalePercent = c.scale;
            const math::Mat4 stepM = scene::tools::ArrayStepMatrix(step, frame, centre);
            const int n1 = std::max(1, c.count1), n2 = c.dimensions >= 2 ? std::max(1, c.count2) : 1, n3 = c.dimensions >= 3 ? std::max(1, c.count3) : 1;
            if (static_cast<long long>(n1) * n2 * n3 > 10000) { statusArea_.SetPrompt(1, L"Array: too many copies (10,000 at most)"); return; }
            std::vector<scene::Node::Id> all;
            for (int k = 0; k < n3; ++k)
                for (int j = 0; j < n2; ++j) {
                    math::Mat4 along = math::Translation(c.offset2 * static_cast<double>(j) + c.offset3 * static_cast<double>(k));
                    for (int i = 0; i < n1; ++i) {
                        if (i > 0) along = along * stepM;          // step^i, then the row and layer offsets
                        if (i == 0 && j == 0 && k == 0) continue;  // the original
                        const auto ids = scene::CloneNodes(scene_, selection, c.instance);
                        for (scene::Node* n : topmost(ids)) n->Transform() = scene::tools::Changed(*n, along);
                        all.insert(all.end(), ids.begin(), ids.end());
                    }
                }
            if (all.empty()) return;
            undo_.Execute(std::make_unique<scene::AddedNodesCommand>("Array", all), scene_);
            statusArea_.SetPrompt(1, L"Array: " + std::to_wstring(all.size()) + L" objects created");
        }
        UpdateSelectionStatus();
        UpdateModifyPanel();
        viewports_.Redraw();
        return;
    }
    if (command == "edit.clone") {
        const auto selection = scene_.Selection();
        if (selection.empty()) return;
        const scene::Node* first = scene_.FindById(selection.front());
        const std::string suggested = first ? scene_.UniqueName(scene::NameBase(first->Name())) : "";
        const ui::CloneChoice choice = ui::ShowCloneDialog(Hwnd(), std::wstring(suggested.begin(), suggested.end()), selection.size() == 1);
        if (!choice.ok) return;
        std::string name;
        for (wchar_t c : choice.name) name += static_cast<char>(c < 0x80 ? c : '?');
        const auto ids = scene::CloneNodes(scene_, scene::groups::WithMembers(scene_, selection), choice.instance,
                                           selection.size() == 1 ? name : "");
        // One undo step: the clones, then the clones selected (as R3 leaves them; a cloned
        // group is selected as a group).
        auto macro = std::make_unique<scene::MacroCommand>("Clone");
        macro->Add(std::make_unique<scene::AddedNodesCommand>("Clone", ids));
        macro->Add(std::make_unique<scene::SelectCommand>(scene::groups::Selectable(scene_, ids)));
        undo_.Execute(std::move(macro), scene_);
        modifyIndex_ = 0;
        subObject_ = false;
        UpdateSelectionStatus();
        UpdateModifyPanel();
        viewports_.Redraw();
        return;
    }
    if (command == "file.save_selected") {
        if (scene_.Selection().empty()) return;
        wchar_t path[MAX_PATH] = L"";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = Hwnd();
        ofn.lpstrFilter = L"maxlike (*.mlk)\0*.mlk\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = L"Save File As";
        ofn.lpstrDefExt = L"mlk";
        ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        if (!GetSaveFileNameW(&ofn)) return;
        // The selected objects only; the scene itself stays as it is (not marked saved).
        const std::vector<uint8_t> bytes = io::SaveNative(scene_, scene::groups::WithMembers(scene_, scene_.Selection()));
        HANDLE file = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        DWORD written = 0;
        const bool ok = file != INVALID_HANDLE_VALUE && WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        if (!ok) MessageBoxW(Hwnd(), L"Could not save the selected objects.", kAppName, MB_OK | MB_ICONWARNING);
        else statusArea_.SetPrompt(1, L"Saved " + std::to_wstring(scene_.Selection().size()) + L" selected objects");
        return;
    }
    if (command == "file.summary_info") {
        ui::ShowSummaryDialog(Hwnd(), scene::Summarize(scene_), description_);
        return;
    }
    if (command.rfind("file.recent.", 0) == 0) {
        const size_t n = static_cast<size_t>(std::atoi(command.c_str() + 12));
        if (n >= 1 && n <= recentFiles_.size() && ConfirmDiscard()) OpenFile(recentFiles_[n - 1]);
        return;
    }
    if (command == "views.show_gizmo") {
        showGizmo_ = !showGizmo_;
        UpdateTransformDisplay();
        viewports_.Redraw();
        return;
    }
    if (command == "views.update_during_spinner_drag") { updateDuringSpinnerDrag_ = !updateDuringSpinnerDrag_; return; }
    if (command == "views.save_active" || command == "views.restore_active") {
        ui::Viewport& v = viewports_.ActiveView();
        SavedView& saved = savedViews_[static_cast<int>(v.Type())];
        if (command == "views.save_active") { saved = {true, v.OrthoState(), v.PerspectiveState()}; }
        else if (saved.saved) { v.SetViewState(saved.ortho, saved.perspective); }
        return;
    }
    if (command == "file.open") {
        wchar_t path[MAX_PATH] = L"";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = Hwnd();
        ofn.lpstrFilter = L"Scenes (*.max, *.mlk)\0*.max;*.mlk\0" L"3D Studio MAX (*.max)\0*.max\0" L"maxlike (*.mlk)\0*.mlk\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = L"Open File";
        ofn.lpstrDefExt = L"max";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        if (ConfirmDiscard() && GetOpenFileNameW(&ofn)) OpenFile(path);
        return;
    }
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
// Before a menu opens: R3's greying (ui::MenuItemEnabled), Undo / Redo named after the
// step they undo (keeping the shortcut), the check marks, the active view's name in Save /
// Restore Active View, and in the File menu the recently opened files above Exit.
void MainFrame::UpdateMenu(HMENU menu) {
    ui::MenuState state;
    state.canUndo = undo_.CanUndo();
    state.canRedo = undo_.CanRedo();
    state.objects = scene_.Nodes().size();
    state.selected = scene_.Selection().size();
    for (scene::Node::Id id : scene_.Selection()) {
        const scene::Node* n = scene_.FindById(id);
        if (!n) continue;
        if (n->IsGroupHead()) { ++state.groups; (n->GroupOpen() ? state.openGroups : state.closedGroups)++; }
        else if (n->Parent() && n->Parent()->IsGroupHead() && n->Parent()->GroupOpen()) { ++state.openMembers; ++state.openGroups; }
    }
    const int activeType = static_cast<int>(viewports_.ActiveView().Type());
    state.savedPerspective = savedViews_[activeType].saved;
    const std::wstring viewName = viewports_.ActiveView().Name();
    // File menu: rebuild the recent-files block (a separator and up to nine entries).
    if (GetSubMenu(GetMenu(Hwnd()), 0) == menu) {
        for (int i = GetMenuItemCount(menu) - 1; i >= 0; --i) {
            const UINT id = GetMenuItemID(menu, i);
            const std::string& name = commands_.Name(static_cast<uint16_t>(id));
            if (name.rfind("file.recent.", 0) == 0) DeleteMenu(menu, static_cast<UINT>(i), MF_BYPOSITION);
        }
        int exitPos = -1;
        for (int i = 0; i < GetMenuItemCount(menu); ++i)
            if (GetMenuItemID(menu, i) == commands_.Find("file.exit")) exitPos = i;
        // A separator we added before (an extra one directly above Exit's own).
        if (exitPos >= 2 && GetMenuItemID(menu, exitPos - 1) == 0 && GetMenuItemID(menu, exitPos - 2) == 0 &&
            (GetMenuState(menu, static_cast<UINT>(exitPos - 2), MF_BYPOSITION) & MF_SEPARATOR)) {
            DeleteMenu(menu, static_cast<UINT>(exitPos - 1), MF_BYPOSITION);
            --exitPos;
        }
        if (exitPos > 0 && !recentFiles_.empty()) {
            int at = exitPos;                                      // insert above Exit's separator's successor
            for (size_t i = 0; i < recentFiles_.size() && i < 9; ++i) {
                const std::wstring text = L"&" + std::to_wstring(i + 1) + L" " + recentFiles_[i];
                InsertMenuW(menu, static_cast<UINT>(at++), MF_BYPOSITION | MF_STRING, commands_.Find("file.recent." + std::to_string(i + 1)), text.c_str());
            }
            InsertMenuW(menu, static_cast<UINT>(at), MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
        }
    }
    for (int i = 0; i < GetMenuItemCount(menu); ++i) {
        const UINT id = GetMenuItemID(menu, i);
        if (id == 0 || id == static_cast<UINT>(-1)) continue;     // separators, submenus
        const std::string& c = commands_.Name(static_cast<uint16_t>(id));
        if (c.empty()) continue;
        UINT flags = MF_BYCOMMAND | (ui::MenuItemEnabled(c, state) ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem(menu, id, flags);
        if (c == "edit.undo" || c == "edit.redo") {
            const bool undo = c == "edit.undo";
            std::wstring text = undo ? L"&Undo" : L"&Redo";
            const std::string what = undo ? undo_.UndoName() : undo_.RedoName();
            if (undo ? state.canUndo : state.canRedo) text += L" " + std::wstring(what.begin(), what.end());
            text += undo ? L"\tCtrl+Z" : L"\tCtrl+A";
            ModifyMenuW(menu, id, MF_BYCOMMAND | MF_STRING | (ui::MenuItemEnabled(c, state) ? MF_ENABLED : MF_GRAYED), id, text.c_str());
        }
        if (c == "views.save_active" || c == "views.restore_active") {
            const std::wstring text = (c == "views.save_active" ? L"&Save Active " : L"Restore Active ") + viewName + L" View";
            ModifyMenuW(menu, id, MF_BYCOMMAND | MF_STRING | (ui::MenuItemEnabled(c, state) ? MF_ENABLED : MF_GRAYED), id, text.c_str());
        }
        if (c == "views.show_gizmo") CheckMenuItem(menu, id, MF_BYCOMMAND | (showGizmo_ ? MF_CHECKED : MF_UNCHECKED));
        if (c == "views.update_during_spinner_drag") CheckMenuItem(menu, id, MF_BYCOMMAND | (updateDuringSpinnerDrag_ ? MF_CHECKED : MF_UNCHECKED));
    }
}

void MainFrame::InstallShortcuts() {
    std::vector<ACCEL> always, plain;
    for (const ui::Shortcut& s : ui::ShortcutsOf(ui::MainMenuSpec())) {
        ACCEL a{};
        a.fVirt = static_cast<BYTE>(FVIRTKEY | (s.ctrl ? FCONTROL : 0) | (s.alt ? FALT : 0) | (s.shift ? FSHIFT : 0));
        a.key = s.key;
        a.cmd = commands_.Find(s.command);
        if (!a.cmd) continue;
        (s.WhileTyping() ? always : plain).push_back(a);
    }
    HACCEL a = always.empty() ? nullptr : CreateAcceleratorTableW(always.data(), static_cast<int>(always.size()));
    HACCEL p = plain.empty() ? nullptr : CreateAcceleratorTableW(plain.data(), static_cast<int>(plain.size()));
    win::App::SetAccelerators(Hwnd(), a, p);
}

void MainFrame::AddRecentFile(const std::wstring& path) {
    recentFiles_ = ui::AddRecentFile(recentFiles_, path);
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kRecentKey, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
    for (size_t i = 0; i < 9; ++i) {
        const std::wstring name = L"File" + std::to_wstring(i + 1);
        if (i < recentFiles_.size())
            RegSetValueExW(key, name.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(recentFiles_[i].c_str()),
                           static_cast<DWORD>((recentFiles_[i].size() + 1) * sizeof(wchar_t)));
        else
            RegDeleteValueW(key, name.c_str());
    }
    RegCloseKey(key);
}

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
        DragAcceptFiles(Hwnd(), TRUE);   // a .max file dropped on the window opens it
        return 0;

    case WM_CLOSE:
        if (!ConfirmDiscard()) return 0;    // cancelled: keep running
        break;

    case WM_DROPFILES: {
        const HDROP drop = reinterpret_cast<HDROP>(wp);
        wchar_t path[MAX_PATH] = L"";
        if (DragQueryFileW(drop, 0, path, MAX_PATH) > 0) {
            DragFinish(drop);
            SetForegroundWindow(Hwnd());
            if (ConfirmDiscard()) OpenFile(path);
        } else {
            DragFinish(drop);
        }
        return 0;
    }

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
        UpdateMenu(reinterpret_cast<HMENU>(wp));
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
