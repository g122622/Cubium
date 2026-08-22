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
#include "../ai/pathfinding/PathNodeType.hpp"
#include "../combat/DifficultyInstance.hpp"
#include "LivingEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace mc {

// 前向声明
class Player;
class Item;
class ItemEntity;

namespace item {
class SpawnEggItem;
} // namespace item

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
 * @brief 拴绳延迟信息
 *
 * 当实体从 NBT 加载时，拴绳的目标实体可能尚未加载，
 * 因此需要延迟解析。此结构体存储从 NBT 中读取的原始数据，
 * 待目标实体加载后再进行实际绑定。
 */
struct LeashDelayInfo {
    /// 拴绳目标为实体时，存储目标实体的 UUID
    std::optional<std::string> targetUuid;
    /// 拴绳目标为栅栏柱时，存储栅栏柱坐标
    std::optional<BlockPos> fencePos;
    /// 延迟绑定的尝试次数（用于超时掉落拴绳）
    i32 resolveTicks = 0;
};

/**
 * @brief Mob 实体基类
 *
 * 所有 AI 生物的基类，包括怪物和动物。
 * 提供 AI 目标系统、控制器、寻路等功能。
 */
class MobEntity : public LivingEntity {
public:
    /**
     * @brief 构造函数
     * @param id 实体ID
     * @param registry ECS 实体注册表，透传给 LivingEntity→Entity 构造函数
     */
    MobEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    ~MobEntity() override;

    // 禁止拷贝
    MobEntity(const MobEntity&) = delete;
    MobEntity& operator=(const MobEntity&) = delete;

    // 禁止移动（基类 LivingEntity 不可移动）
    MobEntity(MobEntity&&) = delete;
    MobEntity& operator=(MobEntity&&) = delete;

    /**
     * @brief 注册同步数据参数
     *
     * 重写 LivingEntity::registerData，注册 DATA_MOB_FLAGS_PARAM。
     * 由于 C++ 虚函数在基类构造函数中不会派发到派生类，
     * 派生类构造函数必须显式调用 registerData()，参考 WolfEntity / AbstractSkeletonEntity 模式。
     */
    void registerData() override;

    // ========== 攻击/激怒状态同步 ==========

    /**
     * @brief 检查是否处于激怒（攻击中）状态
     *
     * 通过 DATA_MOB_FLAGS_PARAM 位 2 (0x04) 同步到客户端，
     * 对应 MC 1.21.11 Mob.isAggressive()。
     * 由 MeleeAttackGoal 等攻击目标在 start/reset 时设置，
     * 驱动 ZombieModel/SkeletonModel 的空手攻击抬臂动画。
     */
    [[nodiscard]] bool isAggressive() const
    {
        return (m_dataManager.get<i8>(DATA_MOB_FLAGS_PARAM) & static_cast<i8>(MOB_FLAG_AGGRESSIVE)) != 0;
    }

    /**
     * @brief 设置激怒（攻击中）状态
     *
     * 写入 DATA_MOB_FLAGS_PARAM 位 2，由 EntityTracker 自动广播到所有观察者客户端。
     * 同时维护 m_aggroed 内存镜像字段以保持向后兼容。
     *
     * @param aggressive 是否处于激怒状态
     */
    void setAggressive(bool aggressive)
    {
        m_aggroed = aggressive;
        const i8 current = m_dataManager.get<i8>(DATA_MOB_FLAGS_PARAM);
        const i8 updated = aggressive ? static_cast<i8>(current | MOB_FLAG_AGGRESSIVE)
                                      : static_cast<i8>(current & ~MOB_FLAG_AGGRESSIVE);
        m_dataManager.set(DATA_MOB_FLAGS_PARAM, updated);
    }

    /**
     * @brief 获取 Mob 标志位参数 ID（供客户端 syncMetadataFromDataManager 使用）
     */
    [[nodiscard]] static u16 getMobFlagsParamId() { return DATA_MOB_FLAGS_PARAM.id(); }

    /**
     * @brief 获取"激怒"标志位的位掩码（供客户端解析 DATA_MOB_FLAGS_PARAM 使用）
     *
     * 对应 MC 1.21.11 Mob.MOB_FLAG_AGGRESSIVE（位 2，0x04）。
     * ClientEntity::syncMetadataFromDataManager 读取 DATA_MOB_FLAGS_PARAM 后，
     * 用此掩码按位与判断 aggressive 位是否置位。
     */
    [[nodiscard]] static constexpr i8 getAggressiveFlagMask() { return MOB_FLAG_AGGRESSIVE; }

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
    [[nodiscard]] virtual LivingEntity* attackTarget() { return m_attackTarget; }
    [[nodiscard]] virtual const LivingEntity* attackTarget() const { return m_attackTarget; }

    /**
     * @brief 设置攻击目标
     *
     * 虚函数，允许IAngerable实体（如猪灵、铁傀儡、末影人等）
     * 在设置攻击目标时同步更新愤怒状态。
     * 通过MobEntity*指针调用时也能正确派发到子类的override。
     */
    virtual void setAttackTarget(LivingEntity* target) { m_attackTarget = target; }

    /**
     * @brief 检查是否处于激怒状态
     * 激怒状态会触发特定的渲染效果
     */
    [[nodiscard]] bool isAggroed() const { return isAggressive(); }

    /**
     * @brief 设置激怒状态
     * 在攻击目标时设置，通过 metadata 同步到客户端
     */
    void setAggroed(bool aggroed) { setAggressive(aggroed); }

    // ========== 刻更新 ==========

    void tick() override;

    /**
     * @brief AI 链 tick（UAF 防护 + senses/selector/navigator/controllers）
     *
     * 承载原 MobEntity::tick() 中 AI 链调用块（含前置 m_attackTarget/m_lastHurtBy 的 isRemoved
     * UAF 防护 + senses/targetSelector/goalSelector/navigator/updateAITasks/
     * updateMovementGoalFlags/moveController/lookController/jumpController）。由桥接壳 system
     * ecs::sys::mobAiTick 在 SystemPhase::PostEntityTick 阶段经 EntityManager::_tickMobAi 调用。
     *
     * 时序保持 1-tick 跨帧语义：aiStep→travel 仍在 LivingEntity::tick()（EntityTick 阶段，
     * 早于 PostEntityTick）内消费上一帧 AI 链写入的 m_moveForward/moveStrafing；本 tickAiChain
     * 在 EntityTick 之后执行，写入的输入供下一帧 aiStep 消费——与 vanilla MobEntity.tick()
     * （先 super.tick() 含 aiStep/travel，后 serverAiStep）时序等价。aiStep 不抽 system（见
     * ecs/README 坑24 与 ecs-wiggly-cat.md 阶段 C+F 计划前提修正）。
     *
     * 门控：m_aiEnabled 仅门控 targetSelector/goalSelector/navigator/updateAITasks/
     * updateMovementGoalFlags；senses 与三个 controller（move/look/jump）在门控外永远执行
     * （对齐 vanilla noAI 时仍跑感知与控制器）。
     */
    void tickAiChain();

    // ========== 属性注册 ==========

    /**
     * @brief 注册默认属性
     *
     * 在 LivingEntity 基础上设置 FOLLOW_RANGE = 16.0
     */
    void registerAttributes() override;

    // ========== AI 更新 ==========

    /**
     * @brief 更新 AI 任务
     *
     * 子类可重写此方法来添加额外的 AI 逻辑。
     * 在 goalSelector.tick() 和控制器更新之间调用。
     */
    virtual void updateAITasks() {}

    /**
     * @brief 更新移动目标标志
     *
     * 根据骑乘状态更新 GoalSelector 的 MOVE/JUMP/LOOK 标志。
     * 每 5 tick 调用一次。
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

    // getRandom() 继承自 Entity 基类，返回持久化随机数生成器的引用

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

    // ========== 寻路惩罚值 (Pathfinding Malus) ==========

    /**
     * @brief 设置指定路径节点类型的寻路惩罚值
     *
     * 对应 Minecraft 原版的 Mob.setPathfindingMalus(PathType, float)。
     * 用于在实体构造函数中声明实体对特定地形（水、岩浆、火焰等）的
     * 寻路代价偏好。WalkNodeProcessor 在创建节点时会读取此值覆盖默认代价。
     *
     * 惩罚值语义：
     * - 负值（如 -1.0F）：完全不可通行，节点会被排除出寻路
     * - 0.0F：无惩罚，按默认路径类型代价处理
     * - 正值（如 8.0F/16.0F）：高代价但可通行，A* 会优先选择更便宜的路径
     *
     * @param pathType 路径节点类型
     * @param malus 惩罚值
     */
    void setPathfindingMalus(entity::ai::pathfinding::PathNodeType pathType, f32 malus) noexcept
    {
        m_pathfindingMalus[static_cast<size_t>(pathType)] = malus;
    }

    /**
     * @brief 获取指定路径节点类型的寻路惩罚值
     *
     * 对应 Minecraft 原版的 Mob.getPathfindingMalus(PathType)。
     *
     * 乘客继承逻辑：若当前实体骑乘在重写了 shouldPassengersInheritMalus()
     * 返回 true 的 Mob 载具上（如炽足兽 Strider），则返回载具的 malus 值；
     * 否则返回自身的 malus 值。这与 MC Java 中"乘客使用载具的 malus"
     * 的行为一致，使骑乘者也能在载具适配的地形（如岩浆）上寻路。
     *
     * 默认值回退：若未通过 setPathfindingMalus() 显式设置该类型，
     * 回退到 PathNodeType 的默认惩罚值（getPathCostPenalty(pathType)，
     * 对应 MC Java 的 PathType.getMalus()）。
     *
     * @param pathType 路径节点类型
     * @return 惩罚值（负值表示不可通行）
     */
    [[nodiscard]] f32 getPathfindingMalus(entity::ai::pathfinding::PathNodeType pathType) const noexcept;

    /**
     * @brief 乘客是否继承载具的寻路惩罚值
     *
     * 对应 Minecraft 原版的 Mob.shouldPassengersInheritMalus()。
     * 默认返回 false。炽足兽（Strider）等载具重写为 true，
     * 使骑乘者能在岩浆上寻路。
     *
     * @return 如果乘客应继承载具的 malus 返回 true
     */
    [[nodiscard]] virtual bool shouldPassengersInheritMalus() const noexcept { return false; }

    // ========== AI 便捷方法 ==========

    /**
     * @brief 获取水平面部旋转速度
     *
     * 用于LookController限制导航路径时的头部旋转。
     * 默认值: 75
     */
    [[nodiscard]] virtual f32 getHorizontalFaceSpeed() const { return 75.0f; }

    /**
     * @brief 获取垂直面部旋转速度
     *
     * 用于LookController限制俯仰角旋转速度。
     * 默认值: 40
     */
    [[nodiscard]] virtual f32 getVerticalFaceSpeed() const { return 40.0f; }

    /**
     * @brief 获取面部旋转速度
     *
     * 用于LookController的默认偏航角旋转速度。
     * 默认值: 10
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

    // ========== 生成初始化 ==========

    /**
     * @brief 完成生成初始化
     *
     * 对应 Minecraft 原版的 Mob.finalizeSpawn()。
     * 在实体被创建并设置好位置后调用，用于根据难度和生成原因
     * 进行初始化，包括：
     * - 设置拾取物品能力
     * - 设置破门能力
     * - 填充默认装备（基于难度）
     * - 附魔默认装备（基于难度）
     * - 子类特定的初始化（如僵尸的武器、万圣节南瓜头等）
     *
     * @param world 世界引用
     * @param difficulty 区域难度实例
     * @param spawnReason 生成原因
     */
    virtual void finalizeSpawn(
        IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason);

    /**
     * @brief 根据难度填充默认装备槽位
     *
     * 对应 Minecraft 原版的 Mob.populateDefaultEquipmentSlots()。
     * 基础实现根据区域难度的 specialMultiplier 决定是否生成护甲，
     * 并根据护甲等级选择护甲材质。
     *
     * 子类可重写此方法以添加实体特定的装备（如僵尸的铁剑/铁锹）。
     *
     * @param random 随机数生成器
     * @param difficulty 区域难度实例
     */
    virtual void populateDefaultEquipmentSlots(
        math::Random& random, const entity::combat::DifficultyInstance& difficulty);

    /**
     * @brief 根据难度附魔默认装备
     *
     * 对应 Minecraft 原版的 Mob.populateDefaultEquipmentEnchantments()。
     * 对主手武器和护甲进行附魔，概率取决于区域难度的 specialMultiplier：
     * - 武器附魔概率: 0.25 * specialMultiplier
     * - 护甲附魔概率: 0.5 * specialMultiplier（每个护甲槽位独立检定）
     *
     * @param random 随机数生成器
     * @param difficulty 区域难度实例
     */
    virtual void populateDefaultEquipmentEnchantments(
        math::Random& random, const entity::combat::DifficultyInstance& difficulty);

    /**
     * @brief 根据装备槽位和护甲等级获取对应的装备物品
     *
     * 对应 Minecraft 原版的 Mob.getEquipmentForSlot()。
     * 护甲等级 0-5 对应的材质：
     * 0 = 皮革, 1 = 铜色(铁), 2 = 金, 3 = 锁链, 4 = 铁, 5 = 钻石
     *
     * @param slot 装备槽位（Head/Chest/Legs/Feet）
     * @param armorLevel 护甲等级 (0-5)
     * @return 对应的物品指针，如果无对应物品则返回 nullptr
     */
    [[nodiscard]] static const Item* getEquipmentForSlot(EquipmentSlot slot, i32 armorLevel);

    // ========== 家范围系统 (Home Position) ==========

    /**
     * @brief 检查当前位置是否在家范围内
     */
    [[nodiscard]] bool isWithinHomeDistanceCurrentPosition() const
    {
        return isWithinHomeDistanceFromPosition(BlockPos(position()));
    }

    /**
     * @brief 检查指定位置是否在家范围内
     *
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
     * @return 家位置
     */
    [[nodiscard]] const BlockPos& homePosition() const { return m_homePosition; }

    /**
     * @brief 获取家范围最大距离
     * @return 家范围半径，-1 表示未设置
     */
    [[nodiscard]] f32 maximumHomeDistance() const { return m_maximumHomeDistance; }

    /**
     * @brief 检查是否有家范围限制
     * @return 如果设置了家范围限制返回 true
     */
    [[nodiscard]] bool hasHome() const { return m_maximumHomeDistance >= 0.0f; }

    /**
     * @brief 清除家范围限制
     */
    void clearHome() { m_maximumHomeDistance = -1.0f; }

    // ========== 拾取物品 (CanPickUpLoot) ==========

    /**
     * @brief 检查生物是否可以拾取物品
     *
     * 当 canPickUpLoot 为 true 时，生物会拾取地上的装备和物品。
     * 由区域难度决定：概率 = 0.55 * specialMultiplier
     *
     * @return 如果生物可以拾取物品返回 true
     */
    [[nodiscard]] bool canPickUpLoot() const { return m_canPickUpLoot; }

    /**
     * @brief 设置生物是否可以拾取物品
     *
     * @param canPickUp 是否可以拾取
     */
    void setCanPickUpLoot(bool canPickUp) { m_canPickUpLoot = canPickUp; }

    /**
     * @brief 拾取范围（对齐 vanilla Mob.getPickupReach）
     *
     * 返回 AABB 在三轴上的 inflate 量。vanilla 默认 Vec3i(1, 0, 1)——
     * 仅水平 ±1 格、Y 不扩展（同高度扫描）。子类可覆写增大（如拾取距离更远的实体）。
     *
     * @return 三轴 inflate 量（块为单位）
     */
    [[nodiscard]] virtual Vector3i getPickupReach() const;

    /**
     * @brief 生物是否想要拾取该物品（对齐 vanilla Mob.wantsToPickUp）
     *
     * 默认实现委托 canHoldItem。子类（如 Fox）可覆写以加入更精细的判断。
     *
     * @param itemStack 待拾取物品堆
     * @return 是否愿意拾取
     */
    [[nodiscard]] virtual bool wantsToPickUp(const ItemStack& itemStack) const;

    /**
     * @brief 生物是否能持有该物品（对齐 vanilla Mob.canHoldItem）
     *
     * 基类默认实现：装备槽为空时可拾取任何可装备物品。子类（如 Fox）覆写为
     * 手持物品语义。vanilla Mob.canHoldItem 默认检查装备槽可替换性。
     *
     * TODO: 基类版仅做装备槽空位检查的简化判定，vanilla 完整 canHoldItem 还含
     *   dropChances 守卫（dropChance<=1 才允许拾取装备）与 canReplaceCurrentItem
     *   装备对比替换逻辑，待装备拾取链路完整补全后对齐。
     *
     * @param itemStack 待判定物品堆
     * @return 是否能持有
     */
    [[nodiscard]] virtual bool canHoldItem(const ItemStack& itemStack) const;

    /**
     * @brief 拾取物品实体（对齐 vanilla Mob.pickUpItem）
     *
     * 由 MobEntity::tick 的 looting 扫描段在 AABB.inflate(getPickupReach) 内发现
     * 可拾取 ItemEntity 且 wantsToPickUp 为真时调用。基类默认实现走装备槽拾取
     * （equipItemIfPossible 语义），子类（如 Fox）覆写为手持物品语义。
     *
     * @param itemEntity 被拾取的物品实体
     */
    virtual void pickUpItem(ItemEntity& itemEntity);

    // ========== 掉落概率 (DropChances) ==========

    /**
     * @brief 默认装备掉落概率
     *
     * 未特别设置时，所有装备槽位的掉落概率为 0.085 (8.5%)。
     */
    static constexpr f32 DEFAULT_EQUIPMENT_DROP_CHANCE = 0.085f;

    /**
     * @brief 保整天花板值
     *
     * 当掉落概率设为 2.0 时，表示物品被保留（总是掉落且不会因消失而丢失）。
     * 任何 > 1.0 的值都被视为"保留"。
     */
    static constexpr f32 PRESERVE_ITEM_DROP_CHANCE = 2.0f;

    /**
     * @brief 获取指定装备槽位的掉落概率
     *
     * 如果槽位超出范围，返回默认值 0.085。
     *
     * @param slot 装备槽位
     * @return 掉落概率（0.0 = 永不掉落, 0.085 = 默认, >1.0 = 保留）
     */
    [[nodiscard]] f32 getEquipmentDropChance(EquipmentSlot slot) const;

    /**
     * @brief 设置指定装备槽位的掉落概率
     *
     * @param slot 装备槽位
     * @param chance 掉落概率（不能为负数）
     */
    void setEquipmentDropChance(EquipmentSlot slot, f32 chance);

    /**
     * @brief 设置指定装备槽位为保整掉落（总是掉落）
     *
     * 将掉落概率设为 2.0（>1.0 表示保留）。
     *
     * @param slot 装备槽位
     */
    void setGuaranteedDrop(EquipmentSlot slot);

    /**
     * @brief 检查指定装备槽位的物品是否被保留（不掉落或总是掉落）
     *
     * 当掉落概率 > 1.0 时，物品被视为保留。
     *
     * @param slot 装备槽位
     * @return 如果物品被保留返回 true
     */
    [[nodiscard]] bool isEquipmentDropPreserved(EquipmentSlot slot) const;

    // ========== 身体护甲 (Body Armor) ==========

    /**
     * @brief 获取身体护甲槽位的物品堆
     *
     * 用于狼铠、鹦鹉螺铠甲、马铠等非玩家实体的身体护甲。
     * 参考: net.minecraft.world.entity.Mob.getBodyArmorItem()
     *
     * @return 身体护甲槽位的物品堆引用
     */
    [[nodiscard]] const ItemStack& getBodyArmorItem() const { return getEquipment(EquipmentSlot::Body); }

    /**
     * @brief 检查是否穿戴了身体护甲
     *
     * 检查 Body 槽位是否有有效（非空）的护甲物品。
     * 参考: net.minecraft.world.entity.Mob.isWearingBodyArmor()
     *
     * @return 如果穿戴了身体护甲返回 true
     */
    [[nodiscard]] bool isWearingBodyArmor() const { return !getEquipment(EquipmentSlot::Body).isEmpty(); }

    /**
     * @brief 设置身体护甲槽位的物品
     *
     * 同时设置该槽位为保整掉落（确保死亡时掉落护甲），
     * 并启用实体持久化（防止穿戴护甲的实体自然消失）。
     * 参考: net.minecraft.world.entity.Mob.setBodyArmorItem()
     *
     * @param stack 要装备的物品堆
     */
    void setBodyArmorItem(const ItemStack& stack);

    // ========== 剪切装备 (Shear Equipment) ==========

    /**
     * @brief 检查玩家是否可以剪切实体的装备
     *
     * 默认行为：实体未被骑乘时允许剪切。
     * 子类可重写此方法添加额外条件（如狼仅允许主人剪切）。
     * 参考: net.minecraft.world.entity.Mob.canShearEquipment()
     *
     * @param player 尝试剪切的玩家
     * @return 如果允许剪切返回 true
     */
    [[nodiscard]] virtual bool canShearEquipment(const Player& player) const;

    /**
     * @brief 尝试用剪刀剪下实体身上的装备
     *
     * 当玩家手持剪刀右键实体时，如果实体穿着可剪切的装备（如狼铠），
     * 则将装备从实体身上剪下并掉落为物品实体。
     * 参考: net.minecraft.world.entity.Entity.attemptToShearEquipment()
     *
     * 流程：
     * 1. 遍历所有装备槽位
     * 2. 找到第一个有装备的槽位
     * 3. 剪刀耐久 -1
     * 4. 将装备从槽位移除（设为空堆）
     * 5. 在实体位置生成物品掉落
     * 6. 播放剪切音效
     * 7. 触发 SHEAR 游戏事件
     *
     * @param player 剪切的玩家
     * @param hand 玩家使用的手
     * @param shears 剪刀物品堆
     * @return 如果成功剪切了装备返回 true
     */
    bool attemptToShearEquipment(Player& player, Hand hand, ItemStack& shears);

    // ========== 掉落表 (DeathLootTable) ==========

    /**
     * @brief 获取实体的战利品表ID
     *
     * 优先使用 NBT 中设置的自定义掉落表（m_deathLootTable），
     * 如果没有自定义掉落表则回退到实体类型的默认掉落表路径。
     * 逻辑：this.lootTable.isPresent() ? this.lootTable : super.getLootTable()
     *
     * @return 战利品表ID字符串，无战利品表时返回空字符串
     */
    [[nodiscard]] std::string getLootTableId() const override;

    /**
     * @brief 获取死亡掉落表 ID（NBT 覆盖值）
     *
     * 如果实体有自定义掉落表（从 NBT 加载），返回自定义掉落表；
     * 否则返回空，表示使用实体类型的默认掉落表。
     *
     * @return 自定义掉落表 ID，如果使用默认掉落表则返回空
     */
    [[nodiscard]] const std::optional<std::string>& deathLootTable() const { return m_deathLootTable; }

    /**
     * @brief 设置自定义死亡掉落表
     *
     * @param lootTableId 掉落表 ID（格式如 "minecraft:entities/zombie"），空表示使用默认
     */
    void setDeathLootTable(std::optional<std::string> lootTableId) { m_deathLootTable = std::move(lootTableId); }

    /**
     * @brief 获取掉落表种子
     *
     * 非零种子用于确定性地生成掉落物。
     *
     * @return 掉落表种子
     */
    [[nodiscard]] i64 lootTableSeed() const { return m_lootTableSeed; }

    /**
     * @brief 设置掉落表种子
     *
     * @param seed 种子值
     */
    void setLootTableSeed(i64 seed) { m_lootTableSeed = seed; }

    // ========== 拴绳系统 (Leash) ==========

    /**
     * @brief 检查实体是否被拴绳拴住
     *
     * @return 如果实体被拴住返回 true
     */
    [[nodiscard]] bool isLeashed() const { return m_isLeashed; }

    /**
     * @brief 获取拴绳目标实体的 UUID
     *
     * 当实体被拴绳拴在另一个实体上时，返回目标实体的 UUID。
     *
     * @return 目标实体 UUID，如果未拴住或拴在栅栏上则返回空
     */
    [[nodiscard]] const std::optional<std::string>& leashHolderUuid() const { return m_leashHolderUuid; }

    /**
     * @brief 获取拴绳目标栅栏柱坐标
     *
     * 当实体被拴绳拴在栅栏柱上时，返回栅栏柱坐标。
     *
     * @return 栅栏柱坐标，如果未拴住或拴在实体上则返回空
     */
    [[nodiscard]] const std::optional<BlockPos>& leashFencePos() const { return m_leashFencePos; }

    /**
     * @brief 获取延迟拴绳信息
     *
     * 从 NBT 加载拴绳数据时，目标实体可能尚未加载到世界中。
     * 此信息在实体 tick 时用于尝试重新绑定拴绳。
     *
     * @return 延迟拴绳信息的引用
     */
    [[nodiscard]] LeashDelayInfo& leashDelayInfo() { return m_leashDelayInfo; }
    [[nodiscard]] const LeashDelayInfo& leashDelayInfo() const { return m_leashDelayInfo; }

    /**
     * @brief 设置拴绳绑定到实体
     *
     * @param holderUuid 目标实体的 UUID
     */
    void setLeashedToEntity(const std::string& holderUuid);

    /**
     * @brief 设置拴绳绑定到栅栏柱
     *
     * @param pos 栅栏柱坐标
     */
    void setLeashedToFence(const BlockPos& pos);

    /**
     * @brief 解除拴绳绑定（不掉落物品）
     *
     * 清除拴绳状态但不掉落拴绳物品。
     * 用于创造模式或拴绳因距离断裂等情况。
     */
    void clearLeash();

    /**
     * @brief 解除拴绳绑定并掉落拴绳物品
     *
     * 清除拴绳状态并在实体位置掉落一个拴绳物品。
     * 用于生存模式下玩家主动解拴或拴绳因距离断裂。
     * 如果 doEntityDrops 游戏规则为 false，则不掉落物品。
     */
    void dropLeash();

    /**
     * @brief 实现 tickLeash 拴绳物理
     *
     * 每tick执行拴绳物理约束：
     * - 恢复延迟加载的拴绳数据
     * - 检查拴绳双方是否存活且可交互
     * - 距离超过断裂距离时断裂
     * - 距离超过弹性距离时施加拉力
     * - 近距离时让实体跟随拴绳持有者
     */
    void tickLeash();

    // ========== 持久化系统 (Persistence) ==========

    /**
     * @brief 检查是否需要持久化（不会消失）
     *
     * 当生物被命名牌命名、拾取装备等情况时，会被标记为持久化，
     * 永远不会因为距离过远而消失。
     *
     * @return 如果实体需要持久化返回 true
     */
    [[nodiscard]] bool isNoDespawnRequired() const { return m_persistenceRequired; }

    /**
     * @brief 启用持久化
     *
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
     * MonsterEntity 重写为 true。
     *
     * @return 如果在和平模式下应消失返回 true
     */
    [[nodiscard]] virtual bool isDespawnPeaceful() const { return false; }

    // ========== 日光检测与亡灵燃烧 ==========

    /**
     * @brief 检查是否暴露在日光下
     *
     * 用于亡灵生物燃烧判断。
     *
     * 检查条件：
     * 1. 不在客户端
     * 2. 世界为白天 (dayTime < 12000)
     * 3. 亮度 > 0.5
     * 4. 随机检查（亮度越高概率越大）
     * 5. 不在水中或雨中
     * 6. 天空可见 (canSeeSky)
     *
     * @return 如果暴露在日光下返回 true
     */
    [[nodiscard]] bool isInDaylight() const;

    /**
     * @brief 获取阳光防护装备槽位
     *
     * 当亡灵生物暴露在阳光下时，此槽位中的物品会替代实体承受耐久损耗。
     * 默认为头部槽位（头盔），僵尸马等覆写为身体槽位。
     *
     * @return 阳光防护装备槽位
     */
    [[nodiscard]] virtual EquipmentSlot sunProtectionSlot() const { return EquipmentSlot::Head; }

    /**
     * @brief 处理亡灵生物在阳光下的燃烧逻辑
     *
     * 当亡灵生物暴露在阳光下时：
     * - 如果防护槽位有可损坏物品，则物品承受耐久损耗而非实体燃烧
     * - 如果防护槽位为空，则实体被点燃 8 秒
     *
     * 物品耐久损耗直接通过 setDamage() 增加，绕过耐久保护附魔，
     * 与 MC 原版 burnUndead() 行为一致。
     */
    void burnUndead();

    // ========== 攻击目标类型判断 ==========

    /**
     * @brief 检查是否可以攻击指定类型的实体
     *
     * 对应 Minecraft 原版的 Mob.canAttackType()。
     * 在目标选择器的 isSuitableTarget 中自动调用，
     * 用于在目标类型层面进行攻击可行性过滤。
     *
     * 基类默认实现排除恶魂（GHAST），因为恶魂悬浮在高空，
     * 大多数近战型 Mob 无法接近恶魂，排除恶魂可以避免 Mob
     * 徒劳地试图攻击一个它们够不着的敌人。
     *
     * 子类可重写以限制攻击目标类型，例如：
     * - IronGolem: 不攻击苦力怕，玩家创建的不攻击玩家
     * - Phantom: 重新允许攻击恶魂（幻翼本身会飞行）
     * - Breeze: 只能攻击玩家和铁傀儡（白名单模式）
     *
     * @param type 目标实体类型
     * @return 是否可以攻击该类型的实体
     */
    [[nodiscard]] virtual bool canAttackType(const entity::EntityType& type) const;

    // ========== 攻击 ==========

    /**
     * @brief 检查实体是否可以使用非近战武器
     *
     * 当实体手持指定物品时，返回 true 表示该物品被视为远程武器，
     * 近战攻击任务将跳过攻击以允许远程攻击任务接管。
     *
     * 默认实现返回 false（大多数生物不使用远程武器）。
     * 骷髅类实体重写此方法以检查弓等远程武器。
     *
     * 对应 MC 原版 Mob.canUseNonMeleeWeapon(ItemStack)。
     *
     * @param stack 要检查的物品堆
     * @return 如果该物品是远程武器且应使用远程攻击则返回 true
     */
    [[nodiscard]] virtual bool canUseNonMeleeWeapon(const ItemStack& stack) const
    {
        (void)stack;
        return false;
    }

    /**
     * @brief 作为生物攻击实体
     *
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

    /**
     * @brief 掉落保留装备并返回保留的槽位集合
     *
     * 遍历所有装备槽位，根据掉落概率和谓词条件处理装备：
     * - 如果装备为空，跳过
     * - 如果装备不满足谓词条件（如绑定诅咒），保留在实体上并记录槽位
     * - 如果装备满足谓词条件且槽位为保留状态（掉落概率 > 1.0），在实体位置掉落该物品
     * - 如果装备满足谓词条件但非保留状态，物品静默消失
     *
     * 对应 MC Java 的 Mob.dropPreservedEquipment(ServerLevel, Predicate<ItemStack>)。
     * 用于实体转化场景（如僵尸村民治愈），控制哪些装备被保留、掉落或丢弃。
     *
     * @param predicate 谓词函数，返回 true 表示物品可以被处理（掉落或丢弃），
     *                  返回 false 表示物品应保留在实体上（如绑定诅咒物品）
     * @return 保留在实体上的装备槽位集合（谓词返回 false 的槽位）
     */
    [[nodiscard]] std::vector<EquipmentSlot> dropPreservedEquipment(
        const std::function<bool(const ItemStack&)>& predicate);

    /**
     * @brief 掉落所有保留装备（无谓词过滤版本）
     *
     * 等效于 dropPreservedEquipment([](const ItemStack&) { return true; })。
     * 所有装备都参与掉落/保留判断，仅根据掉落概率决定行为。
     *
     * @return 保留在实体上的装备槽位集合（此版本始终为空，因为谓词总是返回 true）
     */
    [[nodiscard]] std::vector<EquipmentSlot> dropPreservedEquipment();

    // ========== 玩家交互 ==========

    /**
     * @brief 处理玩家初始交互
     *
     * 重写 Entity::processInitialInteract() 以处理生物特有交互：
     * 1. 命名牌交互：如果玩家手持已命名的命名牌，设置实体的自定义名称
     * 2. 刷怪蛋交互：如果玩家手持与当前实体类型相同的刷怪蛋，生成幼体
     * 3. 拴绳交互：如果玩家手持拴绳
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
     * 由 processInitialInteract() 调用，让子类处理特定交互。
     * 例如：AbstractHorseEntity 在此处理喂食、装备鞍等。
     *
     * @param player 与此实体交互的玩家
     * @param hand 玩家使用的手
     * @return 交互结果类型
     */
    [[nodiscard]] virtual ActionResultType interactMob(Player& player, Hand hand);

    /**
     * @brief 检查此生物是否可以被拴绳拴住
     *
     * 敌对生物（实现了 IMob 接口）不能被拴住。
     * 子类可重写此方法以自定义拴绳行为。
     *
     * @return 如果此生物可以被拴住返回 true
     */
    [[nodiscard]] virtual bool canBeLeashed() const;

    // ========== NBT 序列化 ==========

    /**
     * @brief 序列化 MobEntity 特有数据
     *
     * 写入 CanPickUpLoot, PersistenceRequired, LeftHanded, NoAI 等。
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 反序列化 MobEntity 特有数据
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

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
    bool m_aggroed = false; // 激怒状态
    i32 m_idleTime = 0;     // 空闲时间（用于随机漫步等）
    i32 m_livingSoundTime = 0;

    // 经验值（死亡时掉落）
    i32 m_experienceValue = 0;

    // 家范围系统
    BlockPos m_homePosition;           // 家位置，默认为 (0, 0, 0)
    f32 m_maximumHomeDistance = -1.0f; // 家范围半径，-1 表示未设置

    // 持久化系统
    bool m_persistenceRequired = false; // 是否需要持久化（不消失）

    // 拾取物品
    bool m_canPickUpLoot = false; // 是否可以拾取物品

    // 装备掉落概率
    // 索引与 EquipmentSlot 枚举值对应：
    //   [0] = MainHand, [1] = OffHand, [2] = Feet, [3] = Legs, [4] = Chest, [5] = Head
    // 默认值为 0.085f (DEFAULT_EQUIPMENT_DROP_CHANCE)
    // 大于 1.0 的值表示物品被保留（总是掉落，PRESERVE_ITEM_DROP_CHANCE = 2.0）
    std::array<f32, static_cast<size_t>(EquipmentSlot::Count)> m_equipmentDropChances = {};

    // 死亡掉落表
    // 当 m_deathLootTable 有值时，使用自定义掉落表替代实体类型的默认掉落表
    // 掉落表使用后会被清空（只掉落一次）
    std::optional<std::string> m_deathLootTable; // 掉落表 ID，格式如 "minecraft:entities/zombie"
    i64 m_lootTableSeed = 0;                     // 掉落表种子，0 表示随机

    // 拴绳系统
    bool m_isLeashed = false;                     // 是否被拴绳拴住
    std::optional<std::string> m_leashHolderUuid; // 拴绳目标实体的 UUID（拴在实体上时）
    std::optional<BlockPos> m_leashFencePos;      // 拴绳目标栅栏柱坐标（拴在栅栏上时）
    LeashDelayInfo m_leashDelayInfo;              // 延迟绑定信息（NBT 加载后尚未解析时使用）

    // 寻路惩罚值表（按 PathNodeType 索引，NaN 表示未设置，回退到默认惩罚值）
    // 对应 MC Java Mob.pathfindingMalus: EnumMap<PathType, Float>
    std::array<f32, entity::ai::pathfinding::pathNodeTypeCount()> m_pathfindingMalus{};

    // ========== 装备附魔辅助方法 ==========

    /**
     * @brief 使用刷怪蛋在当前实体位置生成幼体
     *
     * 当玩家对生物右键使用刷怪蛋时调用。如果刷怪蛋的实体类型
     * 与当前实体类型匹配，则生成幼体；否则不生成。
     *
     * @param player 使用刷怪蛋的玩家
     * @param spawnEgg 刷怪蛋物品
     * @param heldItem 玩家手持的物品堆
     * @return 是否成功生成了幼体
     */
    bool _spawnOffspringFromSpawnEgg(Player& player, const item::SpawnEggItem& spawnEgg, ItemStack& heldItem);

    /**
     * @brief 附魔生成的武器
     *
     * 概率 = 0.25 * specialMultiplier，附魔等级范围 5~17
     *
     * @param random 随机数生成器
     * @param difficulty 区域难度实例
     * @param specialMultiplier 区域难度特殊乘数
     */
    void enchantSpawnedWeapon(
        math::Random& random, const entity::combat::DifficultyInstance& difficulty, f32 specialMultiplier);

    /**
     * @brief 附魔生成的护甲
     *
     * 每个护甲槽位独立检定，概率 = 0.5 * specialMultiplier
     *
     * @param random 随机数生成器
     * @param difficulty 区域难度实例
     * @param specialMultiplier 区域难度特殊乘数
     */
    void enchantSpawnedArmor(
        math::Random& random, const entity::combat::DifficultyInstance& difficulty, f32 specialMultiplier);

    // ========== 同步数据参数 ==========

    /**
     * @brief Mob 标志位同步参数
     *
     * 对应 MC 1.21.11 Mob.DATA_MOB_FLAGS_ID。
     * 存储位标志：bit 2 (0x04) = aggressive（激怒/攻击中状态）。
     * 由 setAggressive 写入，由 EntityTracker 自动广播到所有观察者客户端，
     * 客户端 ClientEntity::syncMetadataFromDataManager 读取后驱动
     * ZombieModel/SkeletonModel 的空手攻击抬臂动画。
     */
    static entity::DataParameter<i8> DATA_MOB_FLAGS_PARAM;

    /// 本类继承链标识（parent = LivingEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    // Mob 标志位定义（对应 MC 1.21.11 Mob 的 MOB_FLAG_* 常量）
    static constexpr i8 MOB_FLAG_NO_AI = 1;      // bit 0
    static constexpr i8 MOB_FLAG_LEFTHANDED = 2; // bit 1
    static constexpr i8 MOB_FLAG_AGGRESSIVE = 4; // bit 2
};

} // namespace mc
