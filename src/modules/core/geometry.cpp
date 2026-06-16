#include "geometry.h"

#include <algorithm>
#include <cmath>

namespace app::geom {

bool SolveLinear3x3(double a[3][4]) {
    for (int col = 0; col < 3; ++col) {
        int pivotRow = col;
        double pivotAbs = std::fabs(a[col][col]);
        for (int row = col + 1; row < 3; ++row) {
            const double v = std::fabs(a[row][col]);
            if (v > pivotAbs) {
                pivotAbs = v;
                pivotRow = row;
            }
        }
        if (pivotAbs < 1e-12) {
            return false;
        }
        if (pivotRow != col) {
            for (int k = col; k <= 3; ++k) {
                std::swap(a[col][k], a[pivotRow][k]);
            }
        }
        const double pivot = a[col][col];
        for (int k = col; k <= 3; ++k) {
            a[col][k] /= pivot;
        }
        for (int row = 0; row < 3; ++row) {
            if (row == col) continue;
            const double factor = a[row][col];
            for (int k = col; k <= 3; ++k) {
                a[row][k] -= factor * a[col][k];
            }
        }
    }
    return true;
}

bool SolveLinear8x8(double a[8][9]) {
    for (int col = 0; col < 8; ++col) {
        int pivotRow = col;
        double pivotAbs = std::fabs(a[col][col]);
        for (int row = col + 1; row < 8; ++row) {
            const double v = std::fabs(a[row][col]);
            if (v > pivotAbs) {
                pivotAbs = v;
                pivotRow = row;
            }
        }
        if (pivotAbs < 1e-12) {
            return false;
        }
        if (pivotRow != col) {
            for (int k = col; k <= 8; ++k) {
                std::swap(a[col][k], a[pivotRow][k]);
            }
        }
        const double pivot = a[col][col];
        for (int k = col; k <= 8; ++k) {
            a[col][k] /= pivot;
        }
        for (int row = 0; row < 8; ++row) {
            if (row == col) {
                continue;
            }
            const double factor = a[row][col];
            for (int k = col; k <= 8; ++k) {
                a[row][k] -= factor * a[col][k];
            }
        }
    }
    return true;
}

bool ComputeHomographyDestToSrcRect(
    const std::array<Gdiplus::PointF, 4>& srcPts,
    int outW,
    int outH,
    std::array<double, 8>* outHmat) {
    if (!outHmat || outW <= 1 || outH <= 1) {
        return false;
    }
    const double w = static_cast<double>(outW - 1);
    const double h = static_cast<double>(outH - 1);
    const std::array<Gdiplus::PointF, 4> dstPts = {
        Gdiplus::PointF(0.0f, 0.0f),
        Gdiplus::PointF(static_cast<float>(w), 0.0f),
        Gdiplus::PointF(static_cast<float>(w), static_cast<float>(h)),
        Gdiplus::PointF(0.0f, static_cast<float>(h)),
    };

    double m[8][9] = {};
    for (int i = 0; i < 4; ++i) {
        const double X = dstPts[static_cast<size_t>(i)].X;
        const double Y = dstPts[static_cast<size_t>(i)].Y;
        const double x = srcPts[static_cast<size_t>(i)].X;
        const double y = srcPts[static_cast<size_t>(i)].Y;

        const int r0 = i * 2;
        const int r1 = r0 + 1;
        m[r0][0] = X;
        m[r0][1] = Y;
        m[r0][2] = 1.0;
        m[r0][3] = 0.0;
        m[r0][4] = 0.0;
        m[r0][5] = 0.0;
        m[r0][6] = -x * X;
        m[r0][7] = -x * Y;
        m[r0][8] = x;

        m[r1][0] = 0.0;
        m[r1][1] = 0.0;
        m[r1][2] = 0.0;
        m[r1][3] = X;
        m[r1][4] = Y;
        m[r1][5] = 1.0;
        m[r1][6] = -y * X;
        m[r1][7] = -y * Y;
        m[r1][8] = y;
    }

    if (!SolveLinear8x8(m)) {
        return false;
    }
    for (int i = 0; i < 8; ++i) {
        (*outHmat)[static_cast<size_t>(i)] = m[i][8];
    }
    return true;
}

bool BuildOrderedQuadrilateral(const std::vector<Gdiplus::PointF>& points, std::array<Gdiplus::PointF, 4>* ordered) {
    if (!ordered || points.size() != 4) {
        return false;
    }

    const auto cross = [](const Gdiplus::PointF& a, const Gdiplus::PointF& b, const Gdiplus::PointF& c) {
        const double abx = static_cast<double>(b.X - a.X);
        const double aby = static_cast<double>(b.Y - a.Y);
        const double acx = static_cast<double>(c.X - a.X);
        const double acy = static_cast<double>(c.Y - a.Y);
        return abx * acy - aby * acx;
    };
    const auto segmentsProperlyIntersect = [&](const Gdiplus::PointF& a,
                                               const Gdiplus::PointF& b,
                                               const Gdiplus::PointF& c,
                                               const Gdiplus::PointF& d) {
        const double ab_c = cross(a, b, c);
        const double ab_d = cross(a, b, d);
        const double cd_a = cross(c, d, a);
        const double cd_b = cross(c, d, b);

        const double eps = 1e-9;
        if (std::fabs(ab_c) < eps || std::fabs(ab_d) < eps || std::fabs(cd_a) < eps || std::fabs(cd_b) < eps) {
            return true;
        }
        return (ab_c > 0) != (ab_d > 0) && (cd_a > 0) != (cd_b > 0);
    };
    const auto polygonAreaAbs = [&](const std::array<Gdiplus::PointF, 4>& p) {
        const double s =
            static_cast<double>(p[0].X) * p[1].Y - static_cast<double>(p[1].X) * p[0].Y +
            static_cast<double>(p[1].X) * p[2].Y - static_cast<double>(p[2].X) * p[1].Y +
            static_cast<double>(p[2].X) * p[3].Y - static_cast<double>(p[3].X) * p[2].Y +
            static_cast<double>(p[3].X) * p[0].Y - static_cast<double>(p[0].X) * p[3].Y;
        return std::fabs(s) * 0.5;
    };
    const auto rotateToTopLeft = [&](std::array<Gdiplus::PointF, 4>* p) {
        size_t start = 0;
        float best = (*p)[0].X + (*p)[0].Y;
        for (size_t i = 1; i < 4; ++i) {
            const float v = (*p)[i].X + (*p)[i].Y;
            if (v < best) {
                best = v;
                start = i;
            }
        }
        if (start == 0) {
            return;
        }
        std::array<Gdiplus::PointF, 4> tmp = *p;
        for (size_t i = 0; i < 4; ++i) {
            (*p)[i] = tmp[(start + i) % 4];
        }
    };

    std::array<int, 4> idx = {0, 1, 2, 3};
    double bestArea = -1.0;
    std::array<Gdiplus::PointF, 4> best = {};

    do {
        std::array<Gdiplus::PointF, 4> p = {
            points[static_cast<size_t>(idx[0])],
            points[static_cast<size_t>(idx[1])],
            points[static_cast<size_t>(idx[2])],
            points[static_cast<size_t>(idx[3])],
        };

        if (segmentsProperlyIntersect(p[0], p[1], p[2], p[3])) {
            continue;
        }
        if (segmentsProperlyIntersect(p[1], p[2], p[3], p[0])) {
            continue;
        }

        const double area = polygonAreaAbs(p);
        if (area < 1.0) {
            continue;
        }

        rotateToTopLeft(&p);
        if (area > bestArea) {
            bestArea = area;
            best = p;
        }
    } while (std::next_permutation(idx.begin(), idx.end()));

    if (bestArea < 0.0) {
        return false;
    }
    *ordered = best;
    return true;
}

}  // namespace app::geom

