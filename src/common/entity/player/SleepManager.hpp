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

#include "../../core/Types.hpp"
#include "../../util/Direction.hpp"
#include "../../util/math/Vector3.hpp"
#include "SleepResult.hpp"
#include <optional>

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class Player;

namespace entity {

/**
 * @brief 睡眠管理工具类
 *
 * 提供睡眠相关的静态辅助函数：
 * - 检查是否可以在当前时间睡眠
 * - 计算玩家的起床位置
 * - 检查床周围是否有怪物
 * - 检查床是否被阻挡
 *
 * 参考 MC 1.16.5 ServerPlayerEntity 和 BedBlock
 */
class SleepManager {
public:
    /**
     * @brief 检查是否可以在当前时间睡眠
     *
     * 睡眠时间规则：
     * - 雷暴时任何时间都可以睡眠
     * - 降雨时范围更宽 (12010 - 23991)
     * - 晴天时只能在夜间 (12542 - 23459)
     *
     * @param dayTime 当前昼夜时间 (0-23999)
     * @param isThundering 是否正在雷暴
     * @param isRaining 是否正在降雨
     * @return true 如果可以睡眠
     */
    [[nodiscard]] static bool canSleepAtTime(i64 dayTime, bool isThundering, bool isRaining);

    /**
     * @brief 计算玩家的起床位置
     *
     * 尝试在床周围找到安全的站立位置。
     * 参考 MC 1.16.5 BedBlock.getBedSpawnPosition()
     *
     * @param world 世界引用
     * @param bedPos 床头位置
     * @param bedFacing 床的朝向
     * @return 如果找到合适位置返回位置向量，否则返回 nullopt
     */
    [[nodiscard]] static std::optional<Vector3> findWakeUpPosition(
        const IWorld& world, const BlockPos& bedPos, Direction bedFacing);

    /**
     * @brief 检查玩家是否在床附近
     *
     * 床的有效范围：水平 3 格，垂直 2 格
     * 参考 MC 1.16.5 ServerPlayerEntity.func_241158_g_()
     *
     * @param playerPos 玩家位置
     * @param bedPos 床位置（床的中心）
     * @return true 如果在有效范围内
     */
    [[nodiscard]] static bool isPlayerNearBed(const Vector3& playerPos, const BlockPos& bedPos);

    /**
     * @brief 检查床是否被阻挡（上方没有空间）
     *
     * 检查床头和床尾上方是否有足够的站立空间。
     * 参考 MC 1.16.5 ServerPlayerEntity.func_241156_b_()
     *
     * @param world 世界引用
     * @param bedPos 床头位置
     * @param bedFacing 床的朝向
     * @return true 如果床被阻挡
     */
    [[nodiscard]] static bool isBedObstructed(const IWorld& world, const BlockPos& bedPos, Direction bedFacing);

    /**
     * @brief 检查床周围是否有怪物
     *
     * 在床周围 8x5x8 范围内检测敌对生物。
     * 参考 MC 1.16.5 ServerPlayerEntity.trySleep()
     *
     * @param world 世界引用
     * @param bedPos 床位置
     * @param player 检查的玩家（用于排除）
     * @return true 如果周围有怪物
     */
    [[nodiscard]] static bool isBedSurroundedByMonsters(IWorld& world, const BlockPos& bedPos, const Player& player);

private:
    /**
     * @brief 检查单个方块上方是否有站立空间
     *
     * 需要两个方块高的空间（玩家高度约 1.8 格）
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return true 如果有足够空间
     */
    [[nodiscard]] static bool hasStandingSpace(const IWorld& world, const BlockPos& pos);
};

} // namespace entity
} // namespace mc
