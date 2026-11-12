#pragma once
#include "win/Window.h"
#include "ui/CommandPanelLayout.h"
#include "ui/Rollup.h"
#include "ui/ParametersSpec.h"
#include "ui/SpinnerModel.h"
#include "scene/ParamBlock.h"
#include <memory>
#include <string>
#include <vector>
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
    // The name field was edited (Enter or leaving it); Esc cancels without a call.
    std::function<void(const std::wstring& name)> onNameChanged;

    // The Modify tab (Phase 7): what it shows for the one selected object.
    struct ModifyInfo {
        bool valid = false;                       // exactly one object selected
        std::vector<std::wstring> stack;          // top first; the last entry is the base object
        int current = 0;                          // the entry being shown
        int baseType = -1;                        // the base's Standard Primitive type, or -1
        bool currentIsModifier = false, currentEnabled = true;
        std::string currentClass;                 // the current modifier's class ("Bend"), else empty
        std::vector<std::wstring> subObjectLevels;   // e.g. Edit Mesh: Vertex, Edge, Face, Polygon, Element
        bool subObjectOn = false;                 // the Sub-Object button pressed
        int subObjectLevel = 0;
        bool sliceMode = false;                   // Edit Mesh's Slice Plane is on (Slice available)
        std::wstring selectionText;               // e.g. "4 Vertices Selected" (Edit Mesh)
        const scene::ParamBlock* params = nullptr;   // the current entry's parameters
    };
    void SetModifyInfo(const ModifyInfo& info);
    bool InModify() const { return selected_ == cmdpanel::kModify && modify_.valid; }
    std::function<void(int tab)> onTabSelected;
    std::function<void(const std::wstring& button)> onModifierButton;   // "Bend", "More...", ...
    std::function<void(int index)> onStackSelect;                       // 0 = top of the stack
    std::function<void(int button)> onStackButton;   // 0 on/off, 1 end result, 2 unique, 3 remove, 4 edit
    std::function<void(bool on)> onSubObject;          // the Sub-Object button
    std::function<void(int level)> onSubObjectLevel;   // the level combo or a level icon
    std::function<void(const std::wstring& label)> onRolloutButton;   // a push button in a rollout
    // Keyboard Entry's Create: world position and the typed sizes (Max indices).
    std::function<void(double x, double y, double z, const std::vector<std::pair<int, double>>& sizes)> onKeyboardCreate;

    // What Parameters shows: the block of the object being built or just created (or
    // a template with the defaults). Not owned; must outlive its use or be reset.
    void SetParameterSource(const scene::ParamBlock* block);
    // Name and Color's field: the object's name, or "" for the disabled look.
    void SetObjectName(const std::wstring& name);
    void Refresh() { InvalidateRect(Hwnd(), nullptr, FALSE); }
    int CreateType() const { return createType_; }
    // Activate a primitive (or -1 for none), as clicking its Object Type button does;
    // shows the Create tab, Geometry and Standard Primitives first.
    void SetCreateType(int type);

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
    void DrawRadio(HDC dc, int x, int y, bool checked, bool enabled = true);
    // Rows of one rollout: slot -1 = Keyboard Entry; otherwise slot 0 = the Create
    // panel's Parameters, or slot n = the Modify tab's n-th rollout. Row ids: Keyboard
    // Entry kKeyBase + i, others slot * 100 + i.
    void PaintRows(HDC dc, int titleTop, int slot);
    const params::Spec& SpecAt(int slot) const { return InModify() ? modSpecs_[static_cast<size_t>(slot)] : spec_; }
    int RollIdForSlot(int slot) const { return InModify() ? kRollDynamic + slot : kRollParameters; }
    static constexpr int kRollDynamic = 100;   // Modify-tab rollouts: kRollDynamic + slot

    // Rollouts shown now: Object Type, Name and Color, and while a primitive is active,
    // Creation Method, Keyboard Entry and Parameters.
    enum RolloutId { kRollObjectType, kRollNameColor, kRollCreationMethod, kRollKeyboardEntry, kRollParameters,
                     kRollModifiers, kRollStack, kRollCount };
    rollup::Box Container() const { return InModify() ? rollup::kModifyContainer : rollup::kContainer; }
    // The name field and swatch now shown (Create: in Name and Color; Modify: above the rollup).
    bool NameColorBoxes(cmdpanel::Box& field, cmdpanel::Box& swatch) const;
    void PaintNameColorBoxes(HDC dc, const cmdpanel::Box& field, const cmdpanel::Box& swatch);
    void PaintModifiers(HDC dc, int titleTop);
    void PaintStack(HDC dc, int titleTop);
    void UpdateModifyControls();                 // place / show the stack and level combos
    int ModifierButtonAt(int x, int y) const;    // index into the ten buttons, or -1
    int StackButtonAt(int x, int y) const;       // 0..4, or -1
    std::vector<int> VisibleRollouts() const;
    std::vector<rollup::Item> RolloutItems() const;
    // A rollout's title row (negative when scrolled above the container), or kNoTitle
    // if it is not shown or closed.
    static constexpr int kNoTitle = -1000000;
    int TitleTopOf(int id) const;

    // Parameters interaction.
    enum class Part { None, Field, Up, Down, Check, Radio, Button, Level };
    mutable int radioOption_ = 0;               // the option ParamPartAt found (Part::Radio)
    Part ParamPartAt(int x, int y, int& row) const;
    double ParamValue(const params::Row& r) const;
    // The row with `enabledBy` applied: unavailable while that parameter is off (11.1).
    params::Row Effective(const params::Row& r) const {
        params::Row e = r;
        if (r.enabledBy >= 0) { params::Row p = r; p.paramIndex = r.enabledBy; if (ParamValue(p) == 0) e.enabled = false; }
        return e;
    }
    static constexpr int kKeyBase = 1000;     // Keyboard Entry row ids start here
    const params::Row& RowAt(int id) const;
    int RowTitleTop(int id) const;
    double RowValue(int id) const;
    void RowChanged(int id, double value, bool final);
    bool OnCreateButton(int x, int y) const;
    void BeginTyping(int row);
    void BeginNameTyping();
    static constexpr int kNameRow = -2;       // typingRow_ while editing the name field
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
    bool open_[kRollCount] = {true, true, true, false, true, true, true};   // R3: Keyboard Entry starts collapsed
    ModifyInfo modify_;
    std::vector<params::Spec> modSpecs_;      // the Modify tab's rollouts below the stack
    std::vector<bool> modOpen_;
    HWND stackCombo_ = nullptr, levelCombo_ = nullptr;
    params::Spec spec_;                  // Parameters of the active primitive
    params::KeyboardSpec keyboard_;      // Keyboard Entry of the active primitive
    std::vector<double> keValues_;       // its typed values (X, Y, Z, sizes)
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
