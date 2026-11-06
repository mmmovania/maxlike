# Reference material

- `screenshots/max-r3.1-default-1920x1040.png` — R3.1 default empty scene, source of `docs/spec/01-ui-look.md`.
- `screenshots/r3.1-summary-*.png` — R3.1 File → Summary Info for the cone, cylinder,
  tube, torus, sphere, geosphere and teapot samples: Max's own vertex/face counts
  (`docs/spec/04-primitives.md`).
- `screenshots/r3.1-cylinder-selected-zoomed.png`, `r3.1-cone-selected-zoomed.png` —
  wireframes in R3.1 (views zoomed, so not pixel-comparable): cap spokes hidden, rings
  start at +X, selected objects drawn white.
- `max-reader-handoff.md` — status of the JS `.max` reader. Its regression table's
  triangle counts for parametric primitives other than Box and Pyramid come from three.js
  generators, not from Max (e.g. cone 264, teapot 992; R3.1 says 288 and 1024).
- `max-format.md` — the `.max` format reference from the JS project (✔ verified,
  ◐ partial, ○ unverified), plus §7.6 with what maxlike verified against R3.1.
- `js-loader/max_loader.js` — the JS loader (three.js r150+) the C++ code is ported from.
  Note: except Box and Pyramid, its primitives use three.js geometry generators, which
  match Max's sizes and triangle counts but not necessarily Max's vertex order.

Still to add:
- **For Phase 5** (most needed): R3.1 right after clicking Box, before dragging (the
  checked button and the Creation Method, Keyboard Entry and Parameters rollouts with
  their spinners); one screenshot mid-creation (base dragged, setting the height); a
  rollout collapsed.
- For later steps: R3's Object Color dialog (click the swatch); the Geometry dropdown
  open; the name field with an object selected.
- The sample `.max` files (in `docs/reference/samples/`; `.max` is marked binary in
  `.gitattributes`); Summary Info of box, plane and pyramid; wireframe screenshots at the
  default zoom (pixel-comparable); a shaded Perspective view.

Screenshots: full screen, Max maximised at 1920 × 1080, as the reference.
