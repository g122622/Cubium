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
 * THE SOFTWARE IS PROVIDED "AS IS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "StructurePlacement.hpp"

#include "common/world/biome/BiomeIds.hpp"

#include <mutex>
#include <vector>

namespace mc::world::gen::structure::placement {

/**
 * @brief 同心环放置策略
 *
 * 用于要塞（Stronghold）的放置算法。要塞分布在多个同心环上，
 * 每个环包含不同数量的要塞，越远的环数量越多。
 *
 * MC 1.21+ 共有 128 个要塞，分布在 8 个同心环上：
 * - 第 1 环: 3 个要塞，距离 1408 区块
 * - 第 2 环: 6 个要塞，距离 4736 区块
 * - 第 3 环: 10 个要塞，距离 8064 区块
 * - ...依次递增
 *
 * 位置计算为惰性求值并缓存，仅在第一次需要时计算。
 */
class ConcentricRingsStructurePlacement : public StructurePlacement {
public:
    /**
     * @brief 构造同心环放置策略
     *
     * @param distance 第一个环的距离（区块）
     * @param spread 环内角度偏移的随机范围
     * @param count 总要塞数量（MC 1.21+ 为 128）
     * @param preferredBiomes 首选生物群系列表（用于定位时优先选择）
     * @param salt 种子盐值
     * @param locateOffset 定位偏移
     */
    ConcentricRingsStructurePlacement(i32 distance,
        i32 spread,
        i32 count,
        std::vector<BiomeId> preferredBiomes,
        i64 salt,
        math::Vector3i locateOffset);

    /**
     * @brief 检查指定区块是否是结构生成区块
     *
     * 通过查询缓存的环形位置列表判断。
     * 首次调用时会触发生成所有环形位置。
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

    /// 获取环距离参数
    [[nodiscard]] i32 distance() const { return m_distance; }

    /// 获取角度偏移范围
    [[nodiscard]] i32 spread() const { return m_spread; }

    /// 获取总要塞数量
    [[nodiscard]] i32 count() const { return m_count; }

    /// 获取首选生物群系列表（要塞按此偏置定位）
    [[nodiscard]] const std::vector<BiomeId>& preferredBiomes() const { return m_preferredBiomes; }

    /**
     * @brief 获取所有环形位置（惰性计算并缓存）
     *
     * 首次调用时根据世界种子生成所有要塞位置，后续调用直接返回缓存。
     * 如果世界种子发生变化，会重新计算。
     *
     * @param worldSeed 世界种子
     * @return 所有要塞的区块坐标列表
     */
    const std::vector<world::chunk::ChunkPos>& getRingPositions(i64 worldSeed) const;

private:
    /**
     * @brief 生成所有环形位置
     *
     * 按照 MC 1.21 的 ChunkGeneratorStructureState.generateRingPositions() 算法
     * 生成所有要塞位置。
     *
     * @param worldSeed 世界种子
     * @return 生成的区块坐标列表
     */
    std::vector<world::chunk::ChunkPos> generateRingPositions(i64 worldSeed) const;

    /// 第一个环的距离（区块）
    i32 m_distance;
    /// 角度偏移的随机范围
    i32 m_spread;
    /// 总要塞数量
    i32 m_count;
    /// 首选生物群系列表
    std::vector<BiomeId> m_preferredBiomes;

    /// 缓存的环形位置
    mutable std::optional<std::vector<world::chunk::ChunkPos>> m_cachedPositions;
    /// 缓存对应的种子
    mutable i64 m_cachedSeed = 0;
    /// 保护缓存生成的互斥锁
    mutable std::mutex m_cacheMutex;
};

} // namespace mc::world::gen::structure::placement
