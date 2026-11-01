#include "ui/MenuSpec.h"

namespace ui {

namespace {
MenuItem Cmd(const wchar_t* text, const char* command) { return {text, command, {}}; }
MenuItem Sep() { return {}; }
MenuItem Sub(const wchar_t* text, std::vector<MenuItem> children) {
    return {text, "", std::move(children)};
}
} // namespace

// Reconstructed from memory of R3.1 and NOT yet checked item by item against the
// real program: top-level titles match the reference screenshot; mnemonics and the
// items inside each menu are to be verified (docs/spec/03-menus.md).
// Keyboard shortcuts are left out until accelerators exist.
const std::vector<MenuItem>& MainMenuSpec() {
    static const std::vector<MenuItem> spec = {
        Sub(L"&File", {
            Cmd(L"&New", "file.new"),
            Cmd(L"&Reset", "file.reset"),
            Cmd(L"&Open...", "file.open"),
            Cmd(L"&Save", "file.save"),
            Cmd(L"Save &As...", "file.save_as"),
            Cmd(L"Save Selected...", "file.save_selected"),
            Sep(),
            Cmd(L"XRef Objects...", "file.xref_objects"),
            Cmd(L"XRef Scenes...", "file.xref_scenes"),
            Sep(),
            Cmd(L"&Merge...", "file.merge"),
            Cmd(L"Replace...", "file.replace"),
            Sep(),
            Cmd(L"&Import...", "file.import"),
            Cmd(L"&Export...", "file.export"),
            Cmd(L"Export Selected...", "file.export_selected"),
            Sep(),
            Cmd(L"Archive...", "file.archive"),
            Sep(),
            Cmd(L"Summary Info...", "file.summary_info"),
            Cmd(L"File Properties...", "file.properties"),
            Cmd(L"View File...", "file.view_file"),
            Sep(),
            Cmd(L"E&xit", "file.exit"),
        }),
        Sub(L"&Edit", {
            Cmd(L"&Undo", "edit.undo"),
            Cmd(L"&Redo", "edit.redo"),
            Sep(),
            Cmd(L"&Hold", "edit.hold"),
            Cmd(L"&Fetch", "edit.fetch"),
            Sep(),
            Cmd(L"&Delete", "edit.delete"),
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
                Cmd(L"&Window", "edit.region_window"),
                Cmd(L"&Crossing", "edit.region_crossing"),
            }),
            Sep(),
            Cmd(L"Edit Named Selections...", "edit.named_selections"),
            Sep(),
            Cmd(L"&Properties...", "edit.properties"),
        }),
        Sub(L"&Tools", {
            Cmd(L"Transform Type-&In...", "tools.transform_type_in"),
            Cmd(L"&Display Floater...", "tools.display_floater"),
            Cmd(L"&Selection Floater...", "tools.selection_floater"),
            Sep(),
            Cmd(L"&Mirror...", "tools.mirror"),
            Cmd(L"&Array...", "tools.array"),
            Cmd(L"Snapshot...", "tools.snapshot"),
            Cmd(L"A&lign...", "tools.align"),
            Cmd(L"Align &Normals...", "tools.align_normals"),
            Cmd(L"Place &Highlight", "tools.place_highlight"),
            Cmd(L"Align &Camera", "tools.align_camera"),
            Cmd(L"Align to &View...", "tools.align_to_view"),
            Sep(),
            Cmd(L"Material &Editor...", "tools.material_editor"),
            Cmd(L"Material/Map &Browser...", "tools.material_browser"),
        }),
        Sub(L"&Group", {
            Cmd(L"&Group...", "group.group"),
            Cmd(L"&Ungroup", "group.ungroup"),
            Cmd(L"&Open", "group.open"),
            Cmd(L"&Close", "group.close"),
            Cmd(L"&Attach", "group.attach"),
            Cmd(L"&Detach", "group.detach"),
            Cmd(L"&Explode", "group.explode"),
        }),
        Sub(L"&Views", {
            Cmd(L"&Undo View Change", "views.undo"),
            Cmd(L"&Redo View Change", "views.redo"),
            Cmd(L"&Save Active View", "views.save_active"),
            Cmd(L"Restore Active View", "views.restore_active"),
            Sep(),
            Sub(L"&Grids", {
                Cmd(L"&Show Home Grid", "views.grid_show_home"),
                Cmd(L"&Activate Home Grid", "views.grid_activate_home"),
                Cmd(L"Activate Grid &Object", "views.grid_activate_object"),
                Cmd(L"Align Grid to &View", "views.grid_align_to_view"),
            }),
            Cmd(L"Viewport &Background...", "views.background"),
            Cmd(L"Update Background Image", "views.update_background"),
            Cmd(L"Reset Background Transform", "views.reset_background_transform"),
            Sep(),
            Cmd(L"Show Transform Gizmo", "views.show_gizmo"),
            Cmd(L"Show Ghosting", "views.show_ghosting"),
            Cmd(L"Show Key Times", "views.show_key_times"),
            Cmd(L"Shade Selected", "views.shade_selected"),
            Cmd(L"Show Dependencies", "views.show_dependencies"),
            Sep(),
            Cmd(L"Match Camera to View", "views.match_camera"),
            Cmd(L"Add Default Lights to Scene", "views.add_default_lights"),
            Cmd(L"Redraw All Views", "views.redraw_all"),
            Cmd(L"Activate All Maps", "views.activate_maps"),
            Cmd(L"Deactivate All Maps", "views.deactivate_maps"),
            Cmd(L"Update During Spinner Drag", "views.update_during_spinner_drag"),
            Cmd(L"E&xpert Mode", "views.expert_mode"),
            Sep(),
            Cmd(L"Viewport &Configuration...", "views.configure"),
        }),
        Sub(L"&Rendering", {
            Cmd(L"&Render...", "render.render"),
            Cmd(L"&Video Post...", "render.video_post"),
            Cmd(L"Show &Last Rendering", "render.show_last"),
            Sep(),
            Cmd(L"&Environment...", "render.environment"),
            Cmd(L"E&ffects...", "render.effects"),
            Sep(),
            Cmd(L"&Make Preview...", "render.make_preview"),
            Cmd(L"View &Preview", "render.view_preview"),
            Cmd(L"Re&name Preview...", "render.rename_preview"),
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
            Cmd(L"&Customize User Interface...", "customize.ui"),
            Cmd(L"&Load Custom UI...", "customize.load_ui"),
            Cmd(L"&Save Custom UI As...", "customize.save_ui"),
            Cmd(L"Loc&k UI Layout", "customize.lock_ui"),
            Cmd(L"&Revert to Startup UI Layout", "customize.revert_ui"),
            Sub(L"Show &UI", {
                Cmd(L"Show &Command Panel", "customize.show_command_panel"),
                Cmd(L"Show &Tab Panel", "customize.show_tab_panel"),
                Cmd(L"Show &Floating Toolbars", "customize.show_floating_toolbars"),
                Cmd(L"Show &Main Toolbar", "customize.show_main_toolbar"),
                Cmd(L"Show T&rack Bar", "customize.show_track_bar"),
            }),
            Sep(),
            Cmd(L"Configure &Paths...", "customize.paths"),
            Cmd(L"U&nits Setup...", "customize.units"),
            Cmd(L"&Grid and Snap Settings...", "customize.grid_snap"),
            Cmd(L"&Viewport Configuration...", "customize.viewport_config"),
            Sep(),
            Cmd(L"Pr&eferences...", "customize.preferences"),
        }),
        Sub(L"&MAXScript", {
            Cmd(L"&New Script", "maxscript.new"),
            Cmd(L"&Open Script...", "maxscript.open"),
            Cmd(L"&Run Script...", "maxscript.run"),
            Sep(),
            Cmd(L"MAXScript &Listener...", "maxscript.listener"),
            Cmd(L"&Macro Recorder", "maxscript.macro_recorder"),
        }),
        Sub(L"&Help", {
            Cmd(L"&User Reference", "help.user_reference"),
            Cmd(L"&MAXScript Reference", "help.maxscript_reference"),
            Cmd(L"&Tutorials", "help.tutorials"),
            Cmd(L"&Hotkey Map", "help.hotkey_map"),
            Sep(),
            Cmd(L"&About maxlike...", "help.about"),
        }),
    };
    return spec;
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
