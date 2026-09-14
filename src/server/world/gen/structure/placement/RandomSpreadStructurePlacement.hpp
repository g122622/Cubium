/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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

#include "StructurePlacement.hpp"

#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <memory>
#include <optional>

namespace mc::world::gen::structure::placement {

/**
 * @brief 随机分布放置策略
 *
 * 将世界划分为 spacing x spacing 的网格，在每个网格内随机选择
 * 一个区块作为结构候选位置。大多数结构使用此放置策略。
 *
 * Linear 分布：偏移量在 [0, range) 内均匀随机
 * Triangular 分布：偏移量为两次 [0, range) 随机的平均值，
 * 产生更集中在网格中心的分布
 */
class RandomSpreadStructurePlacement : public StructurePlacement {
public:
    /**
     * @brief 构造随机分布放置策略
     *
     * @param spacing 网格间距（区块）
     * @param separation 最小间距（区块）
     * @param salt 种子盐值
     * @param spreadType 分布类型
     * @param frequencyReduction 频率缩减方法
     * @param frequency 生成频率（0.0 ~ 1.0）
     * @param locateOffset 定位偏移
     * @param exclusionZone 排斥区配置
     */
    RandomSpreadStructurePlacement(i32 spacing,
        i32 separation,
        i64 salt,
        RandomSpreadType spreadType,
        FrequencyReductionMethod frequencyReduction,
        f32 frequency,
        math::Vector3i locateOffset,
        std::optional<ExclusionZone> exclusionZone);

    /**
     * @brief 检查指定区块是否是结构生成区块
     *
     * 步骤：
     * 1. 计算此区块所在的网格位置
     * 2. 计算网格内的候选区块
     * 3. 如果候选区块不等于当前区块，返回 false
     * 4. 通过频率缩减检查
     * 5. 通过排斥区检查
     *
     * @param worldSeed 世界种子
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 是否在此区块生成结构
     */
    bool isStructureChunk(i64 worldSeed, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 克隆此放置策略
     * @return 新的深拷贝实例
     */
    std::unique_ptr<StructurePlacement> clone() const override;

    /**
     * @brief 计算网格内的候选区块位置
     *
     * 对于给定的区块坐标，计算其所在网格中的结构候选位置。
     * 这是 isStructureChunk 的核心计算部分。
     *
     * @param worldSeed 世界种子
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 候选区块坐标
     */
    world::chunk::ChunkPos getPotentialStructureChunk(i64 worldSeed, i32 chunkX, i32 chunkZ) const;

    /// 获取网格间距
    [[nodiscard]] i32 spacing() const { return m_spacing; }

    /// 获取最小间距
    [[nodiscard]] i32 separation() const { return m_separation; }

    /// 获取分布类型
    [[nodiscard]] RandomSpreadType spreadType() const { return m_spreadType; }

private:
    /// 网格间距（区块）
    i32 m_spacing;
    /// 最小间距（区块），偏移范围 = spacing - separation
    i32 m_separation;
    /// 分布类型（Linear 或 Triangular）
    RandomSpreadType m_spreadType;
};

} // namespace mc::world::gen::structure::placement
