# Reference material

- `screenshots/max-r3.1-default-1920x1040.png` — R3.1 default empty scene, source of `docs/spec/01-ui-look.md`.
- `max-reader-handoff.md` — status of the JS `.max` reader.
- `max-format.md` — the `.max` format reference from the JS project (✔ verified,
  ◐ partial, ○ unverified), as of the handoff.
- `js-loader/max_loader.js` — the JS loader (three.js r150+) the C++ code is ported from.
  Note: except Box and Pyramid, its primitives use three.js geometry generators, which
  match Max's sizes and triangle counts but not necessarily Max's vertex order.

Still to add: the sample `.max` files (put them in `docs/reference/samples/`; `.max` is
marked binary in `.gitattributes`) and an R3.1 wireframe screenshot of each.
