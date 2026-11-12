# Phase 11 — Technical documentation

Phase 11 worked through list B of Phase 9's inventory: items that needed evidence from
R3.1. Four batches of screenshots and seven sample files (saved under
`docs/reference/screenshots/r3.1-*.png` and `docs/reference/samples/`) were compared with
maxlike, and maxlike was corrected. Step record: `docs/phases/phase-11-evidence.md`.

Code at the end of Phase 11: tag `phase-11-done` (version 0.11.0).

## Contents
1. Kinds of evidence and what each can settle
2. Measuring which controls are greyed
3. Fitting a drawing to a function (the falloff graph)
4. Dialogs laid out from measured pixels
5. Choosing between hypotheses by fitting (Bend about X)
6. Reading undocumented flags: compare across every sample
7. File structures found
8. Testing notes
9. Results
10. Open items
11. Exercises

---

## 1. Kinds of evidence and what each can settle

| Evidence | Settles | Example |
|---|---|---|
| A screenshot of a panel or menu | layout, wording, which items are available | Edit Mesh at each level; Region's check marks |
| A screenshot of a scene | colours, line styles, results of operations | the open group's #FF7FB2; dotted hidden edges |
| A sample file | storage, parameters, and — opened in maxlike next to a screenshot of it — evaluation | Bend's parameters; Bend about X |
| The absence of something | weak evidence, recorded as inferred | no question when deleting one teapot face |

A screenshot of a dialog cannot be compared pixel for pixel here (the test display does
not paint dialogs), so dialogs were laid out from measurements instead (§4).

---

## 2. Measuring which controls are greyed

Phase 7's comparison of the Edit Mesh panel masks all text, because Wine's font differs
slightly from Windows'. A consequence went unnoticed until now: it never checked whether a
control was available or greyed, since that is a property of the text. Phase 11 added a
second measurement: for each control's text area, is any pixel near-black (available) or
not (greyed, with the white emboss)? Run on 27 controls at all five levels against the
screenshots, it found the levels' differences (Create unavailable at Edge level, By Vertex
above Vertex level, Ignore Visible Edges only at Polygon level, and more) and then
confirmed every one fixed.

The general point: when a comparison masks something for a good reason, list what the
mask hides and check it some other way.

---

## 3. Fitting a drawing to a function (the falloff graph)

Soft Selection's rollout has a curve. Its formula was known (Max's affect-region
function); its scale and placement were not. The drawn curve's pixels were extracted and
the free parameters (centre, half-width, rows for weights 1 and 0) fitted to them. The fit
showed that −Falloff…+Falloff spans the frame's full outer width, a choice nobody would
guess. Rendered, maxlike's box is 93.8 % pixel-identical, the curve within 1 px of R3's on
every row.

A side-by-side picture caught what the numbers missed: the guide lines are the face colour
(very faint), not black, because the first analysis only distinguished white from
non-white.

---

## 4. Dialogs laid out from measured pixels

Each dialog's group-box frames, white fields and buttons were measured in the screenshot
and converted to dialog units (1.5 px per horizontal unit, 1.625 per vertical, for MS Sans
Serif 8). Two dialogs then measured in the running program confirm the conversion: New
Scene 255 x 153 px and the Type-In floater 288 x 101, R3.1's client areas exactly and
within a pixel.

The dialogs also carried behaviour: Type-In offsets in screen axes ("Offset:Screen"),
Align along the reference system with Apply, Array's Totals, Re-Orient and Uniform, New
Scene's options, Merge's object list.

---

## 5. Choosing between hypotheses by fitting (Bend about X)

Opened in maxlike, R3's `bend.max` (Angle 45, Direction 30, axis X) bent the wrong way:
the box narrowed upwards where R3's widens, 86 px off on average, while the bottom
matched exactly. Phase 7 had verified Bend only about Z; for X, maxlike's mapping of the
bend plane came from its own formula.

Instead of reasoning about Max's conventions, the outline was modelled: the projection was
calibrated on maxlike's own render (0.51 px mean error), then all eight sign and order
choices for the bend plane were scored against R3's outline. One fitted to 0.53 px (the
calibration's own level), the next best 20.6 px. With it, maxlike matches R3's Front view
on all 345 rows to 0 px. The Phase 7 test that had encoded the old direction came from
maxlike's formula, not from R3, and was changed with that stated.

---

## 6. Reading undocumented flags: compare across every sample

A node's flags word (chunk 0x0960) was decoded by comparing files. `group.max` showed 0x40
(group head) and 0x100 (member). For Backface Cull, `backface-off.max` against `box.max`
first suggested 0x800. Listing the flags of all 26 sample nodes refuted it: cloned objects
lack 0x800 (they have 0x1000), yet R3 draws them culled. The one bit present in every node
but the backface-off one is 0x00800000. Reading 0x800 would have broken three samples'
renderings verified in Phase 8.

Rule: a bit that differs between two files is a candidate; it is confirmed only if it
explains every file.

---

## 7. File structures found

| Item | Structure |
|---|---|
| Bend / Taper / Twist | class 0x10 / 0x20 / 0x90, superclass 0x810; references: gizmo (Position/Rotation/Scale controller), centre (Bezier Position), old ParamBlock with Max's parameter order (which Phase 7 already used) |
| Controllers | Bezier Position 0x2503 (3 floats), TCB Rotation 0x2504 (Max quaternion), Bezier Scale 0x2505 (scale, axis); now read by shared functions for nodes and gizmos |
| Node flags | 0x0960: parent index, flags; 0x40 group head, 0x100 member, 0x00800000 Backface Cull |
| Group head | a Node whose object is a Dummy |
| Material | node reference 3; Standard (0x2, superclass 0xC00) → shader (reference 2, Blinn) → ParamBlock2 → colour parameter 1 (diffuse) |
| ParamBlock2 colours | entry types 2 (RGB) and 3 (point): three floats after the flags |

The samples had more in them than planned: taper.max and twist.max carry the modifiers
below the top one (at defaults), and bend-gizmo.max's gizmo was dragged and turned about
Y. The importer reads what is stored; the tests were written to the files.

---

## 8. Testing notes

- **Stale binaries.** A link error stopped a build, and a test "passed" with the previous
  binary. Build times are now checked before trusting a run.
- **An edit that never happened.** A Python syntax error stopped an edit script before any
  change; a test still passed because it was the unchanged one. The edit was rerun and the
  file checked.
- **Expectations are hypotheses too.** Several failures were mine: a Window-selection test
  that expected nothing where one square lay inside; a Bend test missing the axis; a
  stacked sample assumed to hold one modifier; a stale capture compared twice.
- **A dialog's controls by keyboard.** Clicks aimed from computed positions missed;
  choosing options with arrow keys and Enter is robust.
- **Line endings.** Insertions into CRLF files twice left bare or doubled endings; a
  check now runs on every changed file before each commit.

---

## 9. Results

- Edit Mesh: availability at all five levels measured and matching (27 controls), the count
  wording, Divide and Bevel, dotted hidden edges, the Delete question only when needed,
  Soft Selection's layout and curve.
- Dialogs: New Scene, Merge's object list, Clone, Type-In, Mirror, Align, Array, Detach and
  Explode names, the save prompt's words.
- Menus and displays: Region (Window / Crossing), Grids, the open group's colour, the
  gizmo's colour at Gizmo level confirmed.
- `.max` files: Bend, Taper, Twist with gizmos; groups; Backface Cull; a material's
  diffuse colour. Bend about X corrected.
- 36 of 114 menu commands; forty test programs; 0 warnings.

---

## 10. Open items

- Still wanted from R3.1: Edit Mesh with the panel scrolled down at Edge and Face level;
  `bend.max` selected at object level (the gizmo's colour there); menus opened with Alt;
  a far Perspective view; the primitives' Creation Method and Keyboard Entry rollouts.
- Unverified: Bend about Y, Taper and Twist about X or Y; the group-open bit; the Delete
  question's look; Bevel by dragging and spinners by dragging; Reference cloning; Mirror IK
  Limits (no IK); materials beyond a Standard diffuse colour.

---

## 11. Exercises

| Step | Try |
|---|---|
| 11.1 | Edit Mesh on a box: visit each level and compare the buttons with the screenshots. |
| 11.2 | File > Merge from `instances.max`: pick two boxes with the name field, merge. |
| 11.2b | Array with Totals: 360 degrees about Z over 8 copies, Re-Orient off. |
| 11.3 | Edit > Region > Window; drag across part of an object. |
| 11.4 | Open `bend.max` and compare the Front view with `r3.1-bend-gizmo-object.png`. |
