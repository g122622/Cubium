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

#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
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
    [[nodiscard]] i32 distance() const
    {
        return m_distance;
    }

    /// 获取角度偏移范围
    [[nodiscard]] i32 spread() const
    {
        return m_spread;
    }

    /// 获取总要塞数量
    [[nodiscard]] i32 count() const
    {
        return m_count;
    }

    /// 获取首选生物群系列表（要塞按此偏置定位）
    [[nodiscard]] const std::vector<BiomeId>& preferredBiomes() const
    {
        return m_preferredBiomes;
    }

    /**
     * @brief 获取所有要塞的环布局起点（惰性计算并缓存）
     *
     * 【只含布局，不含生物群系修正】返回的是每个要塞搜索方形的中心区块，也就是
     * "打算在哪里找"；要塞真正落在哪里还要经一次半径 112 的群系搜索改写，见
     * `resolveRingPositions`。区分两者很重要：两者最多相差 7 个区块。
     *
     * 首次调用时根据世界种子生成，后续直接返回缓存；世界种子变化时重算。
     *
     * @param worldSeed 世界种子
     * @return 所有要塞的环布局起点区块坐标
     */
    const std::vector<world::chunk::ChunkPos>& ringLayoutChunks(i64 worldSeed) const;

    /**
     * @brief 一个要塞的「搜索起点 + 该点专用的 fork 种子」
     *
     * 原版在算出每个起点的同时就对同一个 `randomsource` 调一次 `fork()`，把结果交给
     * 该起点的生物群系搜索。`forkSeed` 就是那次 `fork()` 取走的 `nextLong()`。
     */
    struct RingEntry {
        world::chunk::ChunkPos chunk;
        u64 forkSeed = 0;
    };

    /**
     * @brief 生成全部要塞的搜索起点与各自的 fork 种子
     *
     * 与 `generateRingPositions` 是同一套算法，额外带出 fork 种子，供调用方复刻
     * 原版「同一条随机流里边算起点边 fork」的消耗顺序。
     */
    [[nodiscard]] std::vector<RingEntry> generateRingLayout(i64 worldSeed) const;

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

    /// 经生物群系搜索改写后的最终坐标表（按世界种子缓存）
    mutable std::optional<std::vector<world::chunk::ChunkPos>> m_resolvedPositions;
    /// 该表对应的世界种子
    mutable i64 m_resolvedSeed = 0;

public:
    /**
     * @brief 取经生物群系搜索改写后的最终要塞坐标表（单飞：并发调用只计算一次）
     *
     * 原版把这张表缓存在 `ChunkGeneratorStructureState.ringPositions` 里、与放置规则
     * 同生命周期复用。而 `StructurePlacementState` 每个区块都会新建一个实例，所以缓存
     * 必须落在放置规则上——否则 128 次群系搜索会在每个区块重跑一遍。
     *
     * 计算期间持锁：同时到达的多个区块生成线程只有一个真正计算，其余直接等结果
     * （它们本来也要等这份数据才能继续判定）。
     *
     * @param worldSeed 世界种子（缓存键）
     * @param compute 缓存未命中时的计算体，只在持锁的那一个线程上执行一次
     * @return 最终坐标表；引用在「同一种子的这一次世界生命周期」内稳定
     */
    const std::vector<world::chunk::ChunkPos>& resolveRingPositions(
        i64 worldSeed, const std::function<std::vector<world::chunk::ChunkPos>()>& compute) const;
};

} // namespace mc::world::gen::structure::placement
