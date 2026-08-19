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

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "../MonsterEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"

namespace mc {

// Forward declarations
class Item;

namespace entity {
class ArrowEntity;
}

/**
 * @brief 骷髅系怪物公共中间层
 *
 * 集中承载：
 * - 远程弓箭攻击接口
 * - 拉弓状态与攻击计时
 * - 骷髅系共通属性与基础目标注册
 * - 动态战斗目标切换（setCombatTask 模式）
 *
 * 子类：
 * - SkeletonEntity: 普通骷髅，使用弓远程攻击
 * - StrayEntity: 流浪者，使用弓远程攻击，射出迟缓之箭，作为亡灵会在阳光下燃烧
 * - WitherSkeletonEntity: 凋灵骷髅，使用石剑近战攻击，施加凋零效果，不在阳光下燃烧
 *
 * 关键设计：
 * - registerGoals() 只注册非战斗目标（移动、看向、目标选择）
 * - setCombatTask() 根据装备动态选择战斗目标（远程/近战）
 * - 子类通过装备不同武器来影响 setCombatTask() 的选择
 */
class AbstractSkeletonEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    ~AbstractSkeletonEntity() override;

    AbstractSkeletonEntity(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity& operator=(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity(AbstractSkeletonEntity&&) = delete;
    AbstractSkeletonEntity& operator=(AbstractSkeletonEntity&&) = delete;

    // 亡灵生物归类（对齐 Java AbstractSkeleton.getMobType()==UNDEAD）。基类默认 Undefined，未覆写会导致
    // 骷髅在水中溺水死亡、亡灵杀手附魔无加成、瞬间治疗/伤害药水不反转、凋灵玫瑰不免疫、白天燃烧判定
    // 等亡灵特性失效。子类 SkeletonEntity/StrayEntity/WitherSkeletonEntity/BoggedEntity 继承此覆写。
    [[nodiscard]] CreatureAttribute getCreatureAttribute() const override { return CreatureAttribute::Undead; }

    // ========== 常量 ==========
    /// 这些常量在测试中需要访问

    static constexpr i32 ATTACK_COOLDOWN = 60;      // 攻击冷却（ticks），3秒
    static constexpr f32 ARROW_DAMAGE = 2.0f;       // 箭矢基础伤害
    static constexpr f64 RANGED_ATTACK_SPEED = 1.0; // 远程攻击移动速度
    static constexpr f64 MELEE_ATTACK_SPEED = 1.2;  // 近战攻击移动速度
    static constexpr i32 ATTACK_INTERVAL_MIN = 20;  // 最小攻击间隔（ticks）
    static constexpr i32 ATTACK_INTERVAL_MAX = 40;  // 最大攻击间隔（ticks）
    static constexpr f32 ATTACK_RADIUS = 15.0f;     // 远程攻击半径

    /// 困难难度下的最小攻击间隔（ticks），对应 MC 原版 HARD_ATTACK_INTERVAL
    static constexpr i32 HARD_ATTACK_INTERVAL = 20;
    /// 非困难难度下的最小攻击间隔（ticks），对应 MC 原版 NORMAL_ATTACK_INTERVAL
    static constexpr i32 NORMAL_ATTACK_INTERVAL = 40;
    /// 增大型困难难度最小攻击间隔（ticks），用于射击更慢的骷髅变种
    /// 对应 MC 原版 INCREASED_HARD_ATTACK_INTERVAL
    static constexpr i32 INCREASED_HARD_ATTACK_INTERVAL = 50;
    /// 增大型非困难难度最小攻击间隔（ticks），用于射击更慢的骷髅变种
    /// 对应 MC 原版 INCREASED_NORMAL_ATTACK_INTERVAL
    static constexpr i32 INCREASED_NORMAL_ATTACK_INTERVAL = 70;

    /// 战斗目标优先级
    static constexpr i32 COMBAT_GOAL_PRIORITY = 4;

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // ========== 弓箭状态管理 ==========

    /**
     * @brief 获取是否正在拉弓
     *
     * 通过 DataParameter 同步到客户端，由 ClientEntity::syncMetadataFromDataManager
     * 读取并写入 ClientEntity::m_chargingBow 镜像字段。
     * 渲染层据此设置 SkeletonModel 的 ArmPose::BowAndArrow，触发拉弓动画。
     */
    [[nodiscard]] bool isChargingBow() const { return m_dataManager.get<bool>(DATA_CHARGING_BOW_PARAM); }

    /**
     * @brief 设置拉弓状态
     *
     * 写入 DataParameter，由 EntityTracker 自动广播到所有观察者客户端。
     * 由 AbstractSkeletonEntity::tick 根据 m_attackTimer 推进设置，
     * 由 attackEntityWithRangedAttack 在射击时重置为 false。
     */
    void setChargingBow(bool charging) { m_dataManager.set(DATA_CHARGING_BOW_PARAM, charging); }

    /**
     * @brief 获取拉弓状态参数 ID（供客户端 syncMetadataFromDataManager 使用）
     */
    [[nodiscard]] static u16 getChargingBowParamId() { return DATA_CHARGING_BOW_PARAM.id(); }

    [[nodiscard]] i32 getAttackTimer() const { return m_attackTimer; }
    void setAttackTimer(i32 timer) { m_attackTimer = timer; }

    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(i32 cooldown) { m_attackCooldown = cooldown; }

    // ========== 战斗目标管理 ==========

    /**
     * @brief 重新评估战斗目标
     *
     * 根据装备动态选择战斗目标：
     * - 如果持有弓，使用 RangedBowAttackGoal
     * - 否则使用 MeleeAttackGoal
     *
     * 此方法会先移除所有战斗目标，再根据装备添加正确的目标。
     * 在以下时机被调用：
     * - 构造函数末尾
     * - finalizeSpawn() 中
     * - 装备变更时（setEquipment 触发）
     * - NBT 加载后（addAdditionalSaveData / readAdditionalSaveData）
     */
    virtual void setCombatTask();

    /**
     * @brief 检查实体是否可以使用非近战武器
     *
     * 当实体手持指定物品时，返回 true 表示该物品被视为远程武器。
     * 默认实现检查物品是否为弓。
     * 凋灵骷髅重写此方法返回 false，因为它不使用远程攻击。
     *
     * 对应 MC 原版 AbstractSkeleton.canUseNonMeleeWeapon()。
     *
     * @param stack 要检查的物品堆
     * @return 如果该物品是远程武器则返回 true
     */
    [[nodiscard]] bool canUseNonMeleeWeapon(const ItemStack& stack) const override;

    /**
     * @brief 获取困难难度下的最小攻击间隔
     *
     * 子类可重写此方法以提供不同的攻击间隔（如沼骸骷髅射击更慢）。
     * 默认实现返回 HARD_ATTACK_INTERVAL (20 ticks)。
     *
     * 对应 MC 原版 AbstractSkeleton.getHardAttackInterval()。
     *
     * @return 困难难度下的最小攻击间隔（ticks）
     */
    [[nodiscard]] virtual i32 getHardAttackInterval() const { return HARD_ATTACK_INTERVAL; }

    /**
     * @brief 获取非困难难度下的最小攻击间隔
     *
     * 子类可重写此方法以提供不同的攻击间隔（如沼骸骷髅射击更慢）。
     * 默认实现返回 NORMAL_ATTACK_INTERVAL (40 ticks)。
     *
     * 对应 MC 原版 AbstractSkeleton.getAttackInterval()。
     * 同时重写 IRangedAttackMob::getAttackInterval()（默认返回 20 ticks）。
     *
     * @return 非困难难度下的最小攻击间隔（ticks）
     */
    [[nodiscard]] i32 getAttackInterval() const override { return NORMAL_ATTACK_INTERVAL; }

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 完成骷髅的生成初始化
     *
     * 重写 MobEntity::finalizeSpawn() 以实现骷髅特有的初始化：
     * - 填充默认装备和附魔
     * - 重新评估战斗目标（远程/近战）
     * - 设置拾取物品能力
     * - 万圣节南瓜头（10月31日，25% 概率）
     *
     * @param world 世界引用
     * @param difficulty 区域难度实例
     * @param spawnReason 生成原因
     */
    void finalizeSpawn(mc::IWorld& world,
        const mc::entity::combat::DifficultyInstance& difficulty,
        mc::world::spawn::SpawnReason spawnReason) override;

protected:
    AbstractSkeletonEntity(EntityInstanceId id, ecs::EntityRegistry& registry);

    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 填充默认装备（主手弓）
     *
     * 对应 MC 原版 AbstractSkeleton.populateDefaultEquipmentSlots()：骷髅系怪物
     * 在非和平难度下主手默认持弓（普通骷髅/流浪者远程攻击，凋零骷髅 override 用石剑）。
     * 持弓是 setCombatTask() 判定 shouldUseRanged=true 注册 RangedBowAttackGoal 的前提——
     * 不持弓则退化用 MeleeAttackGoal 近战，与原版骷髅/流浪者远程攻击行为不符。
     * 盔甲由基类 MobEntity::populateDefaultEquipmentSlots 按难度概率填充，此处只补主手弓。
     *
     * @param random 随机源
     * @param difficulty 区域难度实例
     */
    void populateDefaultEquipmentSlots(
        math::Random& random, const entity::combat::DifficultyInstance& difficulty) override;

    /**
     * @brief 定制射出箭矢的钩子（在箭矢发射前调用）
     *
     * attackEntityWithRangedAttack 创建并发射箭矢前调用此钩子，子类可重写以
     * 为箭矢附加药水效果等定制内容。默认实现为空（普通骷髅射普通箭矢）。
     *
     * 流浪者（StrayEntity）重写此方法为箭矢附加缓慢效果，使射出的箭命中目标时
     * 施加 30 秒缓慢 I（对应原版流浪者发射 Arrow of Slowness）。
     *
     * 对应 MC 原版 AbstractSkeleton.getArrow() 子类定制箭矢的扩展点。
     *
     * @param arrow 即将发射的箭矢实体（已设置位置/方向/伤害，尚未 spawn）
     */
    virtual void customizeArrow(entity::ArrowEntity& arrow) {}

    /**
     * @brief 注册同步数据参数
     *
     * 重写 MonsterEntity::registerData，注册 DATA_CHARGING_BOW_PARAM。
     * 由于 C++ 虚函数在基类构造函数中不会派发到派生类，
     * 派生类构造函数必须显式调用 registerData()，参考 WolfEntity 模式。
     */
    void registerData() override;

    /**
     * @brief 装备变更回调
     *
     * 当装备槽位发生变化时，重新评估战斗目标（远程/近战切换）。
     * 对应 MC 原版 AbstractSkeleton.onEquipItem() 中的 reassessWeaponGoal() 调用。
     */
    void setEquipment(EquipmentSlot slot, const ItemStack& stack) override;

    // ========== 弓箭状态 ==========

    i32 m_attackTimer = 0;
    i32 m_attackCooldown = 0;

    /**
     * @brief 拉弓状态同步参数
     *
     * 对应 MC 1.21.11 AbstractSkeleton.DATA_CHARGING_BOW（虽原版骷髅走 isAggressive
     * + isHoldingBow 渲染状态，本项目简化为单独的 chargingBow 布尔字段）。
     * 由 setChargingBow 写入，由 EntityTracker 自动广播到所有观察者客户端。
     * 客户端 ClientEntity::syncMetadataFromDataManager 读取此参数并调用
     * ClientEntity::setChargingBow，驱动 SkeletonModel 的 BowAndArrow 姿态。
     */
    static entity::DataParameter<bool> DATA_CHARGING_BOW_PARAM;

    /// 本类继承链标识（parent = MonsterEntity::classInfo()）。见 Entity::classInfo()。
    static const entity::EntityClassInfo& classInfo();

    // ========== 战斗目标 ==========
    // 注意：战斗目标的唯一所有权归 GoalSelector 所有，不再使用 unique_ptr 成员存储。
    // setCombatTask() 每次调用时创建新的 Goal 对象并转移所有权给 GoalSelector，
    // 通过 removeGoalsOfType() 移除旧目标。这避免了 unique_ptr 和 GoalSelector 之间的所有权冲突。
};

} // namespace mc
