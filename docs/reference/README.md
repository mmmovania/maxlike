# Reference material

- `screenshots/max-r3.1-default-1920x1040.png` — R3.1 default empty scene, source of `docs/spec/01-ui-look.md`.
- `screenshots/r3.1-summary-*.png` — R3.1 File → Summary Info for the cone, cylinder,
  tube, torus, sphere, geosphere and teapot samples: Max's own vertex/face counts
  (`docs/spec/04-primitives.md`).
- `screenshots/r3.1-cylinder-selected-zoomed.png`, `r3.1-cone-selected-zoomed.png` —
  wireframes in R3.1 (views zoomed, so not pixel-comparable): cap spokes hidden, rings
  start at +X, selected objects drawn white.
- `screenshots/r3.1-objects-tab.png` — R3.1 with the tab panel's Objects tab selected:
  source of the Objects strip (step 6.1).
- `screenshots/r3.1-box-created.png` — R3.1 right after creating Box01 (Box still active):
  the creation rollouts, spinners, checked button, enabled name field; source of step 5.3b.
- `max-reader-handoff.md` — status of the JS `.max` reader. Its regression table's
  triangle counts for parametric primitives other than Box and Pyramid come from three.js
  generators, not from Max (e.g. cone 264, teapot 992; R3.1 says 288 and 1024).
- `max-format.md` — the `.max` format reference from the JS project (✔ verified,
  ◐ partial, ○ unverified), plus §7.6 with what maxlike verified against R3.1.
- `js-loader/max_loader.js` — the JS loader (three.js r150+) the C++ code is ported from.
  Note: except Box and Pyramid, its primitives use three.js geometry generators, which
  match Max's sizes and triangle counts but not necessarily Max's vertex order.

Still to add:
- Keyboard Entry opened (Box clicked) to measure its contents; one screenshot
  mid-creation; the tab panel's other tabs (Shapes, Lights & Cameras, Helpers) when
  their features come.
- **For Phase 6**: an object selected in each view (white wireframe, the Perspective
  view's corner brackets, the transform axes) at the default zoom, with Select and Move
  active.
- For later steps: R3's Object Color dialog (click the swatch); the Geometry dropdown
  open; the name field with an object selected.
- The sample `.max` files (in `docs/reference/samples/`; `.max` is marked binary in
  `.gitattributes`); Summary Info of box, plane and pyramid; wireframe screenshots at the
  default zoom (pixel-comparable); a shaded Perspective view.

Screenshots: full screen, Max maximised at 1920 × 1080, as the reference.
