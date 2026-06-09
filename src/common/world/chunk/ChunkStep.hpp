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
 */

#pragma once

#include "ChunkDependencies.hpp"
#include "ChunkStatus.hpp"

namespace mc {

// ============================================================================
// 区块生成步骤（MC 1.21 ChunkStep）
// ============================================================================

/**
 * @brief 区块生成步骤
 *
 * 对应 MC 1.21 的 ChunkStep record。
 * 描述一个区块生成阶段的目标状态、直接依赖、累积依赖和可写半径。
 *
 * directDependencies：本阶段直接要求的邻居区块状态
 *   例如 NOISE 的 directDependencies = [BIOMES(0), STRUCTURE_STARTS(1..8)]
 *   表示半径 0 的邻居需要 BIOMES，半径 1-8 需要 STRUCTURE_STARTS
 *
 * accumulatedDependencies：合并所有前序步骤的直接依赖
 *   例如 SURFACE 的 accumulatedDependencies 合并了 NOISE 和 SURFACE 自身的依赖
 *
 * blockStateWriteRadius：本阶段可以写方块状态的半径
 *   -1 = 不写方块（EMPTY, STRUCTURE_STARTS 等）
 *   0 = 只写中心区块（NOISE, SURFACE, CARVERS）
 *   1 = 写中心区块及 1 格邻居（FEATURES）
 */
class ChunkStep {
public:
    ChunkStep() = default;

    /**
     * @brief 构造区块生成步骤
     * @param targetStatus 目标区块状态
     * @param directDependencies 直接依赖（本阶段要求的邻居状态）
     * @param accumulatedDependencies 累积依赖（合并所有前序步骤的依赖）
     * @param blockStateWriteRadius 可写方块状态的半径
     */
    ChunkStep(const ChunkStatus* targetStatus,
        ChunkDependencies directDependencies,
        ChunkDependencies accumulatedDependencies,
        i32 blockStateWriteRadius)
        : m_targetStatus(targetStatus)
        , m_directDependencies(std::move(directDependencies))
        , m_accumulatedDependencies(std::move(accumulatedDependencies))
        , m_blockStateWriteRadius(blockStateWriteRadius)
    {}

    // === 属性访问 ===

    /** @brief 目标区块状态 */
    [[nodiscard]] const ChunkStatus* targetStatus() const { return m_targetStatus; }

    /** @brief 直接依赖（本阶段要求的邻居状态） */
    [[nodiscard]] const ChunkDependencies& directDependencies() const { return m_directDependencies; }

    /** @brief 累积依赖（合并所有前序步骤的依赖） */
    [[nodiscard]] const ChunkDependencies& accumulatedDependencies() const { return m_accumulatedDependencies; }

    /** @brief 可写方块状态的半径（-1=不写，0=仅中心，1=含1格邻居） */
    [[nodiscard]] i32 blockStateWriteRadius() const { return m_blockStateWriteRadius; }

    /**
     * @brief 获取指定状态在累积依赖中的半径
     * @param status 要查找的状态
     * @return 累积依赖半径，如果 status == targetStatus 则返回 0
     */
    [[nodiscard]] i32 getAccumulatedRadiusOf(const ChunkStatus& status) const
    {
        if (m_targetStatus != nullptr && status == *m_targetStatus) {
            return 0;
        }
        return m_accumulatedDependencies.getRadiusOf(status);
    }

    /**
     * @brief 获取直接依赖半径
     */
    [[nodiscard]] i32 directRadius() const { return m_directDependencies.getRadius(); }

    /**
     * @brief 获取累积依赖半径
     */
    [[nodiscard]] i32 accumulatedRadius() const { return m_accumulatedDependencies.getRadius(); }

private:
    const ChunkStatus* m_targetStatus = nullptr;
    ChunkDependencies m_directDependencies;
    ChunkDependencies m_accumulatedDependencies;
    i32 m_blockStateWriteRadius = -1;
};

} // namespace mc
