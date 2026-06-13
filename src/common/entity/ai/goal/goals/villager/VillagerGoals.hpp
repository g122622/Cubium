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
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/world/block/BlockPos.hpp"
#include <optional>

namespace mc {
namespace entity {

// 前向声明
class VillagerEntity;

namespace ai {
namespace goal {
namespace villager {

/**
 * @brief 村民夜间睡眠目标
 *
 * 村民在夜间寻找床位并睡眠。
 * 需要先通过POI系统绑定床位。
 */
class SleepAtNightGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit SleepAtNightGoal(VillagerEntity* villager);

    /**
     * @brief 检查是否应该开始执行
     */
    [[nodiscard]] bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     */
    [[nodiscard]] bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     */
    void resetTask() override;

    /**
     * @brief 每tick更新
     */
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SleepAtNightGoal"; }

private:
    /**
     * @brief 检查是否是夜间
     */
    [[nodiscard]] bool _isNightTime() const;

    /**
     * @brief 寻找最近的床位
     * @return 床位位置（如果找到）
     */
    [[nodiscard]] std::optional<BlockPos> _findNearestBed() const;

    /**
     * @brief 移动到床位
     */
    void _moveToBed();

    /**
     * @brief 尝试睡眠
     */
    void _trySleep();

    /**
     * @brief 检查床位是否仍然有效
     * @return 床位是否有效
     */
    [[nodiscard]] bool _isBedStillValid() const;

private:
    VillagerEntity* m_villager;
    BlockPos m_bedPos;
    bool m_sleeping = false;
    i32 m_trySleepTicks = 0;                        // 尝试睡眠的tick计数
    static constexpr i32 MAX_TRY_SLEEP_TICKS = 100; // 最大尝试时间
};

/**
 * @brief 村民工作目标
 *
 * 村民在工作时间前往工作站点工作。
 * 包括补货逻辑。
 */
class WorkAtJobSiteGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param villager 村民实体
     */
    explicit WorkAtJobSiteGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "WorkAtJobSiteGoal"; }

protected:
    /**
     * @brief 检查是否是工作时间
     */
    [[nodiscard]] bool _isWorkTime() const;

    /**
     * @brief 检查是否有工作站点
     */
    [[nodiscard]] bool _hasJobSite() const;

    /**
     * @brief 移动到工作站点
     */
    void _moveToJobSite();

    /**
     * @brief 执行工作
     */
    void _doWork();

    /**
     * @brief 检查是否需要补货
     */
    [[nodiscard]] bool _needsRestock() const;

    /**
     * @brief 执行补货
     */
    void _restock();

protected:
    VillagerEntity* m_villager;

private:
    i32 m_workTicks = 0;
    bool m_atJobSite = false;
    i32 m_lastRestockDay = -1;                 // 上次补货的游戏日
    static constexpr i32 WORK_TICKS_MIN = 100; // 最小工作时间
    static constexpr i32 WORK_TICKS_MAX = 600; // 最大工作时间
};

/**
 * @brief 村民寻找工作站点目标
 *
 * 无职业村民寻找可用的工作站点。
 */
class LookForJobSiteGoal : public Goal {
public:
    explicit LookForJobSiteGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "LookForJobSiteGoal"; }

private:
    /**
     * @brief 搜索工作站点
     */
    void _searchForJobSite();

private:
    VillagerEntity* m_villager;
    std::optional<BlockPos> m_targetSite;
    i32 m_searchCooldown = 0;
    static constexpr i32 SEARCH_COOLDOWN = 200; // 搜索冷却
    static constexpr f32 SEARCH_RANGE = 48.0f;  // 搜索范围
};

/**
 * @brief 村民收集物品目标
 *
 * 村民收集地上的食物或种子等物品。
 */
class GatherItemsGoal : public Goal {
public:
    explicit GatherItemsGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "GatherItemsGoal"; }

private:
    /**
     * @brief 查找附近的物品
     */
    void _findNearbyItems();

    /**
     * @brief 移动到物品
     */
    void _moveToItem();

    /**
     * @brief 拾取物品
     */
    void _pickupItem();

private:
    VillagerEntity* m_villager;
    EntityId m_targetItem;
    static constexpr f32 PICKUP_RANGE = 32.0f;   // 搜索范围
    static constexpr f32 PICKUP_DISTANCE = 1.5f; // 拾取距离
};

/**
 * @brief 农民工作目标
 *
 * 农民特有的工作行为：种植、收获、堆肥。
 */
class FarmerWorkGoal : public WorkAtJobSiteGoal {
public:
    explicit FarmerWorkGoal(VillagerEntity* villager);

    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FarmerWorkGoal"; }

private:
    /**
     * @brief 尝试收获成熟作物
     */
    void _tryHarvest();

    /**
     * @brief 尝试种植作物
     */
    void _tryPlant();

    /**
     * @brief 尝试堆肥
     */
    void _tryCompost();

    /**
     * @brief 查找附近的农田
     * @return 农田位置（如果有）
     */
    [[nodiscard]] std::optional<BlockPos> _findFarmland() const;

    /**
     * @brief 检查作物是否成熟
     */
    [[nodiscard]] bool _isCropMature(BlockPos pos) const;

    /**
     * @brief 检查是否可以种植
     */
    [[nodiscard]] bool _canPlant(BlockPos pos) const;

private:
    i32 m_farmerWorkTicks = 0;
    BlockPos m_currentFarmland;
    static constexpr i32 FARMER_WORK_INTERVAL = 20; // 工作间隔
};

/**
 * @brief 村民逃避敌对目标
 *
 * 村民逃离僵尸、掠夺者等敌对生物。
 */
class AvoidHostileGoal : public Goal {
public:
    explicit AvoidHostileGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "AvoidHostileGoal"; }

private:
    /**
     * @brief 查找最近的敌对生物
     */
    void _findNearestHostile();

    /**
     * @brief 计算逃跑方向
     */
    void _fleeFromHostile();

private:
    VillagerEntity* m_villager;
    EntityId m_hostileEntity;
    BlockPos m_fleeTarget;
    static constexpr f32 FLEE_RANGE = 8.0f;     // 敌对生物触发距离
    static constexpr f32 FLEE_DISTANCE = 16.0f; // 逃跑距离
    static constexpr f32 FLEE_SPEED = 0.6f;     // 逃跑速度倍率
};

/**
 * @brief 村民前往床位目标
 *
 * 夜间前往床上睡觉的导航目标。
 */
class GoToBedGoal : public Goal {
public:
    explicit GoToBedGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "GoToBedGoal"; }

private:
    VillagerEntity* m_villager;
    BlockPos m_bedPos;
    bool m_reachedBed = false;
    static constexpr f32 SPEED_MODIFIER = 0.5f;
};

/**
 * @brief 村民繁殖目标
 *
 * 村民繁殖行为，需要足够的食物和床位。
 */
class VillagerBreedGoal : public Goal {
public:
    explicit VillagerBreedGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "VillagerBreedGoal"; }

private:
    /**
     * @brief 检查是否有足够的床位
     */
    [[nodiscard]] bool _hasEnoughBeds() const;

    /**
     * @brief 检查是否愿意繁殖
     */
    [[nodiscard]] bool _isWillingToBreed() const;

    /**
     * @brief 寻找繁殖伙伴
     */
    void _findPartner();

    /**
     * @brief 移动到伙伴
     */
    void _moveToPartner();

    /**
     * @brief 生成幼年村民
     */
    void _spawnChild();

private:
    VillagerEntity* m_villager;
    EntityId m_partnerId;
    i32 m_breedTicks = 0;
    static constexpr i32 BREED_TICKS = 60; // 繁殖动画时长
    static constexpr f32 BREED_DISTANCE = 2.0f;
};

/**
 * @brief 村民聚集目标
 *
 * 村民在聚集活动期间与其他村民互动。
 * 包括流言传播和物品分享。
 */
class CongregateGoal : public Goal {
public:
    explicit CongregateGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "CongregateGoal"; }

private:
    /**
     * @brief 查找交互目标
     */
    void _findInteractionTarget();

    /**
     * @brief 传播流言
     */
    void _spreadGossip();

    /**
     * @brief 分享物品（农民分享食物）
     */
    void _shareItems();

private:
    VillagerEntity* m_villager;
    EntityId m_targetVillagerId;
    i32 m_interactCooldown = 0;
    static constexpr i32 INTERACTION_DURATION = 100;  // 交互持续时间
    static constexpr f32 INTERACTION_DISTANCE = 5.0f; // 交互距离
};

/**
 * @brief 村民看向实体目标
 *
 * 村民随机看向附近的实体（村民、玩家、猫等）。
 */
class LookAtEntitiesGoal : public Goal {
public:
    explicit LookAtEntitiesGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "LookAtEntitiesGoal"; }

private:
    /**
     * @brief 选择目标类型
     */
    void _selectTargetType();

private:
    VillagerEntity* m_villager;
    EntityId m_lookTargetId;
    i32 m_lookTime = 0;

    enum class TargetType : u8 { Villager, Player, Cat, Creature };
    TargetType m_targetType = TargetType::Villager;

    static constexpr f32 LOOK_RANGE = 8.0f;   // 看向距离
    static constexpr f32 LOOK_CHANCE = 0.02f; // 触发概率
    static constexpr i32 LOOK_MIN_TIME = 40;  // 最小看向时间
    static constexpr i32 LOOK_MAX_TIME = 80;  // 最大看向时间
};

/**
 * @brief 村民分享物品目标
 *
 * 农民分享食物给其他村民。
 */
class ShareItemsGoal : public Goal {
public:
    explicit ShareItemsGoal(VillagerEntity* villager);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "ShareItemsGoal"; }

private:
    /**
     * @brief 检查是否有多余的食物可以分享
     */
    [[nodiscard]] bool _canAbandonItems() const;

    /**
     * @brief 检查目标是否需要食物
     * @param target 目标村民
     * @return 目标村民的食物点数是否低于需求阈值
     */
    [[nodiscard]] bool _targetNeedsFoodForTarget(VillagerEntity* target) const;

    /**
     * @brief 分享食物给目标
     */
    void _shareFoodWithTarget();

private:
    VillagerEntity* m_villager;
    EntityId m_targetVillagerId;
    i32 m_shareCooldown = 0;
    static constexpr f32 SHARE_DISTANCE = 2.0f; // 分享距离
    static constexpr i32 SHARE_COOLDOWN = 200;  // 分享冷却
};

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
