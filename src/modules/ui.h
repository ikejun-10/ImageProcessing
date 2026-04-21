#pragma once

#include <windows.h>
#include <gdiplus.h>

#include "state.h"

namespace app::ui {

// Layout / invalidation helpers
app::ViewLayout GetViewLayout(HWND hwnd);
RECT ToWinRect(const Gdiplus::Rect& r);
void InvalidateImageAreas(HWND hwnd, bool left, bool right);

// Menu / controls creation
HMENU CreateMainMenu();
void CreateOrUpdateResetAndModeButtons(HWND hwnd);
void CreateOrUpdateSliders(HWND hwnd);
void CreateOrUpdateBrightnessControls(HWND hwnd);
void UpdateSliderLabels();
void UpdateBrightnessLabels();

// Edit-box input helpers
bool TryReadIntFromEdit(HWND edit, int* outValue);
LRESULT CALLBACK EditEnterSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData);

// Import/export dialogs
bool SaveCorrectedImageWithDialog(HWND hwnd);

}  // namespace app::ui

