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
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include <functional>
#include <optional>
#include <string>

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
 * 参考: net.minecraft.world.entity.ai.goal.UseItemGoal
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
     * @param soundEvent 使用物品完成时播放的声音事件
     * @param condition 使用条件判断函数
     */
    UseItemGoal(
        class MobEntity* mob, const ItemStack& stack, const ResourceLocation& soundEvent, UseCondition condition);

    /**
     * @brief 检查是否应该开始执行
     *
     * 当条件满足时返回 true。MC原版UseItemGoal没有冷却机制，
     * 由条件函数本身（如隐身效果是否存在）防止重复触发。
     */
    [[nodiscard]] bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     *
     * 当实体仍在使用物品时返回 true。
     */
    [[nodiscard]] bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     *
     * 将物品放入主手并开始使用。
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     *
     * 清空主手物品并播放完成音效。
     */
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "UseItemGoal"; }

private:
    class MobEntity* m_mob;
    ItemStack m_itemStack;
    ResourceLocation m_soundEvent;
    UseCondition m_condition;
};

/**
 * @brief 看向顾客目标
 *
 * 商人在交易时看向顾客，但不移动位置。
 *
 * 参考: net.minecraft.world.entity.ai.goal.LookAtTradingPlayerGoal
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

    static constexpr i32 LOOK_MIN_TIME = 40;   // 最小看向时间（ticks）
    static constexpr i32 LOOK_MAX_TIME = 80;   // 最大看向时间（ticks）
    static constexpr f32 LOOK_DISTANCE = 8.0f; // 看向距离（格）
};

/**
 * @brief 与玩家交易目标
 *
 * 使商人实体在玩家附近时停止移动并准备交易。
 *
 * 参考: net.minecraft.world.entity.ai.goal.TradeWithPlayerGoal
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

    [[nodiscard]] std::string getTypeName() const override { return "TradeWithPlayerGoal"; }

private:
    class MobEntity* m_mob;
    Player* m_customer = nullptr;

    static constexpr f64 TRADE_DISTANCE_SQ = 16.0; // 交易交互距离平方（4格）
};

/**
 * @brief 流浪商人向目标移动目标
 *
 * 流浪商人向指定的游荡目标点移动。
 * 距离目标超过10格时，先向目标方向移动10格作为中间航点（分段接近策略）；
 * 距离10格以内时，直接导航到目标点。
 *
 * 参考: net.minecraft.world.entity.npc.wanderingtrader.WanderingTrader.WanderToPositionGoal
 */
class MoveToWanderTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param trader 流浪商人实体
     * @param stopDistance 停止距离（到达此距离内视为已到达）
     * @param speed 移动速度
     */
    MoveToWanderTargetGoal(class MobEntity* trader, f64 stopDistance, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "MoveToWanderTargetGoal"; }

    static constexpr f64 INTERMEDIATE_DISTANCE = 10.0; // 远距离时分段接近的中间航点距离

private:
    /**
     * @brief 检查是否在目标距离范围外
     * @param pos 目标位置
     * @param distance 距离阈值
     * @return 是否超出范围
     */
    [[nodiscard]] bool _isOutsideDistance(const BlockPos& pos, f64 distance) const;

private:
    class MobEntity* m_mob;
    f64 m_stopDistance;
    f64 m_speed;
    BlockPos m_wanderTarget;
};

} // namespace wandering_trader
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
