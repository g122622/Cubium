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

#include "../../../../../core/Types.hpp"
#include "../../../../../util/math/Vector3.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../Goal.hpp"
#include <string>

namespace mc {

// Forward declarations
class DolphinEntity;
class Player;
class ItemEntity;
class LivingEntity;
class ItemStack;

namespace entity::ai::goal {

/**
 * @brief 海豚跳跃目标
 *
 * 海豚跳出水面跳跃。
 *
 * 执行条件:
 * - 随机概率触发 (1/chance)
 * - 检查前方跳跃路径上有足够的水
 * - 检查水面上方有足够的空气空间
 *
 * 行为:
 * - startExecuting: 根据朝向设置跳跃速度
 * - tick: 在空中时调整俯仰角
 */
class DolphinJumpGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param dolphin 海豚实体
     * @param chance 触发概率倒数（每 tick 有 1/chance 的概率触发）
     */
    DolphinJumpGoal(DolphinEntity* dolphin, i32 chance);

    ~DolphinJumpGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    [[nodiscard]] bool isPreemptible() const override { return false; }
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "DolphinJumpGoal"; }

    // 公开以便测试
    static constexpr i32 JUMP_DISTANCES[] = {0, 1, 4, 5, 6, 7};

private:
    /**
     * @brief 检查指定距离处是否可以跳跃
     * @param pos 起始位置
     * @param dx X方向偏移
     * @param dz Z方向偏移
     * @param scale 距离缩放
     * @return 如果该位置可以跳跃返回 true
     */
    [[nodiscard]] bool _canJumpTo(const BlockPos& pos, i32 dx, i32 dz, i32 scale) const;

    /**
     * @brief 检查指定距离处上方是否有空气
     * @param pos 起始位置
     * @param dx X方向偏移
     * @param dz Z方向偏移
     * @param scale 距离缩放
     * @return 如果上方有空气返回 true
     */
    [[nodiscard]] bool _isAirAbove(const BlockPos& pos, i32 dx, i32 dz, i32 scale) const;

    DolphinEntity* m_dolphin;
    i32 m_chance;
    bool m_inWater = false;
};

/**
 * @brief 海豚游向宝藏目标
 *
 * 当海豚被喂食鱼后，引导玩家到附近的宝藏结构。
 *
 * 执行条件:
 * - 海豚已经得到了鱼 (hasGotFish = true)
 * - 空气值 >= 100
 *
 * 行为:
 * - startExecuting: 通过 minecraft:dolphin_located 结构标签查找附近的沉船或海底废墟
 * - tick: 向宝藏位置游泳，如果接近目标则重新规划路径
 * - resetTask: 到达宝藏后清除鱼的标记
 *
 * 参考: net.minecraft.world.entity.animal.dolphin.Dolphin.DolphinSwimToTreasureGoal (MC 1.21.11)
 */
class SwimToTreasureGoal : public Goal {
public:
    explicit SwimToTreasureGoal(DolphinEntity* dolphin);

    ~SwimToTreasureGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    [[nodiscard]] bool isPreemptible() const override { return false; }
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SwimToTreasureGoal"; }

    // 公开以便测试
    static constexpr i32 MIN_AIR = 100;
    static constexpr f32 ARRIVE_DISTANCE = 4.0f;
    static constexpr f32 CLOSE_TO_TARGET_DISTANCE = 12.0f;
    /// 搜索半径（方块单位）。
    /// 对应 MC 1.21.11 DolphinSwimToTreasureGoal.start() 中的 findNearestMapStructure(..., 50, false)。
    /// MC 的 50 是 RandomSpread 放置策略的网格步数（spacing steps），
    /// 对于沉船（spacing=24）约等于 50*24=1200 方块。此处使用等价的方块距离。
    static constexpr i32 SEARCH_RADIUS_BLOCKS = 1200;

private:
    DolphinEntity* m_dolphin;
    bool m_failed = false;
};

/**
 * @brief 海豚与玩家同游目标
 *
 * 当玩家在水中游泳时，海豚会跟随玩家并给予"海豚的恩惠"效果。
 *
 * 执行条件:
 * - 附近有正在游泳的玩家
 * - 海豚的攻击目标不是该玩家
 *
 * 行为:
 * - startExecuting: 给玩家添加海豚的恩惠效果
 * - tick: 跟随玩家，持续添加效果
 * - resetTask: 清除目标玩家
 */
class SwimWithPlayerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param dolphin 海豚实体
     * @param speed 跟随速度
     */
    SwimWithPlayerGoal(DolphinEntity* dolphin, f64 speed);

    ~SwimWithPlayerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SwimWithPlayerGoal"; }

    // 公开以便测试
    static constexpr f32 SEARCH_RADIUS = 10.0f;
    static constexpr f32 CLOSE_DISTANCE_SQ = 6.25f; // 2.5 * 2.5
    static constexpr f32 MAX_DISTANCE_SQ = 256.0f;  // 16 * 16
    static constexpr i32 EFFECT_DURATION = 100;     // 5秒 = 100 tick
    static constexpr i32 EFFECT_INTERVAL = 6;       // 每 6 tick 添加效果

private:
    /**
     * @brief 查找附近正在游泳的玩家
     * @return 如果找到返回玩家指针，否则返回 nullptr
     */
    [[nodiscard]] Player* _findSwimmingPlayer() const;

    DolphinEntity* m_dolphin;
    f64 m_speed;
    Player* m_targetPlayer = nullptr;
};

/**
 * @brief 海豚玩物品目标
 *
 * 海豚会拾取水中的物品并扔出来玩。
 *
 * 执行条件:
 * - 冷却时间已过
 * - 附近有可拾取的物品实体（在水中）
 * - 或海豚正在手中持有物品
 *
 * 行为:
 * - startExecuting: 向物品移动
 * - tick: 拾取物品或扔出物品
 * - resetTask: 扔出手中物品
 */
class PlayWithItemsGoal : public Goal {
public:
    explicit PlayWithItemsGoal(DolphinEntity* dolphin);

    ~PlayWithItemsGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "PlayWithItemsGoal"; }

    // 公开以便测试
    static constexpr f32 SEARCH_RADIUS = 8.0f;
    static constexpr f32 THROW_VELOCITY = 0.3f;
    static constexpr i32 PICKUP_DELAY = 40;  // 扔出物品的拾取延迟
    static constexpr i32 MIN_COOLDOWN = 100; // 最小冷却时间

private:
    /**
     * @brief 扔出物品
     * @param stack 要扔出的物品堆
     */
    void _throwItem(ItemStack& stack);

    /**
     * @brief 查找附近的物品实体
     * @return 如果找到返回物品实体，否则返回 nullptr
     */
    [[nodiscard]] ItemEntity* _findNearbyItem() const;

    DolphinEntity* m_dolphin;
    i32 m_cooldown = 0;
};

/**
 * @brief 海豚跟随船目标状态枚举
 */
enum class BoatFollowState : u8 {
    GoToBoat,         // 游向船
    GoInBoatDirection // 跟随船的行进方向
};

/**
 * @brief 海豚跟随船目标
 *
 * 当玩家驾驶船时，海豚会游向船并跟随船的行进方向。
 *
 * 执行条件:
 * - 5格范围内有船
 * - 船上有玩家正在驾驶（按移动键）
 *
 * 行为:
 * - GoToBoat 状态: 游向船尾后方位置
 * - GoInBoatDirection 状态: 跟随船的行进方向，游向船前方
 *
 * 状态转换:
 * - GoToBoat -> GoInBoatDirection: 距离玩家 < 4 格
 * - GoInBoatDirection -> GoToBoat: 距离玩家 > 12 格
 */
class FollowBoatGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param dolphin 海豚实体
     */
    explicit FollowBoatGoal(DolphinEntity* dolphin);

    ~FollowBoatGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool isPreemptible() const override { return true; }
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FollowBoatGoal"; }

private:
    /**
     * @brief 查找附近正在被玩家驾驶的船
     * @return 如果找到返回船上的玩家，否则返回 nullptr
     */
    [[nodiscard]] Player* _findPlayerDrivingBoat();

    /**
     * @brief 检查玩家是否正在操作船移动
     * @param player 玩家
     * @return 如果玩家正在按移动键返回 true
     */
    [[nodiscard]] static bool _isPlayerOperatingBoat(const Player& player);

    DolphinEntity* m_dolphin;
    Player* m_player = nullptr;
    BoatFollowState m_state = BoatFollowState::GoToBoat;
    i32 m_navigationTimer = 0;

    // 常量
    static constexpr f32 SEARCH_RADIUS = 5.0f;                // 搜索船的范围
    static constexpr f32 GO_TO_BOAT_SPEED = 0.015f;           // 游向船的速度
    static constexpr f32 GO_IN_DIRECTION_SPEED = 0.01f;       // 跟随方向的速度
    static constexpr f32 SWITCH_TO_FOLLOW_DISTANCE = 4.0f;    // 切换到跟随状态的距离
    static constexpr f32 SWITCH_TO_APPROACH_DISTANCE = 12.0f; // 切换回接近状态的距离
    static constexpr i32 NAVIGATION_UPDATE_INTERVAL = 10;     // 导航更新间隔（ticks）
    static constexpr f32 NAVIGATE_SPEED = 1.0f;               // 导航速度
};

} // namespace entity::ai::goal
} // namespace mc
