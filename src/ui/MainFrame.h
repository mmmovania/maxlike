#pragma once
#include "win/Window.h"
#include "ui/CommandTable.h"
#include "scene/Scene.h"
#include "scene/Undo.h"
#include "scene/CreationTool.h"
#include "scene/MoveMath.h"
#include "ui/CommandPanel.h"
#include "ui/StatusBar.h"
#include "ui/TabPanel.h"
#include "ui/TimeSlider.h"
#include "ui/TrackBar.h"
#include "ui/ViewPanel.h"
#include "ui/Toolbar.h"
#include <array>
#include <string>

namespace ui {

// Top-level application window. Owns the main regions and lays them out.
class MainFrame : public win::Window {
public:
    ~MainFrame() override;

    bool Create();
    void SetDocumentName(const std::wstring& name); // updates "<name> - maxlike"
    scene::Scene& Scene() { return scene_; }
    bool LoadDemoScene(const std::string& which);   // --demo-scene
    // Opens a .max file (Phase 8): replaces the scene if the file can be read, otherwise
    // leaves it untouched; reports what could not be imported.
    bool OpenFile(const std::wstring& path);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.MainFrame"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    bool CreateRegions();
    void LayoutRegions(int clientWidth, int clientHeight);
    void RecreateFonts();
    void OnPaint();
    void OnCommand(const std::string& command);
    void UpdateEditMenu(HMENU menu);   // "Undo <name>" / "Redo <name>", greyed when empty
    void ShowFrame(int frame);

    std::array<win::Window*, 8> Regions();

    HFONT uiFont_ = nullptr;
    CommandTable commands_;
    scene::Scene scene_;            // the document; declared before the views that show it
    scene::UndoStack undo_;         // every scene change goes through here (Phase 5 on)
    scene::CreationTool creation_{scene_, undo_};   // builds objects from viewport mouse events
    std::shared_ptr<scene::Object> panelObject_;    // what Parameters shows (keeps it alive)
    std::shared_ptr<scene::Object> template_;       // defaults before anything is created
    bool paramDragging_ = false;                    // a spinner drag in progress...
    double paramDragStart_ = 0;                     // ...and the value it started from
    void UpdateCreatePanel();                       // Parameters, name, colour from the creation tool
    void UpdateSelectionStatus();                   // "None Selected" / "1 Object Selected" / ...
    // The toolbar's transform tool and axis constraint (R3's defaults: Select, XY).
    enum class Tool { Select, Move, Rotate, Scale };
    Tool tool_ = Tool::Select;
    scene::move::ScaleMode scaleMode_ = scene::move::ScaleMode::Uniform;
    bool constraint_[3] = {true, true, false};
    void UpdateTransformDisplay();
    void ShowCoordinates(const math::Vec3& v);       // the status bar's X/Y/Z, three decimals
    // A move in progress: each moved node's starting transform, world position and
    // parent matrix (children of moved nodes move with them, not twice).
    struct Moving {
        scene::Node::Id id; scene::PRS start; math::Vec3 startWorld;
        bool hasParent; math::Mat4 parentWorld; math::Quat parentRotation;
    };
    std::vector<Moving> moving_;
    void DeleteSelection();
    scene::Node* NameColorTarget() const;          // whose name and colour the panel shows
    // The Modify tab: the stack entry shown (0 = top) and filling the panel from the
    // selected object.
    int modifyIndex_ = 0;
    // Sub-object editing (Phase 7): the Sub-Object button and level; a vertex drag in
    // progress (the Edit Mesh's moves before it, the vertices, the object's inverse matrix).
    bool subObject_ = false;
    int subLevel_ = 0;                                  // 0 Vertex (others not implemented)
    struct VertexMove {
        bool active = false;
        std::shared_ptr<scene::EditMeshModifier> mod;
        std::vector<scene::EditMeshModifier::Move> before;
        std::vector<uint32_t> vertices;
        math::Mat4 worldToObject;
    } vertexMove_;
    std::shared_ptr<scene::EditMeshModifier> VertexEditingMod() const;   // non-null while editing vertices
    void UpdateModifyPanel();
    // The selected object's stack: its Derived Object (or null) and the entry shown.
    std::shared_ptr<scene::DerivedObject> SelectedStack(scene::Node** node = nullptr) const;
    std::shared_ptr<scene::Modifier> CurrentModifier() const;   // null when the base is shown
    std::wstring docName_ = L"Untitled";

    // Regions, in the order of FrameLayout.
    TabPanel    tabPanel_;
    Toolbar     mainToolbar_;
    Toolbar     objectsToolbar_;    // the tab panel's Objects strip, in the toolbar's place
    ViewPanel   viewports_;
    TimeSlider  timeSlider_;
    TrackBar    trackBar_;
    CommandPanel commandPanel_;
    StatusBar   statusArea_;
};

} // namespace ui
