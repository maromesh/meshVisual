## Shape Editor

This repo includes a small point-based shape editor that saves directly into the `.shape` files used by the visualizer.

Build it with:

```bash
cmake --build build --target shape_editor
```

Run it with an optional output name:

```bash
./build/shape_editor heart
```

Controls:

- Left click adds a point
- Backspace removes the last point
- `C` clears the canvas
- Enter saves the points to `assets/shapes/<name>.shape`
- Esc closes the window

Saved points are centered and uniformly scaled so the longest axis fits the existing `[-1, 1]` style shape format.
