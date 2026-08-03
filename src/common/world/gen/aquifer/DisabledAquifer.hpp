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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "Aquifer.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/aquifer/FluidStatus.hpp"
#include <utility>

namespace mc::world::gen::aquifer {

/**
 * @brief 禁用含水层的实现
 *
 * 在 finalDensity < 0 时直接返回全局流体（海平面水或熔岩）。
 * 用于下界和末地，或者设置中禁用含水层的情况。
 */
class DisabledAquifer final : public Aquifer {
public:
    explicit DisabledAquifer(FluidPicker globalFluidPicker)
        : m_globalFluidPicker(std::move(globalFluidPicker))
    {}

    [[nodiscard]] const BlockState* computeSubstance(i32 blockX, i32 blockY, i32 blockZ, f64 densityValue) override;

    [[nodiscard]] bool shouldScheduleFluidUpdate() const override { return false; }

private:
    FluidPicker m_globalFluidPicker;
};

} // namespace mc::world::gen::aquifer
