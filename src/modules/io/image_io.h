#pragma once

#include <windows.h>

namespace app::io {

bool OpenAndLoadImage(HWND hwnd);
bool SaveCorrectedImageWithDialog(HWND hwnd);

}  // namespace app::io

