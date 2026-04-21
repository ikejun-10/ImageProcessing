# imageMaker

A single Win32 + GDI+ desktop app. Load an image, select a region using **Quad (4 points)** or **Ellipse**, preview the corrected output, and export as PNG/JPEG.

## Project layout

- `src/`: source code
- `bin/`: output directory for the executable (`ImageProcessing.exe`)
- `build/`: CMake build directory (recommended)
- `docs/`: notes / architecture docs

Inside `src/`, the code is organized into:

- `src/app/`: app entry point only (`main.cpp`, `window_proc.*`)
- `src/modules/`: all application modules (state/ui/view/render/corrector/geometry + input/IO/edit-apply). **Includes keep using** `#include "state.h"` etc via the CMake include path.

### `src/app/` files (after split)

- `main.cpp`: entry point (`wWinMain`), init, message loop
- `window_proc.*`: `WndProc` and message dispatch

## Build (CMake)

Example with Visual Studio (MSVC):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

On success, `bin/ImageProcessing.exe` will be generated.

## How to use

1) Launch `bin/ImageProcessing.exe`  
2) `File -> Import Picture...` to load a PNG/JPEG  
3) In the left pane, select using Quad (4 points) or switch mode to Ellipse and adjust (drag/resize/rotate)  
4) `File -> Export Corrected...` to save as PNG/JPEG

