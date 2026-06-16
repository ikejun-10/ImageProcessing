#pragma once

#include <windows.h>

namespace app::input {

// Handles mouse/scroll related messages (selection, zoom/pan, sliders).
// Returns true if handled and sets outResult.
bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* outResult);

}  // namespace app::input

