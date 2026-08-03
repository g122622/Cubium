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

#include "AbstractSkeletonEntity.hpp"

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/FleeSunGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/RestrictSunGoal.hpp"
#include "common/entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/AbstractArrowEntity.hpp"
#include "common/entity/entities/projectile/ProjectileHelper.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/SpecialDates.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <cmath>
#include <memory>
#include <utility>

namespace mc {

// ==================== 静态成员初始化 ====================
entity::DataParameter<bool> AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM =
    entity::EntityDataManager::createKey<bool>();

const entity::EntityClassInfo& AbstractSkeletonEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"AbstractSkeletonEntity", &MonsterEntity::classInfo()};
    return s_classInfo;
}

AbstractSkeletonEntity::AbstractSkeletonEntity(EntityInstanceId id)
    : MonsterEntity(id)
{
    // 战斗目标不再在构造函数中创建，而是在 setCombatTask() 中按需创建。
    // setCombatTask() 会在 registerGoals() 之后（构造函数末尾）或
    // finalizeSpawn() / setEquipment() 时被调用。

    // 显式调用 registerData() 注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类（Entity::Entity 内部调用
    // registerData() 时调用的是 Entity::registerData 而非
    // AbstractSkeletonEntity::registerData），必须在派生类构造函数中显式调用，
    // 参考 WolfEntity 模式。
    registerData();
}

AbstractSkeletonEntity::~AbstractSkeletonEntity() = default;

void AbstractSkeletonEntity::registerData()
{
    // 先调用父类方法，确保基类数据参数已注册
    MonsterEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册拉弓状态数据参数，用于客户端-服务端同步
    // 默认值为 false（未拉弓），由 setChargingBow 写入，
    // 由 tick 根据 m_attackTimer 推进，由 attackEntityWithRangedAttack 重置。
    m_dataManager.registerParam(DATA_CHARGING_BOW_PARAM, false);
}

void AbstractSkeletonEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    if (target == nullptr || world() == nullptr) {
        return;
    }

    // 重置弓箭状态
    setChargingBow(false);
    m_attackTimer = 0;
    m_attackCooldown = ATTACK_COOLDOWN;

    // 创建箭矢实体
    auto arrow = entity::ArrowEntity::createFromShooter(*this, world());
    if (arrow == nullptr) {
        return;
    }

    // 计算射击方向
    f64 dx = target->x() - x();
    f64 dy = target->getY(0.3333333333333333) - arrow->y();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 不精确度：难度越高，不精确度越低，箭矢越精准
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(world()->difficulty());

    // 使用生物箭矢伤害公式设置基础伤害
    arrow->setBaseDamageFromMob(charge);

    // 发射箭矢：速度固定为 1.6F，Y轴补偿 horizontalDist * 0.2 用于抛物线弹道
    constexpr f32 ARROW_VELOCITY = 1.6f;
    arrow->shoot(static_cast<f32>(dx),
        static_cast<f32>(dy + horizontalDist * 0.2),
        static_cast<f32>(dz),
        ARROW_VELOCITY,
        inaccuracy);

    // 播放射箭音效
    math::Random& rng = getRandom();
    f32 pitch = 1.0f / (rng.nextFloat() * 0.4f + 0.8f);
    playSound(SoundEvents::ENTITY_SKELETON_SHOOT, 1.0f, pitch);

    // 将箭矢添加到世界
    world()->spawnEntity(std::move(arrow));
}

void AbstractSkeletonEntity::tick()
{
    MonsterEntity::tick();

    if (m_attackCooldown > 0) {
        --m_attackCooldown;
    }

    // 拉弓状态同步：当正在使用物品（弓蓄力）且主手为弓时，设为拉弓状态。
    // 对应 MC 1.21.11 AbstractSkeletonRenderer 通过 isUsingItem + BOW 物品判断拉弓。
    // 通过 DataParameter setter 写入，触发 EntityTracker 自动广播到客户端，
    // 客户端 ClientEntity::syncMetadataFromDataManager 读取并设置 m_chargingBow，
    // 驱动 SkeletonModel 的 BowAndArrow 姿态。
    // 仅在服务端执行（客户端走 ClientEntity::tick，不会调用此方法）。
    const bool wasCharging = isChargingBow();
    const bool nowCharging = isUsingItem() && getMainHandItem().getItem() == Items::BOW;
    if (wasCharging != nowCharging) {
        setChargingBow(nowCharging);
    }

    if (m_attackTimer > 0) {
        --m_attackTimer;
        if (m_attackTimer == 0) {
            setChargingBow(false);
        }
    }
}

void AbstractSkeletonEntity::setCombatTask()
{
    // 对应 MC 原版 AbstractSkeleton.reassessWeaponGoal()

    // 先移除所有现有的战斗目标，再根据装备添加正确的目标。
    // 使用 removeGoalsOfType 按类型移除，避免 unique_ptr 与 GoalSelector 之间的所有权冲突：
    // GoalSelector 拥有 Goal 的所有权，removeGoalsOfType 会正确销毁旧目标并释放内存。
    m_goalSelector.removeGoalsOfType<entity::ai::goal::RangedBowAttackGoal>();
    m_goalSelector.removeGoalsOfType<entity::ai::goal::MeleeAttackGoal>();

    // 检查主手/副手是否持有弓
    // 对应 MC 原版 AbstractSkeleton.reassessWeaponGoal()
    // 安全检查：构造阶段或客户端不执行装备检查逻辑
    bool shouldUseRanged = true; // 默认使用远程攻击（普通骷髅和流浪者默认持弓）
    if (world() != nullptr && !world()->isClientSide()) {
        Hand weaponHand = getWeaponHoldingHand(*this, Items::BOW);
        const ItemStack& weaponStack = getEquipment(LivingEntity::handToEquipmentSlot(weaponHand));
        const Item* weaponItem = weaponStack.getItem();
        shouldUseRanged = (weaponItem != nullptr && canUseNonMeleeWeapon(weaponStack));
    }

    if (shouldUseRanged) {
        // 持弓 -> 注册远程攻击目标（创建新实例并转移所有权给 GoalSelector）
        // 对应 MC 原版 AbstractSkeleton.reassessWeaponGoal()：
        //   根据当前游戏难度调整最小攻击间隔
        //   - 困难难度: 使用 getHardAttackInterval()（普通骷髅 20 ticks）
        //   - 其他难度: 使用 getAttackInterval()（普通骷髅 40 ticks）
        i32 minAttackInterval = getHardAttackInterval();
        if (world() != nullptr && world()->difficulty() != Difficulty::Hard) {
            minAttackInterval = getAttackInterval();
        }

        auto bowGoal = std::make_unique<entity::ai::goal::RangedBowAttackGoal>(
            this, RANGED_ATTACK_SPEED, ATTACK_INTERVAL_MIN, ATTACK_INTERVAL_MAX);
        bowGoal->setMinAttackInterval(minAttackInterval);
        m_goalSelector.addGoal(COMBAT_GOAL_PRIORITY, std::move(bowGoal));
    } else {
        // 不持弓 -> 注册近战攻击目标（创建新实例并转移所有权给 GoalSelector）
        m_goalSelector.addGoal(
            COMBAT_GOAL_PRIORITY, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, MELEE_ATTACK_SPEED, false));
    }
}

bool AbstractSkeletonEntity::canUseNonMeleeWeapon(const ItemStack& stack) const
{
    // 默认实现：检查物品的 UseAction 是否为 Bow
    // 对应 MC 原版 AbstractSkeleton.canUseNonMeleeWeapon()
    const Item* item = stack.getItem();
    return item != nullptr && item->getUseAction(stack) == UseAction::Bow;
}

void AbstractSkeletonEntity::setEquipment(EquipmentSlot slot, const ItemStack& stack)
{
    // 先调用基类实现设置装备
    MonsterEntity::setEquipment(slot, stack);

    // 装备变更时重新评估战斗目标
    // 对应 MC 原版 AbstractSkeleton.onEquipItem() 中的 reassessWeaponGoal() 调用
    // 仅在主手/副手装备变更时触发，且仅在服务端执行
    if ((slot == EquipmentSlot::MainHand || slot == EquipmentSlot::OffHand) && world() != nullptr &&
        !world()->isClientSide()) {
        setCombatTask();
    }
}

void AbstractSkeletonEntity::registerGoals()
{
    MonsterEntity::registerGoals();

    // 注意：战斗目标（远程/近战）通过 setCombatTask() 添加，不在这里注册
    // 子类（如 WitherSkeletonEntity）可以重写 setCombatTask() 来选择近战

    // ========== 行为目标 (goalSelector) ==========

    // 优先级 2: 限制阳光（不在阳光下移动）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RestrictSunGoal>(this));

    // 优先级 3: 躲避阳光
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::FleeSunGoal>(this, 1.0));

    // 优先级 3: 躲避狼（6格检测距离，1.0远距离速度，1.2近距离速度）
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(
            this, 6.0f, 1.0, 1.2, [](const LivingEntity* entity) -> bool {
                if (!entity) return false;
                return entity->entityType() == entity::VanillaEntityTypeKeys::WOLF;
            }));

    // 优先级 4: 战斗目标（通过 setCombatTask() 动态添加）
    // 参考 setCombatTask()

    // 优先级 5: 避水随机行走
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::WaterAvoidingRandomWalkingGoal>(this, 1.0));

    // 优先级 6: 看向玩家
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 优先级 6: 随机看向
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::LookRandomlyGoal>(this));

    // ========== 目标选择器 (targetSelector) ==========

    // 优先级 1: 被攻击后反击
    // MC 原版: targetSelector.addGoal(1, HurtByTargetGoal(this))
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this));

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true));

    // 优先级 3: 攻击铁傀儡
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));

    // 优先级 3: 攻击幼年海龟（陆地上不在水中的幼体，10 tick 间隔检查）
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<TurtleEntity>>(this,
            true, // checkSight
            10,   // reciprocalChance — 每 10 tick 检查一次
            [](const LivingEntity* entity) -> bool {
                // BABY_ON_LAND_SELECTOR: 只攻击陆地上不在水中的幼年海龟
                const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
                if (!turtle) return false;
                return turtle->isChild() && !turtle->isInWater();
            }));
}

void AbstractSkeletonEntity::registerAttributes()
{
    MonsterEntity::registerAttributes();

    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, ARROW_DAMAGE);
}

void AbstractSkeletonEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    MonsterEntity::finalizeSpawn(world, difficulty, spawnReason);

    math::Random& rng = getRandom();

    // 重新评估战斗目标（远程/近战）
    setCombatTask();

    // 设置拾取物品能力
    setCanPickUpLoot(rng.nextFloat() < 0.55f * difficulty.getSpecialMultiplier());

    // 万圣节南瓜头：10月31日，25% 概率
    if (util::SpecialDates::isHalloween() && rng.nextFloat() < 0.25f) {
        if (getEquipment(EquipmentSlot::Head).isEmpty()) {
            const Item* pumpkinItem = rng.nextFloat() < 0.1f ? Items::JACK_O_LANTERN : Items::CARVED_PUMPKIN;
            if (pumpkinItem != nullptr) {
                setEquipment(EquipmentSlot::Head, ItemStack(*pumpkinItem, 1));
                setEquipmentDropChance(EquipmentSlot::Head, 0.0f);
            }
        }
    }
}

} // namespace mc
