/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/gen/density/ast/DensityEvalHelpers.hpp"

#include "common/util/assert/AssertAll.hpp"

#include <algorithm>
#include <cmath>

namespace mc::world::gen::density::ast::eval_helpers {

f64 clampedLerp(f64 delta, f64 from, f64 to) noexcept
{
    if (delta <= 0.0) {
        return from;
    }
    if (delta >= 1.0) {
        return to;
    }
    return from + delta * (to - from);
}

f64 clampedMap(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax) noexcept
{
    const f64 t = (fromMax == fromMin) ? 0.0 : (value - fromMin) / (fromMax - fromMin);
    return clampedLerp(std::clamp(t, 0.0, 1.0), toMin, toMax);
}

f64 getRarity(WeirdType type, f64 value) noexcept
{
    if (type == WeirdType::Type1) {
        if (value < -0.5) {
            return 0.75;
        }
        if (value < 0.0) {
            return 1.0;
        }
        if (value < 0.5) {
            return 1.5;
        }
        return 2.0;
    }
    // Type2
    if (value < -0.75) {
        return 0.5;
    }
    if (value < -0.5) {
        return 0.75;
    }
    if (value < 0.5) {
        return 1.0;
    }
    if (value < 0.75) {
        return 2.0;
    }
    return 3.0;
}

i64 findSplineRange(const std::vector<f64>& locations, f64 point) noexcept
{
    const size_t n = locations.size();
    if (n == 0) {
        return -1;
    }
    if (point < locations[0]) {
        return -1;
    }
    if (point >= locations[n - 1]) {
        return static_cast<i64>(n - 1);
    }
    // 标准上界二分：找最大 r 使 locations[r] <= point < locations[r+1]。
    size_t lo = 0;
    size_t hi = n - 1;
    while (lo < hi) {
        const size_t mid = lo + (hi - lo + 1) / 2;
        if (locations[mid] <= point) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    return static_cast<i64>(lo);
}

f64 splineLinearExtend(f64 point, f64 location, f64 value, f64 derivative) noexcept
{
    return derivative == 0.0 ? value : value + derivative * (point - location);
}

f64 evalSpline(const CompiledSpline& spline, f64 point, i32 x, i32 y, i32 z)
{
    const size_t n = spline.locations.size();
    MC_ASSERT_RELEASE_MSG(!spline.valueEvaluators.empty(), "spline must have at least one point");
    if (n == 1) {
        const f64 v = spline.valueEvaluators[0]->eval(x, y, z);
        return splineLinearExtend(point, spline.locations[0], v, spline.derivatives[0]);
    }

    const i64 range = findSplineRange(spline.locations, point);
    if (range < 0) {
        // point < locations[0]
        const f64 v = spline.valueEvaluators[0]->eval(x, y, z);
        return splineLinearExtend(point, spline.locations[0], v, spline.derivatives[0]);
    }
    const size_t last = n - 1;
    if (static_cast<size_t>(range) >= last) {
        // point >= locations[last]
        const f64 v = spline.valueEvaluators[last]->eval(x, y, z);
        return splineLinearExtend(point, spline.locations[last], v, spline.derivatives[last]);
    }

    // 正常区间 [range, range+1]。
    const size_t r = static_cast<size_t>(range);
    const f64 loc0 = spline.locations[r];
    const f64 loc1 = spline.locations[r + 1];
    const f64 width = loc1 - loc0;
    const f64 t = width == 0.0 ? 0.0 : (point - loc0) / width;
    const f64 v0 = spline.valueEvaluators[r]->eval(x, y, z);
    const f64 v1 = spline.valueEvaluators[r + 1]->eval(x, y, z);
    const f64 onDist = v1 - v0;
    const f64 f4 = spline.derivatives[r] * width - onDist;
    const f64 f5 = -spline.derivatives[r + 1] * width + onDist;
    // Hermite 三次：v0 + t*(v1-v0) + t*(1-t)*(f4*(1-t) + f5*t)
    return v0 + t * onDist + t * (1.0 - t) * (f4 * (1.0 - t) + f5 * t);
}

f64 evalFindTopSurface(
    const CompiledDensityFunction& densitySub, f64 upperVal, i32 lowerBound, i32 cellHeight, i32 x, i32 z)
{
    const i32 i = static_cast<i32>(std::floor(upperVal / static_cast<f64>(cellHeight))) * cellHeight;
    if (i <= lowerBound) {
        return static_cast<f64>(lowerBound);
    }
    for (i32 j = i; j >= lowerBound; j -= cellHeight) {
        if (densitySub.eval(x, j, z) > 0.0) {
            return static_cast<f64>(j);
        }
    }
    return static_cast<f64>(lowerBound);
}

} // namespace mc::world::gen::density::ast::eval_helpers
