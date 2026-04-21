#pragma once

#include <array>
#include <windows.h>
#include <gdiplus.h>
#include <vector>

namespace app::geom {

bool SolveLinear3x3(double a[3][4]);
bool SolveLinear8x8(double a[8][9]);

bool ComputeHomographyDestToSrcRect(
    const std::array<Gdiplus::PointF, 4>& srcPts,
    int outW,
    int outH,
    std::array<double, 8>* outHmat);

bool BuildOrderedQuadrilateral(const std::vector<Gdiplus::PointF>& points, std::array<Gdiplus::PointF, 4>* ordered);

}  // namespace app::geom

