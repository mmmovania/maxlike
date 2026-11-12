#include "ui/MenuSpec.h"

namespace ui {

namespace {
MenuItem Cmd(const wchar_t* text, const char* command) { return {text, command, {}}; }
MenuItem Check(const wchar_t* text, const char* command) { MenuItem m{text, command, {}}; m.checked = true; return m; }
MenuItem Sep() { return {}; }
MenuItem Sub(const wchar_t* text, std::vector<MenuItem> children) {
    return {text, "", std::move(children)};
}
} // namespace

// The menus of 3ds Max R3.1, item by item from screenshots of every menu open
// (docs/reference/screenshots/r3.1-menu-*.png): labels, separators, the shortcut column
// (after a tab), the two items ticked by default. "About maxlike" replaces R3's "About 3D
// Studio MAX..."; its mnemonic is B so it does not clash with "Additional Help". The
// mnemonic letters are Phase 1's (R3 hides underlines until Alt; NOTE: unverified). The
// recently opened files are inserted above Exit when the File menu opens.
const std::vector<MenuItem>& MainMenuSpec() {
    static const std::vector<MenuItem> spec = {
        Sub(L"&File", {
            Cmd(L"&New...\tCtrl+N", "file.new"),
            Cmd(L"&Reset", "file.reset"),
            Cmd(L"&Open...\tCtrl+O", "file.open"),
            Sep(),
            Cmd(L"&Save\tCtrl+S", "file.save"),
            Cmd(L"Save &As...", "file.save_as"),
            Cmd(L"Save Selected...", "file.save_selected"),
            Sep(),
            Cmd(L"XRef Objects...", "file.xref_objects"),
            Cmd(L"XRef Scenes...", "file.xref_scenes"),
            Sep(),
            Cmd(L"&Merge...", "file.merge"),
            Cmd(L"Replace...", "file.replace"),
            Cmd(L"Merge Animation...", "file.merge_animation"),
            Sep(),
            Cmd(L"&Import...", "file.import"),
            Cmd(L"&Export...", "file.export"),
            Cmd(L"Export Selected...", "file.export_selected"),
            Sep(),
            Cmd(L"Archive...", "file.archive"),
            Cmd(L"Summary Info...", "file.summary_info"),
            Cmd(L"Properties...", "file.properties"),
            Cmd(L"View File...", "file.view_file"),
            Sep(),
            Cmd(L"E&xit", "file.exit"),
        }),
        Sub(L"&Edit", {
            Cmd(L"&Undo\tCtrl+Z", "edit.undo"),
            Cmd(L"&Redo\tCtrl+A", "edit.redo"),
            Sep(),
            Cmd(L"&Hold\tAlt+Ctrl+H", "edit.hold"),
            Cmd(L"&Fetch\tAlt+Ctrl+F", "edit.fetch"),
            Sep(),
            Cmd(L"&Delete\tDelete", "edit.delete"),
            Cmd(L"&Clone", "edit.clone"),
            Sep(),
            Cmd(L"Select &All", "edit.select_all"),
            Cmd(L"Select &None", "edit.select_none"),
            Cmd(L"Select &Invert", "edit.select_invert"),
            Sub(L"Select &By", {
                Cmd(L"&Color", "edit.select_by_color"),
                Cmd(L"&Name...", "edit.select_by_name"),
            }),
            Sub(L"Re&gion", {
                // Check items, Crossing by default (r3.1-menu-region.png).
                Check(L"&Window", "edit.region_window"),
                Check(L"&Crossing", "edit.region_crossing"),
            }),
            Sep(),
            Cmd(L"Edit Named Selections...", "edit.named_selections"),
            Sep(),
            Cmd(L"&Properties...", "edit.properties"),
        }),
        Sub(L"&Tools", {
            Cmd(L"Transform Type-&In...\tF12", "tools.transform_type_in"),
            Cmd(L"&Display Floater...", "tools.display_floater"),
            Cmd(L"&Selection Floater...", "tools.selection_floater"),
            Cmd(L"&Mirror...", "tools.mirror"),
            Cmd(L"&Array...", "tools.array"),
            Cmd(L"Snapshot...", "tools.snapshot"),
            Cmd(L"A&lign...\tAlt+A", "tools.align"),
            Cmd(L"&Normal Align...\tAlt+N", "tools.align_normals"),
            Cmd(L"Place &Highlight\tCtrl+H", "tools.place_highlight"),
            Cmd(L"Align &Camera...", "tools.align_camera"),
            Cmd(L"Align to &View...", "tools.align_to_view"),
            Cmd(L"Material &Editor...\tM", "tools.material_editor"),
            Cmd(L"Material/Map &Browser...", "tools.material_browser"),
            Cmd(L"Spacing Tool...\tShift+I", "tools.spacing"),
        }),
        Sub(L"&Group", {
            Cmd(L"&Group", "group.group"),
            Cmd(L"&Open", "group.open"),
            Cmd(L"&Close", "group.close"),
            Cmd(L"&Ungroup", "group.ungroup"),
            Cmd(L"&Explode", "group.explode"),
            Cmd(L"&Detach", "group.detach"),
            Cmd(L"&Attach", "group.attach"),
        }),
        Sub(L"&Views", {
            Cmd(L"&Undo\tShift+Z", "views.undo"),
            Cmd(L"&Redo\tShift+A", "views.redo"),
            Sep(),
            Cmd(L"&Save Active Perspective View", "views.save_active"),
            Cmd(L"Restore Active Perspective View", "views.restore_active"),
            Sub(L"&Grids", {
                // r3.1-menu-grids.png: Show Home Grid ticked; the others greyed (no grid objects).
                Check(L"&Show Home Grid", "views.grid_show_home"),
                Cmd(L"&Activate Home Grid", "views.grid_activate_home"),
                Cmd(L"Activate Grid &Object", "views.grid_activate_object"),
                Cmd(L"Align To &View", "views.grid_align_to_view"),
            }),
            Sep(),
            Cmd(L"Viewport &Background...\tAlt+B", "views.background"),
            Cmd(L"Update Background Image\tAlt+Shift+Ctrl+B", "views.update_background"),
            Cmd(L"Reset Background Transform", "views.reset_background_transform"),
            Sep(),
            Check(L"Show Transform Gizmo", "views.show_gizmo"),
            Cmd(L"Show Ghosting", "views.show_ghosting"),
            Cmd(L"Show Key Times", "views.show_key_times"),
            Cmd(L"Shade Selected", "views.shade_selected"),
            Cmd(L"Show Dependencies", "views.show_dependencies"),
            Sep(),
            Cmd(L"Match Camera to View\tCtrl+C", "views.match_camera"),
            Cmd(L"Add Default Lights to Scene...", "views.add_default_lights"),
            Sep(),
            Cmd(L"Redraw All Views\t1", "views.redraw_all"),
            Cmd(L"Deactivate All Maps", "views.deactivate_maps"),
            Check(L"Update During Spinner Drag", "views.update_during_spinner_drag"),
            Sep(),
            Cmd(L"E&xpert Mode\tCtrl+X", "views.expert_mode"),
        }),
        Sub(L"&Rendering", {
            Cmd(L"&Render...", "render.render"),
            Cmd(L"&Video Post...", "render.video_post"),
            Cmd(L"Show &Last Rendering\tCtrl+I", "render.show_last"),
            Sep(),
            Cmd(L"&Environment...", "render.environment"),
            Cmd(L"E&ffects...", "render.effects"),
            Sep(),
            Cmd(L"&Make Preview...", "render.make_preview"),
            Cmd(L"View &Preview...", "render.view_preview"),
            Cmd(L"Re&name Preview...", "render.rename_preview"),
            Sep(),
            Cmd(L"RAM Player...", "render.ram_player"),
        }),
        Sub(L"Trac&k View", {
            Cmd(L"&Open Track View", "trackview.open"),
            Cmd(L"&New Track View", "trackview.new"),
            Cmd(L"&Delete Track View...", "trackview.delete"),
        }),
        Sub(L"&Schematic View", {
            Cmd(L"&Open Schematic View", "schematic.open"),
            Cmd(L"&New Schematic View", "schematic.new"),
            Cmd(L"&Delete Schematic View...", "schematic.delete"),
        }),
        Sub(L"&Customize", {
            Cmd(L"&Load Custom UI...", "customize.load_ui"),
            Cmd(L"&Save Custom UI As...", "customize.save_ui"),
            Sep(),
            Cmd(L"Loc&k UI Layout", "customize.lock_ui"),
            Cmd(L"&Revert to Startup UI Layout", "customize.revert_ui"),
            Cmd(L"&Customize UI...", "customize.ui"),
            Sep(),
            Cmd(L"Configure &Paths...", "customize.paths"),
            Cmd(L"Pr&eferences...", "customize.preferences"),
            Sep(),
            Cmd(L"&Viewport Configuration...", "customize.viewport_config"),
            Sep(),
            Cmd(L"U&nits Setup...", "customize.units"),
            Cmd(L"&Grid and Snap Settings...", "customize.grid_snap"),
        }),
        Sub(L"&MAXScript", {
            Cmd(L"&New Script", "maxscript.new"),
            Cmd(L"&Open Script...", "maxscript.open"),
            Cmd(L"&Run Script...", "maxscript.run"),
            Sep(),
            Cmd(L"MAXScript &Listener...\tF11", "maxscript.listener"),
            Sep(),
            Cmd(L"&Macro Recorder", "maxscript.macro_recorder"),
        }),
        Sub(L"&Help", {
            Cmd(L"&Online Reference...", "help.user_reference"),
            Cmd(L"&MAXScript Reference...", "help.maxscript_reference"),
            Cmd(L"&Learning 3D Studio MAX...", "help.learning"),
            Sep(),
            Cmd(L"&Additional Help...", "help.additional"),
            Cmd(L"&Connect to Support and Information...", "help.support"),
            Cmd(L"&Plug-in Information...", "help.plugin_information"),
            Sep(),
            Cmd(L"A&bout maxlike...", "help.about"),
        }),
    };
    return spec;
}

namespace {

void Collect(const std::vector<MenuItem>& items, std::vector<Shortcut>& out, std::vector<std::wstring>* unparsed) {
    for (const MenuItem& it : items) {
        if (it.IsSubmenu()) { Collect(it.children, out, unparsed); continue; }
        const size_t tab = it.text.find(L'\t');
        if (tab == std::wstring::npos || it.command.empty()) continue;
        std::wstring rest = it.text.substr(tab + 1);
        Shortcut s;
        s.command = it.command;
        bool ok = true;
        // Modifiers, each followed by '+'; then the key.
        for (;;) {
            if (rest.rfind(L"Ctrl+", 0) == 0) { s.ctrl = true; rest.erase(0, 5); }
            else if (rest.rfind(L"Alt+", 0) == 0) { s.alt = true; rest.erase(0, 4); }
            else if (rest.rfind(L"Shift+", 0) == 0) { s.shift = true; rest.erase(0, 6); }
            else break;
        }
        if (rest.size() == 1 && ((rest[0] >= L'A' && rest[0] <= L'Z') || (rest[0] >= L'0' && rest[0] <= L'9'))) s.key = static_cast<uint16_t>(rest[0]);
        else if (rest == L"Delete") s.key = 0x2E;
        else if (rest.size() >= 2 && rest[0] == L'F') {
            int n = 0;
            for (size_t i = 1; i < rest.size(); ++i) { if (rest[i] < L'0' || rest[i] > L'9') { ok = false; break; } n = n * 10 + (rest[i] - L'0'); }
            if (n < 1 || n > 24) ok = false;
            else s.key = static_cast<uint16_t>(0x70 + n - 1);
        } else ok = false;
        if (ok) out.push_back(s);
        else if (unparsed) unparsed->push_back(it.text.substr(tab + 1));
    }
}

} // namespace

std::vector<Shortcut> ShortcutsOf(const std::vector<MenuItem>& spec, std::vector<std::wstring>* unparsed) {
    std::vector<Shortcut> out;
    Collect(spec, out, unparsed);
    return out;
}

bool MenuItemEnabled(const std::string& c, const MenuState& s) {
    // Nothing to act on yet (no animation, background image, cameras, lights to add,
    // renderings, saved Track / Schematic Views, groups): greyed, as R3 shows them.
    static const char* const kNever[] = {
        "views.grid_activate_home", "views.grid_activate_object", "views.grid_align_to_view",   // 11.3: no grid objects
        "file.merge_animation", "views.undo", "views.redo", "views.update_background",
        "views.reset_background_transform", "views.match_camera", "views.add_default_lights",
        "render.show_last", "trackview.delete", "schematic.delete"};
    for (const char* n : kNever) if (c == n) return false;
    if (c == "edit.undo") return s.canUndo;
    if (c == "edit.redo") return s.canRedo;
    if (c == "views.restore_active") return s.savedPerspective;
    // Need a selection.
    if (c == "file.save_selected" || c == "file.export_selected" || c == "edit.clone" || c == "edit.select_none" ||
        c == "edit.properties" || c == "group.group") return s.selected > 0;
    // Groups (Phase 10).
    if (c == "group.open") return s.closedGroups > 0;
    if (c == "group.close") return s.openGroups > 0;
    if (c == "group.ungroup" || c == "group.explode") return s.groups > 0;
    if (c == "group.detach") return s.openMembers > 0;
    if (c == "group.attach") return s.selected > 0;
    // Need objects.
    if (c == "edit.select_all" || c == "edit.select_invert") return s.objects > 0;
    return true;
}

std::wstring PlainMenuText(const std::wstring& text) {
    std::wstring out;
    for (size_t i = 0; i < text.size(); ++i) {
        const wchar_t c = text[i];
        if (c == L'\t') break;
        if (c == L'&') {
            if (i + 1 < text.size() && text[i + 1] == L'&') { out += L'&'; ++i; } // "&&" = literal '&'
            continue;
        }
        out += c;
    }
    return out;
}

} // namespace ui
