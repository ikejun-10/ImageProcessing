#include "edit_apply.h"

#include <algorithm>

#include "corrector.h"
#include "state.h"
#include "ui.h"

namespace app::edit {

namespace {
using app::g_brightness;
using app::g_brightnessEdit;
using app::g_brightnessSlider;
using app::g_heightEdit;
using app::g_heightSlider;
using app::g_outputHeight;
using app::g_outputRotateDeg;
using app::g_outputWidth;
using app::g_rotateEdit;
using app::g_rotateSlider;
using app::g_widthEdit;
using app::g_widthSlider;

using app::kBrightnessEditId;
using app::kColorSliderMax;
using app::kColorSliderMin;
using app::kHeightEditId;
using app::kRotateEditId;
using app::kSliderMax;
using app::kSliderMin;
using app::kWidthEditId;
}  // namespace

bool ApplyNumericEditValue(HWND hwnd, UINT editId) {
    if (editId == kWidthEditId) {
        int v = 0;
        if (app::ui::TryReadIntFromEdit(g_widthEdit, &v)) {
            g_outputWidth = std::clamp(v, kSliderMin, kSliderMax);
            SendMessageW(g_widthSlider, TBM_SETPOS, TRUE, g_outputWidth);
            app::ui::UpdateSliderLabels();
            app::corrector::UpdateCorrectedPreview();
            app::ui::InvalidateImageAreas(hwnd, false, true);
        } else {
            app::ui::UpdateSliderLabels();
        }
        return true;
    }
    if (editId == kHeightEditId) {
        int v = 0;
        if (app::ui::TryReadIntFromEdit(g_heightEdit, &v)) {
            g_outputHeight = std::clamp(v, kSliderMin, kSliderMax);
            SendMessageW(g_heightSlider, TBM_SETPOS, TRUE, g_outputHeight);
            app::ui::UpdateSliderLabels();
            app::corrector::UpdateCorrectedPreview();
            app::ui::InvalidateImageAreas(hwnd, false, true);
        } else {
            app::ui::UpdateSliderLabels();
        }
        return true;
    }
    if (editId == kRotateEditId) {
        int v = 0;
        if (app::ui::TryReadIntFromEdit(g_rotateEdit, &v)) {
            g_outputRotateDeg = std::clamp(v, -180, 180);
            SendMessageW(g_rotateSlider, TBM_SETPOS, TRUE, g_outputRotateDeg);
            app::ui::UpdateSliderLabels();
            app::ui::InvalidateImageAreas(hwnd, false, true);
        } else {
            app::ui::UpdateSliderLabels();
        }
        return true;
    }
    if (editId == kBrightnessEditId) {
        int v = 0;
        if (app::ui::TryReadIntFromEdit(g_brightnessEdit, &v)) {
            const int val = std::clamp(v, -100, 100);
            g_brightness = std::clamp(val + 100, kColorSliderMin, kColorSliderMax);
            SendMessageW(g_brightnessSlider, TBM_SETPOS, TRUE, g_brightness);
            app::ui::UpdateBrightnessLabels();
            app::ui::InvalidateImageAreas(hwnd, true, true);
        } else {
            app::ui::UpdateBrightnessLabels();
        }
        return true;
    }
    return false;
}

}  // namespace app::edit

