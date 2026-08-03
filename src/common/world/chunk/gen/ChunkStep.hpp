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

#include "common/core/Types.hpp"
#include "common/world/chunk/gen/ChunkDependencies.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"

#include <cstddef>
#include <utility>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 区块生成步骤
// ============================================================================

/**
 * @brief 区块生成步骤
 *
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

    /**
     * @brief 邻居读取半径（= accumulatedRadius）
     *
     * 对齐 Moonrise 的 neighbourReadRadius：本步骤读取邻居数据时需要的最大半径。
     * schedule 遍历 `[center±radius, center±radius]` 范围内的邻居，
     * 对每个邻居调用 getRequiredStatusAtRadius(distance) 查询其所需状态。
     */
    [[nodiscard]] i32 neighbourReadRadius() const { return accumulatedRadius(); }

    /**
     * @brief 按距离查询邻居所需状态（byRadius[] 查找表）
     *
     * 对齐 Moonrise 的 ChunkStepMixin.moonrise$getRequiredStatusAtRadius。
     * 返回距离中心 `radius`（Chebyshev 距离）的邻居必须达到的 ChunkStatus。
     *
     * 语义：生成 `targetStatus` 时，距离为 `radius` 的邻居必须至少完成到
     * `getRequiredStatusAtRadius(radius)` 返回的状态。
     *
     * - 半径 0（中心区块本身）返回 `accumulatedDependencies.get(0)`，即中心区块需要的前一步状态
     *   （对 EMPTY 为 nullptr，对其他状态等于 targetStatus.parent()）
     * - 半径 `accumulatedRadius` 返回最外圈邻居所需状态（通常 STRUCTURE_STARTS）
     * - 超过 `accumulatedRadius` 的半径越界，返回 nullptr
     *
     * 该表在 ChunkPyramid 构建步骤时由 buildRequiredStatusByRadius 填充。
     *
     * @param radius Chebyshev 距离（0 = 中心，accumulatedRadius = 最外圈）
     * @return 该半径邻居所需状态；越界返回 nullptr
     */
    [[nodiscard]] const ChunkStatus* getRequiredStatusAtRadius(i32 radius) const
    {
        if (radius < 0 || radius >= static_cast<i32>(m_requiredStatusByRadius.size())) {
            return nullptr;
        }
        return m_requiredStatusByRadius[static_cast<size_t>(radius)];
    }

    /**
     * @brief 构建 byRadius[] 查找表（由 ChunkPyramid 在构造步骤后调用）
     *
     * 对齐 Moonrise ChunkStepMixin.init：
     * - byRadius[0] = accumulatedDependencies.get(0)（中心区块需要的前一步状态）
     *   注：用 get(0) 而非 targetStatus->parent()，因为 Cubium 的 EMPTY.parent() 返回自身（非 nullptr）
     * - 从 targetStatus.parent() 向下遍历到 EMPTY（不含 EMPTY），对每个状态 status：
     *   radius = accumulatedDependencies.getRadiusOf(status)
     *   对 j = 0..radius，若 byRadius[j] 未设置则设为 status
     *
     * 结果：byRadius[radius] 为该距离邻居所需状态，与 accumulatedDependencies.get(radius) 一致。
     * 高状态（最近的前驱）优先覆盖其半径范围，低状态只填空隙。
     *
     * @param emptyStatus EMPTY 状态（byRadius 构建的终止条件，通常为 ChunkStatuses::EMPTY）
     */
    void buildRequiredStatusByRadius(const ChunkStatus& emptyStatus)
    {
        // byRadius 大小 = accumulatedRadius + 1（邻居读取半径范围 [0, accumulatedRadius]）
        const i32 radius = accumulatedRadius();
        m_requiredStatusByRadius.assign(static_cast<size_t>(radius + 1), nullptr);

        // byRadius[0] = 中心区块需要的前一步状态。
        // 注意：Cubium 的 ChunkStatus 构造函数将根状态 EMPTY 的 parent 设为自身（非 nullptr），
        // 因此不能直接用 m_targetStatus->parent() 判断（EMPTY 会得到自身）。
        // accumulatedDependencies.get(0) 对 EMPTY 返回 nullptr，对其他状态返回前一步状态，
        // 与 Moonrise 的 byRadius[0] = targetStatus.getParent() 语义一致且更健壮。
        m_requiredStatusByRadius[0] = m_accumulatedDependencies.get(0);

        // 从 targetStatus.parent() 向下遍历到 EMPTY（不含 EMPTY），高状态优先填充
        for (const ChunkStatus* status = (m_targetStatus != nullptr ? m_targetStatus->parent() : nullptr);
            status != nullptr && *status != emptyStatus;
            status = status->parent()) {
            const i32 r = m_accumulatedDependencies.getRadiusOf(*status);
            // 对 j = 0..r，若 byRadius[j] 未设置则设为 status。
            // 高状态优先（外层循环从高到低），低状态只填空隙，
            // 结果与 accumulatedDependencies.get(radius) 一致。
            // bounds 检查 j < size() 防御 getRadiusOf 返回值越界（正常情况 r <= radius < size）。
            for (i32 j = 0; j <= r; ++j) {
                if (j < static_cast<i32>(m_requiredStatusByRadius.size()) &&
                    m_requiredStatusByRadius[static_cast<size_t>(j)] == nullptr) {
                    m_requiredStatusByRadius[static_cast<size_t>(j)] = status;
                }
            }
        }
    }

    /**
     * @brief byRadius[] 查找表是否已构建
     */
    [[nodiscard]] bool hasRequiredStatusByRadius() const { return !m_requiredStatusByRadius.empty(); }

private:
    const ChunkStatus* m_targetStatus = nullptr;
    ChunkDependencies m_directDependencies;
    ChunkDependencies m_accumulatedDependencies;
    i32 m_blockStateWriteRadius = -1;

    /**
     * @brief byRadius[] 查找表：邻居 Chebyshev 距离 → 该邻居所需 ChunkStatus
     *
     * 对齐 Moonrise ChunkStepMixin.byRadius。在构造时由
     * ChunkPyramid 调用 buildRequiredStatusByRadius() 填充。
     * 空表表示尚未填充（对 ChunkStep 默认构造的临时对象）。
     */
    std::vector<const ChunkStatus*> m_requiredStatusByRadius;
};

} // namespace mc::world::chunk
