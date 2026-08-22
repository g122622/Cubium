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

#include "MobEntity.hpp"
#include "../../core/Types.hpp"
#include "../../item/Items.hpp"
#include "../../item/core/ActionResult.hpp"
#include "../../item/core/Item.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../item/enchantment/EnchantmentHelper.hpp"
#include "../../item/enchantment/enchantments/AllEnchantments.hpp"
#include "../../item/items/special/NameTagItem.hpp"
#include "../../item/items/special/SpawnEggItem.hpp"
#include "../../sound/SoundEvents.hpp"
#include "../../util/math/random/Random.hpp"
#include "../../world/IWorld.hpp"
#include "../../world/gameevent/GameEvents.hpp"
#include "../../world/gamerule/GameRules.hpp"
#include "../ai/EntitySenses.hpp"
#include "../ai/controller/JumpController.hpp"
#include "../ai/controller/LookController.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/pathfinding/PathNavigator.hpp"
#include "../attribute/Attributes.hpp"
#include "../combat/DifficultyInstance.hpp"
#include "../combat/PlayerAttackHelper.hpp"
#include "../core/AgeableEntity.hpp"
#include "../damage/DamageSource.hpp"
#include "../ecs/components/MobFlagComponent.hpp"
#include "../entities/hanging/HangingEntity.hpp"
#include "../entities/item/ItemEntity.hpp"
#include "../entities/player/Player.hpp"
#include "../entities/vehicle/BoatEntity.hpp"
#include "../experience/ExperienceDropHandler.hpp"
#include "../serialization/EntityNbtKeys.hpp"
#include "../serialization/NbtHelper.hpp"
#include "../utils/ItemDropHelper.hpp"
#include "EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"

#include "common/core/Result.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathFinder.hpp"
#include "common/entity/ai/pathfinding/PathNodeType.hpp"
#include "common/entity/ai/pathfinding/WalkNodeProcessor.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// 创建 MobEntity 默认寻路器：WalkNodeProcessor → PathFinder → PathNavigator，并绑定实体。
// 修复历史 bug：原构造 m_navigator(std::make_unique<PathNavigator>(this)) 走 PathNavigator(MobEntity*)
// 构造分支，m_pathFinder 永远为 nullptr，致 PathNavigator::moveTo 在 !m_pathFinder 处早退返回 false，
// 全仓所有 MobEntity 的主动寻路（MeleeAttackGoal/FoxFollowTargetGoal/RandomWalkingGoal 等）全部失效，
// 实体 attackTarget 设了却永不移动。子类（如 RavagerEntity）可自行覆盖 m_navigator 用定制 NodeProcessor。
// 对应 vanilla Mob.createNavigation() → GroundPathNavigation(new WalkNodeProcessor())。
namespace {
std::unique_ptr<entity::ai::pathfinding::PathNavigator> _createDefaultNavigator(LivingEntity* mob)
{
    auto nodeProcessor = std::make_unique<entity::ai::pathfinding::WalkNodeProcessor>();
    auto pathFinder = std::make_unique<entity::ai::pathfinding::PathFinder>(std::move(nodeProcessor));
    auto navigator = std::make_unique<entity::ai::pathfinding::PathNavigator>(std::move(pathFinder));
    navigator->setEntity(mob);
    return navigator;
}
} // namespace

// ==================== 静态成员初始化 ====================
entity::DataParameter<i8> MobEntity::DATA_MOB_FLAGS_PARAM = entity::EntityDataManager::createKey<i8>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = LivingEntity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& MobEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"MobEntity", &LivingEntity::classInfo()};
    return s_classInfo;
}

MobEntity::MobEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : LivingEntity(id, nullptr, registry)
    , m_lookController(std::make_unique<entity::ai::controller::LookController>(this))
    , m_moveController(std::make_unique<entity::ai::controller::MovementController>(this))
    , m_jumpController(std::make_unique<entity::ai::controller::JumpController>(this))
    , m_senses(std::make_unique<entity::ai::EntitySenses>(this))
    , m_navigator(_createDefaultNavigator(this))
{
    // 初始化装备掉落概率为默认值
    m_equipmentDropChances.fill(DEFAULT_EQUIPMENT_DROP_CHANCE);

    // 初始化寻路惩罚值表为 NaN（表示"未设置"，回退到 PathNodeType 默认代价）
    // 对应 MC Java 的 EnumMap<PathType, Float>：get() 返回 null 时回退到 getMalus()
    m_pathfindingMalus.fill(std::numeric_limits<f32>::quiet_NaN());

    // 显式调用 registerData() 注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类（Entity::Entity 内部调用
    // registerData() 时调用的是 Entity::registerData 而非 MobEntity::registerData），
    // 必须在派生类构造函数中显式调用，参考 WolfEntity / AbstractSkeletonEntity 模式。
    registerData();
}

MobEntity::~MobEntity() = default;

void MobEntity::registerData()
{
    // 先调用父类方法，确保基类数据参数已注册
    LivingEntity::registerData();

    // 标记当前正在注册 MobEntity 类的字段，使 registerParam 沿 MobEntity 继承链
    // 分配 id（续接 LivingEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册 Mob 标志位数据参数，默认值为 0（无标志）
    // 对应 MC 1.21.11 Mob.DATA_MOB_FLAGS_ID，存储 noAI/leftHanded/aggressive 位标志。
    m_dataManager.registerParam(DATA_MOB_FLAGS_PARAM, static_cast<i8>(0));
}

void MobEntity::registerAttributes()
{
    // 在 LivingEntity 基础上注册和设置属性
    LivingEntity::registerAttributes();

    // 注册并设置跟随范围，默认值为 16.0
    attributes().registerAttribute(*entity::attribute::Attributes::followRange());
    attributes().setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

entity::ai::controller::LookController* MobEntity::lookController()
{
    return m_lookController.get();
}

const entity::ai::controller::LookController* MobEntity::lookController() const
{
    return m_lookController.get();
}

entity::ai::controller::MovementController* MobEntity::moveController()
{
    return m_moveController.get();
}

const entity::ai::controller::MovementController* MobEntity::moveController() const
{
    return m_moveController.get();
}

entity::ai::controller::JumpController* MobEntity::jumpController()
{
    return m_jumpController.get();
}

const entity::ai::controller::JumpController* MobEntity::jumpController() const
{
    return m_jumpController.get();
}

entity::ai::pathfinding::PathNavigator* MobEntity::navigator()
{
    return m_navigator.get();
}

const entity::ai::pathfinding::PathNavigator* MobEntity::navigator() const
{
    return m_navigator.get();
}

entity::ai::EntitySenses* MobEntity::senses()
{
    return m_senses.get();
}

const entity::ai::EntitySenses* MobEntity::senses() const
{
    return m_senses.get();
}

f32 MobEntity::getPathfindingMalus(entity::ai::pathfinding::PathNodeType pathType) const noexcept
{
    // 对应 MC Java Mob.getPathfindingMalus(PathType)：
    //   if (this.getControlledVehicle() instanceof Mob mob1 && mob1.shouldPassengersInheritMalus()) {
    //       mob = mob1;
    //   } else {
    //       mob = this;
    //   }
    //   Float f = mob.pathfindingMalus.get(p_326934_);
    //   return f == null ? p_326934_.getMalus() : f;
    //
    // 项目无 getControlledVehicle()，使用 getVehicle() + world()->getEntity() 解引用，
    // 与 MobEntity.cpp::isInDaylight() 中既有的解引用模式一致。

    const MobEntity* malusSource = this;

    const EntityInstanceId vehicleId = getVehicle();
    if (vehicleId != INVALID_ENTITY_ID && m_world != nullptr) {
        const Entity* vehicle = m_world->getEntity(vehicleId);
        const MobEntity* vehicleMob = dynamic_cast<const MobEntity*>(vehicle);
        if (vehicleMob != nullptr && vehicleMob->shouldPassengersInheritMalus()) {
            malusSource = vehicleMob;
        }
    }

    const f32 stored = malusSource->m_pathfindingMalus[static_cast<size_t>(pathType)];
    if (std::isnan(stored)) {
        // 未显式设置，回退到 PathNodeType 默认代价（对应 MC 的 PathType.getMalus()）
        return entity::ai::pathfinding::getPathCostPenalty(pathType);
    }
    return stored;
}

bool MobEntity::isBeingRidden() const
{
    return hasPassengers();
}

void MobEntity::clearNavigation()
{
    if (m_navigator) {
        m_navigator->clearPath();
    }
}

void MobEntity::playAmbientSound()
{
    auto soundEvent = getAmbientSound();
    if (soundEvent.has_value()) {
        playSound(*soundEvent, getSoundVolume(), getSoundPitch());
    }
}

void MobEntity::playAttackSound(LivingEntity& target)
{
    (void)target;
}

void MobEntity::lookAt(const Entity& target, f32 deltaYaw, f32 deltaPitch)
{
    lookAt(target.x(), target.y() + target.eyeHeight(), target.z(), deltaYaw, deltaPitch);
}

void MobEntity::lookAt(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch)
{
    if (m_lookController) {
        m_lookController->setLookPosition(x, y, z, deltaYaw, deltaPitch);
    }
}

void MobEntity::tick()
{
    // 更新父类（LivingEntity::tick() 已经调用 aiStep()）
    LivingEntity::tick();

    // UAF 修复：若攻击目标已被标记移除（死亡/卸载/discard），立即清空 m_attackTarget，
    // 避免后续 targetSelector/goalSelector 中持裸指针的 goal 解引用悬垂内存。
    // 必须在 m_targetSelector.tick()/m_goalSelector.tick() 之前执行。
    if (m_attackTarget != nullptr && m_attackTarget->isRemoved()) {
        setAttackTarget(nullptr);
    }

    // 同类 UAF 防护：m_lastHurtBy 也是裸指针，HurtByTargetGoal::shouldExecute 会解引用
    // attacker->isAlive()。寻路修复后实体能主动攻击，被攻击者反击链路激活，若 lastHurtBy
    // 实体已移除（EntityManager 同步 erase+析构无"先标记后析构"窗口）则悬垂解引用段错误。
    // 详见 memory: goal-entity-ptr-uaf（同模式裸指针 UAF 架构问题）。
    if (m_lastHurtBy != nullptr && m_lastHurtBy->isRemoved()) {
        setLastHurtBy(nullptr);
    }

    // 空闲时间在 tick 开头递增
    ++m_idleTime;

    // 环境声音检查
    if (isAlive()) {
        math::Random& random = getRandom();
        if (random.nextInt(1000) < m_livingSoundTime++) {
            m_livingSoundTime = -getTalkInterval();
            playAmbientSound();
        }
    }

    // updateEntityActionState() 顺序:
    // 1. 感知更新
    if (m_senses) {
        m_senses->tick();
    }

    // 2. 目标选择器 (先于 goalSelector)
    // 3. 行为目标选择器
    if (m_aiEnabled) {
        m_targetSelector.tick();
        m_goalSelector.tick();

        // 4. 导航器更新
        if (m_navigator) {
            m_navigator->tick();
        }

        // 5. AI 任务更新 (子类可重写)
        updateAITasks();

        // 每 5 tick 更新移动目标标志
        if (m_ticksExisted % 5 == 0) {
            updateMovementGoalFlags();
        }
    }

    // 6. 控制器更新 (顺序: move -> look -> jump)
    if (m_moveController) {
        m_moveController->tick();
    }
    if (m_lookController) {
        m_lookController->tick();
    }
    if (m_jumpController) {
        m_jumpController->tick();
    }

    // 注意：aiStep() 已在 LivingEntity::tick() 中调用，这里不需要再次调用

    // 拾取掉落物（对齐 vanilla Mob.aiStep 的 "looting" 段，Mob.java:444-462）。
    // canPickUpLoot() && isAlive() && !dead && mobGriefing 游戏规则 时，扫描
    // getBoundingBox().inflate(getPickupReach()) 内的 ItemEntity，对每个未移除、
    // 非空、可拾取（pickupDelay<=0）且 wantsToPickUp 的物品实体调用 pickUpItem。
    // pickUpItem 由子类覆写（Fox→手持物品语义；基类→装备槽语义）。
    // TODO: vanilla 还检查 serverlevel.getGameRules().get(MOB_GRIEFING)，本实现已对齐。
    //   基类 pickUpItem 的装备槽替换逻辑（equipItemIfPossible/canReplaceCurrentItem）为简化实现，
    //   待装备拾取链路完整补全后对齐。
    if (m_world != nullptr && canPickUpLoot() && isAlive() && !isDead() &&
        m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING)) {
        const Vector3i reach = getPickupReach();
        const AxisAlignedBB searchBox =
            boundingBox().expand(static_cast<f32>(reach.x), static_cast<f32>(reach.y), static_cast<f32>(reach.z));

        auto nearbyEntities = m_world->getEntitiesInAABB(searchBox, this);
        for (Entity* entity : nearbyEntities) {
            if (entity == nullptr || entity->isRemoved()) {
                continue;
            }
            if (entity->entityType() != entity::VanillaEntityTypeKeys::ITEM) {
                continue;
            }

            auto* itemEntity = static_cast<ItemEntity*>(entity);
            const ItemStack& stack = itemEntity->getItemStack();
            if (stack.isEmpty() || !itemEntity->canBePickedUp()) {
                continue;
            }
            if (!wantsToPickUp(stack)) {
                continue;
            }

            pickUpItem(*itemEntity);
        }
    }

    // 拴绳物理约束
    tickLeash();
}

void MobEntity::updateMovementGoalFlags()
{
    // 根据骑乘状态更新目标标志
    // 如果被骑乘，禁用 MOVE/JUMP/LOOK 标志
    bool canMove = !isBeingRidden();

    m_goalSelector.setFlag(entity::ai::GoalFlag::Move, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Jump, canMove);
    m_goalSelector.setFlag(entity::ai::GoalFlag::Look, canMove);
}

std::optional<ResourceLocation> MobEntity::getAmbientSound() const
{
    // 默认实现拼接 minecraft:entity.<typeId>.ambient。具体实体若 vanilla sounds.json
    // 不存在该 key（只有 ambient_land/ambient_water/idle_air 等变体，或根本没有环境音），
    // 必须 override 本方法返回正确常量或 std::nullopt，否则 SoundEngine 会打印
    // "Sound event not found" 告警。vanilla Mob 默认返回 null（不播放），与本实现不同。
    //
    // TODO: 以下实体尚未在 Cubium 实现，待后续补齐实体类时一并 override getAmbientSound：
    //   - allay        悦灵        有手物品→ALLAY_AMBIENT_WITH_ITEM / 无→ALLAY_AMBIENT_WITHOUT_ITEM
    //   - armadillo    犧狳        受惊(scared)→nullopt / 否则→ARMADILLO_AMBIENT
    //   - camel        骆驼        CAMEL_AMBIENT（声音键 entity.camel.ambient）
    //   - frog         青蛙        按变种 swamp/jungle/cold→FROG_AMBIENT_*（不同变种不同音）
    //   - goat         山羊        GOAT_AMBIENT / 幼体尖叫概率→GOAT_SCREAMING_AMBIENT
    //   - creaking     嘎枝        CREAKING_AMBIENT（1.21 新实体，声音键 entity.creaking.ambient）
    //   - happy_ghast  快乐恶魂    HAPPY_GHAST_AMBIENT（1.21 新实体）
    return makeSoundEventId("ambient");
}

void MobEntity::playHurtSound(DamageSource& source)
{
    m_livingSoundTime = -getTalkInterval();
    LivingEntity::playHurtSound(source);
}

void MobEntity::dropExperience()
{
    // 对齐 MC Java 1.21.11 LivingEntity.dropExperience 守卫（LivingEntity.java:1498-1506）：
    // 普通生物死亡需 !wasExperienceConsumed && (isAlwaysExperienceDropper ||
    // (lastHurtByPlayerMemoryTime>0 && shouldDropExperience && doMobLoot)) 才掉经验。
    // MobEntity isAlwaysExperienceDropper 默认 false，故需被玩家伤害过 + doMobLoot=true。
    // 修复前 MobEntity::dropExperience 不查守卫直接掉落，致 test.kill（虚空伤害无玩家来源）
    // 的普通生物仍掉经验球、doMobLoot=false 时仍掉经验球——与 vanilla 偏差。
    if (m_experienceValue <= 0 || m_world == nullptr) {
        return;
    }
    if (!shouldDropExperienceOnDeath(*m_world)) {
        return;
    }
    // 标记经验已消费，防异常重复掉落（对齐 vanilla skipDropExperience，LivingEntity.java:1639）。
    skipDropExperience();
    math::Random& rng = getRandom();
    entity::ExperienceDropHandler::spawnHostileMobExperience(m_world, x(), y(), z(), m_experienceValue, &rng);
}

void MobEntity::dropCustomDeathLoot(DamageSource& cause, bool recentlyHitByPlayer)
{
    // 对齐 MC Java 1.21.11 Mob.dropCustomDeathLoot（Mob.java:846-877）。
    // 先调基类 dropCustomDeathLoot（LivingEntity 基类为空，子类首领生物可在此补充特殊掉落）。
    LivingEntity::dropCustomDeathLoot(cause, recentlyHitByPlayer);

    if (m_world == nullptr) {
        return;
    }

    // 击杀者 LivingEntity（vanilla cause.getEntity() instanceof LivingEntity）：用于掠夺附魔
    // processEquipmentDropChance 加成（vanilla Mob.java:853-855）。Cubium 掠夺附魔体系是 1.20 风格的
    // LootingEnchantBonusFunction（loot function），未实现 1.21 的 equipment_drops effect component 子系统，
    // 故此处暂不应用掠夺加成（TODO，待 effect component 体系就绪接入）。
    Entity* causingEntity = cause.getEntity();
    (void)causingEntity; // 预留 processEquipmentDropChance 接入点，当前未使用。

    math::Random& rng = getRandom();

    // 遍历所有装备槽位（对齐 vanilla for (EquipmentSlot : EquipmentSlot.VALUES)）。
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        const EquipmentSlot slot = static_cast<EquipmentSlot>(i);

        // 获取槽位物品引用（vanilla this.getItemBySlot(equipmentslot)）。
        // 注意：掉落前需取一份可修改拷贝用于耐久度随机化（vanilla 直接改 itemstack 引用，
        // 因 ItemBySlot 返回的是装备数组内对象；Cubium getEquipment 返回 const 引用，setDamage 需可写，
        // 故先拷贝，随机化后用拷贝掉落，再清空原槽位）。
        const ItemStack& equipmentRef = getEquipment(slot);
        const f32 dropChance = getEquipmentDropChance(slot); // vanilla dropChances.byEquipment(equipmentslot)

        // f == 0.0：永不掉落（vanilla Mob.java:850 if (f != 0.0F) 守卫）。
        if (dropChance == 0.0f) {
            continue;
        }

        const bool isPreserved = isEquipmentDropPreserved(slot); // vanilla dropChances.isPreserved(equipmentslot)

        // TODO: 掠夺附魔加成（vanilla f = EnchantmentHelper.processEquipmentDropChance(serverlevel, livingentity,
        // cause, f)）。
        //       Cubium 未实现 1.21 equipment_drops effect component 子系统，当前 f 不变。待 effect component
        //       体系就绪后，在此用 causingEntity（LivingEntity）的武器掠夺等级修正 f。

        // 掉落条件（对齐 vanilla Mob.java:864-865）：
        //   !itemstack.isEmpty()
        //   && !EnchantmentHelper.has(itemstack, PREVENT_EQUIPMENT_DROP)  // 绑定诅咒
        //   && (p_21387_ || flag)  // recentlyHitByPlayer || isPreserved
        //   && random.nextFloat() < f
        // PREVENT_EQUIPMENT_DROP 在 1.21 仅绑定诅咒贡献，Cubium 用 hasBindingCurse 等价判定。
        if (equipmentRef.isEmpty()) {
            continue;
        }
        if (item::enchant::EnchantmentHelper::hasBindingCurse(equipmentRef)) {
            continue;
        }
        if (!(recentlyHitByPlayer || isPreserved)) {
            continue;
        }
        if (rng.nextFloat() >= dropChance) {
            continue;
        }

        // 拷贝一份用于掉落（耐久度随机化需可写）。
        ItemStack dropStack(equipmentRef);

        // 耐久度随机化（对齐 vanilla Mob.java:866-872）：仅非保整（!flag）且可损伤装备随机化，
        // 把掉落装备设为接近破损（vanilla setDamageValue(maxDamage - random)）。
        if (!isPreserved && dropStack.isDamageable()) {
            const i32 maxDamage = dropStack.getMaxDamage();
            // vanilla: Math.max(itemstack.getMaxDamage() - 3, 1)
            const i32 innerBound = std::max(maxDamage - 3, 1);
            // vanilla: this.random.nextInt(1 + this.random.nextInt(innerBound))
            const i32 remaining = rng.nextInt(1 + rng.nextInt(innerBound));
            // vanilla: itemstack.setDamageValue(itemstack.getMaxDamage() - <remaining>)
            // Cubium m_damage 语义 = 已消耗耐久（同 vanilla damageValue），故 setDamage(maxDamage - remaining)。
            dropStack.setDamage(maxDamage - remaining);
        }

        // 在实体位置掉落（vanilla this.spawnAtLocation(serverlevel, itemstack)）。
        ItemDropHelper::spawnItemAtEntity(this, dropStack, 0.5f, rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);

        // 清空槽位（vanilla this.setItemSlot(equipmentslot, ItemStack.EMPTY)）。
        setEquipment(slot, ItemStack());
    }
}

std::vector<EquipmentSlot> MobEntity::dropPreservedEquipment(const std::function<bool(const ItemStack&)>& predicate)
{
    std::vector<EquipmentSlot> preservedSlots;

    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        EquipmentSlot slot = static_cast<EquipmentSlot>(i);
        const ItemStack& equipment = getEquipment(slot);

        if (equipment.isEmpty()) {
            continue;
        }

        if (!predicate(equipment)) {
            // 谓词返回 false：物品不满足条件（如绑定诅咒），保留在实体上
            // 记录此槽位以便调用者后续处理（如转移到新实体）
            preservedSlots.push_back(slot);
        } else if (isEquipmentDropPreserved(slot)) {
            // 谓词返回 true 且掉落概率 > 1.0（保留状态）：在实体位置掉落该物品
            // 对应 MC Java 的 spawnAtLocation()
            if (m_world != nullptr) {
                math::Random& rng = getRandom();
                ItemDropHelper::spawnItemAtEntity(this, equipment, 0.0f, rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
            }
            // 清空该槽位，防止重复掉落
            setEquipment(slot, ItemStack());
        }
        // 谓词返回 true 且非保留状态（默认 8.5%）：物品静默消失，不做任何处理
        // 对应 MC Java 中 dropPreservedEquipment 不处理非保留装备的行为
    }

    return preservedSlots;
}

std::vector<EquipmentSlot> MobEntity::dropPreservedEquipment()
{
    return dropPreservedEquipment([](const ItemStack&) { return true; });
}

bool MobEntity::isInDaylight() const
{
    // 对齐 vanilla 1.21 Mob.isSunBurnTick()：白天 + 亮度>0.5 + 非水中/雨中 + 天空可见 即返回 true。
    // 此前实现含一个项目自创的随机检查（rng.nextFloat()*30 < (brightness-0.4)*2，正午仅 ~4%/tick
    // 触发），vanilla 无此随机检查——亮度达标就每 tick 必然燃烧。随机检查把燃烧变成概率事件，
    // 在光照重算竞态压缩有效燃烧窗口时引入 flaky（skeleton_burns_in_daylight 偶发超时根因之二）。
    // 移除随机检查对齐 vanilla 确定性燃烧语义。调用方仅 burnUndead。
    if (m_world == nullptr || m_world->isClientSide()) {
        return false;
    }

    // dayTime < 12000 为白天
    if (!m_world->isDaytime()) {
        return false;
    }

    // getBrightness() > 0.5F（getBrightness 对齐 vanilla getLightLevelDependentMagicValue，
    // 正午露天 skyDarkening=0 → rawBrightness=15 → f=1.0 → f1=1.0 >0.5）
    f32 brightness = getBrightness();
    if (brightness <= 0.5f) {
        return false;
    }

    // 在水中或雨中时不燃烧
    if (isWet()) {
        return false;
    }

    // 获取检测位置
    // 如果骑乘船，检测位置向上偏移一格
    BlockPos pos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::round(y())), static_cast<i32>(std::floor(z())));

    // 如果实体骑乘船，检测位置向上偏移一格
    // 原因：船在水面上，生物坐在船中位置较低，需要向上偏移才能正确检测天空可见性
    if (isRiding()) {
        EntityInstanceId vehicleId = getVehicle();
        if (vehicleId != INVALID_ENTITY_ID && m_world != nullptr) {
            const Entity* vehicle = m_world->getEntity(vehicleId);
            if (vehicle != nullptr && dynamic_cast<const entity::BoatEntity*>(vehicle) != nullptr) {
                pos = pos.up();
            }
        }
    }

    return m_world->canSeeSky(pos);
}

void MobEntity::burnUndead()
{
    if (!isAlive() || !isInDaylight()) {
        return;
    }

    // 获取防护槽位中的物品
    EquipmentSlot protectionSlot = sunProtectionSlot();
    ItemStack& protectionItem = getMutableEquipment(protectionSlot);

    if (!protectionItem.isEmpty()) {
        // 防护槽位有物品：如果物品可损坏，则物品承受耐久损耗
        // 注意：此处直接增加伤害值，绕过耐久保护附魔，与 MC 原版行为一致
        if (protectionItem.isDamageable()) {
            math::Random& rng = getRandom();
            i32 addedDamage = rng.nextInt(2); // 0 或 1
            if (addedDamage > 0) {
                i32 newDamage = protectionItem.getDamage() + addedDamage;
                i32 maxDamage = protectionItem.getMaxDamage();

                // 在物品被销毁之前保存物品引用，用于 onEquippedItemBroken 回调
                const Item* brokenItem = (newDamage >= maxDamage) ? protectionItem.getItem() : nullptr;

                protectionItem.setDamage(newDamage);

                // 物品损坏时触发回调：广播装备破损动画、播放音效
                if (brokenItem != nullptr) {
                    onEquippedItemBroken(*brokenItem, protectionSlot);
                }
            }
        }
        // 如果物品不可损坏（如附魔绑定/无限耐久），实体也不会燃烧
    } else {
        // 防护槽位为空：实体被点燃 8 秒
        igniteForSeconds(8.0f);
    }
}

bool MobEntity::canAttackType(const entity::EntityType& type) const
{
    // 对应 MC 原版 Mob.canAttackType()
    // MC 原版基类排除恶魂：return p_21399_ != EntityType.GHAST;
    // 恶魂悬浮在下界高空，大多数近战型 Mob 无法接近恶魂，
    // 将恶魂排除在攻击目标之外可以避免 Mob 徒劳地试图攻击一个它们够不着的敌人
    // 指针比较：调用方传入的 type 必来自注册表（entityType() 解引用），与
    // VanillaEntityTypeKeys::GHAST 同源，可安全指针比较。
    return &type != entity::VanillaEntityTypeKeys::GHAST;
}

bool MobEntity::attackEntityAsMob(LivingEntity& target)
{
    // 1. 获取攻击伤害属性
    f32 attackDamage = static_cast<f32>(getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE, 1.0));

    // 2. 获取主手武器，应用附魔伤害加成
    const ItemStack& mainHand = getMainHandItem();

    if (!mainHand.isEmpty()) {
        // 附魔伤害加成（锋利、亡灵杀手、节肢杀手）
        attackDamage +=
            entity::combat::PlayerAttackHelper::getEnchantmentDamageBonus(mainHand, target.getCreatureAttribute());
    }

    // 3. 火焰附加（在攻击前应用，用于燃烧传递判定）
    i32 fireAspectLevel = 0;
    if (!mainHand.isEmpty()) {
        fireAspectLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
            mainHand, &item::enchant::AllEnchantments::FIRE_ASPECT);
    }

    // 4. 创建伤害来源并应用伤害
    EntityDamageSource damageSource = DamageSources::mobAttack(this);

    // 如果有火焰附加，在攻击前点燃目标 1 秒（用于燃烧传递）
    if (fireAspectLevel > 0) {
        target.igniteForTicks(20); // 1 秒 = 20 ticks
    }

    // 保存目标 hurt() 前的速度，用于 causeExtraKnockback 的速度修正
    Vector3 preHurtVelocity = target.velocity();

    bool attacked = target.hurt(damageSource, attackDamage);

    if (attacked) {
        // 5. 应用额外击退（附魔击退 + 攻击击退属性）
        // getKnockback() 返回 (ATTACK_KNOCKBACK + 附魔击退) / 2.0
        f32 knockbackStrength = getKnockback(target);
        causeExtraKnockback(target, knockbackStrength, preHurtVelocity);

        // 6. 应用火焰附加（攻击后应用完整燃烧时间）
        if (fireAspectLevel > 0) {
            // 火焰附加持续时间 = level * 4 秒
            target.igniteForSeconds(static_cast<f32>(fireAspectLevel) * 4.0f);
        }

        // 7. 武器损耗
        // 注意：MC Java 版的 Mob.doHurtTarget() 仅调用 hurtEnemy()（空操作，仅 MaceItem 等重写），
        // 不调用 postHurtEnemy()，因此 Mob 近战攻击不会造成武器耐久损耗。
        // 武器耐久损耗仅通过 Player.itemAttackInteraction() -> postHurtEnemy() 路径发生。
        // 如果未来需要为特定 Mob 添加武器损耗，应在子类的 attackEntityAsMob 中调用
        // LivingEntity::hurtAndBreak(getMutableMainHandItem(), 1, this, EquipmentSlot::MainHand)。

        // 8. 设置最后攻击者
        target.setLastHurtBy(this);

        // 9. 播放攻击声音
        playAttackSound(target);
    }

    return attacked;
}

// ========== 玩家交互 ==========

ActionResultType MobEntity::processInitialInteract(Player& player, Hand hand)
{
    if (!isAlive()) {
        return ActionResultType::Pass;
    }

    ItemStack& heldItem = player.getHeldItem(hand);
    const Item* item = heldItem.getItem();

    // 1. 命名牌交互：如果玩家手持命名牌，调用 NameTagItem 的交互方法
    //    命名牌必须先在铁砧上命名，才能对生物使用
    //    注意: ItemStack::getItem() 返回 const Item*，需要转换为非 const
    //    这是安全的，因为 itemInteractionForEntity 只修改 ItemStack 参数，不修改 Item 本身
    if (item != nullptr && dynamic_cast<const item::items::NameTagItem*>(item) != nullptr) {
        bool success = const_cast<Item*>(item)->itemInteractionForEntity(heldItem, player, *this, hand);
        if (success) {
            return ActionResultType::Success;
        }
    }

    // 2. 刷怪蛋交互：如果玩家手持刷怪蛋，生成幼体
    //    SpawnEggItem 已在 Items::_registerSpawnEggs() 中注册（67 种刷怪蛋），
    //    玩家手持匹配类型的刷怪蛋右键对应生物时，通过 _spawnOffspringFromSpawnEgg 生成幼体。
    //    仅 AgeableEntity 子类（如 PigEntity、CowEntity）支持幼体生成。
    if (item != nullptr) {
        auto* spawnEgg = dynamic_cast<const item::SpawnEggItem*>(item);
        if (spawnEgg != nullptr) {
            if (m_world != nullptr && !m_world->isClientSide()) {
                if (_spawnOffspringFromSpawnEgg(player, *spawnEgg, heldItem)) {
                    return ActionResultType::Success;
                }
            } else {
                // 客户端直接预测成功
                return ActionResultType::Success;
            }
            return ActionResultType::Pass;
        }
    }

    // 3. 拴绳交互
    if (item != nullptr && item == Items::LEAD) {
        // 服务端处理拴绳逻辑
        if (m_world != nullptr && !m_world->isClientSide()) {
            // 如果实体已被当前玩家拴住，解除拴绳（掉落拴绳物品）
            if (isLeashed() && leashHolderUuid().has_value() && *leashHolderUuid() == player.uuid()) {
                if (player.abilities().creativeMode) {
                    // 创造模式：解除拴绳但不掉落物品
                    clearLeash();
                } else {
                    // 生存模式：解除拴绳并掉落拴绳物品
                    dropLeash();
                }
                m_world->gameEvent(gameevent::GameEvents::ENTITY_INTERACT,
                    BlockPos(static_cast<i32>(std::floor(x())),
                        static_cast<i32>(std::floor(y())),
                        static_cast<i32>(std::floor(z()))),
                    nullptr);
                return ActionResultType::Success;
            }

            // 如果实体可以被拴住且未被其他玩家拴住，将拴绳拴在实体上
            if (canBeLeashed() && isAlive()) {
                // 检查实体当前是否被其他玩家拴住
                if (isLeashed() && leashHolderUuid().has_value() && *leashHolderUuid() != player.uuid()) {
                    // 已被其他玩家拴住，不允许拴
                    return ActionResultType::Fail;
                }

                // 如果已拴在栅栏或其他实体上，先解除旧拴绳
                if (isLeashed()) {
                    dropLeash();
                }

                // 拴到玩家身上
                setLeashedToEntity(player.uuid());
                enablePersistence();

                // 消耗一个拴绳物品
                if (!player.abilities().creativeMode) {
                    heldItem.shrink(1);
                }

                // 播放拴绳绑定的音效
                if (m_world != nullptr) {
                    playSound(SoundEvents::ENTITY_LEASH_KNOT_PLACE, 1.0f, 1.0f);
                }

                return ActionResultType::Success;
            }
        } else {
            // 客户端：拴绳交互直接预测成功
            return ActionResultType::Success;
        }
        return ActionResultType::Pass;
    }

    // 4. 剪刀剪切装备（如狼铠）
    //    参考: net.minecraft.world.entity.Entity.interact() 中的剪刀分支
    //    条件：手持剪刀 + canShearEquipment + 玩家未蹲下 + 身体护甲非空
    //    在 interactMob 之前处理，与 MC 原版一致
    if (item != nullptr && item == Items::SHEARS && !player.isSneaking()) {
        if (canShearEquipment(player) && isWearingBodyArmor()) {
            if (m_world != nullptr && !m_world->isClientSide()) {
                if (attemptToShearEquipment(player, hand, heldItem)) {
                    return ActionResultType::Success;
                }
            } else {
                // 客户端预测成功
                return ActionResultType::Success;
            }
        }
    }

    // 调用子类的交互逻辑
    ActionResultType result = interactMob(player, hand);
    if (result == ActionResultType::Success || result == ActionResultType::Consume) {
        return result;
    }

    // 调用父类实现
    return LivingEntity::processInitialInteract(player, hand);
}

ActionResultType MobEntity::interactMob(Player& /*player*/, Hand /*hand*/)
{
    // 基类默认返回 Pass，子类可重写以处理特定交互
    return ActionResultType::Pass;
}

bool MobEntity::canBeLeashed() const
{
    // 敌对生物不能被拴住（MobFlagComponent 标记组件，IMob 接口的 tag 层）
    return !hasComponent<ecs::MobFlagComponent>();
}

bool MobEntity::_spawnOffspringFromSpawnEgg(Player& player, const item::SpawnEggItem& spawnEgg, ItemStack& heldItem)
{
    // 检查刷怪蛋的实体类型是否与当前实体类型匹配
    // 只有相同类型的刷怪蛋才能在该实体上生成幼体
    // 使用实体类型名称字符串进行比较，避免 EntityType 不可拷贝的问题
    const std::string& eggEntityTypeName = spawnEgg.getEntityType().name();
    if (eggEntityTypeName != getTypeId()) {
        return false;
    }

    // 检查实体类型是否可序列化（与 MC 一致性检查）
    const entity::EntityType* myType = entity::EntityRegistry::instance().getType(getTypeId());
    if (myType == nullptr || !myType->serializable()) {
        return false;
    }

    if (m_world == nullptr) {
        return false;
    }

    // 创建幼体实体
    // 通过世界获取 ECS 实体注册表（ServerWorld 持有 m_entityRegistry）
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return false;
    }
    auto baby = myType->create(m_world, *registry);
    if (baby == nullptr) {
        return false;
    }

    // 转换为 MobEntity 进行设置
    auto* babyMob = dynamic_cast<MobEntity*>(baby.get());
    if (babyMob == nullptr) {
        return false;
    }

    // 设置为幼体（仅年龄型实体支持）
    // 对齐 Java SpawnEggItem.spawnOffspringFromSpawnEgg：调 setBaby(true) 后检查 isBaby()。
    // Java 中 Mob.setBaby 基类为空实现，仅 AgeableMob override 使 isBaby 生效；
    // 故非年龄型实体 setBaby 无效、isBaby() 仍为 false，Java 据此 return empty 放弃生成。
    // 此处保持与 Java 一致：非年龄型实体不生成。
    auto* babyAgeable = dynamic_cast<AgeableEntity*>(babyMob);
    if (babyAgeable != nullptr) {
        // 年龄型实体（动物等）：通过 AgeableEntity::setChild 设置幼体状态
        babyAgeable->setChild(true);
    } else {
        // 非年龄型实体：setChild 无效，与 Java 一致放弃生成幼体
        return false;
    }

    // 将幼体放置在父实体位置
    babyMob->setPosition(x(), y(), z());
    babyMob->setRotation(yaw(), pitch());

    // 初始化生成
    entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*m_world,
        BlockPos(static_cast<i32>(std::floor(x())), static_cast<i32>(y()), static_cast<i32>(std::floor(z()))));
    babyMob->finalizeSpawn(*m_world, difficultyInstance, world::spawn::SpawnReason::SpawnEgg);

    // 将幼体添加到世界
    m_world->spawnEntity(std::move(baby));

    // 消耗一个刷怪蛋（创造模式不消耗）
    if (!player.isCreative()) {
        heldItem.shrink(1);
    }

    return true;
}

// ============================================================================
// 掉落概率
// ============================================================================

f32 MobEntity::getEquipmentDropChance(EquipmentSlot slot) const
{
    auto idx = static_cast<size_t>(slot);
    if (idx >= m_equipmentDropChances.size()) {
        return DEFAULT_EQUIPMENT_DROP_CHANCE;
    }
    return m_equipmentDropChances[idx];
}

void MobEntity::setEquipmentDropChance(EquipmentSlot slot, f32 chance)
{
    auto idx = static_cast<size_t>(slot);
    if (idx < m_equipmentDropChances.size()) {
        m_equipmentDropChances[idx] = chance;
    }
}

void MobEntity::setGuaranteedDrop(EquipmentSlot slot)
{
    setEquipmentDropChance(slot, PRESERVE_ITEM_DROP_CHANCE);
}

bool MobEntity::isEquipmentDropPreserved(EquipmentSlot slot) const
{
    return getEquipmentDropChance(slot) > 1.0f;
}

void MobEntity::setBodyArmorItem(const ItemStack& stack)
{
    setEquipment(EquipmentSlot::Body, stack);
    setGuaranteedDrop(EquipmentSlot::Body);
    enablePersistence();
}

bool MobEntity::canShearEquipment(const Player& /*player*/) const
{
    return !isRiding();
}

bool MobEntity::attemptToShearEquipment(Player& player, Hand hand, ItemStack& shears)
{
    // 遍历所有装备槽位，找到第一个有装备的槽位
    // 参考: net.minecraft.world.entity.Entity.attemptToShearEquipment()
    // MC 1.21.11 中通过 Equippable.canBeSheared() 判断，本项目通过 Body 槽位有装备判断
    // （目前只有狼铠可被剪切，马铠等后续可扩展）
    const ItemStack& bodyArmor = getEquipment(EquipmentSlot::Body);
    if (bodyArmor.isEmpty()) {
        return false;
    }

    // 剪刀耐久 -1
    LivingEntity::hurtAndBreak(
        shears, 1, &player, hand == Hand::MainHand ? EquipmentSlot::MainHand : EquipmentSlot::OffHand);

    // 取出身体护甲并清空槽位
    ItemStack droppedArmor = bodyArmor;
    setEquipment(EquipmentSlot::Body, ItemStack{});

    // 在实体位置生成掉落物
    if (m_world != nullptr) {
        ItemDropHelper::spawnItemAtEntity(this, droppedArmor, 0.5f, m_world->getRandom());
    }

    // 触发 SHEAR 游戏事件
    if (m_world != nullptr) {
        m_world->gameEvent(gameevent::GameEvents::SHEAR,
            BlockPos(static_cast<i32>(std::floor(x())),
                static_cast<i32>(std::floor(y())),
                static_cast<i32>(std::floor(z()))),
            nullptr);
    }

    // 播放剪切音效（狼铠使用 ARMOR_UNEQUIP_WOLF 音效）
    playSound(SoundEvents::ITEM_ARMOR_UNEQUIP_WOLF, 1.0f, 1.0f);

    return true;
}

std::string MobEntity::getLootTableId() const
{
    // NBT 覆盖优先：如果实体从存档加载了自定义掉落表，使用它
    if (m_deathLootTable.has_value() && !m_deathLootTable->empty()) {
        return *m_deathLootTable;
    }
    // 回退到实体类型的默认掉落表路径
    return Entity::getLootTableId();
}

// ============================================================================
// 拴绳系统
// ============================================================================

void MobEntity::setLeashedToEntity(const std::string& holderUuid)
{
    m_isLeashed = true;
    m_leashHolderUuid = holderUuid;
    m_leashFencePos = std::nullopt;

    // 广播拴绳链接变更给客户端
    if (m_world != nullptr && !m_world->isClientSide()) {
        // 通过 UUID 索引进行 O(1) 查找，替代 getEntitiesInRange 的 O(n) 遍历
        Entity* holder = m_world->getEntityByUuid(holderUuid);
        if (holder != nullptr) {
            m_world->broadcastSetEntityLink(id(), holder->id());
        }
    }
}

void MobEntity::setLeashedToFence(const BlockPos& pos)
{
    m_isLeashed = true;
    m_leashHolderUuid = std::nullopt;
    m_leashFencePos = pos;

    // 广播拴绳链接变更给客户端
    if (m_world != nullptr && !m_world->isClientSide()) {
        // 查找栅栏位置的拴绳结实体
        Vector3 centerPos(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f);
        auto entities = m_world->getEntitiesInRange(centerPos, 2.0f);
        for (Entity* entity : entities) {
            auto* knot = dynamic_cast<entity::LeashKnotEntity*>(entity);
            if (knot != nullptr && knot->isAlive() && knot->getHangingBlockPos() == pos) {
                m_world->broadcastSetEntityLink(id(), knot->id());
                // 成功广播，清除延迟绑定信息
                m_leashDelayInfo.fencePos = std::nullopt;
                m_leashDelayInfo.targetUuid = std::nullopt;
                m_leashDelayInfo.resolveTicks = 0;
                return;
            }
        }
        // 未找到拴绳结实体，保留 fencePos 延迟信息，tickLeash 会重试
    }
}

void MobEntity::clearLeash()
{
    // 广播拴绳解除给客户端（必须在清除状态之前发送，因为需要被拴实体的ID）
    if (m_isLeashed && m_world != nullptr && !m_world->isClientSide()) {
        m_world->broadcastSetEntityLink(id(), EntityInstanceId(0));
    }

    m_isLeashed = false;
    m_leashHolderUuid = std::nullopt;
    m_leashFencePos = std::nullopt;
    m_leashDelayInfo.targetUuid = std::nullopt;
    m_leashDelayInfo.fencePos = std::nullopt;
}

void MobEntity::dropLeash()
{
    clearLeash();

    // 掉落拴绳物品
    if (m_world != nullptr && m_world->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
        if (Items::LEAD != nullptr) {
            ItemStack stack(*Items::LEAD, 1);
            math::Random& rng = m_world->getRandom();
            ItemDropHelper::spawnItemAtEntity(this, stack, 0.5f, rng, ItemDropHelper::DEFAULT_PICKUP_DELAY);
        }
    }
}

void MobEntity::tickLeash()
{
    // 只在服务端且被拴住时执行
    if (!m_isLeashed || m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 恢复延迟加载的拴绳数据（对应 MC Java 的 Leashable.restoreLeashFromSave）
    // 当实体从 NBT 加载时，拴绳的目标可能尚未就绪，需要每 tick 尝试解析。
    if (m_leashDelayInfo.targetUuid.has_value() || m_leashDelayInfo.fencePos.has_value()) {
        if (m_leashDelayInfo.targetUuid.has_value()) {
            // 拴在实体上：通过 UUID 索引进行 O(1) 查找
            Entity* holder = m_world->getEntityByUuid(*m_leashDelayInfo.targetUuid);
            if (holder != nullptr && holder->isAlive()) {
                // 找到目标实体，完成延迟绑定并广播给客户端
                m_world->broadcastSetEntityLink(id(), holder->id());
                m_leashDelayInfo.targetUuid = std::nullopt;
                m_leashDelayInfo.fencePos = std::nullopt;
                m_leashDelayInfo.resolveTicks = 0;
            } else if (++m_leashDelayInfo.resolveTicks >= 100) {
                // 超过 100 tick 仍无法绑定，掉落拴绳（与 MC Java 一致）
                dropLeash();
            }
        } else if (m_leashDelayInfo.fencePos.has_value()) {
            // 拴在栅栏上：尝试查找或创建栅栏位置的拴绳结实体并广播给客户端
            auto* knot = entity::LeashKnotEntity::getOrCreateKnot(*m_world, *m_leashDelayInfo.fencePos);
            if (knot != nullptr) {
                m_world->broadcastSetEntityLink(id(), knot->id());
                m_leashDelayInfo.fencePos = std::nullopt;
                m_leashDelayInfo.resolveTicks = 0;
            } else if (++m_leashDelayInfo.resolveTicks >= 100) {
                // 超过 100 tick 仍无法绑定，掉落拴绳
                dropLeash();
            }
        }
        return;
    }

    // 获取拴绳持有者位置
    Vector3d holderPos;
    bool holderValid = false;

    if (m_leashHolderUuid.has_value()) {
        // 拴在实体上：通过 UUID 索引进行 O(1) 查找，替代 getEntitiesInRange 的 O(n) 遍历
        Entity* holder = m_world->getEntityByUuid(*m_leashHolderUuid);
        if (holder != nullptr && holder->isAlive()) {
            holderPos = Vector3d(holder->x(), holder->y(), holder->z());
            holderValid = true;
        }
    } else if (m_leashFencePos.has_value()) {
        // 拴在栅栏上
        holderPos = Vector3d(m_leashFencePos->x + 0.5, m_leashFencePos->y + 0.5, m_leashFencePos->z + 0.5);
        holderValid = true;
    }

    if (!holderValid) {
        // 无法找到持有者，掉落拴绳
        dropLeash();
        return;
    }

    // 计算与持有者的距离
    Vector3d mobPos(m_builtIn.stateVector->m_pos.x, m_builtIn.stateVector->m_pos.y, m_builtIn.stateVector->m_pos.z);
    f64 distance = mobPos.distance(holderPos);

    // 拴绳断裂距离
    constexpr f64 LEASH_SNAP_DISTANCE = 12.0;
    // 拴绳弹性距离
    constexpr f64 LEASH_ELASTIC_DISTANCE = 6.0;

    if (distance > LEASH_SNAP_DISTANCE) {
        // 距离超过断裂距离，拴绳断裂
        if (m_world != nullptr) {
            playSound(SoundEvents::ENTITY_LEASH_KNOT_BREAK, 1.0f, 1.0f);
        }
        dropLeash();
        return;
    }

    if (distance > LEASH_ELASTIC_DISTANCE) {
        // 距离超过弹性距离，施加拉力
        // 计算从实体指向持有者的方向
        Vector3d direction = holderPos - mobPos;
        f64 length = direction.length();
        if (length > 0.001) {
            direction = direction / length;
        }

        // 弹性拉力：距离超过弹性距离的部分作为力
        f64 force = (distance - LEASH_ELASTIC_DISTANCE) * 0.5;
        Vector3d deltaMovement(direction.x * force * 0.8, direction.y * force * 0.2, direction.z * force * 0.8);

        // 施加拉力（添加到速度向量）
        m_builtIn.velocity->m_velocity.x += static_cast<f32>(deltaMovement.x);
        m_builtIn.velocity->m_velocity.y += static_cast<f32>(deltaMovement.y);
        m_builtIn.velocity->m_velocity.z += static_cast<f32>(deltaMovement.z);
    }
}

// ============================================================================
// NBT 序列化
// ============================================================================

void MobEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    LivingEntity::addAdditionalSaveData(tag);

    // CanPickUpLoot (byte) - 是否可以拾取物品
    tag.put(nbt_keys::CAN_PICK_UP_LOOT, static_cast<i8>(m_canPickUpLoot ? 1 : 0));

    // PersistenceRequired (byte) - 是否需要持久化
    tag.put(nbt_keys::PERSISTENCE_REQUIRED, static_cast<i8>(m_persistenceRequired ? 1 : 0));

    // LeftHanded (byte) - 左撇子
    tag.put(nbt_keys::LEFT_HANDED, static_cast<i8>(m_primaryHand == HandSide::Left ? 1 : 0));

    // NoAI (byte) - 无 AI
    tag.put(nbt_keys::NO_AI, static_cast<i8>(m_aiEnabled ? 0 : 1));

    // DropChances（compound，仅包含非默认值）
    // 新格式：drop_chances compound，与 MC Java DropChances.filterDefaultValues 一致。
    // 旧格式 HandDropChances/ArmorDropChances（float list）已废弃，不再写入，
    // 但加载时仍兼容旧格式以保证存档兼容性。
    {
        nbt::tags::compound_tag dropChancesTag;
        // 装备槽位名称映射
        static constexpr struct {
            EquipmentSlot slot;
            const char* name;
        } slotNames[] = {
            {EquipmentSlot::MainHand, "mainhand"},
            {EquipmentSlot::OffHand, "offhand"},
            {EquipmentSlot::Feet, "feet"},
            {EquipmentSlot::Legs, "legs"},
            {EquipmentSlot::Chest, "chest"},
            {EquipmentSlot::Head, "head"},
            {EquipmentSlot::Body, "body"},
            {EquipmentSlot::Saddle, "saddle"},
        };
        for (const auto& entry : slotNames) {
            f32 chance = m_equipmentDropChances[static_cast<size_t>(entry.slot)];
            // 仅写入非默认值（与 MC DropChances.filterDefaultValues 一致）
            if (chance != DEFAULT_EQUIPMENT_DROP_CHANCE) {
                dropChancesTag.put(entry.name, chance);
            }
        }
        if (!dropChancesTag.value.empty()) {
            tag.value.emplace(
                nbt_keys::DROP_CHANCES, std::make_unique<nbt::tags::compound_tag>(std::move(dropChancesTag)));
        }
    }

    // Leash (compound) - 拴绳数据
    // 格式：Leash = {UUIDMost: long, UUIDLeast: long}（拴在实体上）
    //   或  Leash = {X: int, Y: int, Z: int}（拴在栅栏柱上）
    if (m_isLeashed) {
        nbt::tags::compound_tag leashTag;
        if (m_leashHolderUuid.has_value()) {
            // 拴在实体上：写入 UUIDMost + UUIDLeast
            nbt_helper::putUuid(leashTag, *m_leashHolderUuid);
        } else if (m_leashFencePos.has_value()) {
            // 拴在栅栏柱上：写入 X + Y + Z
            leashTag.put(nbt_keys::LEASH_X, m_leashFencePos->x);
            leashTag.put(nbt_keys::LEASH_Y, m_leashFencePos->y);
            leashTag.put(nbt_keys::LEASH_Z, m_leashFencePos->z);
        }
        if (!leashTag.value.empty()) {
            tag.value.emplace(nbt_keys::LEASH, std::make_unique<nbt::tags::compound_tag>(std::move(leashTag)));
        }
    }

    // DeathLootTable / DeathLootTableSeed
    // 仅在有自定义掉落表时写入，否则使用实体类型的默认掉落表。
    if (m_deathLootTable.has_value()) {
        tag.put(nbt_keys::DEATH_LOOT_TABLE, std::string(*m_deathLootTable));
        if (m_lootTableSeed != 0) {
            tag.put(nbt_keys::DEATH_LOOT_TABLE_SEED, m_lootTableSeed);
        }
    }
}

Result<void> MobEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用基类实现
    MC_TRY(LivingEntity::readAdditionalSaveData(tag));

    // CanPickUpLoot (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::CAN_PICK_UP_LOOT)) {
        m_canPickUpLoot = *val;
    }

    // PersistenceRequired (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::PERSISTENCE_REQUIRED)) {
        m_persistenceRequired = *val;
    }

    // LeftHanded (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::LEFT_HANDED)) {
        m_primaryHand = *val ? HandSide::Left : HandSide::Right;
    }

    // NoAI (byte)
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::NO_AI)) {
        m_aiEnabled = !(*val);
    }

    // HandDropChances / ArmorDropChances / drop_chances
    // 读取时优先使用新格式 drop_chances（compound），然后回退到旧格式。
    // 新格式中的槽位会覆盖旧格式的值，未指定的槽位保持默认值。
    {
        bool readNewFormat = false;

        // 新格式：drop_chances（compound）
        if (auto* dropChancesCompound = nbt_helper::tryGetCompound(tag, nbt_keys::DROP_CHANCES)) {
            readNewFormat = true;
            // 装备槽位名称映射
            static constexpr struct {
                const char* name;
                EquipmentSlot slot;
            } slotNames[] = {
                {"mainhand", EquipmentSlot::MainHand},
                {"offhand", EquipmentSlot::OffHand},
                {"feet", EquipmentSlot::Feet},
                {"legs", EquipmentSlot::Legs},
                {"chest", EquipmentSlot::Chest},
                {"head", EquipmentSlot::Head},
                {"body", EquipmentSlot::Body},
                {"saddle", EquipmentSlot::Saddle},
            };
            for (const auto& entry : slotNames) {
                if (auto chance = nbt_helper::tryGetFloat(*dropChancesCompound, entry.name)) {
                    m_equipmentDropChances[static_cast<size_t>(entry.slot)] = *chance;
                }
            }
        }

        // 旧格式：HandDropChances（float list，长度 2）
        if (auto* handList = nbt_helper::tryGetList(tag, nbt_keys::HAND_DROP_CHANCES)) {
            if (handList->element_id() == nbt::TagId::Float) {
                auto& floatList = dynamic_cast<const nbt::tags::float_list_tag&>(*handList);
                if (floatList.value.size() >= 1) {
                    // 仅在新格式未设置时使用旧格式的值
                    if (!readNewFormat ||
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::MainHand)] ==
                            DEFAULT_EQUIPMENT_DROP_CHANCE) {
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::MainHand)] = floatList.value[0];
                    }
                }
                if (floatList.value.size() >= 2) {
                    if (!readNewFormat ||
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::OffHand)] ==
                            DEFAULT_EQUIPMENT_DROP_CHANCE) {
                        m_equipmentDropChances[static_cast<size_t>(EquipmentSlot::OffHand)] = floatList.value[1];
                    }
                }
            }
        }

        // 旧格式：ArmorDropChances（float list，长度 4）
        if (auto* armorList = nbt_helper::tryGetList(tag, nbt_keys::ARMOR_DROP_CHANCES)) {
            if (armorList->element_id() == nbt::TagId::Float) {
                auto& floatList = dynamic_cast<const nbt::tags::float_list_tag&>(*armorList);
                constexpr std::array<EquipmentSlot, 4> armorSlots = {
                    EquipmentSlot::Feet, EquipmentSlot::Legs, EquipmentSlot::Chest, EquipmentSlot::Head};
                for (size_t i = 0; i < armorSlots.size() && i < floatList.value.size(); ++i) {
                    if (!readNewFormat ||
                        m_equipmentDropChances[static_cast<size_t>(armorSlots[i])] == DEFAULT_EQUIPMENT_DROP_CHANCE) {
                        m_equipmentDropChances[static_cast<size_t>(armorSlots[i])] = floatList.value[i];
                    }
                }
            }
        }
    }

    // Leash (compound) - 拴绳数据
    // 读取拴绳绑定信息，支持实体 UUID 和栅栏柱坐标两种格式
    {
        auto* leashCompound = nbt_helper::tryGetCompound(tag, nbt_keys::LEASH);
        if (leashCompound != nullptr) {
            // 检查是拴在实体上（UUIDMost + UUIDLeast）还是栅栏柱上（X + Y + Z）
            auto uuidMost = nbt_helper::tryGetLong(*leashCompound, nbt_keys::LEASH_UUID_MOST);
            auto uuidLeast = nbt_helper::tryGetLong(*leashCompound, nbt_keys::LEASH_UUID_LEAST);

            if (uuidMost.has_value() && uuidLeast.has_value()) {
                // 拴在实体上
                std::string uuid = nbt_helper::getUuid(*leashCompound);
                if (!uuid.empty()) {
                    m_isLeashed = true;
                    m_leashHolderUuid = uuid;
                    m_leashDelayInfo.targetUuid = uuid;
                    // 延迟绑定：目标实体可能尚未加载到世界中，
                    // tickLeash() 会在每 tick 中尝试通过 UUID 查找目标实体
                    // 并完成实际绑定（对应 MC Java 的 restoreLeashFromSave）。
                }
            } else {
                // 拴在栅栏柱上
                auto x = nbt_helper::tryGetInt(*leashCompound, nbt_keys::LEASH_X);
                auto y = nbt_helper::tryGetInt(*leashCompound, nbt_keys::LEASH_Y);
                auto z = nbt_helper::tryGetInt(*leashCompound, nbt_keys::LEASH_Z);
                if (x.has_value() && y.has_value() && z.has_value()) {
                    m_leashDelayInfo.fencePos = BlockPos(*x, *y, *z);
                    // 拴在栅栏柱上的情况可以立即绑定
                    setLeashedToFence(*m_leashDelayInfo.fencePos);
                }
            }
        } else if (m_isLeashed) {
            // 如果之前被拴住，但 NBT 中没有 Leash 数据，则解除拴绳
            clearLeash();
        }
    }

    // DeathLootTable / DeathLootTableSeed
    if (auto lootTableStr = nbt_helper::tryGetString(tag, nbt_keys::DEATH_LOOT_TABLE)) {
        m_deathLootTable = *lootTableStr;
    } else {
        m_deathLootTable = std::nullopt;
    }

    if (auto seed = nbt_helper::tryGetLong(tag, nbt_keys::DEATH_LOOT_TABLE_SEED)) {
        m_lootTableSeed = *seed;
    } else {
        m_lootTableSeed = 0;
    }

    return Result<void>::ok();
}

// ============================================================================
// 拾取物品 (CanPickUpLoot)
// ============================================================================

Vector3i MobEntity::getPickupReach() const
{
    // 对齐 vanilla Mob.ITEM_PICKUP_REACH = Vec3i(1, 0, 1)——仅水平 ±1 格、Y 不扩展。
    return Vector3i(1, 0, 1);
}

bool MobEntity::wantsToPickUp(const ItemStack& itemStack) const
{
    // 对齐 vanilla Mob.wantsToPickUp 默认实现 = canHoldItem。
    return canHoldItem(itemStack);
}

bool MobEntity::canHoldItem(const ItemStack& itemStack) const
{
    // 简化实现：物品非空即视为可持有（基类无装备槽语义时保守允许）。
    // TODO: vanilla Mob.canHoldItem 默认检查装备槽可替换性（getEquipmentSlotForItem +
    //   canReplaceCurrentItem + dropChances 守卫），待装备拾取链路完整补全后对齐。
    //   子类（如 FoxEntity）覆写为手持物品语义，不依赖本基类实现。
    return !itemStack.isEmpty();
}

void MobEntity::pickUpItem(ItemEntity& itemEntity)
{
    // 基类默认实现：装备槽拾取语义（对齐 vanilla Mob.pickUpItem→equipItemIfPossible）。
    // TODO: 完整装备拾取链路（equipItemIfPossible/canReplaceCurrentItem/getEquipmentSlotForItem
    //   /onItemPickup/take）尚未实现，基类暂不处理。子类（如 FoxEntity）覆写 pickUpItem
    //   实现具体拾取逻辑（手持物品）。待装备槽拾取链路补全后在此对齐 vanilla。
    (void)itemEntity;
}

// ============================================================================
// 生成初始化
// ============================================================================

void MobEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    (void)world;
    (void)spawnReason;

    // 注意：Mob 基类不设置 canPickUpLoot（对齐 MC Java 1.21.11 Mob.finalizeSpawn，其只设置
    //   FOLLOW_RANGE 修饰符与左撇子，不调 setCanPickUpLoot）。Java 中拾取能力由各子类自行决定：
    //   - Villager 构造函数 setCanPickUpLoot(true)（Villager.java:196）
    //   - Fox 构造函数 setCanPickUpLoot(true)（Fox.java:148）
    //   - Zombie/Husk/AbstractSkeleton 在各自 finalizeSpawn 按 0.55*specialMultiplier 概率设置
    //   - Piglin/Pillager 构造函数 setCanPickUpLoot(true)
    //   其他 mob 默认 canPickUpLoot=false（不拾取）。
    // 此前基类在此随机 setCanPickUpLoot(true) 是偏差——会覆盖 Villager 构造函数确定的 true
    // （导致村民拾取不确定），也会让本不该拾取的 mob 随机获得拾取能力。已移除，由子类各自对齐。
    // TODO: Piglin/Pillager 的 setCanPickUpLoot(true) 待相关实体实现时补齐。
    (void)difficulty;

    // 填充默认装备（基于难度）
    populateDefaultEquipmentSlots(getRandom(), difficulty);

    // 附魔默认装备（基于难度）
    populateDefaultEquipmentEnchantments(getRandom(), difficulty);
}

void MobEntity::populateDefaultEquipmentSlots(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty)
{
    f32 specialMultiplier = difficulty.getSpecialMultiplier();

    // 概率 = 0.15 * specialMultiplier，决定是否穿戴任何护甲
    if (random.nextFloat() >= 0.15f * specialMultiplier) {
        return;
    }

    // 随机生成护甲等级 (0~2 为基础，最多+3)
    i32 armorLevel = random.nextInt(3);
    for (i32 j = 1; j <= 3; ++j) {
        if (random.nextFloat() < 0.1087f) {
            ++armorLevel;
        }
    }

    // 根据 Hard 难度决定中断概率
    // Hard: 0.1 (更可能填满全身护甲)
    // 其他: 0.25
    f32 skipChance = (difficulty.getDifficulty() == Difficulty::Hard) ? 0.1f : 0.25f;

    // 按 Head -> Chest -> Legs -> Feet 顺序填充护甲
    // 护甲生成顺序
    static constexpr EquipmentSlot armorSlots[] = {
        EquipmentSlot::Head,
        EquipmentSlot::Chest,
        EquipmentSlot::Legs,
        EquipmentSlot::Feet,
    };

    bool firstSlot = true;
    for (EquipmentSlot slot : armorSlots) {
        // 第一个槽位必定尝试填充，之后的槽位有一定概率跳过
        if (!firstSlot && random.nextFloat() < skipChance) {
            break;
        }
        firstSlot = false;

        // 只在槽位为空时填充
        if (getEquipment(slot).isEmpty()) {
            const Item* item = getEquipmentForSlot(slot, armorLevel);
            if (item != nullptr) {
                setEquipment(slot, ItemStack(*item, 1));
            }
        }
    }
}

void MobEntity::populateDefaultEquipmentEnchantments(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty)
{
    f32 specialMultiplier = difficulty.getSpecialMultiplier();

    // 附魔主手武器（概率 = 0.25 * specialMultiplier）
    enchantSpawnedWeapon(random, difficulty, specialMultiplier);

    // 附魔护甲（每个护甲槽位独立检定，概率 = 0.5 * specialMultiplier）
    enchantSpawnedArmor(random, difficulty, specialMultiplier);
}

const Item* MobEntity::getEquipmentForSlot(EquipmentSlot slot, i32 armorLevel)
{
    // 对应 Minecraft 原版 Mob.getEquipmentForSlot()
    // armorLevel: 0=皮革, 1=铜, 2=金, 3=锁链, 4=铁, 5=钻石

    switch (slot) {
        case EquipmentSlot::Head:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_HELMET;
                case 1:
                    return Items::COPPER_HELMET;
                case 2:
                    return Items::GOLDEN_HELMET;
                case 3:
                    return Items::CHAINMAIL_HELMET;
                case 4:
                    return Items::IRON_HELMET;
                case 5:
                    return Items::DIAMOND_HELMET;
                default:
                    return nullptr;
            }
        case EquipmentSlot::Chest:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_CHESTPLATE;
                case 1:
                    return Items::COPPER_CHESTPLATE;
                case 2:
                    return Items::GOLDEN_CHESTPLATE;
                case 3:
                    return Items::CHAINMAIL_CHESTPLATE;
                case 4:
                    return Items::IRON_CHESTPLATE;
                case 5:
                    return Items::DIAMOND_CHESTPLATE;
                default:
                    return nullptr;
            }
        case EquipmentSlot::Legs:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_LEGGINGS;
                case 1:
                    return Items::COPPER_LEGGINGS;
                case 2:
                    return Items::GOLDEN_LEGGINGS;
                case 3:
                    return Items::CHAINMAIL_LEGGINGS;
                case 4:
                    return Items::IRON_LEGGINGS;
                case 5:
                    return Items::DIAMOND_LEGGINGS;
                default:
                    return nullptr;
            }
        case EquipmentSlot::Feet:
            switch (armorLevel) {
                case 0:
                    return Items::LEATHER_BOOTS;
                case 1:
                    return Items::COPPER_BOOTS;
                case 2:
                    return Items::GOLDEN_BOOTS;
                case 3:
                    return Items::CHAINMAIL_BOOTS;
                case 4:
                    return Items::IRON_BOOTS;
                case 5:
                    return Items::DIAMOND_BOOTS;
                default:
                    return nullptr;
            }
        default:
            return nullptr;
    }
}

void MobEntity::enchantSpawnedWeapon(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty, f32 specialMultiplier)
{
    (void)difficulty;
    ItemStack mainHand = getEquipment(EquipmentSlot::MainHand);
    if (!mainHand.isEmpty() && random.nextFloat() < 0.25f * specialMultiplier) {
        // 附魔等级范围：5~17
        i32 level = 5 + random.nextInt(13);
        mainHand = item::enchant::EnchantmentHelper::addRandomEnchantment(random, mainHand, level, false);
        setEquipment(EquipmentSlot::MainHand, mainHand);
    }
}

void MobEntity::enchantSpawnedArmor(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty, f32 specialMultiplier)
{
    (void)difficulty;
    // 对每个护甲槽位独立检定
    static constexpr EquipmentSlot armorSlots[] = {
        EquipmentSlot::Head,
        EquipmentSlot::Chest,
        EquipmentSlot::Legs,
        EquipmentSlot::Feet,
    };

    for (EquipmentSlot slot : armorSlots) {
        ItemStack armor = getEquipment(slot);
        if (!armor.isEmpty() && random.nextFloat() < 0.5f * specialMultiplier) {
            // 附魔等级范围：5~17
            i32 level = 5 + random.nextInt(13);
            armor = item::enchant::EnchantmentHelper::addRandomEnchantment(random, armor, level, false);
            setEquipment(slot, armor);
        }
    }
}

} // namespace mc
