#include "ui/MainFrame.h"
#include <shellapi.h>
#include <commdlg.h>
#include "io/MaxImport.h"
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
    if (!subObject_ || subLevel_ != 0 || !commandPanel_.InModify()) return nullptr;
    return std::dynamic_pointer_cast<scene::EditMeshModifier>(CurrentModifier());
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
        if (info.subObjectLevels.empty()) subObject_ = false;
        info.subObjectOn = subObject_;
        info.subObjectLevel = subLevel_;
        if (const auto em = std::dynamic_pointer_cast<scene::EditMeshModifier>(mod)) {
            // NOTE: R3's wording for one vertex or none is unverified.
            info.selectionText = std::to_wstring(em->SelectedVertices().size()) + L" Vertices Selected";
        }
    }
    commandPanel_.SetModifyInfo(info);
    // The views draw and pick vertices while an Edit Mesh is at Vertex level.
    ui::Viewport::VertexEditing editing;
    if (const auto em = VertexEditingMod()) {
        editing.on = true;
        editing.node = node->GetId();
        editing.stack = stack;
        editing.mod = em;
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

bool MainFrame::OpenFile(const std::wstring& path) {
    const size_t slash = path.find_last_of(L"\\/");
    const std::wstring fileName = slash == std::wstring::npos ? path : path.substr(slash + 1);
    // Read with Win32 (wide paths on every compiler).
    std::vector<uint8_t> bytes;
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
    // Import into a new scene first: a file that cannot be read leaves the current one.
    scene::Scene imported;
    io::ImportReport report;
    std::string error;
    if (!io::ImportMax(bytes, imported, report, error)) {
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
    // R3 restores the viewports saved in the file.
    {
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
        statusArea_.SetPrompt(1, label + L" (not implemented yet)");
    };
    commandPanel_.onSubObjectLevel = [this](int level) {
        subLevel_ = level;
        subObject_ = true;                                      // a level icon or the combo enters sub-object mode
        if (level != 0) statusArea_.SetPrompt(1, L"This sub-object level is not implemented yet");
        UpdateModifyPanel();
        viewports_.Redraw();
    };
    // Vertex selection: a click replaces it (Ctrl adds, Alt removes); one "Select" step.
    viewports_.onVertexSelect = [this](const std::vector<uint32_t>& verts, bool, bool ctrl, bool alt) {
        const auto mod = VertexEditingMod();
        if (!mod) return;
        std::vector<uint32_t> next = mod->SelectedVertices();
        if (ctrl) next.insert(next.end(), verts.begin(), verts.end());
        else if (alt) next.erase(std::remove_if(next.begin(), next.end(), [&](uint32_t v) {
                                     return std::find(verts.begin(), verts.end(), v) != verts.end(); }), next.end());
        else next = verts;
        std::sort(next.begin(), next.end());
        next.erase(std::unique(next.begin(), next.end()), next.end());
        if (next == mod->SelectedVertices()) return;
        undo_.Execute(std::make_unique<scene::SelectVerticesCommand>(mod, next), scene_);
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
        if (const auto mod = VertexEditingMod()) {
            scene::Node* node = nullptr;
            SelectedStack(&node);
            vertexMove_ = {true, mod, mod->Moves(), mod->SelectedVertices(), math::AffineInverse(node->ObjectMatrix())};
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
    };
    // Move follows the mouse in the world; Rotate and Scale follow its vertical movement,
    // each object about its own pivot (NOTE: R3's pivot options are not implemented yet).
    viewports_.onMoveDrag = [this](const math::Vec3& delta, int up) {
        if (vertexMove_.active) {
            // Vertices move in the object's own space (NOTE: Rotate and Scale at vertex
            // level are not implemented; only Move drags vertices).
            if (tool_ != Tool::Move) return;
            const math::Vec3 d = math::TransformPoint(vertexMove_.worldToObject, delta) -
                                 math::TransformPoint(vertexMove_.worldToObject, {0, 0, 0});
            vertexMove_.mod->SetMoves(scene::EditMeshModifier::AddToMoves(vertexMove_.before, vertexMove_.vertices, d));
            ShowCoordinates(delta);
            viewports_.Redraw();
            return;
        }
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
        if (!moving_.empty() || vertexMove_.active) return;
        if (ok) ShowCoordinates(p);
        else statusArea_.SetCoordinates(L"", L"", L"");
    };
    viewports_.onMoveEnd = [this](bool commit) {
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

    if (command == "file.open") {
        wchar_t path[MAX_PATH] = L"";
        OPENFILENAMEW ofn{};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = Hwnd();
        ofn.lpstrFilter = L"3D Studio MAX (*.max)\0*.max\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrTitle = L"Open File";
        ofn.lpstrDefExt = L"max";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        if (GetOpenFileNameW(&ofn)) OpenFile(path);
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

    case WM_DROPFILES: {
        const HDROP drop = reinterpret_cast<HDROP>(wp);
        wchar_t path[MAX_PATH] = L"";
        if (DragQueryFileW(drop, 0, path, MAX_PATH) > 0) {
            DragFinish(drop);
            SetForegroundWindow(Hwnd());
            OpenFile(path);
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
