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
#include "../../../../../resource/ResourceLocation.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../../../core/Entity.hpp"
#include "../../../../core/LivingEntity.hpp"
#include "../../Goal.hpp"
#include <functional>
#include <optional>

namespace mc {

// 前向声明
class MobEntity;
class CreatureEntity;
class Player;
class ItemStack;
class IWorld;
class BlockPos;

namespace entity {

// 前向声明
class AbstractVillagerEntity;
class WanderingTraderEntity;

namespace ai {
namespace goal {
namespace wandering_trader {

/**
 * @brief 使用物品目标
 *
 * 使生物在特定条件下使用物品（如喝药水）。
 * 流浪商人使用此目标在夜间喝隐身药水、白天喝牛奶恢复可见。
 *
 * 参考 MC 1.16.5 UseItemGoal
 */
class UseItemGoal : public Goal {
public:
    /**
     * @brief 使用条件判断函数类型
     * @param mob 生物实体
     * @return 是否应该使用物品
     */
    using UseCondition = std::function<bool(class MobEntity*)>;

    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param stack 要使用的物品堆
     * @param soundEvent 使用物品时播放的声音事件
     * @param condition 使用条件判断函数
     */
    UseItemGoal(
        class MobEntity* mob, const ItemStack& stack, const ResourceLocation& soundEvent, UseCondition condition);

    /**
     * @brief 检查是否应该开始执行
     *
     * MC 1.16.5: 检查条件函数是否返回 true
     */
    [[nodiscard]] bool shouldExecute() override;

    /**
     * @brief 开始执行
     *
     * MC 1.16.5: 设置正在使用物品状态
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     *
     * MC 1.16.5: 清除正在使用物品状态，冷却时间重置
     */
    void resetTask() override;

    /**
     * @brief 每tick更新
     *
     * MC 1.16.5: 消耗物品并应用效果
     */
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "UseItemGoal"; }

private:
    /**
     * @brief 应用物品效果
     *
     * 处理药水效果、牛奶清除效果等
     */
    void applyItemEffect();

private:
    class MobEntity* m_mob;
    ItemStack m_itemStack;
    ResourceLocation m_soundEvent;
    UseCondition m_condition;
    i32 m_useDuration = 0;  // 使用时长计数
    i32 m_cooldown = 0;     // 冷却时间
    bool m_isUsing = false; // 是否正在使用

    static constexpr i32 ITEM_USE_DURATION = 32; // 物品使用时长（ticks）
    static constexpr i32 COOLDOWN_TICKS = 60;    // 冷却时间（ticks）
};

/**
 * @brief 看向顾客目标
 *
 * 商人在交易时看向顾客，但不移动位置。
 * 参考 MC 1.16.5 LookAtCustomerGoal
 */
class LookAtCustomerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     */
    explicit LookAtCustomerGoal(class MobEntity* mob);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "LookAtCustomerGoal"; }

private:
    class MobEntity* m_mob;
    Player* m_customer = nullptr;
    i32 m_lookTime = 0;

    static constexpr i32 LOOK_MIN_TIME = 40; // 最小看向时间（ticks）
    static constexpr i32 LOOK_MAX_TIME = 80; // 最大看向时间（ticks）
};

/**
 * @brief 与玩家交易目标
 *
 * 使商人实体在玩家附近时停止移动并准备交易。
 * 参考 MC 1.16.5 TradeWithPlayerGoal
 */
class TradeWithPlayerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物（通常是 AbstractVillagerEntity）
     */
    explicit TradeWithPlayerGoal(class MobEntity* mob);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "TradeWithPlayerGoal"; }

private:
    class MobEntity* m_mob;
    Player* m_customer = nullptr;

    static constexpr f32 TRADE_DISTANCE = 2.5f; // 交易交互距离
};

/**
 * @brief 流浪商人向目标移动目标
 *
 * 流浪商人向指定的游荡目标点移动。
 * 参考 MC 1.16.5 WanderingTraderEntity.MoveToGoal
 */
class MoveToWanderTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param trader 流浪商人实体
     * @param maxDistance 最大移动距离
     * @param speed 移动速度
     */
    MoveToWanderTargetGoal(class MobEntity* trader, f64 maxDistance, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "MoveToWanderTargetGoal"; }

private:
    /**
     * @brief 检查是否在目标距离范围内
     * @param pos 目标位置
     * @param distance 距离阈值
     * @return 是否超出范围
     */
    [[nodiscard]] bool isOutsideDistance(const BlockPos& pos, f64 distance) const;

    /**
     * @brief 计算移动目标位置
     * @param target 目标方块位置
     * @return 移动目标坐标
     */
    Vector3 calculateMoveTarget(const BlockPos& target) const;

private:
    class MobEntity* m_mob;
    f64 m_maxDistance;
    f64 m_speed;
    BlockPos m_wanderTarget;

    static constexpr f64 CLOSE_ENOUGH_DISTANCE = 10.0; // 近距离移动阈值
};

} // namespace wandering_trader
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
