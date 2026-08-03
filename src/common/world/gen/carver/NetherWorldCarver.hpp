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

#pragma once

#include "CaveCarver.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/carver/CarverConfiguration.hpp"
#include "common/world/gen/carver/CarvingContext.hpp"

namespace mc {

/**
 * @brief 下界雕刻器
 *
 * 专门用于下界维度的洞穴生成。
 * 与主世界洞穴的主要区别：
 *
 * - getCaveBound() 返回 10（洞穴更少但更大）
 * - getThickness() 返回更大的半径：(nextFloat() * 2.0 + nextFloat()) * 2.0
 * - getYScale() 返回 5.0（洞穴更扁平）
 * - 不执行草地/菌丝表面替换（handlesSurfaceReplacement = false）
 * - 不检查流体（shouldCheckForFluid = false，下界有熔岩）
 * - getCarveState() 不使用含水层系统，直接判断 Y 阈值填充熔岩或空气
 */
class NetherWorldCarver : public CaveCarver {
public:
    NetherWorldCarver();

    ~NetherWorldCarver() override = default;

protected:
    /** @brief 下界洞穴最大数量上限为 10 */
    [[nodiscard]] i32 getCaveBound() const noexcept override { return 10; }

    /** @brief 下界洞穴半径更大：(nextFloat() * 2.0 + nextFloat()) * 2.0 */
    [[nodiscard]] f32 getThickness(math::IRandom& rng) const override;

    /** @brief 下界洞穴更扁平，Y 缩放因子为 5.0 */
    [[nodiscard]] f64 getYScale() const noexcept override { return 5.0; }

    /** @brief 下界不执行草地/菌丝表面替换 */
    [[nodiscard]] bool handlesSurfaceReplacement() const noexcept override { return false; }

    /** @brief 下界不检查流体（有熔岩） */
    [[nodiscard]] bool shouldCheckForFluid() const noexcept override { return false; }

    /**
     * @brief 下界雕刻后方块状态：Y <= minY + 31 填充熔岩，否则填充空气
     */
    [[nodiscard]] const BlockState* getCarveState(CarvingContext& context,
        i32 worldX,
        i32 worldY,
        i32 worldZ,
        const CaveCarverConfiguration& config) const override;
};

} // namespace mc
