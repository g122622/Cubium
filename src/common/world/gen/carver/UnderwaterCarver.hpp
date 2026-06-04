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

namespace mc::world::gen::carver {

/**
 * @brief 水下洞穴雕刻器
 *
 * 继承自 CaveWorldCarver。
 * 与普通洞穴的区别：
 * - 可雕刻方块包含水下相关方块（水、熔岩、黑曜石、空气等）
 * - 不检测区域是否在水下（始终可以在水下生成）
 * - Y==10 处有特殊逻辑：25% 岩浆块，75% 黑曜石
 * - Y<10 填充熔岩，Y>10 填充水
 */
class UnderwaterCaveCarver : public CaveCarver {
public:
    UnderwaterCaveCarver();

    ~UnderwaterCaveCarver() noexcept override = default;

    /**
     * @brief 在区块中雕刻水下洞穴
     * 重写以实现MC原版的填充逻辑
     */
    bool carve(ChunkPrimer& chunk,
        const BiomeProvider& biomeProvider,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        CarvingMask& carvingMask,
        const ProbabilityConfig& config) override;

protected:
    /**
     * @brief 检查椭球位置是否有效
     * 水下洞穴始终返回 false（不跳过任何位置）
     */
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const override;

    /**
     * @brief 检查方块是否可雕刻
     * 水下洞穴包含更多可雕刻方块
     */
    [[nodiscard]] static bool isUnderwaterCarvable(const BlockState& state) noexcept;

    /**
     * @brief 雕刻单个椭球区域（水下版本）
     * 实现MC原版的Y==10特殊逻辑
     */
    bool carveEllipsoidUnderwater(ChunkPrimer& chunk,
        const BiomeProvider& biomeProvider,
        i32 seaLevel,
        ChunkCoord chunkX,
        ChunkCoord chunkZ,
        f32 centerX,
        f32 centerY,
        f32 centerZ,
        f32 horizontalRadius,
        f32 verticalRadius,
        CarvingMask& carvingMask,
        i64 seed);

    /**
     * @brief 检查椭球是否在雕刻范围内（水下版本，不检查流体）
     */
    [[nodiscard]] static bool isInCarvingRangeUnderwater(
        ChunkCoord chunkX, ChunkCoord chunkZ, f32 x, f32 z, i32 step, i32 maxSteps, f32 radius) noexcept;
};

/**
 * @brief 水下峡谷雕刻器
 *
 * 继承自 CanyonWorldCarver。
 * 与普通峡谷的区别类似水下洞穴。
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
    [[nodiscard]] bool shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const override;
};

/**
 * @brief 创建水下洞穴雕刻器
 */
std::unique_ptr<UnderwaterCaveCarver> createUnderwaterCaveCarver();

/**
 * @brief 创建水下峡谷雕刻器
 */
std::unique_ptr<UnderwaterCanyonCarver> createUnderwaterCanyonCarver();

} // namespace mc::world::gen::carver

// 向后兼容：在 mc 命名空间中提供类型别名
namespace mc {
using UnderwaterCaveCarver = world::gen::carver::UnderwaterCaveCarver;
using UnderwaterCanyonCarver = world::gen::carver::UnderwaterCanyonCarver;
} // namespace mc
