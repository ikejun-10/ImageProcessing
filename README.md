**English** | [日本語](README.ja.md)

# imageProcessing

A single-EXE Windows desktop app that rectifies a photo of paper or a circular object shot at an angle into a head-on view.
Load an image, enclose the target with Quad (4 points) or Ellipse, and a projective-transformed, rectified preview is shown in real time — exportable as PNG / JPEG.

> 💡 **Want to try it right away?** Download `ImageProcessing.exe` from [Releases](https://github.com/ikejun-10/ImageProcessing/releases/latest) and run it directly (no installation, no extra libraries).

![App screenshot](img/app.png)

---

## Features

- Import / Export: load and save PNG / JPEG
- Two selection modes
  - Quad (4 points): enclose the target with 4 points to correct a distorted quadrilateral into a head-on rectangle
  - Ellipse: draw / move / resize / rotate an ellipse to crop and rectify its interior
- Left pane: zoom with the wheel (centered on the cursor) / pan with middle-button drag
- Right pane: real-time preview of the result (reflecting output size and rotation)
- Output adjustment: set output width / height / rotation angle via slider or numeric input (EditBox)
- Brightness: adjust brightness with a slider

---

## Usage

1. Launch `bin\ImageProcessing.exe`
2. Load a PNG / JPEG from the menu `File → Import Picture...`
3. Select the target in the left pane
   - Quad mode: click the four corners (any order — they are sorted into the correct order internally)
   - Ellipse mode: switch the mode, draw an ellipse, and move / resize / rotate it with the handles
4. Check the rectified preview in the right pane
   - Fine-tune output width / height / rotation / brightness with sliders or numeric input
5. Save as PNG / JPEG via `File → Export Corrected...`
   - Outside the boundary: black in Quad mode, transparent in Ellipse mode (saved with alpha in PNG)

### Quad (4-point) correction

Enclose a quadrilateral shot at an angle with 4 points to rectify it into a head-on rectangle.

![Quad correction](img/quad.png)

### Ellipse correction

Draw an ellipse to crop its interior, then adjust rotation and size to rectify it.

![Ellipse correction](img/ellipse.png)

---

## Build

Assumes CMake + MSVC (Visual Studio 2022):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

On success, `bin\ImageProcessing.exe` is produced.

---

## Tech stack

| Area | Technology |
|---|---|
| Language | C++17 |
| GUI | Win32 API (raw) |
| Rendering | GDI+ (PNG/JPEG I/O, brightness adjustment) |
| Correction algorithm | Homography / inverse-mapping interpolation |
| Build | CMake |

---

## Design & implementation

### 1. Solving the homography (projective transform) as an 8-variable linear system

The transform that maps a distorted quadrilateral onto a head-on rectangle is expressed by a 3×3 homography matrix `H` (8 degrees of freedom, with `h8 = 1` fixed).

```
x = (h0·X + h1·Y + h2) / (h6·X + h7·Y + 1)
y = (h3·X + h4·Y + h5) / (h6·X + h7·Y + 1)
```

Four point correspondences yield 2 equations each — 8 in total — solved by Gauss–Jordan elimination with partial pivoting (to avoid numerical instability).

### 2. Reordering the 4 points deterministically

The user's click order is free, but unless it is clockwise the homography twists into a bow-tie. So a full permutation search (4! = 24) discards orderings whose opposite edges cross and picks the one with the largest area. The result is unique and deterministic for the same set of vertices.

### 3. Inverse-mapping bilinear interpolation

Forward mapping (input→output) leaves holes and overlaps in the output, so inverse mapping (output→input) is used instead. For each output pixel, the 4 nearest pixels around the mapped real-valued coordinate are weighted-averaged for smooth correction.
The pixel format is `PixelFormat32bppPARGB` (premultiplied alpha).

### 4. Local coordinate system for the ellipse (Ellipse mode)

The ellipse is represented as `(center cx, cy / radii a, b / rotation θ)`. The output grid is mapped into the ellipse-local coordinates, then converted to image coordinates while accounting for rotation θ.

```
srcX = cx + lx·cosθ − ly·sinθ
srcY = cy + lx·sinθ + ly·cosθ
```

If the normalized coordinate satisfies `nx² + ny² > 1`, the pixel is judged outside the ellipse and made transparent → clean alpha when saved as PNG.

### 5. Brightness via the GDI+ `ColorMatrix`

Putting `b` in the last row of the 5×5 `ColorMatrix` (an affine transform) adds a bias to every RGB channel — adjusting brightness with nothing more.

### 6. Cursor-centered zoom

By keeping the image coordinate under the cursor fixed and recomputing the pan offset, zooming in/out stays intuitive and the point of interest never drifts.

### 7. Win32 craftsmanship

- EditBox subclassing: `SetWindowSubclass` captures Enter and notifies the parent when a value is committed
- Double buffering: everything is drawn to an off-screen `Bitmap` and blitted once to prevent flicker

---

## Project structure

```
src/
├── app/                      # App entry point (wWinMain / WndProc)
└── modules/
    ├── core/                 # State, geometry primitives (linear solver, homography, 4-point ordering)
    ├── io/                   # Import / Export dialogs and saving
    ├── view/                 # Display geometry, coordinate transforms, hit testing, rendering
    ├── controls/             # Layout, EditBox handling, mouse/wheel input
    └── correction/           # Building the corrected preview (Quad / Ellipse)
```
