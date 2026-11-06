#pragma once
#include "win/Window.h"
#include "ui/CommandTable.h"
#include "scene/Scene.h"
#include "scene/Undo.h"
#include "scene/CreationTool.h"
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

    std::array<win::Window*, 7> Regions();

    HFONT uiFont_ = nullptr;
    CommandTable commands_;
    scene::Scene scene_;            // the document; declared before the views that show it
    scene::UndoStack undo_;         // every scene change goes through here (Phase 5 on)
    scene::CreationTool creation_{scene_, undo_};   // builds objects from viewport mouse events
    std::wstring docName_ = L"Untitled";

    // Regions, in the order of FrameLayout.
    TabPanel    tabPanel_;
    Toolbar     mainToolbar_;
    ViewPanel   viewports_;
    TimeSlider  timeSlider_;
    TrackBar    trackBar_;
    CommandPanel commandPanel_;
    StatusBar   statusArea_;
};

} // namespace ui
