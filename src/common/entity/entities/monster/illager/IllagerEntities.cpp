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

#include "IllagerEntities.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/entities/monster/illager/AbstractIllagerEntity.hpp"
#include "common/entity/entities/monster/illager/AbstractRaiderEntity.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "entity/ai/goal/goals/SwimGoal.hpp"
#include "entity/ai/goal/goals/attack/RangedAttackGoals.hpp"
#include "entity/ai/goal/goals/interact/BreakDoorGoal.hpp"
#include "entity/ai/goal/goals/interact/RaiderOpenDoorGoal.hpp"
#include "entity/ai/goal/goals/target/TargetGoals.hpp"
#include "entity/ai/pathfinding/PathNavigator.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/combat/DifficultyHelper.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/entities/passive/golem/IronGolemEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/entities/projectile/AbstractArrowEntity.hpp"
#include "entity/entities/villager/AbstractVillagerEntity.hpp"
#include "entity/interfaces/ICrossbowUser.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "item/items/weapon/ArrowItem.hpp"
#include "item/items/weapon/CrossbowItem.hpp"
#include "sound/SoundEvents.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"

#include <cmath>
#include <memory>
#include <utility>

namespace mc {

// ==================== VindicatorJohnnyAttackGoal ====================
//
// 对齐 vanilla 1.21.11 Vindicator.VindicatorJohnnyAttackGoal（Vindicator.java:209-224）。
// 卫道士被命名为 "Johnny" 时（isJohnny=true），攻击所有可攻击生物，而非仅玩家/村民/铁傀儡。
//   vanilla: extends NearestAttackableTargetGoal<LivingEntity>，构造传谓词
//   `(target) -> target.attackable()`，canUse() = isJohnny && super.canUse()。
//
// Cubium 实现：继承 NearestAttackableTargetGoal<LivingEntity>（checkSight=true, chance=0，
// 谓词 isAlive 对齐 vanilla attackable() 对普通 LivingEntity 的语义），shouldExecute 加
// isJohnny 门控后委托基类。基类 shouldExecute 内部经 findClosestEntity + isSuitableTarget
// （含 canAttackType 排除 GHAST、Player 创造/观察模式、isAlliedTo 同队门控）+ checkSight
// + 自定义谓词选取最近目标。
//
// TODO: vanilla Johnny 谓词 attackable() 不含 team 检查，会攻击同队灾厄村民；Cubium
// isSuitableTarget 含 isAlliedTo 门控，致 Johnny 不攻击同队灾厄村民，与 vanilla 存在
// 边缘偏差。完整对齐需在 Johnny goal 中绕过 isAlliedTo（完全重写 shouldExecute），
// 但 isSuitableTarget 的 Player 游戏模式门控需保留，暂委托基类以复用已测试逻辑。

class VindicatorJohnnyAttackGoal : public entity::ai::goal::NearestAttackableTargetGoal<LivingEntity> {
public:
    explicit VindicatorJohnnyAttackGoal(VindicatorEntity* vindicator)
        // 对齐 vanilla super(mob, LivingEntity.class, chance=0, checkSight=true, checkCustom=true, predicate)。
        // 谓词 isAlive 对齐 vanilla attackable()（LivingEntity 默认 true，过滤死亡目标）。
        : NearestAttackableTargetGoal<LivingEntity>(vindicator,
              true, // checkSight
              0,    // chance（每 tick 检查）
              [](const LivingEntity* target) -> bool { return target != nullptr && target->isAlive(); })
        , m_vindicator(vindicator)
    {
        MC_ASSERT_RELEASE(vindicator != nullptr);
    }

    ~VindicatorJohnnyAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override
    {
        if (m_vindicator == nullptr || !m_vindicator->isJohnny()) {
            return false;
        }
        return NearestAttackableTargetGoal<LivingEntity>::shouldExecute();
    }

    [[nodiscard]] std::string getTypeName() const override { return "VindicatorJohnnyAttackGoal"; }

private:
    VindicatorEntity* m_vindicator;
};

// ==================== 同步链标识（透传层，无自身同步字段） ====================
const entity::EntityClassInfo& PillagerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"PillagerEntity", &AbstractIllagerEntity::classInfo()};
    return s_classInfo;
}

const entity::EntityClassInfo& VindicatorEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"VindicatorEntity", &AbstractIllagerEntity::classInfo()};
    return s_classInfo;
}

// ==================== PillagerEntity ====================

std::unique_ptr<Entity> PillagerEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<PillagerEntity>(EntityInstanceId(0), registry);
}

PillagerEntity::PillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractIllagerEntity(id, registry)
{
    registerAttributes();

    // 补调 registerGoals：基类构造期间 vtable 指向基类，派生 override 永不执行，须在派生类构造
    // 显式调用。Pillager 的 registerGoals 加专属 SwimGoal / Crossbow / 近战等目标。
    registerGoals();

    // 灾厄村民（非亡灵）不在阳光下燃烧。MonsterEntity::handleDaylightBurning() 读成员
    // m_burnsInDaylight（而非虚函数 shouldBurnInDaylight()，后者全仓零调用是遗留死代码 API），
    // 故用 setBurnsInDaylight(false) 生效。
    setBurnsInDaylight(false);

    // 默认主手持弩：掠夺者使用弩远程攻击，RangedCrossbowAttackGoal::shouldExecute 依赖主手持弩
    // （_isHoldingCrossbow 判 getUseAction==Crossbow），不持弩则弩攻击 goal 永不启动。
    // GameTest 的 test.spawn 不走 finalizeSpawn/populateDefaultEquipmentSlots，故构造期补弩
    // 确保 GameTest spawn 的掠夺者也能远程攻击。isEmpty 守卫避免自然生成路径重复给弩。
    if (getEquipment(EquipmentSlot::MainHand).isEmpty() && Items::CROSSBOW != nullptr) {
        setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::CROSSBOW, 1));
    }
}

void PillagerEntity::attackEntityWithRangedAttack(LivingEntity* target, f32 charge)
{
    // charge 参数对弩不重要，弩使用固定速度
    MC_UNUSED(charge);

    if (!target || !m_world) return;

    // 获取主手弩
    ItemStack& crossbow = getMutableMainHandItem();
    const Item* item = crossbow.getItem();

    // 检查是否是弩
    if (item == nullptr || item->getUseAction(crossbow) != UseAction::Crossbow) {
        return;
    }

    // 调用 shootCrossbow 发射弩箭
    shootCrossbow(target, crossbow, 1.0f);
}

void PillagerEntity::onCrossbowLoadComplete(ItemStack& crossbow)
{
    // 装填完成后重置空闲时间，防止立即消失
    setIdleTime(0);

    // 装填完成时播放音效（如果需要）
    // 播放音效在 CrossbowItem 中已处理
    MC_UNUSED(crossbow);
}

void PillagerEntity::shootCrossbow(LivingEntity* target, ItemStack& crossbow, f32 charge)
{
    if (!target || !m_world || !crossbow.getItem()) return;

    MC_UNUSED(charge);

    // 计算弹道
    f64 dx = target->x() - x();
    f64 dz = target->z() - z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 目标高度偏移：目标高度的 1/3（瞄准躯干下部，对齐 MC AbstractSkeleton.performRangedAttack）
    // 弹道高度补偿：水平距离 * 0.2
    f64 dy = target->getY(0.3333333333333333) - (getEyeY() - 0.15) + horizontalDist * 0.2;

    // 计算弹道偏移角度（多重射击支持）
    // 掠夺者只有一支箭，偏移为 0
    f32 projectileAngle = 0.0f;

    // 确定速度
    f32 velocity = 1.6f; // 掠夺者使用的速度
    const item::CrossbowItem* crossbowItem = dynamic_cast<const item::CrossbowItem*>(crossbow.getItem());
    if (crossbowItem && item::CrossbowItem::hasChargedProjectile(crossbow, Items::FIREWORK_ROCKET)) {
        velocity = 1.6f; // 烟花速度
    } else {
        velocity = 3.15f; // 箭矢速度
    }

    // 计算难度相关的不精确度：14 - difficulty.getId() * 4
    // Peaceful=14, Easy=10, Normal=6, Hard=2
    f32 inaccuracy = entity::combat::DifficultyHelper::getRangedAttackInaccuracy(m_world->difficulty());

    // 创建箭矢实体
    // 掠夺者不消耗弹药，直接创建箭矢
    // ECS 迁移：实体构造需要 registry 句柄（m_world 已判空，此处 registry 必非空）
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }
    auto arrow = std::make_unique<entity::ArrowEntity>(EntityInstanceId(0), *registry);
    arrow->setTypeId(entity::EntityTypeKeys::ARROW); // 工厂绕过补救：直接构造缺 typeId
    arrow->setWorld(m_world);
    arrow->setPosition(x(), static_cast<f32>(getEyeY() - 0.15), z());
    arrow->setShooter(this);

    // 设置箭矢属性
    arrow->setShotFromCrossbow(true);
    arrow->setDamage(5.0f); // 掠夺者箭矢伤害

    // 计算发射方向（考虑偏移角度）
    f32 yaw = this->yaw();
    f32 pitch = this->pitch();

    // 如果有目标，计算指向目标的方向
    if (horizontalDist > 0.001) {
        yaw = static_cast<f32>(std::atan2(dz, dx) * 180.0 / math::PI) - 90.0f;
        pitch = static_cast<f32>(std::atan2(dy, horizontalDist) * 180.0 / math::PI);
    }

    // 应用弹道偏移角度（用于多重射击）
    if (projectileAngle != 0.0f) {
        yaw += projectileAngle;
    }

    // 发射箭矢
    arrow->shootFrom(*this, pitch, yaw, 0.0f, velocity, inaccuracy);

    // 生成箭矢实体
    m_world->spawnEntity(std::move(arrow));

    // 播放发射音效
    playSound(SoundEvents::ITEM_CROSSBOW_SHOOT, 1.0f, getRandom().nextFloat() * 0.4f + 0.8f);

    // 清除弩的装填状态
    if (crossbowItem) {
        item::CrossbowItem::setCharged(crossbow, false);
        item::CrossbowItem::clearProjectiles(crossbow);
    }
}

void PillagerEntity::registerGoals()
{
    AbstractIllagerEntity::registerGoals();

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 2: 寻找目标（袭击模式专用，这里简化处理）
    // m_goalSelector.addGoal(2, std::make_unique<FindTargetGoal>(this, 10.0f));

    // 优先级 3: 弩远程攻击
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::RangedCrossbowAttackGoal>(this, 1.0, 8.0f));

    // 优先级 8: 随机行走
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 0.6));

    // 优先级 9: 看向玩家
    m_goalSelector.addGoal(9,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 15.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 10: 看向生物
    m_goalSelector.addGoal(10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 15.0f));

    // 目标选择器
    // 优先级 1: 被攻击后反击并呼叫支援，但不会反击其他灾厄村民
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 0));

    // 优先级 3: 攻击村民（穿透墙壁感知）
    m_targetSelector.addGoal(3,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>>(this, false));

    // 优先级 3: 攻击铁傀儡
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));
}

void PillagerEntity::registerAttributes()
{
    AbstractIllagerEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 32.0);
}

// ==================== VindicatorEntity ====================

std::unique_ptr<Entity> VindicatorEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<VindicatorEntity>(EntityInstanceId(0), registry);
}

VindicatorEntity::VindicatorEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractIllagerEntity(id, registry)
{
    registerAttributes();

    // 灾厄村民（非亡灵）不在阳光下燃烧。MonsterEntity::handleDaylightBurning() 读成员
    // m_burnsInDaylight（而非虚函数 shouldBurnInDaylight()，后者全仓零调用是遗留死代码 API），
    // 故用 setBurnsInDaylight(false) 生效。
    setBurnsInDaylight(false);

    // 补调 registerGoals：基类构造期间 vtable 指向基类，派生 override 永不执行，须在派生类构造
    // 显式调用。Vindicator 的 registerGoals 加专属破门 / 近战等目标。
    registerGoals();

    // 默认主手持铁斧：卫道士近战伤害依赖主手武器加成。LivingEntity::detectEquipmentUpdates
    // 在装备变化时把 ToolItem 的 ATTACK_DAMAGE 修饰符（铁斧 +9）叠到属性上，徒手则只有基础
    // ATTACK_DAMAGE(5.0)，伤害偏离原版。GameTest 的 test.spawn 不走 finalizeSpawn/
    // populateDefaultEquipmentSlots，故构造期补铁斧使伤害与原版普通难度（5+9=14）一致。
    // isEmpty 守卫避免自然生成路径重复给斧。
    if (getEquipment(EquipmentSlot::MainHand).isEmpty() && Items::IRON_AXE != nullptr) {
        setEquipment(EquipmentSlot::MainHand, ItemStack(*Items::IRON_AXE, 1));
    }
}

void VindicatorEntity::setCustomNameComponent(std::unique_ptr<text::ITextComponent> name)
{
    // 对齐 vanilla Vindicator.setCustomName（Vindicator.java:145-150）：调基类设置名称后，
    // 若当前未激活 Johnny 且新名称纯文本为 "Johnny"，则锁存激活 Johnny 模式。
    // 经 Entity::setCustomName(string) 委托 setCustomNameComponent，命名牌等文本路径同样触发。
    Entity::setCustomNameComponent(std::move(name));
    if (!m_isJohnny) {
        if (customNameText() == "Johnny") {
            m_isJohnny = true;
        }
    }
}

void VindicatorEntity::registerGoals()
{
    AbstractIllagerEntity::registerGoals();

    // 卫道士需要开启导航器的开门能力，以便破门目标能够激活
    auto* nav = navigator();
    if (nav) {
        nav->setCanOpenDoors(true);
    }

    // 优先级 0: 游泳
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // 优先级 1: 破门（Normal+Hard 难度，对齐 Java 1.21.11 Vindicator.java:52 DOOR_BREAKING_PREDICATE）。
    // 卫道士在袭击中破门，Normal 与 Hard 均可；Easy/Peaceful 不破门（区别于僵尸仅 Hard）。
    m_goalSelector.addGoal(1,
        std::make_unique<entity::ai::goal::BreakDoorGoal>(
            this, entity::ai::goal::defaultDoorBreakDifficultyPredicate()));

    // 优先级 2: 袭击期间开门（不关门，不需要 mobGriefing 规则和难度检查）
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::RaiderOpenDoorGoal>(this));

    // 优先级 3: 寻找目标（袭击模式专用，简化处理）
    // m_goalSelector.addGoal(3, std::make_unique<FindTargetGoal>(this, 10.0f));

    // 优先级 4: 近战攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, false));

    // 优先级 8: 随机行走
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::RandomWalkingGoal>(this, 0.6));

    // 优先级 9: 看向玩家
    m_goalSelector.addGoal(9,
        std::make_unique<entity::ai::goal::LookAtGoal>(
            this, 3.0f, entity::ai::goal::LookAtGoal::DEFAULT_LOOK_CHANCE, entity::ai::goal::TypeFilter<Player>{}));

    // 优先级 10: 看向生物
    m_goalSelector.addGoal(10, std::make_unique<entity::ai::goal::LookAtGoal>(this, 8.0f));

    // 目标选择器
    // 优先级 1: 被攻击后反击并呼叫支援，但不会反击其他灾厄村民
    m_targetSelector.addGoal(
        1, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true, [](const LivingEntity* attacker) -> bool {
            return dynamic_cast<const AbstractRaiderEntity*>(attacker) != nullptr;
        }));

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(this, true, 0));

    // 优先级 3: 攻击村民
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>>(this, true));

    // 优先级 3: 攻击铁傀儡
    m_targetSelector.addGoal(
        3, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>>(this, true));

    // 优先级 4: Johnny 彩蛋——命名为 "Johnny" 时攻击所有可攻击生物。对齐 vanilla
    //   Vindicator.registerGoals:73 `targetSelector.addGoal(4, new VindicatorJohnnyAttackGoal(this))`。
    //   isJohnny 由 setCustomNameComponent 在命名为 "Johnny" 时锁存。shouldExecute 门控 isJohnny，
    //   非 Johnny 时永不触发（零开销）；Johnny 时选取最近可攻击 LivingEntity 作为目标。
    m_targetSelector.addGoal(4, std::make_unique<VindicatorJohnnyAttackGoal>(this));
}

void VindicatorEntity::registerAttributes()
{
    AbstractIllagerEntity::registerAttributes();

    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 24.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.35);
    // 基础攻击伤害为 5.0；主手铁斧通过装备修饰符额外 +9，命中总伤害 14（普通难度）。
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 12.0);
}

} // namespace mc
