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

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <functional>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mc::world::chunk {

/**
 * @brief 区块距离图 - 管理 BFS 级别传播
 *
 * 核心算法：Dijkstra 风格的最短路径传播。
 *
 * 工作原理：
 * 1. 源区块设置初始级别（如玩家位置 level=23）
 * 2. 级别向相邻区块传播，每次传播 +1
 * 3. 相邻区块的级别 = min(当前级别, 源级别+1)
 *
 * @example
 * @code
 * ChunkDistanceGraph graph;
 *
 * // 设置回调函数
 * graph.setLevelChangeCallback([](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
 *     if (newLevel <= 34 && oldLevel > 34) {
 *         // 区块被加载
 *     } else if (newLevel > 34 && oldLevel <= 34) {
 *         // 区块被卸载
 *     }
 * });
 *
 * // 设置源区块（玩家位置）
 * graph.updateSourceLevel(0, 0, 23, true);  // level = 33 - 10（视距）
 *
 * // 处理更新
 * graph.processUpdates(1000);
 * @endcode
 *
 * @note 必须调用 processUpdates() 才能处理更新队列中的更新
 */
class ChunkDistanceGraph {
public:
    using ChunkCallback = std::function<void(ChunkCoord, ChunkCoord, i32, i32)>;

    /// 最大级别（未加载）= ChunkPyramid::maxLevel() + 1 = 45
    /// 此值在初始化时由 ChunkPyramid 动态计算，静态常量用于编译期初始化。
    /// 实际值通过 maxLevel() 方法获取。
    static constexpr i32 MAX_LEVEL = 45;

    ChunkDistanceGraph() noexcept = default;
    virtual ~ChunkDistanceGraph() = default;

    /**
     * @brief 更新源位置级别
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param level 新级别
     * @param isDecreasing true 表示级别降低（区块变得更重要）
     *
     * @note 此方法将更新加入队列，需要调用 processUpdates() 处理
     *
     * @example
     * @code
     * // 玩家进入区块，级别降低
     * graph.updateSourceLevel(0, 0, 23, true);
     *
     * // 玩家离开区块，级别升高
     * graph.updateSourceLevel(0, 0, 34, false);
     * @endcode
     */
    void updateSourceLevel(ChunkCoord x, ChunkCoord z, i32 level, bool isDecreasing);

    /**
     * @brief 处理所有待处理的更新
     *
     * @param maxToProcess 最大处理数量
     * @return 实际处理的更新数量
     *
     * @note 必须定期调用此方法来处理更新队列
     * @note 大量更新可能需要多次调用
     */
    i32 processUpdates(i32 maxToProcess);

    /**
     * @brief 获取区块当前级别
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 区块级别，如果未设置返回 MAX_LEVEL
     */
    [[nodiscard]] i32 getLevel(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 设置区块级别变化回调
     *
     * @param callback 回调函数 (x, z, oldLevel, newLevel)
     *
     * @note 级别降低意味着区块变得更重要（需要加载）
     * @note 级别升高意味着区块变得更不重要（可能卸载）
     */
    void setLevelChangeCallback(ChunkCallback callback) { m_levelChangeCallback = std::move(callback); }

    /**
     * @brief 清空所有数据
     *
     * @warning 这会清空所有区块的级别信息和更新队列
     */
    void clear();

protected:
    /**
     * @brief 获取源级别
     *
     * 子类可以重写此方法以提供票据源。
     * 默认实现返回 MAX_LEVEL（无票据）。
     */
    [[nodiscard]] virtual i32 getSourceLevel(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 当区块级别改变时调用
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param oldLevel 旧级别
     * @param newLevel 新级别
     */
    virtual void onLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);

    /// 区块位置转换为键（供子类使用）
    [[nodiscard]] static u64 posToKey(ChunkCoord x, ChunkCoord z) { return mc::math::chunkPosToId(x, z); }

    /// 键转换为区块位置（供子类使用）
    static void keyToPos(u64 key, ChunkCoord& x, ChunkCoord& z) { mc::math::idToChunkPos(key, x, z); }

private:
    void _enqueueUpdate(ChunkCoord x, ChunkCoord z);

    /// 传播级别到相邻区块
    void _propagateToNeighbors(ChunkCoord x, ChunkCoord z);

    /// 区块级别映射
    std::unordered_map<u64, i32> m_levels;

    /// 源级别映射（由 updateSourceLevel 写入）
    std::unordered_map<u64, i32> m_sourceLevels;

    /// 待处理更新队列（区块 key）
    std::queue<u64> m_updateQueue;

    /// 去重集合，避免同一 key 重复入队导致批处理配额被无效消耗
    std::unordered_set<u64> m_pendingKeys;

    /// 级别改变回调
    ChunkCallback m_levelChangeCallback;
};

} // namespace mc::world::chunk
