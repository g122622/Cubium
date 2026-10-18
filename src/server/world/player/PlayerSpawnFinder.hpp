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
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>

namespace mc::server {

class ServerWorld;

/**
 * @brief 玩家出生位置搜索器
 *
 * 以建议位置为中心，在 spawnRadius 游戏规则限制的方形范围内按确定的伪随机顺序尝试候选列，
 * 返回第一个"玩家碰撞箱无碰撞且无液体"的位置；全部失败时回退到建议列的高度修正结果。
 *
 * 仅当维度有天空光照且世界默认游戏模式不是冒险模式时才进行半径搜索（下界、末地与冒险模式
 * 世界直接使用建议列）。半径还会被世界边界距离压缩：边界距离不足半径时取边界距离，
 * 边界距离 <= 1 时强制为 1。
 */
class PlayerSpawnFinder {
public:
    /**
     * @brief 寻找玩家出生位置
     *
     * @param world 世界
     * @param suggestedPos 建议出生位置（方块坐标）
     * @return 玩家脚底位置；方块未加载或高度不可用时返回 nullopt
     *
     * @note 本方法会同步加载候选区块，必须在主线程调用。
     */
    [[nodiscard]] static std::optional<Vector3> findSpawn(ServerWorld& world, const BlockPos& suggestedPos);

    /**
     * @brief 高度修正：把位置沿 Y 轴调整到"碰撞箱无碰撞且无液体"的最近高度
     *
     * 先向上找到第一个可站立位置，再向下滚动到该竖直间隙的最底部，最后回到间隙底部。
     */
    [[nodiscard]] static std::optional<Vector3> fixupSpawnHeight(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 玩家碰撞箱（宽 0.6，高 1.8）在指定方块柱上是否无碰撞且无液体
     */
    [[nodiscard]] static bool noCollisionNoLiquid(ServerWorld& world, const BlockPos& pos);

    /**
     * @brief 获取候选数量：min(1024, (2r+1)^2)
     */
    [[nodiscard]] static i32 candidateCount(i32 radius) noexcept;

    /**
     * @brief 获取与候选数量互素的步长：n <= 16 时取 n-1，否则取 17
     */
    [[nodiscard]] static i32 coprime(i32 candidateCount) noexcept;

private:
    /// 最大候选数上限
    static constexpr i32 ABSOLUTE_MAX_ATTEMPTS = 1024;

    PlayerSpawnFinder() = delete;
};

} // namespace mc::server