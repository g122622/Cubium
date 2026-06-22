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

namespace mc {

// Forward declarations
class IWorld;
class Player;

namespace entity {

/**
 * @brief 猪灵AI工具类
 *
 * 提供猪灵愤怒相关的静态方法，对应 MC Java 的 PiglinAi 工具方法。
 * 当前实现核心的 angerNearbyPiglins 方法，用于在玩家打开/破坏
 * 被猪灵守护的容器时激怒附近的猪灵。
 */
class PiglinAi {
public:
    /**
     * @brief 激怒附近的猪灵
     *
     * 当玩家打开或破坏被猪灵守护的容器时调用。
     * 参考 MC 1.21.11 PiglinAi.angerNearbyPiglins()
     *
     * @param world 世界引用
     * @param player 触发愤怒的玩家
     * @param requireLineOfSight 是否需要视线检查
     *   - true: 打开容器时使用，只有能看到玩家的猪灵才会被激怒
     *   - false: 破坏方块时使用，所有空闲的猪灵都会被激怒
     */
    static void angerNearbyPiglins(IWorld& world, Player& player, bool requireLineOfSight);

    /**
     * @brief 检查玩家是否穿戴金盔甲
     *
     * 穿戴金盔甲的玩家不会成为猪灵的攻击目标。
     * 参考 MC 1.21.11 PiglinAi.isWearingGold()
     *
     * @param player 要检查的玩家
     * @return 如果玩家穿戴了金盔甲中的任意一件则返回true
     */
    static bool isWearingGold(Player& player);

private:
    /// 猪灵感知玩家打开容器/破坏方块的距离（方块）
    static constexpr f32 PLAYER_ANGER_RANGE = 16.0f;

    /// 愤怒持续时间（ticks），30秒
    static constexpr i32 ANGER_DURATION = 600;
};

} // namespace entity
} // namespace mc
