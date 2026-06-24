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
#include "../combat/DifficultyInstance.hpp"
#include "EntitySpawnPlacementRegistry.hpp"
#include "LivingEntity.hpp"
#include <memory>
#include <optional>
#include <variant>

namespace mc {

// 前向声明
class Player;
class Item;

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
     */
    MobEntity(EntityId id);

    ~MobEntity() override;

    // 禁止拷贝
    MobEntity(const MobEntity&) = delete;
    MobEntity& operator=(const MobEntity&) = delete;

    // 禁止移动（基类 LivingEntity 不可移动）
    MobEntity(MobEntity&&) = delete;
    MobEntity& operator=(MobEntity&&) = delete;

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
    [[nodiscard]] bool isAggroed() const { return m_aggroed; }

    /**
     * @brief 设置激怒状态
     * 在攻击目标时设置
     */
    void setAggroed(bool aggroed) { m_aggroed = aggroed; }

    // ========== 刻更新 ==========

    void tick() override;

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
     * @param canPickUp 是否可以拾取物品
     */
    void setCanPickUpLoot(bool canPickUp) { m_canPickUpLoot = canPickUp; }

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

    // ========== 掉落表 (DeathLootTable) ==========

    /**
     * @brief 获取实体的战利品表ID
     *
     * 优先使用 NBT 中设置的自定义掉落表（m_deathLootTable），
     * 如果没有自定义掉落表则回退到实体类型的默认掉落表路径。
     * 对齐 MC Java 中 Mob.getLootTable() 的逻辑：
     * this.lootTable.isPresent() ? this.lootTable : super.getLootTable()
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
     * @brief 解除拴绳绑定
     */
    void clearLeash();

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
     * @param typeId 目标实体类型ID
     * @return 是否可以攻击该类型的实体
     */
    [[nodiscard]] virtual bool canAttackType(entity::EntityTypeId typeId) const;

    // ========== 攻击 ==========

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

    // ========== 玩家交互 ==========

    /**
     * @brief 处理玩家初始交互
     *
     * 重写 Entity::processInitialInteract() 以处理生物特有交互：
     * 1. 命名牌交互：如果玩家手持已命名的命名牌，设置实体的自定义名称
     * 2. 刷怪蛋交互：如果玩家手持与当前实体类型相同的刷怪蛋，生成幼体
     * 3. 拴绳交互：如果玩家手持拴绳（TODO: 待 Leashable 接口完善后实现）
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
};

} // namespace mc
