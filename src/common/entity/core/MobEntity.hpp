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

#include "../../util/math/random/Random.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../ai/goal/GoalSelector.hpp"
#include "LivingEntity.hpp"
#include <memory>

namespace mc {

// 前向声明
class Player;

namespace entity::ai::controller {
class LookController;
class MovementController;
class JumpController;
} // namespace entity::ai::controller

namespace entity::ai {
class EntitySenses;
}

namespace entity::ai::pathfinding {
class PathNavigator;
}

/**
 * @brief Mob 实体基类
 *
 * 所有 AI 生物的基类，包括怪物和动物。
 * 提供 AI 目标系统、控制器、寻路等功能。
 *
 * 参考 MC 1.16.5 MobEntity
 */
class MobEntity : public LivingEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    MobEntity(LegacyEntityType type, EntityId id);

    ~MobEntity() override;

    // 禁止拷贝
    MobEntity(const MobEntity&) = delete;
    MobEntity& operator=(const MobEntity&) = delete;

    // 允许移动
    MobEntity(MobEntity&&) = default;
    MobEntity& operator=(MobEntity&&) = default;

    // ========== AI 目标系统 ==========

    /**
     * @brief 获取行为目标选择器
     */
    [[nodiscard]] entity::ai::GoalSelector& goalSelector() { return m_goalSelector; }
    [[nodiscard]] const entity::ai::GoalSelector& goalSelector() const { return m_goalSelector; }

    /**
     * @brief 获取目标选择器（攻击目标等）
     */
    [[nodiscard]] entity::ai::GoalSelector& targetSelector() { return m_targetSelector; }
    [[nodiscard]] const entity::ai::GoalSelector& targetSelector() const { return m_targetSelector; }

    /**
     * @brief 注册 AI 目标
     *
     * 子类应重写此方法来注册自己的 AI 目标。
     */
    virtual void registerGoals() {}

    /**
     * @brief 获取环境声音间隔
     */
    [[nodiscard]] virtual i32 getTalkInterval() const { return 80; }

    /**
     * @brief 播放环境声音
     */
    void playAmbientSound();

    /**
     * @brief 播放近战攻击声音
     */
    virtual void playAttackSound(LivingEntity& target);

    // ========== 控制器 ==========

    /**
     * @brief 获取视线控制器
     */
    [[nodiscard]] entity::ai::controller::LookController* lookController();
    [[nodiscard]] const entity::ai::controller::LookController* lookController() const;

    /**
     * @brief 获取移动控制器
     */
    [[nodiscard]] entity::ai::controller::MovementController* moveController();
    [[nodiscard]] const entity::ai::controller::MovementController* moveController() const;

    /**
     * @brief 获取跳跃控制器
     */
    [[nodiscard]] entity::ai::controller::JumpController* jumpController();
    [[nodiscard]] const entity::ai::controller::JumpController* jumpController() const;

    // ========== 目标 ==========

    /**
     * @brief 获取攻击目标
     */
    [[nodiscard]] LivingEntity* attackTarget() { return m_attackTarget; }
    [[nodiscard]] const LivingEntity* attackTarget() const { return m_attackTarget; }

    /**
     * @brief 设置攻击目标
     */
    void setAttackTarget(LivingEntity* target) { m_attackTarget = target; }

    /**
     * @brief 检查是否处于激怒状态
     * MC 1.16.5: 激怒状态会触发特定的渲染效果
     */
    [[nodiscard]] bool isAggroed() const { return m_aggroed; }

    /**
     * @brief 设置激怒状态
     * MC 1.16.5: 在攻击目标时设置
     */
    void setAggroed(bool aggroed) { m_aggroed = aggroed; }

    // ========== 刻更新 ==========

    void tick() override;

    // ========== 属性注册 ==========

    /**
     * @brief 注册默认属性
     *
     * MC 1.16.5: MobEntity 在 LivingEntity 基础上设置 FOLLOW_RANGE = 16.0
     */
    void registerAttributes() override;

    // ========== AI 更新 ==========

    /**
     * @brief 更新 AI 任务
     *
     * 子类可重写此方法来添加额外的 AI 逻辑。
     * 在 goalSelector.tick() 和控制器更新之间调用。
     *
     * 参考 MC 1.16.5 MobEntity.updateAITasks()
     */
    virtual void updateAITasks() {}

    /**
     * @brief 更新移动目标标志
     *
     * 根据骑乘状态更新 GoalSelector 的 MOVE/JUMP/LOOK 标志。
     * MC 1.16.5 每 5 tick 调用一次。
     *
     * 参考 MC 1.16.5 MobEntity.updateMovementGoalFlags()
     */
    void updateMovementGoalFlags();

    // ========== AI 辅助方法 ==========

    /**
     * @brief 获取空闲时间
     */
    [[nodiscard]] i32 idleTime() const { return m_idleTime; }

    /**
     * @brief 设置空闲时间
     */
    void setIdleTime(i32 time) { m_idleTime = time; }

    /**
     * @brief 获取随机数生成器（基于实体ID和tick）
     */
    [[nodiscard]] math::Random getRandom() const;

    /**
     * @brief 检查是否被骑乘
     */
    [[nodiscard]] bool isBeingRidden() const;

    /**
     * @brief 获取导航器
     */
    [[nodiscard]] entity::ai::pathfinding::PathNavigator* navigator();
    [[nodiscard]] const entity::ai::pathfinding::PathNavigator* navigator() const;
    [[nodiscard]] entity::ai::EntitySenses* senses();
    [[nodiscard]] const entity::ai::EntitySenses* senses() const;

    // ========== AI 便捷方法 ==========

    /**
     * @brief 获取水平面部旋转速度
     *
     * 参考 MC 1.16.5 MobEntity.getHorizontalFaceSpeed()
     * 用于LookController限制导航路径时的头部旋转。
     * MC 1.16.5 默认值: 75
     */
    [[nodiscard]] virtual f32 getHorizontalFaceSpeed() const { return 75.0f; }

    /**
     * @brief 获取垂直面部旋转速度
     *
     * 参考 MC 1.16.5 MobEntity.getVerticalFaceSpeed()
     * 用于LookController限制俯仰角旋转速度。
     * MC 1.16.5 默认值: 40
     */
    [[nodiscard]] virtual f32 getVerticalFaceSpeed() const { return 40.0f; }

    /**
     * @brief 获取面部旋转速度
     *
     * 参考 MC 1.16.5 MobEntity.getFaceRotSpeed()
     * 用于LookController的默认偏航角旋转速度。
     * MC 1.16.5 默认值: 10
     */
    [[nodiscard]] virtual f32 getFaceRotSpeed() const { return 10.0f; }

    /**
     * @brief 清除导航路径
     *
     * 安全地清除导航器的路径，内部处理空指针检查。
     */
    void clearNavigation();

    /**
     * @brief 看向指定实体
     *
     * 使用视线控制器看向目标实体的眼睛位置。
     * @param target 目标实体
     * @param deltaYaw 最大偏航角变化速度（默认10）
     * @param deltaPitch 最大俯仰角变化速度（默认10）
     */
    void lookAt(const Entity& target, f32 deltaYaw = 10.0f, f32 deltaPitch = 10.0f);

    /**
     * @brief 看向指定位置
     *
     * 使用视线控制器看向指定位置。
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param deltaYaw 最大偏航角变化速度（默认10）
     * @param deltaPitch 最大俯仰角变化速度（默认10）
     */
    void lookAt(f64 x, f64 y, f64 z, f32 deltaYaw = 10.0f, f32 deltaPitch = 10.0f);

    // ========== 经验值 ==========

    /**
     * @brief 获取经验值
     *
     * 死亡时掉落的经验值数量。
     */
    [[nodiscard]] i32 experienceValue() const { return m_experienceValue; }

    /**
     * @brief 设置经验值
     * @param value 经验值
     */
    void setExperienceValue(i32 value) { m_experienceValue = value; }

    // ========== 家范围系统 (Home Position) ==========

    /**
     * @brief 检查当前位置是否在家范围内
     *
     * MC 1.16.5: isWithinHomeDistanceCurrentPosition()
     * @return 如果当前位置在家范围内返回 true
     */
    [[nodiscard]] bool isWithinHomeDistanceCurrentPosition() const
    {
        return isWithinHomeDistanceFromPosition(BlockPos(position()));
    }

    /**
     * @brief 检查指定位置是否在家范围内
     *
     * MC 1.16.5: isWithinHomeDistanceFromPosition(BlockPos)
     * 如果未设置家范围（maximumHomeDistance == -1.0F），总是返回 true
     *
     * @param pos 要检查的位置
     * @return 如果位置在家范围内返回 true
     */
    [[nodiscard]] bool isWithinHomeDistanceFromPosition(const BlockPos& pos) const
    {
        if (m_maximumHomeDistance < 0.0f) {
            return true; // 未设置家范围，任何位置都允许
        }
        // MC 1.16.5: this.homePosition.distanceSq(pos) < (double)(this.maximumHomeDistance * this.maximumHomeDistance)
        f64 dx = static_cast<f64>(m_homePosition.x - pos.x);
        f64 dy = static_cast<f64>(m_homePosition.y - pos.y);
        f64 dz = static_cast<f64>(m_homePosition.z - pos.z);
        f64 distSq = dx * dx + dy * dy + dz * dz;
        f64 maxDistSq = static_cast<f64>(m_maximumHomeDistance) * static_cast<f64>(m_maximumHomeDistance);
        return distSq < maxDistSq;
    }

    /**
     * @brief 设置家位置和范围
     *
     * MC 1.16.5: setHomePosAndDistance(BlockPos, int)
     * @param pos 家位置
     * @param distance 家范围半径
     */
    void setHomePosAndDistance(const BlockPos& pos, i32 distance)
    {
        m_homePosition = pos;
        m_maximumHomeDistance = static_cast<f32>(distance);
    }

    /**
     * @brief 获取家位置
     *
     * MC 1.16.5: getHomePosition()
     * @return 家位置
     */
    [[nodiscard]] const BlockPos& homePosition() const { return m_homePosition; }

    /**
     * @brief 获取家范围最大距离
     *
     * MC 1.16.5: getMaximumHomeDistance()
     * @return 家范围半径，-1 表示未设置
     */
    [[nodiscard]] f32 maximumHomeDistance() const { return m_maximumHomeDistance; }

    /**
     * @brief 检查是否有家范围限制
     *
     * MC 1.16.5: detachHome()
     * @return 如果设置了家范围限制返回 true
     */
    [[nodiscard]] bool hasHome() const { return m_maximumHomeDistance >= 0.0f; }

    /**
     * @brief 清除家范围限制
     *
     * MC 1.16.5: 将最大距离设为 -1.0F
     */
    void clearHome() { m_maximumHomeDistance = -1.0f; }

    // ========== 持久化系统 (Persistence) ==========

    /**
     * @brief 检查是否需要持久化（不会消失）
     *
     * MC 1.16.5: MobEntity.isNoDespawnRequired()
     * 当生物被命名牌命名、拾取装备等情况时，会被标记为持久化，
     * 永远不会因为距离过远而消失。
     *
     * @return 如果实体需要持久化返回 true
     */
    [[nodiscard]] bool isNoDespawnRequired() const { return m_persistenceRequired; }

    /**
     * @brief 启用持久化
     *
     * MC 1.16.5: MobEntity.enablePersistence()
     * 标记实体为持久化，使其不会被自然消失机制清除。
     * 调用场景：
     * - 命名牌命名时
     * - 拾取装备时
     * - NBT 数据加载时
     */
    void enablePersistence() { m_persistenceRequired = true; }

    /**
     * @brief 检查是否应阻止消失
     *
     * MC 1.16.5: MobEntity.preventDespawn()
     * 当实体正在被骑乘时，不应消失。
     * 子类可以重写此方法添加额外的阻止消失条件。
     * 例如：AbstractFishEntity 在从桶放出时也应阻止消失。
     *
     * @return 如果实体应阻止消失返回 true
     */
    [[nodiscard]] virtual bool preventDespawn() const { return isRiding(); }

    /**
     * @brief 检查是否可以消失
     *
     * MC 1.16.5: MobEntity.canDespawn(double)
     * 子类可重写此方法来自定义消失行为。
     * 例如：AnimalEntity 返回 false（动物不会自然消失）
     *
     * @param distanceToClosestPlayer 到最近玩家的距离
     * @return 如果实体可以消失返回 true
     */
    [[nodiscard]] virtual bool canDespawn(double distanceToClosestPlayer) const
    {
        (void)distanceToClosestPlayer;
        return true;
    }

    /**
     * @brief 检查在和平模式下是否应消失
     *
     * MC 1.16.5: MobEntity.isDespawnPeaceful()
     * MonsterEntity 重写为 true。
     *
     * @return 如果在和平模式下应消失返回 true
     */
    [[nodiscard]] virtual bool isDespawnPeaceful() const { return false; }

    // ========== 日光检测 ==========

    /**
     * @brief 检查是否暴露在日光下
     *
     * 参考 MC 1.16.5 MobEntity.isInDaylight()
     * 用于怪物燃烧（僵尸、骷髅等）和幻翼燃烧。
     *
     * 检查条件：
     * 1. 世界为白天 (dayTime < 12000)
     * 2. 天空可见 (canSeeSky)
     * 3. 亮度 > 0.5
     * 4. 不在水中或雨中
     *
     * @return 如果暴露在日光下返回 true
     */
    [[nodiscard]] bool isInDaylight() const;

    // ========== 攻击 ==========

    /**
     * @brief 作为生物攻击实体
     *
     * MC 1.16.5 MobEntity.attackEntityAsMob()
     * 执行近战攻击，包括：
     * 1. 获取攻击伤害属性
     * 2. 应用附魔伤害加成（锋利、亡灵杀手、节肢杀手）
     * 3. 应用击退
     * 4. 应用火焰附加
     * 5. 设置最后攻击者
     *
     * @param target 目标实体（必须是 LivingEntity）
     * @return 是否攻击成功
     */
    virtual bool attackEntityAsMob(LivingEntity& target);

    // ========== 掉落 ==========

    /**
     * @brief 掉落经验
     *
     * 重写 LivingEntity::dropExperience()，在死亡时生成经验球。
     */
    void dropExperience() override;

    // ========== 玩家交互 ==========

    /**
     * @brief 处理玩家初始交互
     *
     * MC 1.16.5: MobEntity.processInitialInteract()
     * 重写 Entity::processInitialInteract() 以处理生物特有交互：
     * 1. 检查拴绳（如果玩家手持拴绳）
     * 2. 检查命名牌
     * 3. 检查刷怪蛋
     * 4. 调用 interactMob() 让子类处理特定交互
     *
     * @param player 与此实体交互的玩家
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    ActionResultType processInitialInteract(Player& player, Hand hand) override;

    /**
     * @brief 子类实现的交互逻辑
     *
     * MC 1.16.5: MobEntity.func_230254_b_()
     * 由 processInitialInteract() 调用，让子类处理特定交互。
     * 例如：AbstractHorseEntity 在此处理喂食、装备鞍等。
     *
     * @param player 与此实体交互的玩家
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    [[nodiscard]] virtual ActionResultType interactMob(Player& player, Hand hand);

protected:
    /**
     * @brief 获取环境声音
     */
    [[nodiscard]] virtual std::optional<ResourceLocation> getAmbientSound() const;

    void playHurtSound(DamageSource& source) override;

    // AI 目标选择器
    entity::ai::GoalSelector m_goalSelector;
    entity::ai::GoalSelector m_targetSelector;

    // 控制器
    std::unique_ptr<entity::ai::controller::LookController> m_lookController;
    std::unique_ptr<entity::ai::controller::MovementController> m_moveController;
    std::unique_ptr<entity::ai::controller::JumpController> m_jumpController;
    std::unique_ptr<entity::ai::EntitySenses> m_senses;

    // 寻路器
    std::unique_ptr<entity::ai::pathfinding::PathNavigator> m_navigator;

    // 攻击目标
    LivingEntity* m_attackTarget = nullptr;

    // AI 状态
    bool m_aiEnabled = true;
    bool m_aggroed = false; // MC 1.16.5: 激怒状态
    i32 m_idleTime = 0;     // 空闲时间（用于随机漫步等）
    i32 m_livingSoundTime = 0;

    // 经验值（死亡时掉落）
    i32 m_experienceValue = 0;

    // 家范围系统 (MC 1.16.5 MobEntity)
    BlockPos m_homePosition;           // 家位置，默认为 (0, 0, 0)
    f32 m_maximumHomeDistance = -1.0f; // 家范围半径，-1 表示未设置

    // 持久化系统 (MC 1.16.5 MobEntity)
    bool m_persistenceRequired = false; // 是否需要持久化（不消失）
};

} // namespace mc
