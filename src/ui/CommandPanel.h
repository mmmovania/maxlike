#pragma once
#include "win/Window.h"
#include "ui/CommandPanelLayout.h"
#include "ui/Rollup.h"
#include "ui/ParametersSpec.h"
#include "ui/SpinnerModel.h"
#include "scene/ParamBlock.h"
#include <memory>
#include <string>
#include <functional>

namespace ui {

// The command panel on the right: six tabs (Create, Modify, Hierarchy, Motion,
// Display, Utilities) over a raised body that will hold the rollouts. Drawn to the
// measurements in docs/spec/01-ui-look.md; the outer frame is Edge::CommandPanel.
class CommandPanel : public win::Window {
public:
    std::function<void(int tab)> onTabChanged;
    std::function<void(int category)> onCategoryChanged;
    std::function<void(const std::wstring& subcategory)> onSubcategoryChanged;
    // A primitive button was checked (its index in StandardPrimitiveNames) or unchecked (-1).
    std::function<void(int type)> onCreateTypeChanged;
    std::function<void(unsigned rgb)> onObjectColorChanged;   // 0xRRGGBB
    // A Parameters value changed: `final` is false while dragging a spinner (preview),
    // true when a change is complete (one undo step later).
    std::function<void(int paramIndex, double value, bool final)> onParamChanged;
    std::function<void(int method)> onCreationMethodChanged;

    // What Parameters shows: the block of the object being built or just created (or
    // a template with the defaults). Not owned; must outlive its use or be reset.
    void SetParameterSource(const scene::ParamBlock* block);
    // Name and Color's field: the object's name, or "" for the disabled look.
    void SetObjectName(const std::wstring& name);
    void Refresh() { InvalidateRect(Hwnd(), nullptr, FALSE); }
    int CreateType() const { return createType_; }

    // The colour shown in Name and Color (the next object's colour, or the selection's).
    unsigned ObjectColor() const { return objectColor_; }
    void SetObjectColor(unsigned rgb);

    bool Create(HWND parent);
    int SelectedTab() const { return selected_; }
    void SelectTab(int tab);
    int SelectedCategory() const { return category_; }
    void SelectCategory(int category);

protected:
    const wchar_t* ClassName() const override { return L"maxlike.CommandPanel"; }
    LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
    void OnPaint();
    void PaintTabs(HDC dc);
    void PaintCategories(HDC dc);
    void FillDropdown();
    void UpdateCreateControls();     // the dropdown shows on the Create tab only
    bool ShowsRollouts() const;      // Create > Geometry > Standard Primitives
    void PaintRollouts(HDC dc);
    void PaintObjectType(HDC dc, int titleTop);
    void PaintNameAndColor(HDC dc, int titleTop);
    void PaintCreationMethod(HDC dc, int titleTop);
    void PaintParameters(HDC dc, int titleTop);

    // Rollouts shown now: Object Type, Name and Color, and while a primitive is active,
    // Creation Method, Keyboard Entry and Parameters.
    enum RolloutId { kRollObjectType, kRollNameColor, kRollCreationMethod, kRollKeyboardEntry, kRollParameters, kRollCount };
    std::vector<int> VisibleRollouts() const;
    std::vector<rollup::Item> RolloutItems() const;
    int TitleTopOf(int id) const;             // -1 if not shown or closed

    // Parameters interaction.
    enum class Part { None, Field, Up, Down, Check };
    Part ParamPartAt(int x, int y, int& row) const;
    double ParamValue(const params::Row& r) const;
    void BeginTyping(int row);
    void EndTyping(bool commit);
    static LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
    void PickObjectColor();
    void SetScroll(int scroll);
    int  TypeButtonAt(int x, int y) const;

    int selected_ = cmdpanel::kCreate;   // the reference shows the Create tab
    HBITMAP icons_ = nullptr;            // res/panel_tab_icons.bmp, 22 x 20 cells
    HBITMAP categoryIcons_ = nullptr;    // res/panel_category_icons.bmp, 16 x 16 cells
    int category_ = cmdpanel::kGeometry; // the reference shows Geometry pressed
    HWND dropdown_ = nullptr;            // classic COMBOBOX (draws R3's look itself)
    bool open_[kRollCount] = {true, true, true, false, true};   // R3: Keyboard Entry starts collapsed
    params::Spec spec_;                  // Parameters of the active primitive
    int method_ = 1;                     // Creation Method radio
    const scene::ParamBlock* source_ = nullptr;
    std::wstring objectName_;
    // A spinner being pressed / dragged.
    int spinRow_ = -1;
    Part spinPart_ = Part::None;
    int spinPressY_ = 0;
    bool spinMoved_ = false;
    std::unique_ptr<SpinnerModel> spinModel_;
    // Typing into a spinner field: a real EDIT placed over it while typing.
    HWND typing_ = nullptr;
    int typingRow_ = -1;
    WNDPROC editOldProc_ = nullptr;
    int scroll_ = 0;                     // pixels the rollup is scrolled up
    int createType_ = -1;                // checked primitive button, -1 = none
    unsigned objectColor_ = cmdpanel::kReferenceObjectColor;
    bool dragging_ = false;              // scrolling the rollup by dragging (hand cursor)
    int dragStartY_ = 0, dragStartScroll_ = 0;
    HFONT font_ = nullptr;               // set by the parent with WM_SETFONT; not owned
};

} // namespace ui
