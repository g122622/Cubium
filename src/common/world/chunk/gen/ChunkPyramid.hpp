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
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include <cstddef>
#include <utility>
#include <vector>

namespace mc::world::chunk {

// ============================================================================
// 区块生成金字塔
// ============================================================================

/**
 * @brief 区块生成金字塔
 *
 * 区块生成金字塔，定义区块生成的完整步骤序列和依赖关系。
 *
 * GENERATION_PYRAMID 定义了从 EMPTY 到 FULL 的 12 个步骤：
 *
 * | 步骤             | 直接依赖                                  | 可写半径 |
 * |-----------------|-------------------------------------------|---------|
 * | EMPTY           | （无）                                     | -1      |
 * | STRUCTURE_STARTS| （前一步）                                  | -1      |
 * | STRUCTURE_REFERENCES | STRUCTURE_STARTS(8)                    | -1      |
 * | BIOMES          | STRUCTURE_STARTS(8)                        | -1      |
 * | NOISE           | STRUCTURE_STARTS(8), BIOMES(1)             | 0       |
 * | SURFACE         | STRUCTURE_STARTS(8), BIOMES(1)             | 0       |
 * | CARVERS         | STRUCTURE_STARTS(8)                        | 0       |
 * | FEATURES        | STRUCTURE_STARTS(8), CARVERS(1)            | 1       |
 * | INITIALIZE_LIGHT| （前一步）                                  | -1      |
 * | LIGHT           | INITIALIZE_LIGHT(1)                        | -1      |
 * | SPAWN           | BIOMES(1)                                  | -1      |
 * | FULL            | （前一步）                                  | -1      |
 *
 * 累积依赖通过合并所有前序步骤的直接依赖自动计算。
 */
class ChunkPyramid {
public:
    /**
     * @brief 从步骤列表构造金字塔
     * @param steps 步骤列表（必须按顺序）
     */
    explicit ChunkPyramid(std::vector<ChunkStep> steps)
        : m_steps(std::move(steps))
    {}

    /** @brief 获取步骤列表 */
    [[nodiscard]] const std::vector<ChunkStep>& steps() const { return m_steps; }

    /**
     * @brief 根据目标状态获取对应的步骤
     * @param status 目标区块状态
     * @return 对应的步骤引用
     */
    [[nodiscard]] const ChunkStep& getStepTo(const ChunkStatus& status) const
    {
        return m_steps[static_cast<size_t>(status.ordinal())];
    }

    /**
     * @brief 获取生成金字塔（GENERATION_PYRAMID）
     *
     * 首次调用时构建，后续调用返回缓存实例。
     */
    [[nodiscard]] static const ChunkPyramid& generationPyramid();

    /**
     * @brief 获取加载金字塔（LOADING_PYRAMID）
     *
     * 用于从存档加载区块的步骤序列。与 GENERATION_PYRAMID 的关键区别：
     * - 大多数步骤的直接依赖仅为前一步（半径 0）
     * - 只有 LIGHT 依赖 INITIALIZE_LIGHT(1)
     * - 所有步骤的 blockStateWriteRadius = -1（不写方块）
     *
     * 首次调用时构建，后续调用返回缓存实例。
     */
    [[nodiscard]] static const ChunkPyramid& loadingPyramid();

    // === ChunkLevel 合并的方法 ===

    /**
     * @brief FULL 区块周围的最大依赖半径
     *
     * 从 GENERATION_PYRAMID 中 FULL 步骤的 accumulatedDependencies 计算得出。
     * 当前值为 11（STRUCTURE_STARTS 的累积半径，由依赖偏移合并扩展）。
     */
    static i32 radiusAroundFullChunk();

    /**
     * @brief 最大区块级别 (= FULL_CHUNK_LEVEL + radiusAroundFullChunk() = 44)
     */
    static i32 maxLevel();

    /**
     * @brief 从票据级别推导需要的生成状态
     */
    static const ChunkStatus* generationStatus(i32 level);

    /**
     * @brief 从生成状态推导对应的票据级别
     */
    static i32 byStatus(const ChunkStatus& status);

    /**
     * @brief 从距离推导 FULL 区块周围所需的生成状态
     */
    static const ChunkStatus* getStatusAroundFullChunk(i32 distance);

private:
    std::vector<ChunkStep> m_steps;
};

} // namespace mc::world::chunk
