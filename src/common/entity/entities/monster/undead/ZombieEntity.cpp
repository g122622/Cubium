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

#include "ZombieEntity.hpp"
#include "DrownedEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/interact/BreakDoorGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/AttributeModifier.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/passive/golem/IronGolemEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/SpecialDates.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/spawn/IWorldSpawnAdapter.hpp"

namespace mc {

// 使用序列化命名空间
using namespace entity::serialization;

// 属性修饰符ID常量
namespace {
const std::string BABY_SPEED_BOOST_ID = "baby";
const std::string REINFORCEMENT_CALLER_CHARGE_ID = "reinforcement_caller_charge";
const std::string ZOMBIE_REINFORCEMENT_CALLEE_CHARGE_ID = "reinforcement_callee_charge";
const std::string LEADER_ZOMBIE_BONUS_ID = "leader_zombie_bonus";
const std::string ZOMBIE_RANDOM_SPAWN_BONUS_ID = "zombie_random_spawn_bonus";
const std::string RANDOM_SPAWN_BONUS_ID = "random_spawn_bonus";
} // namespace

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = MonsterEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点：子类 ClassRegisterGuard 沿父链查找最高 id
// 时穿过本类（lastAssignedId=-1）直达父链已分配 id 的基类，子类首字段续接其后。
const entity::EntityClassInfo& ZombieEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"ZombieEntity", &MonsterEntity::classInfo()};
    return s_classInfo;
}

ZombieEntity::ZombieEntity(EntityInstanceId id)
    : MonsterEntity(id)
{
    // 僵尸可以在阳光下燃烧
    setBurnsInDaylight(true);

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> ZombieEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ZombieEntity>(EntityInstanceId(0));
}

std::optional<ResourceLocation> ZombieEntity::getAmbientSound() const
{
    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> ZombieEntity::getHurtSound(DamageSource& /*source*/) const
{
    return makeSoundEventId("hurt");
}

std::optional<ResourceLocation> ZombieEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

std::optional<ResourceLocation> ZombieEntity::getStepSound() const
{
    return makeSoundEventId("step");
}

void ZombieEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    // 僵尸播放固定的脚步声，忽略脚下方块类型
    auto sound = getStepSound();
    if (sound) {
        playSound(*sound, 0.15f, 1.0f);
    }
}

void ZombieEntity::setBreakDoorsAbility(bool canBreak)
{
    if (m_canBreakDoors == canBreak) {
        return;
    }

    m_canBreakDoors = canBreak;

    // 对应 MC Java 版 Zombie.setCanBreakDoors：
    // 破门能力与导航器开门能力绑定
    auto* nav = navigator();
    if (nav) {
        nav->setCanOpenDoors(canBreak);
    }

    // 动态添加/移除破门目标
    if (canBreak && m_breakDoorGoal == nullptr) {
        m_breakDoorGoal =
            new entity::ai::goal::BreakDoorGoal(this, entity::ai::goal::defaultDoorBreakDifficultyPredicate());
        m_goalSelector.addGoal(1, m_breakDoorGoal);
    } else if (!canBreak && m_breakDoorGoal != nullptr) {
        m_goalSelector.removeGoal(m_breakDoorGoal);
        m_breakDoorGoal = nullptr;
    }
}

void ZombieEntity::startDrowning(i32 conversionTime)
{
    // 开始溺水转化
    m_converting = true;
    m_conversionTime = conversionTime;
}

void ZombieEntity::setBaby(bool baby)
{
    if (m_isBaby == baby) {
        return;
    }

    m_isBaby = baby;
    refreshDimensions();

    // 婴儿僵尸速度加成：+50%（MultiplyBase 操作）
    if (baby) {
        entity::attribute::AttributeModifier modifier(
            BABY_SPEED_BOOST_ID, "Baby speed boost", 0.5, entity::attribute::Operation::MultiplyBase);
        m_attributes.addModifier(entity::attribute::Attributes::MOVEMENT_SPEED, modifier);
    } else {
        m_attributes.removeModifier(entity::attribute::Attributes::MOVEMENT_SPEED, BABY_SPEED_BOOST_ID);
    }
}

void ZombieEntity::trySummonReinforcements(LivingEntity* explicitTarget)
{
    // 增援召唤的唯一入口
    // 检查前置条件：世界引用、难度、概率
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 必须在困难模式下
    if (!entity::combat::DifficultyHelper::canZombieReinforce(worldPtr->difficulty())) {
        return;
    }

    // 增援概率检查
    f64 spawnChance = m_attributes.getValue(entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS);
    if (getRandom().nextDouble() >= spawnChance) {
        return;
    }

    // 确定攻击目标：优先使用显式目标，其次使用当前攻击目标
    LivingEntity* target = explicitTarget;
    if (target == nullptr) {
        target = attackTarget();
    }
    if (target == nullptr) {
        return;
    }

    _trySpawnReinforcement(*worldPtr, *target);
}

bool ZombieEntity::canSummonReinforcements() const
{
    return m_attributes.getValue(entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS) > 0.0;
}

void ZombieEntity::_trySpawnReinforcement(IWorld& world, LivingEntity& target)
{
    // 检查 doMobSpawning 游戏规则
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_MOB_SPAWNING)) {
        return;
    }

    // 获取当前实体类型（用于创建同类型的增援僵尸）
    const std::string entityTypeId = getTypeId();
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* entityType = registry.getType(entityTypeId);
    if (entityType == nullptr || !entityType->canSummon()) {
        return;
    }

    math::Random& rng = getRandom();

    // 僵尸位置（取整）
    i32 baseX = math::floorTo<i32>(m_position.x);
    i32 baseY = math::floorTo<i32>(m_position.y);
    i32 baseZ = math::floorTo<i32>(m_position.z);

    // 创建 ISpawnWorldReader 适配器，用于 EntitySpawnPlacementRegistry 的生成位置检查
    world::spawn::IWorldSpawnAdapter spawnWorldAdapter(world);

    // 最多尝试 50 次寻找有效生成位置
    for (i32 attempt = 0; attempt < REINFORCEMENT_ATTEMPTS; ++attempt) {
        // MC 1.21.11 Zombie.hurtServer() 增援位置计算：
        // 各轴偏移 = nextInt(7, 40) * nextInt(-1, 1)
        // nextInt(-1, 1) 产生 {-1, 0, 1}，所以偏移可以为 0（同轴位置）
        i32 offsetX = rng.nextInt(REINFORCEMENT_RANGE_MIN, REINFORCEMENT_RANGE_MAX) * rng.nextInt(-1, 1);
        i32 offsetY = rng.nextInt(REINFORCEMENT_RANGE_MIN, REINFORCEMENT_RANGE_MAX) * rng.nextInt(-1, 1);
        i32 offsetZ = rng.nextInt(REINFORCEMENT_RANGE_MIN, REINFORCEMENT_RANGE_MAX) * rng.nextInt(-1, 1);

        i32 spawnX = baseX + offsetX;
        i32 spawnY = baseY + offsetY;
        i32 spawnZ = baseZ + offsetZ;

        BlockPos spawnPos(spawnX, spawnY, spawnZ);

        // 检查生成位置是否在世界范围内
        if (!world.isWithinWorldBounds(spawnPos)) {
            continue;
        }

        // 使用 EntitySpawnPlacementRegistry 检查生成位置有效性
        // 对应 MC 原版 SpawnPlacements.isSpawnPositionOk() 和 SpawnPlacements.checkSpawnRules()
        // - 对于僵尸：PlacementType::OnGround，检查脚底支撑 + 生成位和上方位可通行
        // - 对于溺尸：PlacementType::InWater，检查生成位在水中 + 下方在水中 + 上方非实心
        // - 谓词检查：canMonsterSpawnInLightPredicate（目前为空，光照检查在 NaturalSpawner 中进行）
        Vector3i spawnVec(spawnX, spawnY, spawnZ);
        if (!world::spawn::EntitySpawnPlacementRegistry::canSpawnEntity(
                entityTypeId, spawnWorldAdapter, world::spawn::SpawnReason::Reinforcement, spawnVec, rng)) {
            continue;
        }

        // 检查附近7格内无存活玩家
        // 对应 MC 原版 ServerLevel.hasNearbyAlivePlayer(x, y, z, 7.0)
        Player* nearbyPlayer = world.getClosestPlayer(
            Vector3(static_cast<f64>(spawnX) + 0.5, static_cast<f64>(spawnY), static_cast<f64>(spawnZ) + 0.5), 7.0f);
        if (nearbyPlayer != nullptr && nearbyPlayer->isAlive()) {
            continue;
        }

        // 创建增援僵尸实体
        std::unique_ptr<Entity> newEntity = entityType->create(&world);
        if (newEntity == nullptr) {
            continue;
        }

        auto* reinforcement = dynamic_cast<ZombieEntity*>(newEntity.get());
        if (reinforcement == nullptr) {
            continue;
        }

        // 设置位置
        reinforcement->setPosition(
            Vector3(static_cast<f64>(spawnX) + 0.5, static_cast<f64>(spawnY), static_cast<f64>(spawnZ) + 0.5));

        // 检查实体碰撞和方块碰撞
        // 对应 MC 原版 ServerLevel.noCollision(zombie) 和 ServerLevel.isUnobstructed(zombie)
        AxisAlignedBB reinBox = reinforcement->boundingBox();
        if (world.hasEntityCollision(reinBox, this)) {
            continue;
        }
        if (world.hasBlockCollision(reinBox)) {
            continue;
        }

        // 检查不在液体中（溺尸可以在水中生成）
        // 对应 MC 原版 zombie.canSpawnInLiquids() || !level.containsAnyLiquid(zombie.getBoundingBox())
        if (!reinforcement->canSpawnInLiquids() && world.containsAnyLiquid(reinBox)) {
            continue;
        }

        // 初始化生成属性（使用位置感知的区域难度）
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(world,
            BlockPos(static_cast<i32>(std::floor(reinforcement->x())),
                static_cast<i32>(reinforcement->y()),
                static_cast<i32>(std::floor(reinforcement->z()))));
        reinforcement->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Reinforcement);

        // 设置攻击目标
        reinforcement->setAttackTarget(&target);

        // 生成到世界
        EntityInstanceId spawnedId = world.spawnEntity(std::move(newEntity));
        if (spawnedId == 0) {
            continue;
        }

        // 召唤成功：给召唤者施加 caller charge 修饰符
        // MC 原版：如果已有修饰符则累加，否则新建
        f64 callerChargeValue = REINFORCEMENT_CALLEE_CHARGE; // -0.05
        if (m_attributes.hasModifier(
                entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, REINFORCEMENT_CALLER_CHARGE_ID)) {
            f64 existingValue = m_attributes.getModifierValue(
                entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, REINFORCEMENT_CALLER_CHARGE_ID, 0.0);
            callerChargeValue = existingValue + REINFORCEMENT_CALLEE_CHARGE;
            m_attributes.removeModifier(
                entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, REINFORCEMENT_CALLER_CHARGE_ID);
        }
        entity::attribute::AttributeModifier callerModifier(REINFORCEMENT_CALLER_CHARGE_ID,
            "Reinforcement caller charge",
            callerChargeValue,
            entity::attribute::Operation::Addition);
        m_attributes.addModifier(entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, callerModifier);

        // 给被召唤的增援僵尸施加 callee charge 修饰符（防止连锁增援）
        // 注意：此时 newEntity 已被 spawnEntity 移走，需要通过 ID 查找
        Entity* spawnedEntity = world.getEntity(spawnedId);
        if (spawnedEntity != nullptr) {
            auto* spawnedZombie = dynamic_cast<ZombieEntity*>(spawnedEntity);
            if (spawnedZombie != nullptr) {
                entity::attribute::AttributeModifier calleeModifier(ZOMBIE_REINFORCEMENT_CALLEE_CHARGE_ID,
                    "Reinforcement callee charge",
                    REINFORCEMENT_CALLEE_CHARGE,
                    entity::attribute::Operation::Addition);
                spawnedZombie->attributes().addModifier(
                    entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, calleeModifier);
            }
        }

        // 生成成功，退出循环
        return;
    }
}

bool ZombieEntity::hurt(DamageSource& source, f32 amount)
{
    if (!MonsterEntity::hurt(source, amount)) {
        return false;
    }

    // 增援召唤逻辑
    // 对应 MC 1.21.11 Zombie.hurtServer() 中的增援逻辑
    // hurt() 中的特殊处理：优先使用伤害来源实体作为增援的攻击目标
    LivingEntity* explicitTarget = nullptr;
    Entity* sourceEntity = source.getEntity();
    if (sourceEntity != nullptr) {
        explicitTarget = dynamic_cast<LivingEntity*>(sourceEntity);
    }
    trySummonReinforcements(explicitTarget);

    return true;
}

bool ZombieEntity::attackEntityAsMob(LivingEntity& target)
{
    // 首先调用父类方法进行基础攻击
    if (!MonsterEntity::attackEntityAsMob(target)) {
        return false;
    }

    // 燃烧传递逻辑
    // 如果僵尸正在燃烧，且主手为空，且目标是生物
    // 则有概率点燃目标
    if (isOnFire() && getMainHandItem().isEmpty()) {
        IWorld* worldPtr = world();
        if (worldPtr) {
            // 获取区域难度（使用位置感知的区域难度，与 MC 原版一致）
            entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*worldPtr,
                BlockPos(static_cast<i32>(std::floor(x())), static_cast<i32>(y()), static_cast<i32>(std::floor(z()))));
            f32 effectiveDifficulty = difficultyInstance.getEffectiveDifficulty();

            // 燃烧概率 = effectiveDifficulty * 0.3
            // 区域难度范围：Easy 0.75~1.375, Normal 1.5~3.5, Hard 2.25~6.75
            // 基础值（新世界）：Easy 0.75, Normal 2.0, Hard 3.0
            math::Random& rng = getRandom();
            if (rng.nextFloat() < effectiveDifficulty * 0.3f) {
                // 燃烧时间 = 2 * effectiveDifficulty（秒）
                f32 fireSeconds = 2.0f * effectiveDifficulty;
                target.igniteForSeconds(fireSeconds);
            }
        }
    }

    return true;
}

void ZombieEntity::tick()
{
    MonsterEntity::tick();

    // 更新溺水转化
    _updateDrowning();
}

void ZombieEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // 僵尸 AI 目标
    // 目标选择器（行为）
    // 优先级 2: 近战攻击
    m_goalSelector.addGoal(2, new entity::ai::goal::MeleeAttackGoal(this, 1.0, false));

    // 注意：MC原版僵尸不会躲避阳光，它们只是直接在阳光下燃烧
    // 骷髅才会使用 FleeSunGoal 和 RestrictSunGoal 来躲避阳光

    // 优先级 7: 避水随机行走
    m_goalSelector.addGoal(7, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 1.0));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(
        8, new entity::ai::goal::LookAtGoal(this, 8.0f, 0.02f, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 8: 随机看向
    m_goalSelector.addGoal(8, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择器（攻击目标）
    // 优先级 1: 被攻击后反击，呼叫同类（但不警醒僵尸猪灵）
    // MC 原版: targetSelector.addGoal(1, HurtByTargetGoal(this).setAlertOthers(ZombifiedPiglin.class))
    {
        auto hurtByTarget = std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true);
        hurtByTarget->setAlertOthers([](const LivingEntity* ally) -> bool {
            // MC 原版使用精确类匹配（== ZombifiedPiglin.class），不警醒僵尸猪灵
            return ally != nullptr && ally->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN;
        });
        m_targetSelector.addGoal(1, std::move(hurtByTarget));
    }

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, new entity::ai::goal::NearestAttackableTargetGoal<Player>(this, true));

    // 优先级 3: 攻击村民（不需要视线检查）
    m_targetSelector.addGoal(3,
        new entity::ai::goal::NearestAttackableTargetGoal<entity::AbstractVillagerEntity>(this,
            false)); // checkSight=false — 僵尸可以穿过墙壁感知村民

    // 优先级 3: 攻击铁傀儡
    m_targetSelector.addGoal(3, new entity::ai::goal::NearestAttackableTargetGoal<IronGolemEntity>(this, true));

    // 优先级 5: 攻击幼年海龟（仅陆地上的幼体，10 tick 间隔检查）
    m_targetSelector.addGoal(5,
        new entity::ai::goal::NearestAttackableTargetGoal<TurtleEntity>(this,
            true, // checkSight
            10,   // reciprocalChance — 每 10 tick 检查一次
            [](const LivingEntity* entity) -> bool {
                // BABY_ON_LAND_SELECTOR: 只攻击陆地上不在水中的幼年海龟
                const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
                if (!turtle) return false;
                return turtle->isChild() && !turtle->isInWater();
            }));
}

void ZombieEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 僵尸属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 2.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 35.0);

    // 注册僵尸增援概率属性
    auto reinforcementAttr = entity::attribute::Attributes::zombieSpawnReinforcements();
    m_attributes.registerAttribute(*reinforcementAttr);
}

void ZombieEntity::convertToDrowned()
{
    // 转化为溺尸
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        return;
    }

    // 1. 从注册表获取溺尸实体类型
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* drownedType = registry.getType("minecraft:drowned");

    // 2. 创建溺尸实体
    std::unique_ptr<Entity> newEntity;
    if (drownedType && drownedType->canSummon()) {
        newEntity = drownedType->create(worldPtr);
    } else {
        // 回退：直接创建实体类
        newEntity = std::make_unique<DrownedEntity>(EntityInstanceId(0));
    }

    DrownedEntity* drowned = dynamic_cast<DrownedEntity*>(newEntity.get());
    if (drowned == nullptr) {
        return;
    }

    // 3. 复制位置和旋转
    drowned->setPosition(m_position);
    drowned->setRotation(m_yaw, m_pitch);

    // 4. 复制生命值（按比例）
    f32 healthRatio = health() / maxHealth();
    drowned->setHealth(drowned->maxHealth() * healthRatio);

    // 5. 复制装备（所有槽位）
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        EquipmentSlot slot = static_cast<EquipmentSlot>(i);
        const ItemStack& equipment = getEquipment(slot);
        if (!equipment.isEmpty()) {
            drowned->setEquipment(slot, equipment);
        }
    }

    // 6. 复制婴儿状态
    drowned->setBaby(m_isBaby);

    // 7. 复制自定义名称
    if (hasCustomName()) {
        drowned->setCustomName(customNameText());
        drowned->setCustomNameVisible(isCustomNameVisible());
    }

    // 8. 复制持久化状态
    if (isNoDespawnRequired()) {
        drowned->enablePersistence();
    }

    // 9. 释放所有权并生成到世界
    newEntity.release();
    EntityInstanceId newId = worldPtr->spawnEntity(std::unique_ptr<Entity>(drowned));

    if (newId == 0) {
        // 生成失败，删除实体
        delete drowned;
        return;
    }

    // 10. 清空原实体装备（防止死亡时掉落）
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        setEquipment(static_cast<EquipmentSlot>(i), ItemStack());
    }

    // 11. 播放转化音效
    playSound(SoundEvents::ENTITY_ZOMBIE_CONVERTED_TO_DROWNED, 1.0f, 1.0f);
    worldPtr->playEvent(world::WorldEvents::ZOMBIE_CONVERT_TO_DROWNED_SOUND,
        BlockPos(static_cast<i32>(m_position.x), static_cast<i32>(m_position.y), static_cast<i32>(m_position.z)),
        0);

    // 12. 重置转化状态
    m_converting = false;
    m_conversionTime = 0;
    m_inWaterTime = 0;

    // 13. 移除原实体
    remove();
}

void ZombieEntity::_updateDrowning()
{
    IWorld* worldPtr = world();
    if (!worldPtr) {
        return;
    }

    // 检查是否在水中
    bool inWater = isInWater();

    if (inWater && shouldDrown()) {
        m_inWaterTime++;

        // 开始转化条件：在水下 30 秒 (600 ticks)
        if (m_inWaterTime >= IN_WATER_TIME_THRESHOLD && !m_converting) {
            startDrowning(CONVERSION_DURATION);
        }
    } else {
        m_inWaterTime = 0;
    }

    // 更新转化计时器
    if (m_converting && m_conversionTime > 0) {
        m_conversionTime--;

        // 转化完成
        if (m_conversionTime <= 0) {
            convertToDrowned();
        }
    }
}

// ============================================================================
// NBT 序列化
// ============================================================================

void ZombieEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    // 先调用基类实现
    MonsterEntity::addAdditionalSaveData(tag);

    // IsBaby (byte/bool) - 是否是婴儿僵尸
    tag.put(nbt_keys::IS_BABY, static_cast<i8>(m_isBaby ? 1 : 0));

    // DrownedConversionTime (i32) - 溺水转化时间
    // 只有正在转化时才保存（>= 0 表示正在转化）
    if (m_converting && m_conversionTime >= 0) {
        tag.put(nbt_keys::DROWNED_CONVERSION_TIME, m_conversionTime);
    }

    // InWaterTime (i32) - 在水中的时间
    // 只有当与溺水转化相关时才保存
    if (m_inWaterTime > 0) {
        tag.put(nbt_keys::IN_WATER_TIME, m_inWaterTime);
    }

    // CanBreakDoors (byte/bool) - 是否可以破门
    if (m_canBreakDoors) {
        tag.put(nbt_keys::CAN_BREAK_DOORS, static_cast<i8>(1));
    }
}

Result<void> ZombieEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    // 先调用基类实现
    MC_TRY(MonsterEntity::readAdditionalSaveData(tag));

    // IsBaby (byte/bool) - 是否是婴儿僵尸
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::IS_BABY)) {
        setBaby(*val);
    }

    // DrownedConversionTime (i32) - 溺水转化时间
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::DROWNED_CONVERSION_TIME)) {
        if (*val >= 0) {
            m_conversionTime = *val;
            m_converting = true;
        }
    }

    // InWaterTime (i32) - 在水中的时间
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::IN_WATER_TIME)) {
        m_inWaterTime = *val;
    }

    // CanBreakDoors (byte/bool) - 是否可以破门
    if (auto val = nbt_helper::tryGetBool(tag, nbt_keys::CAN_BREAK_DOORS)) {
        setBreakDoorsAbility(*val);
    }

    return Result<void>::ok();
}

// ============================================================================
// 生成初始化
// ============================================================================

void ZombieEntity::finalizeSpawn(
    IWorld& world, const entity::combat::DifficultyInstance& difficulty, world::spawn::SpawnReason spawnReason)
{
    // 调用父类 finalizeSpawn，处理拾取物品能力、默认装备和附魔
    MonsterEntity::finalizeSpawn(world, difficulty, spawnReason);

    f32 specialMultiplier = difficulty.getSpecialMultiplier();

    // 设置破门能力：概率 = specialMultiplier * 0.1
    math::Random& rng = getRandom();
    if (rng.nextFloat() < specialMultiplier * 0.1f) {
        setBreakDoorsAbility(true);
    }

    // 僵尸特有的装备已在 populateDefaultEquipmentSlots 覆写中处理

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

    // 处理属性修饰符（随机增援概率、击退抗性、跟随范围、领袖僵尸判定）
    _handleAttributes(rng, specialMultiplier);
}

void ZombieEntity::_handleAttributes(math::Random& rng, f32 specialMultiplier)
{

    // 随机设置增援概率基础值（0.0 ~ 0.1）
    m_attributes.setBaseValue(entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, rng.nextDouble() * 0.1);

    // 击退抗性添加随机生成加成
    entity::attribute::AttributeModifier knockbackModifier(
        RANDOM_SPAWN_BONUS_ID, "Random spawn bonus", rng.nextDouble() * 0.05, entity::attribute::Operation::Addition);
    m_attributes.addModifier(entity::attribute::Attributes::KNOCKBACK_RESISTANCE, knockbackModifier);

    // 跟随范围添加随机生成加成（条件性）
    f64 followRangeBonus = rng.nextDouble() * 1.5 * static_cast<f64>(specialMultiplier);
    if (followRangeBonus > 1.0) {
        entity::attribute::AttributeModifier followModifier(ZOMBIE_RANDOM_SPAWN_BONUS_ID,
            "Zombie random spawn bonus",
            followRangeBonus,
            entity::attribute::Operation::MultiplyTotal);
        m_attributes.addModifier(entity::attribute::Attributes::FOLLOW_RANGE, followModifier);
    }

    // 领袖僵尸判定（概率 = specialMultiplier * 0.05）
    if (rng.nextFloat() < specialMultiplier * 0.05f) {
        // 领袖僵尸增援概率增加
        entity::attribute::AttributeModifier leaderReinforcementModifier(LEADER_ZOMBIE_BONUS_ID,
            "Leader zombie bonus",
            rng.nextDouble() * 0.25 + 0.5,
            entity::attribute::Operation::Addition);
        m_attributes.addModifier(
            entity::attribute::Attributes::ZOMBIE_SPAWN_REINFORCEMENTS, leaderReinforcementModifier);

        // 领袖僵尸最大生命值增加
        entity::attribute::AttributeModifier leaderHealthModifier(LEADER_ZOMBIE_BONUS_ID,
            "Leader zombie bonus",
            rng.nextDouble() * 3.0 + 1.0,
            entity::attribute::Operation::MultiplyTotal);
        m_attributes.addModifier(entity::attribute::Attributes::MAX_HEALTH, leaderHealthModifier);

        // 领袖僵尸可以破门
        setBreakDoorsAbility(true);
    }
}

void ZombieEntity::populateDefaultEquipmentSlots(
    math::Random& random, const entity::combat::DifficultyInstance& difficulty)
{
    // 先调用父类方法：基于难度填充护甲
    MonsterEntity::populateDefaultEquipmentSlots(random, difficulty);

    // 僵尸特有：主手武器
    // Hard 难度: 5% 概率，其他难度: 1% 概率
    f32 weaponChance = (difficulty.getDifficulty() == Difficulty::Hard) ? 0.05f : 0.01f;
    if (random.nextFloat() < weaponChance) {
        i32 weaponType = random.nextInt(6);
        const Item* weapon = nullptr;
        if (weaponType == 0) {
            weapon = Items::IRON_SWORD;
        } else {
            weapon = Items::IRON_SHOVEL;
        }
        if (weapon != nullptr) {
            setEquipment(EquipmentSlot::MainHand, ItemStack(*weapon, 1));
        }
    }
}

} // namespace mc
