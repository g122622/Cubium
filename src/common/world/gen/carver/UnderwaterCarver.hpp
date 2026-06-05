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

#include "../../../core/Types.hpp"
#include "CanyonCarver.hpp"
#include "CaveCarver.hpp"
#include "WorldCarver.hpp"
#include <memory>

namespace mc {

/**
 * @brief 水下洞穴雕刻器
 *
 * 继承自 CaveCarver，用于水下环境的洞穴生成。
 *
 * MC原版没有单独的水下雕刻器类。水下雕刻行为通过配置区分：
 * - 可雕刻方块包含水下方块（水、熔岩、沙子、沙砾等）
 * - 不检查流体区域（可以在水下生成）
 * - 使用 Aquifer 系统决定填充内容（水/空气/熔岩）
 * - 当前实现暂时使用水位线来决定填充类型
 *
 * 使用方法：
 * @code
 * UnderwaterCaveCarver carver;
 * CarvingMask mask(chunkX, chunkZ);
 * ProbabilityConfig config(0.066f);
 * carver.carve(chunk, biomeSource, seaLevel, chunkX, chunkZ, mask, config);
 * @endcode
 */
class UnderwaterCaveCarver : public CaveCarver {
public:
    UnderwaterCaveCarver();

    ~UnderwaterCaveCarver() noexcept override = default;

protected:
    /**
     * @brief 检查椭球位置是否有效
     * 水下洞穴使用与普通洞穴相同的椭球检测
     */
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const noexcept override;

    /**
     * @brief 不检查流体
     * 水下雕刻器可以在水中生成
     */
    [[nodiscard]] bool shouldCheckForFluid() const override { return false; }

    /**
     * @brief 不执行草地/菌丝表面替换
     * 水下环境不需要此处理
     */
    [[nodiscard]] bool handlesSurfaceReplacement() const override { return false; }

    /**
     * @brief 检查方块是否可雕刻
     * 水下雕刻器包含更多可雕刻方块（水、熔岩、空气等）
     */
    [[nodiscard]] bool canCarveBlock(const BlockState* state, const BlockState* aboveState) const override;
};

/**
 * @brief 水下峡谷雕刻器
 *
 * 继承自 CanyonCarver，用于水下环境的峡谷生成。
 * 与普通峡谷的区别：
 * - 不检查流体
 * - 不执行草地表面替换
 * - 可雕刻方块包含水下方块
 */
class UnderwaterCanyonCarver : public CanyonCarver {
public:
    UnderwaterCanyonCarver();

    ~UnderwaterCanyonCarver() noexcept override = default;

protected:
    /**
     * @brief 检查椭球位置是否有效
     * 水下峡谷使用与普通峡谷相同的厚度检测
     */
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const noexcept override;

    /**
     * @brief 不检查流体
     * 水下雕刻器可以在水中生成
     */
    [[nodiscard]] bool shouldCheckForFluid() const override { return false; }

    /**
     * @brief 不执行草地/菌丝表面替换
     */
    [[nodiscard]] bool handlesSurfaceReplacement() const override { return false; }

    /**
     * @brief 检查方块是否可雕刻
     * 水下雕刻器包含更多可雕刻方块（水、熔岩、空气等）
     */
    [[nodiscard]] bool canCarveBlock(const BlockState* state, const BlockState* aboveState) const override;
};

} // namespace mc
