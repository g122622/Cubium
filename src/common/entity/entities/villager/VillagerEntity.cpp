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

#include "VillagerEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/ai/brain/sensor/Sensors.hpp"
#include "common/entity/ai/brain/task/Task.hpp"
#include "common/entity/ai/brain/task/tasks/action/ActionTasks.hpp"     // AttackTask
#include "common/entity/ai/brain/task/tasks/interact/InteractTasks.hpp" // 用于 Brain 任务注册
#include "common/entity/ai/brain/task/tasks/movement/MovementTasks.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/PanicGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/special/WanderingTraderGoals.hpp"
#include "common/entity/ai/goal/goals/villager/AvoidHostileGoal.hpp"
#include "common/entity/ai/goal/goals/villager/CongregateGoal.hpp"
#include "common/entity/ai/goal/goals/villager/FarmerWorkGoal.hpp"
#include "common/entity/ai/goal/goals/villager/GatherItemsGoal.hpp"
#include "common/entity/ai/goal/goals/villager/LookAtEntitiesGoal.hpp"
#include "common/entity/ai/goal/goals/villager/LookForJobSiteGoal.hpp"
#include "common/entity/ai/goal/goals/villager/ShareItemsGoal.hpp"
#include "common/entity/ai/goal/goals/villager/SleepAtNightGoal.hpp"
#include "common/entity/ai/goal/goals/villager/VillagerBreedGoal.hpp"
#include "common/entity/ai/goal/goals/villager/WorkAtJobSiteGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/monster/illager/WitchEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieVillagerEntity.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/experience/ExperienceDropHandler.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/VillageGossipType.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "common/world/village/raid/RaiderType.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "common/world/village/trade/WanderingTraderTrades.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace entity {

// ============================================================================
// VillagerEntity 静态常量
// ============================================================================

const std::unordered_map<const Item*, i32>& VillagerEntity::foodPoints()
{
    static const std::unordered_map<const Item*, i32> s_foodPoints = {
        {Items::BREAD, 4},
        {Items::POTATO, 1},
        {Items::CARROT, 1},
        {Items::BEETROOT, 1},
    };
    return s_foodPoints;
}

// ============================================================================
// VillagerEntity
// ============================================================================

std::unique_ptr<Entity> VillagerEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<VillagerEntity>(0, registry);
}

VillagerEntity::VillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractVillagerEntity(id, registry)
    , m_brain(std::make_unique<VillagerBrain>())
{
    // 对齐 MC Java 1.21.11 Villager 构造函数（Villager.java:196）：setCanPickUpLoot(true)
    // 使村民能拾取食物/种子（Mob.aiStep looting 段 → Villager.pickUpItem）。MobEntity::finalizeSpawn
    // 基类不再随机覆盖此标志（已对齐 Java 移除基类随机 setCanPickUpLoot），故村民恒为 true。
    setCanPickUpLoot(true);

    registerAttributes();
    registerGoals();
    initializeBrain();
}

void VillagerEntity::tick()
{
    AbstractVillagerEntity::tick();

    // Brain tick 已上移至 BrainTickSystem（PostEntityTick 阶段，见 EntityManager::_tickBrains）。
    // m_brain 成员与 brain() 访问器保留不变，Goal/Task/Sensor 仍经 owner->brain() 访问。

    // 更新声音冷却
    if (m_soundCooldown > 0) {
        m_soundCooldown--;
    }

    // 突袭恐慌流汗粒子效果：每 tick 有 1/100 概率检查是否在活跃突袭中，
    // 如果是则广播 VillagerSplash (42) 粒子效果
    if (m_world && getRandom().nextInt(100) == 0) {
        auto* raidManager = m_world->raidManager();
        if (raidManager != nullptr) {
            BlockPos villagerPos(static_cast<i32>(x()), static_cast<i32>(y()), static_cast<i32>(z()));
            auto* raid = raidManager->getRaidAt(villagerPos);
            if (raid != nullptr && raid->status() == world::village::raid::RaidStatus::Ongoing) {
                m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::VillagerSplash));
            }
        }
    }

    // 处理交易声望和粒子效果
    // 每tick检查 m_lastTradedPlayer，非空时触发声望事件和开心粒子，然后置空
    if (m_lastTradedPlayer != nullptr && m_world) {
        _handleTradeReputation();
        m_lastTradedPlayer = nullptr;
    }

    // 处理交易升级计时器
    // 仅在非交易状态时递减计时器，计时器到期时升级并生成新等级的交易
    if (!isTrading() && m_updateMerchantTimer > 0) {
        m_updateMerchantTimer--;
        if (m_updateMerchantTimer <= 0) {
            if (m_increaseProfessionLevelOnUpdate) {
                // 升级并补充新等级的交易（追加而非替换）
                _increaseMerchantCareer();
                m_increaseProfessionLevelOnUpdate = false;
            }

            // 升级后给予村民再生效果 I（持续200 tick = 10秒）
            addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Regeneration,
                200,   // 持续时间（tick）
                0,     // amplifier（0 = Level I）
                false, // ambient
                true,  // visible
                true   // showIcon
                ));
        }
    }

    // 工作站点检查由 WorkAtJobSiteGoal 自动处理
    // - shouldExecute() 检查是否是工作时间 (2000-9000 ticks) 和是否有工作站点
    // - tick() 中使用 isWithinDistance() 检查是否在工作站点附近
    // - Schedule 系统在 2000 ticks 时自动切换到 WORK 活动
}

void VillagerEntity::die(DamageSource& cause)
{
    // 村民→僵尸村民感染转化（对齐 MC Java 1.21.11 Zombie.killedEntity +
    // Zombie.convertVillagerToZombieVillager）。在被僵尸系生物杀死时，按难度概率转化为僵尸村民
    // 而非真正死亡。转化成功则提前 return，跳过下方 POI 释放（转化内已调 releaseAllPois）、
    // 流言更新与父类 die（避免掉落经验/物品与重复死亡流程）。
    if (_tryConvertToZombieVillager(cause)) {
        return;
    }

    // 死亡时释放所有占用的POI（床位、工作站、聚集点）并通知村庄管理器离开
    releaseAllPois();

    // 如果被玩家杀死，向村庄添加 MajorNegative 流言
    if (m_world) {
        Entity* sourceEntity = cause.getEntity();
        Player* killer = dynamic_cast<Player*>(sourceEntity);
        if (killer != nullptr) {
            auto* villageManager = m_world->villageManager();
            if (villageManager != nullptr) {
                world::village::Village* village = villageManager->getVillageAt(
                    BlockPos(static_cast<i32>(x()), static_cast<i32>(y()), static_cast<i32>(z())));
                if (village != nullptr) {
                    village->addGossip(killer->playerId(), world::village::VillageGossipType::MajorNegative, 25);
                }
            }
        }
    }

    // 调用父类 die() 处理通用死亡逻辑
    AbstractVillagerEntity::die(cause);
}

bool VillagerEntity::_tryConvertToZombieVillager(DamageSource& cause)
{
    // 对齐 MC Java 1.21.11 Zombie.killedEntity(ServerLevel, LivingEntity, DamageSource)：
    // 仅当 (难度为 Normal 或 Hard) && 目标是 Villager 时尝试感染。
    //   - Normal：50% 概率感染（Java random.nextBoolean()，true 则放弃感染直接死亡）
    //   - Hard：100% 感染
    //   - Easy/Peaceful：0%（getVillagerInfectionChance 返回 0）
    // DifficultyHelper::getVillagerInfectionChance 已封装此难度→概率映射。

    // 必须有世界引用以读取难度、创建/生成实体
    if (m_world == nullptr) {
        return false;
    }

    // 伤害来源实体（真正攻击者）。getEntity 返回直接来源，getTrueSource 返回真正来源（如投射物的射手）。
    // Java Zombie.killedEntity 在 LivingEntity.die 链中以击杀者（getEntity）为参数派发，此处取 getEntity
    // 对齐（僵尸近战杀死村民时 getEntity 即该僵尸）。
    Entity* sourceEntity = cause.getEntity();
    if (sourceEntity == nullptr) {
        // 投射物/间接伤害：退回真正来源（射手）
        sourceEntity = cause.getTrueSource();
    }

    // 仅僵尸系生物（ZombieEntity 及其子类 Husk/ZombieVillager）可感染村民。
    // dynamic_cast<ZombieEntity*> 覆盖 Zombie/Husk/ZombieVillager，排除 ZombifiedPiglin
    // （它继承 MonsterEntity 而非 ZombieEntity，对齐 Java：ZombifiedPiglin 不 override killedEntity，
    //  走 Monster 基类空实现，不感染村民）。
    auto* zombie = dynamic_cast<ZombieEntity*>(sourceEntity);
    if (zombie == nullptr) {
        return false;
    }

    // 难度感染概率门控
    const Difficulty difficulty = m_world->difficulty();
    const f32 infectionChance = combat::DifficultyHelper::getVillagerInfectionChance(difficulty);
    if (infectionChance <= 0.0f) {
        return false; // Easy/Peaceful 不感染
    }

    // Normal 50% 概率：用村民自身随机数判定。Hard（100%）时 infectionChance=1.0 必过。
    // 对齐 Java Zombie.killedEntity：Normal 难度 nextBoolean() 为 true 时 return 不感染。
    math::Random& rng = getRandom();
    if (rng.nextFloat() >= infectionChance) {
        return false;
    }

    // ===== 满足感染条件，执行转化（参照 ZombieVillagerEntity::finishConverting 反向范式 +
    // ZombieEntity::convertToDrowned 装备/状态复制范式）=====

    // 转化前先释放村民占用的 POI（床位/工作站/聚集点）——村民即将不复存在，其占用须清理。
    // releaseAllPois 内部有 m_poisReleased 守卫，后续 remove() 再调幂等。
    releaseAllPois();

    // 1. 创建 ZombieVillagerEntity（优先经 EntityRegistry 工厂，失败回退直接构造 + setTypeId 补 typeId）
    auto& registry = EntityRegistry::instance();
    const entity::EntityType* zvType = registry.getType("minecraft:zombie_villager");

    auto* ecsReg = &ecsRegistry();
    std::unique_ptr<Entity> newEntity;
    if (zvType && zvType->canSummon()) {
        newEntity = zvType->create(m_world, *ecsReg);
    } else {
        newEntity = std::make_unique<ZombieVillagerEntity>(EntityInstanceId(0), *ecsReg);
        newEntity->setTypeId(EntityTypeKeys::ZOMBIE_VILLAGER); // 工厂绕过补救：直接构造缺 typeId
    }

    if (newEntity == nullptr) {
        spdlog::error("VillagerEntity::_tryConvertToZombieVillager: failed to create zombie_villager entity");
        return false;
    }

    auto* zombieVillager = dynamic_cast<ZombieVillagerEntity*>(newEntity.get());
    if (zombieVillager == nullptr) {
        spdlog::error("VillagerEntity::_tryConvertToZombieVillager: created entity is not a ZombieVillagerEntity");
        return false;
    }

    // 2. 复制位置和旋转（对齐 Java ConversionParams.single(villager, true, true) 保留位置/旋转）
    zombieVillager->setPosition(m_builtIn.stateVector->m_pos);
    zombieVillager->setRotation(m_builtIn.rotation->m_rot.x, m_builtIn.rotation->m_rot.y);

    // 3. 继承村民数据（职业/类型/等级/经验）——对齐 Java setVillagerData(getVillagerData())
    zombieVillager->setVillagerData(m_villagerData);

    // 4. 复制婴儿状态（村民 isChild → 僵尸村民 setBaby，对齐 finishConverting 反向 setChild(isBaby())）
    zombieVillager->setBaby(isChild());

    // 5. 复制装备（逐槽，对齐 Java ConversionParams 保留装备语义）
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        EquipmentSlot slot = static_cast<EquipmentSlot>(i);
        const ItemStack& equipment = getEquipment(slot);
        if (!equipment.isEmpty()) {
            zombieVillager->setEquipment(slot, equipment);
        }
    }

    // 6. 复制自定义名称、持久化状态（对齐 convertToDrowned 范式）
    if (hasCustomName()) {
        zombieVillager->setCustomName(customNameText());
        zombieVillager->setCustomNameVisible(isCustomNameVisible());
    }
    if (isNoDespawnRequired()) {
        zombieVillager->enablePersistence();
    }

    // 7. finalizeSpawn（SpawnReason::Conversion，按位置感知区域难度初始化属性——对齐 Java
    // convertVillagerToZombieVillager 内 finalizeSpawn(... EntitySpawnReason.CONVERSION ...)）
    {
        combat::DifficultyInstance difficultyInstance = combat::DifficultyInstance::at(*m_world,
            BlockPos(static_cast<i32>(std::floor(x())), static_cast<i32>(y()), static_cast<i32>(std::floor(z()))));
        zombieVillager->finalizeSpawn(*m_world, difficultyInstance, world::spawn::SpawnReason::Conversion);
    }

    // 8. 释放所有权并生成到世界
    newEntity.release();
    EntityInstanceId newId = m_world->spawnEntity(std::unique_ptr<Entity>(zombieVillager));

    if (newId == 0) {
        // 生成失败：清理已创建实体，回退到正常死亡流程
        spdlog::error("VillagerEntity::_tryConvertToZombieVillager: failed to spawn zombie_villager entity");
        delete zombieVillager;
        return false;
    }

    // 9. 播放感染音效 + 世界事件（对齐 Java levelEvent 1026 = ZOMBIE_INFECTED）
    playSound(SoundEvents::ENTITY_ZOMBIE_INFECT, 1.0f, 1.0f);
    m_world->playEvent(world::WorldEvents::ZOMBIE_INFECT_SOUND,
        BlockPos(static_cast<i32>(m_builtIn.stateVector->m_pos.x),
            static_cast<i32>(m_builtIn.stateVector->m_pos.y),
            static_cast<i32>(m_builtIn.stateVector->m_pos.z)),
        0);

    // 10. 清空原村民装备（防止后续 remove/死亡流程掉落）并移除原村民
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        setEquipment(static_cast<EquipmentSlot>(i), ItemStack());
    }

    remove();

    return true;
}

void VillagerEntity::remove()
{
    // 村民被移除时释放POI并通知村庄。村民不会因距离远而消失，
    // 但当确实被移除时（死亡动画结束、区块卸载等）需要清理POI占用。

    releaseAllPois();

    // 调用父类 remove()
    AbstractVillagerEntity::remove();
}

// ============================================================================
// 雷击转女巫（对齐 vanilla Villager#thunderHit）
// ============================================================================

void VillagerEntity::onStruckByLightning(entity::LightningBoltEntity* lightning)
{
    // 客户端不执行实体转化逻辑
    if (m_world == nullptr || m_world->isClientSide()) {
        return;
    }

    // 对齐 vanilla Villager#thunderHit（Villager.java:773-787）：
    //   if (level.getDifficulty() != PEACEFUL) {
    //       Witch witch = convertTo(WITCH, ConversionParams.single(this, false, false), p -> {
    //           p.finalizeSpawn(level, difficulty, CONVERSION, null);
    //           p.setPersistenceRequired();
    //           this.releaseAllPois();
    //       });
    //       if (witch == null) super.thunderHit(level, lightning);   // 转化失败回退基类受5伤害
    //   } else {
    //       super.thunderHit(level, lightning);                      // 和平难度也调基类受5伤害
    //   }
    // 转化成功时不调 super（女巫不受伤），原体经 convertTo 内部 discard。ConversionParams 第三个
    // 参数 false 表示不保留装备（女巫不继承村民装备），仅保留位置与旋转。
    if (m_world->difficulty() == Difficulty::Peaceful) {
        AbstractVillagerEntity::onStruckByLightning(lightning);
        return;
    }

    // 经 EntityType 工厂创建女巫，避免本目录反向依赖 monster/illager 目录
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* witchType = registry.getType(entity::EntityTypeKeys::WITCH);

    auto* ecsReg = &ecsRegistry();
    std::unique_ptr<Entity> newEntity;
    if (witchType != nullptr && witchType->canSummon()) {
        newEntity = witchType->create(m_world, *ecsReg);
    } else {
        // 工厂绕过补救：直接构造缺 typeId，手动补齐
        newEntity = std::make_unique<WitchEntity>(EntityInstanceId(0), *ecsReg);
        newEntity->setTypeId(entity::EntityTypeKeys::WITCH);
    }

    if (newEntity == nullptr) {
        spdlog::error("VillagerEntity::onStruckByLightning: failed to create witch entity");
        AbstractVillagerEntity::onStruckByLightning(lightning);
        return;
    }

    auto* witch = dynamic_cast<WitchEntity*>(newEntity.get());
    if (witch == nullptr) {
        spdlog::error("VillagerEntity::onStruckByLightning: created entity is not a WitchEntity");
        AbstractVillagerEntity::onStruckByLightning(lightning);
        return;
    }

    // 复制位置和旋转（对齐 Java ConversionParams.single(villager, false, false) 保留位置/旋转）
    witch->setPosition(m_builtIn.stateVector->m_pos);
    witch->setRotation(m_builtIn.rotation->m_rot.x, m_builtIn.rotation->m_rot.y);

    // 复制自定义名称（对齐 convertTo 保留自定义名语义）
    if (hasCustomName()) {
        witch->setCustomName(customNameText());
        witch->setCustomNameVisible(isCustomNameVisible());
    }

    // 闪电转化的女巫需持久化留存（对齐 vanilla setPersistenceRequired）
    witch->enablePersistence();

    // finalizeSpawn（SpawnReason::Conversion，按位置感知区域难度初始化属性——对齐 Java
    // finalizeSpawn(... EntitySpawnReason.CONVERSION ...)）
    {
        combat::DifficultyInstance difficultyInstance = combat::DifficultyInstance::at(*m_world,
            BlockPos(static_cast<i32>(std::floor(x())), static_cast<i32>(y()), static_cast<i32>(std::floor(z()))));
        witch->finalizeSpawn(*m_world, difficultyInstance, world::spawn::SpawnReason::Conversion);
    }

    // 转化前释放村民占用的 POI（对齐 vanilla releaseAllPois）。
    // releaseAllPois 内部有 m_poisReleased 守卫，后续 remove() 再调幂等。
    releaseAllPois();

    // 释放所有权并生成女巫到世界
    newEntity.release();
    EntityInstanceId newId = m_world->spawnEntity(std::unique_ptr<Entity>(witch));

    if (newId == 0) {
        // 生成失败：清理已创建实体，回退基类受 5 伤害（vanilla convertTo 返 null 同语义）
        spdlog::error("VillagerEntity::onStruckByLightning: failed to spawn witch entity");
        delete witch;
        AbstractVillagerEntity::onStruckByLightning(lightning);
        return;
    }

    // 移除原村民（对齐 vanilla convertTo 成功：不调 super、discard 原体）
    remove();
}

void VillagerEntity::releaseAllPois()
{
    // 防止 die() 和 remove() 双重释放
    // 死亡流程：die() -> releaseAllPois() -> ... -> tickDeath() -> remove() -> releaseAllPois()
    // 第二次调用时 POI 已释放，此处提前返回避免冗余操作
    if (m_poisReleased) {
        return;
    }
    m_poisReleased = true;

    if (!m_world) {
        return;
    }

    auto* villageManager = m_world->villageManager();
    if (villageManager == nullptr) {
        return;
    }

    const auto villagerId = static_cast<u64>(id());

    // 释放该村民占用的所有POI（床位、工作站等）
    villageManager->getPOIStorage().releaseAllByOwner(villagerId);

    // 通知村庄管理器该村民已离开村庄
    villageManager->onVillagerLeave(villagerId);

    // 清除睡眠状态（如果正在睡眠）
    if (isSleeping()) {
        stopSleeping();
    }
}

void VillagerEntity::setLastHurtBy(LivingEntity* attacker)
{
    // 当被玩家攻击时，广播愤怒粒子效果并触发声望事件
    if (attacker != nullptr && m_world && attacker != this) {
        Player* player = dynamic_cast<Player*>(attacker);

        // 触发村庄声望事件 VILLAGER_HURT
        if (player != nullptr) {
            auto* villageManager = m_world->villageManager();
            if (villageManager != nullptr) {
                world::village::Village* village = villageManager->getVillageAt(
                    BlockPos(static_cast<i32>(x()), static_cast<i32>(y()), static_cast<i32>(z())));
                if (village != nullptr) {
                    village->addGossip(player->playerId(), world::village::VillageGossipType::MinorNegative, 5);
                }
            }

            // 只有被玩家攻击时才广播愤怒粒子效果
            if (isAlive()) {
                m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::VillagerAngry));
            }
        }
    }

    // 调用父类方法更新 m_lastHurtBy
    AbstractVillagerEntity::setLastHurtBy(attacker);
}

void VillagerEntity::initializeBrain()
{
    if (!m_brain) {
        return;
    }

    using namespace ai::brain::memory;
    using namespace ai::brain::sensor;

    // 注册记忆模块 - 使用 MemoryModuleTypes 标准类型
    // 注意：必须先调用 MemoryModuleTypes::initialize() 初始化
    m_brain->registerMemory(MemoryModuleTypes::HOME);
    m_brain->registerMemory(MemoryModuleTypes::JOB_SITE);
    m_brain->registerMemory(MemoryModuleTypes::POTENTIAL_JOB_SITE);
    m_brain->registerMemory(MemoryModuleTypes::MEETING_POINT);
    m_brain->registerMemory(MemoryModuleTypes::NEAREST_BED);
    m_brain->registerMemory(MemoryModuleTypes::WALK_TARGET);
    m_brain->registerMemory(MemoryModuleTypes::LOOK_TARGET);
    m_brain->registerMemory(MemoryModuleTypes::VISIBLE_MOBS);
    m_brain->registerMemory(MemoryModuleTypes::NEAREST_HOSTILE);
    m_brain->registerMemory(MemoryModuleTypes::NEAREST_PLAYERS);
    m_brain->registerMemory(MemoryModuleTypes::NEAREST_VISIBLE_PLAYER);
    m_brain->registerMemory(MemoryModuleTypes::HURT_BY);
    m_brain->registerMemory(MemoryModuleTypes::HURT_BY_ENTITY);
    m_brain->registerMemory(MemoryModuleTypes::AVOID_TARGET);
    m_brain->registerMemory(MemoryModuleTypes::LAST_SLEPT);
    m_brain->registerMemory(MemoryModuleTypes::LAST_WORKED_AT_POI);
    m_brain->registerMemory(MemoryModuleTypes::VISIBLE_VILLAGER_BABIES);
    m_brain->registerMemory(MemoryModuleTypes::NEAREST_VISIBLE_ADULT);

    // 移动任务所需的额外记忆模块
    m_brain->registerMemory(MemoryModuleTypes::PATH);
    m_brain->registerMemory(MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE);
    m_brain->registerMemory(MemoryModuleTypes::HIDING_PLACE);
    m_brain->registerMemory(MemoryModuleTypes::ATTACK_TARGET);
    m_brain->registerMemory(MemoryModuleTypes::ATTACK_COOLING_DOWN);
    m_brain->registerMemory(MemoryModuleTypes::INTERACTION_TARGET);

    // 门交互任务所需的记忆模块
    m_brain->registerMemory(MemoryModuleTypes::INTERACTABLE_DOORS);
    m_brain->registerMemory(MemoryModuleTypes::OPENED_DOORS);

    // 注册传感器
    m_brain->registerSensor(std::make_unique<NearestPlayersSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<NearestVisibleLivingEntitySensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<HurtBySensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<VillagerHostilesSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<WorkStationSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<VillagePoiSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<BabySensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<InteractableDoorsSensor<VillagerEntity>>());

    // 设置日程
    m_brain->setSchedule(&ai::brain::schedule::Schedule::VILLAGER_DEFAULT);

    // 设置默认活动
    m_brain->setDefaultActivities({ai::brain::schedule::Activity::IDLE});
    m_brain->setFallbackActivity(ai::brain::schedule::Activity::IDLE);

    // 注册核心移动任务 - 所有活动都使用
    using namespace ai::brain::task::movement;
    using namespace ai::brain::task::interact;
    using namespace ai::brain::task::action;
    using TaskPtr = std::unique_ptr<ai::brain::task::Task<VillagerEntity>>;

    // 辅助函数：从 unique_ptr 列表构建 vector
    auto makeTasks = [](TaskPtr first, auto&&... rest) -> std::vector<TaskPtr> {
        std::vector<TaskPtr> tasks;
        tasks.push_back(std::move(first));
        (tasks.push_back(std::move(rest)), ...);
        return tasks;
    };

    // IDLE 活动：随机漫步、看向实体、开关门
    m_brain->registerActivity(ai::brain::schedule::Activity::IDLE,
        0,
        makeTasks(std::make_unique<InteractWithDoorTask<VillagerEntity>>()),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::IDLE,
        1,
        makeTasks(
            std::make_unique<MoveToTargetTask<VillagerEntity>>(), std::make_unique<StrollTask<VillagerEntity>>(0.6f)),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::IDLE,
        2,
        makeTasks(std::make_unique<LookAtEntityTask<VillagerEntity>>()),
        {},
        {});

    // WORK 活动：移动到目标、随机漫步（低频率）、开关门
    m_brain->registerActivity(ai::brain::schedule::Activity::WORK,
        0,
        makeTasks(std::make_unique<InteractWithDoorTask<VillagerEntity>>()),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::WORK,
        1,
        makeTasks(std::make_unique<MoveToTargetTask<VillagerEntity>>(),
            std::make_unique<StrollTask<VillagerEntity>>(0.4f, 80)),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::WORK,
        2,
        makeTasks(std::make_unique<LookAtEntityTask<VillagerEntity>>()),
        {},
        {});

    // MEET 活动：村民互动、移动到目标、随机漫步（较高频率）、看向实体、开关门
    m_brain->registerActivity(ai::brain::schedule::Activity::MEET,
        0,
        makeTasks(std::make_unique<InteractWithDoorTask<VillagerEntity>>()),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::MEET,
        1,
        makeTasks(std::make_unique<VillagerInteractTask<VillagerEntity>>(),
            std::make_unique<MoveToTargetTask<VillagerEntity>>(),
            std::make_unique<StrollTask<VillagerEntity>>(0.5f, 60)),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::MEET,
        2,
        makeTasks(std::make_unique<LookAtEntityTask<VillagerEntity>>(8.0f, 0.05f)),
        {},
        {});

    // PANIC 活动：逃跑、移动到目标、开关门
    m_brain->registerActivity(ai::brain::schedule::Activity::PANIC,
        -1,
        makeTasks(std::make_unique<InteractWithDoorTask<VillagerEntity>>()),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::PANIC,
        0,
        makeTasks(std::make_unique<FleeTask<VillagerEntity>>(0.6f, 10.0f),
            std::make_unique<MoveToTargetTask<VillagerEntity>>()),
        {{MemoryModuleTypes::AVOID_TARGET, MemoryModuleStatus::VALUE_PRESENT}},
        {MemoryModuleTypes::WALK_TARGET});

    // HIDE 活动：寻找隐蔽点、移动到目标
    m_brain->registerActivity(ai::brain::schedule::Activity::HIDE,
        0,
        makeTasks(std::make_unique<FindHiddenBlockTask<VillagerEntity>>(1.0f),
            std::make_unique<MoveToTargetTask<VillagerEntity>>()),
        {},
        {});

    // PRE_RAID / RAID / FIGHT 活动：追逐攻击目标、近战攻击、移动到目标
    m_brain->registerActivity(ai::brain::schedule::Activity::FIGHT,
        0,
        makeTasks(std::make_unique<ChaseTask<VillagerEntity>>(1.0f, 2.0f),
            std::make_unique<AttackTask<VillagerEntity>>(20),
            std::make_unique<MoveToTargetTask<VillagerEntity>>()),
        {{MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT}},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::RAID,
        0,
        makeTasks(std::make_unique<ChaseTask<VillagerEntity>>(1.0f, 2.0f),
            std::make_unique<AttackTask<VillagerEntity>>(20),
            std::make_unique<MoveToTargetTask<VillagerEntity>>()),
        {{MemoryModuleTypes::ATTACK_TARGET, MemoryModuleStatus::VALUE_PRESENT}},
        {});

    // AVOID 活动：逃跑、移动到目标
    m_brain->registerActivity(ai::brain::schedule::Activity::AVOID,
        0,
        makeTasks(
            std::make_unique<FleeTask<VillagerEntity>>(1.0f), std::make_unique<MoveToTargetTask<VillagerEntity>>()),
        {{MemoryModuleTypes::AVOID_TARGET, MemoryModuleStatus::VALUE_PRESENT}},
        {});

    // REST 活动：随机漫步（低频率）、看向实体、开关门
    m_brain->registerActivity(ai::brain::schedule::Activity::REST,
        0,
        makeTasks(std::make_unique<InteractWithDoorTask<VillagerEntity>>()),
        {},
        {});

    m_brain->registerActivity(ai::brain::schedule::Activity::REST,
        1,
        makeTasks(std::make_unique<MoveToTargetTask<VillagerEntity>>(),
            std::make_unique<StrollTask<VillagerEntity>>(0.3f, 120)),
        {},
        {});
}

void VillagerEntity::setProfession(VillagerProfession profession)
{
    m_villagerData.setProfession(profession);

    // 根据职业更新交易列表
    updateOffers();
}

bool VillagerEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 村民用食物繁殖：面包、土豆、胡萝卜、甜菜根
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;

    return item == Items::BREAD || item == Items::POTATO || item == Items::CARROT || item == Items::BEETROOT;
}

i32 VillagerEntity::countFoodPointsInInventory() const
{
    const IInventory& inv = inventory();
    i32 total = 0;
    for (const auto& [item, points] : foodPoints()) {
        total += inv.countItem(*item) * points;
    }
    return total;
}

bool VillagerEntity::hasExcessFood() const
{
    return countFoodPointsInInventory() >= EXCESS_FOOD_THRESHOLD;
}

bool VillagerEntity::wantsMoreFood() const
{
    return countFoodPointsInInventory() < WANTS_MORE_FOOD_THRESHOLD;
}

bool VillagerEntity::canPickUpItem(const ItemStack& itemStack) const
{
    // 村民可以拾取的默认物品列表
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;

    // 默认可拾取物品：面包、土豆、胡萝卜、小麦、小麦种子、甜菜根、甜菜根种子
    static const Item* allowedItems[] = {Items::BREAD,
        Items::POTATO,
        Items::CARROT,
        Items::WHEAT,
        Items::WHEAT_SEEDS,
        Items::BEETROOT,
        Items::BEETROOT_SEEDS};

    // 检查是否在默认列表中
    for (const Item* allowedItem : allowedItems) {
        if (item == allowedItem) {
            // 还需要检查库存是否有空间
            return m_inventory && m_inventory->canAddItem(itemStack);
        }
    }

    // 农民职业特有物品已在默认列表中，无需额外检查

    return false;
}

bool VillagerEntity::wantsToPickUp(const ItemStack& itemStack) const
{
    // 对齐 MC Java 1.21.11 Villager.wantsToPickUp：村民关心食物/种子类物品且库存可放入时才拾取。
    // 基类 MobEntity::wantsToPickUp 默认实现为 canHoldItem（装备槽语义，!isEmpty 即 true），
    // 不区分物品类型，故村民必须覆写为转调 canPickUpItem（食物/种子语义 + 库存 canAddItem 校验），
    // 否则 MobEntity::tick 的 looting 扫描会对任何非空物品都判定 wantsToPickUp=true 并调 pickUpItem。
    return canPickUpItem(itemStack);
}

void VillagerEntity::pickUpItem(ItemEntity& itemEntity)
{
    // 对齐 MC Java 1.21.11 Villager.pickUpItem → InventoryCarrier.pickUpItem（静态方法）。
    // 将 ItemEntity 的物品堆放入村民库存（SimpleInventory::addItem），处理部分装入的剩余 count：
    //   - 全部装入（addItem 返回空堆）→ itemEntity.remove() 移除物品实体（对齐 Java discard）
    //   - 部分装入（addItem 返回剩余堆）→ 把剩余 count 写回 ItemEntity（对齐 Java setCount，物品留地）
    // 拾取后若库存食物点数达到繁殖门槛，调 setWillingToBreed(true) 驱动 VillagerBreedGoal。

    const ItemStack& itemStack = itemEntity.getItemStack();
    if (itemStack.isEmpty()) {
        return;
    }

    // 二次校验可放入（对齐 Java InventoryCarrier.pickUpItem 的 canAddItem 短路）。
    // wantsToPickUp 已校验过 canAddItem，但 addItem 可能因并发变动失败，此处再守卫一次。
    if (!m_inventory || !m_inventory->canAddItem(itemStack)) {
        return;
    }

    const i32 originalCount = itemStack.getCount();

    // addItem 返回剩余未装入的 ItemStack（对齐 Java SimpleContainer.addItem 语义）。
    ItemStack remainder = m_inventory->addItem(itemStack);
    const i32 pickedUpCount = originalCount - remainder.getCount();
    if (pickedUpCount <= 0) {
        return; // 实际未装入任何物品，保留 ItemEntity 不动
    }

    if (remainder.isEmpty()) {
        // 全部装入，移除物品实体（对齐 Java p_219614_.discard()）
        itemEntity.remove();
    } else {
        // 部分装入，把剩余 count 写回 ItemEntity（对齐 Java itemstack.setCount(itemstack1.getCount())）
        itemEntity.setItemStack(remainder);
    }

    // 拾取后检查繁殖意愿：库存食物点数 >= 繁殖门槛（WANTS_MORE_FOOD_THRESHOLD=12）则标记愿意繁殖。
    // 对齐 Java 1.21.11 Villager.canBreed() 的 `foodLevel + countFoodPointsInInventory() >= 12` 门槛
    // （Cubium 暂无 foodLevel 字段，用 willingToBreed 布尔替代，参见头文件 TODO）。
    if (countFoodPointsInInventory() >= WANTS_MORE_FOOD_THRESHOLD) {
        setWillingToBreed(true);
    }
}

std::unique_ptr<AgeableEntity> VillagerEntity::createChild()
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }
    auto child = std::make_unique<VillagerEntity>(0, *registry);
    child->setTypeId(EntityTypeKeys::VILLAGER); // 工厂绕过补救：直接构造缺 typeId
    child->setChild(true);

    // 继承村民类型
    child->setVillagerType(m_villagerData.type());

    // 设置位置
    child->setPosition(x(), y(), z());

    return child;
}

bool VillagerEntity::canWork() const
{
    // 不是傻子且有工作站点
    return !isNitwit() && (m_workStation.x != 0 || m_workStation.y != 0 || m_workStation.z != 0);
}

void VillagerEntity::rest()
{
    // 停止工作状态
    // 村民的睡眠由Brain系统自动管理：
    // - Schedule::VILLAGER_DEFAULT 在游戏时间12000 ticks时切换到 Activity::REST
    // - SleepAtNightGoal 在REST活动期间自动检查睡眠条件并执行睡眠
    m_working = false;
    m_atWorkstation = false;
}

void VillagerEntity::work()
{
    m_working = true;
    m_workTime++;

    // 在工作站点时检查是否需要补货
    // 补货检查由 WorkAtJobSiteGoal::tick() 通过 shouldRestock() 触发，
    // 此处不再直接处理补货逻辑，避免与 Goal 层的补货逻辑冲突。
}

void VillagerEntity::play()
{
    // 村民互动由 Brain 系统的任务自动执行：
    // - 成年村民在 MEET 活动期间聚集在会议点（钟）附近
    // - 幼年村民在 PLAY 活动期间一起玩耍
    //
    // 具体行为由以下组件实现：
    // 1. CongregateGoal - 聚集在会议点附近
    // 2. ShareItemsGoal - 分享物品（农民分享食物给其他村民）
    // 3. GossipSpreadGoal - 村民间流言传播
    // 4. LookAtGoal - 看向其他村民/玩家/猫
    //
    // 这些行为在 registerGoals() 中注册，由 Brain 系统根据活动自动调度

    // 触发流言传播检查
    trySpreadGossip();
}

void VillagerEntity::spreadGossipTo(VillagerEntity* other)
{
    if (!other || !m_world) return;

    // 每次传播最多传播 10 条流言
    // 冷却时间 1200 tick (60秒)
    i64 currentTime = m_world->currentTick();
    i64 otherTime = other->m_lastGossipSpreadTime;

    // 检查冷却时间
    if (currentTime < m_lastGossipSpreadTime + 1200L || currentTime < otherTime + 1200L) {
        return;
    }

    // 获取村庄管理器
    auto* villageManager = m_world->villageManager();
    if (!villageManager) return;

    // 获取村民所在村庄的流言管理器
    // 注意：流言是村庄级别的，不是村民级别的
    // 这里简化实现，实际需要从 Village 获取流言管理器
    (void)villageManager; // 避免未使用警告

    // 更新传播时间
    m_lastGossipSpreadTime = currentTime;
    other->m_lastGossipSpreadTime = currentTime;

    // 流言传播时会减少衰减值
    // transferFrom() 方法会在传播时减少流言值
    // 这里简化实现，实际需要 VillageGossipManager::transferFrom()

    // 尝试生成铁傀儡（如果村民足够多且声誉足够高）
    // 这里简化，实际需要检查村庄条件
}

void VillagerEntity::trySpreadGossip()
{
    if (!m_world) return;

    // 检查冷却时间
    i64 currentTime = m_world->currentTick();
    if (currentTime < m_lastGossipSpreadTime + 1200L) {
        return; // 60秒冷却
    }

    // 从 Brain 获取交互目标（存 id：id 永不悬垂，经 getEntity(id) 反查 + isAlive 校验）
    auto targetMemory = m_brain->getMemory<EntityInstanceId>(ai::brain::memory::MemoryModuleTypes::INTERACTION_TARGET);
    if (!targetMemory.has_value() || *targetMemory == INVALID_ENTITY_ID) {
        return;
    }

    Entity* targetEntity = m_world->getEntity(*targetMemory);
    if (targetEntity == nullptr || !targetEntity->isAlive()) {
        return;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(targetEntity);
    if (target == nullptr) {
        return;
    }

    // 检查是否是村民
    VillagerEntity* otherVillager = dynamic_cast<VillagerEntity*>(target);
    if (!otherVillager) {
        return;
    }

    // 检查距离（在交互范围内）
    f32 distSq = distanceSqTo(*otherVillager);
    if (distSq > 5.0f * 5.0f) {
        return;
    }

    // 传播流言
    spreadGossipTo(otherVillager);
}

void VillagerEntity::registerGoals()
{
    AgeableEntity::registerGoals();

    using namespace ai::goal::villager;

    // 优先级1: 逃避敌对生物（最高优先级）
    m_goalSelector.addGoal(1, std::make_unique<AvoidHostileGoal>(this));

    // 优先级2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<VillagerBreedGoal>(this));

    // 优先级3: 夜间睡眠
    m_goalSelector.addGoal(3, std::make_unique<SleepAtNightGoal>(this));

    // 优先级3: 工作时间工作（与睡眠互斥）
    m_goalSelector.addGoal(3, std::make_unique<WorkAtJobSiteGoal>(this));

    // 优先级4: 寻找工作站点
    m_goalSelector.addGoal(4, std::make_unique<LookForJobSiteGoal>(this));

    // 优先级5: 收集物品
    m_goalSelector.addGoal(5, std::make_unique<GatherItemsGoal>(this));

    // 优先级6: 村民聚集（MEET 活动期间）
    m_goalSelector.addGoal(6, std::make_unique<CongregateGoal>(this));

    // 优先级7: 分享物品（农民分享食物）
    m_goalSelector.addGoal(7, std::make_unique<ShareItemsGoal>(this));

    // 优先级8: 看向实体（村民、玩家、猫等）
    m_goalSelector.addGoal(8, std::make_unique<LookAtEntitiesGoal>(this));

    // 农民特殊目标（替代普通工作目标）
    if (m_villagerData.profession() == VillagerProfession::Farmer) {
        m_goalSelector.addGoal(3, std::make_unique<FarmerWorkGoal>(this));
    }
}

void VillagerEntity::registerAttributes()
{
    AgeableEntity::registerAttributes();

    // 村民属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
}

// ========== 补货系统 ==========

bool VillagerEntity::shouldRestock()
{
    if (!m_world) return false;

    // 补货判定逻辑：
    // 1. 检测是否跨天（距上次补货超过12000tick 或 新的游戏日开始）
    // 2. 若跨天，重置每日补货次数（同时补偿需求更新）
    // 3. 返回 allowedToRestock() && needsToRestock()

    const i64 gameTime = static_cast<i64>(m_world->currentTick());

    // 检测是否跨天：距上次补货超过12000tick（10分钟）或新的游戏日开始
    bool dayRollover = false;
    {
        const i64 lastRestockThreshold = m_lastRestockGameTime + 12000L;
        const bool timeThreshold = gameTime > lastRestockThreshold;

        // 游戏天数 = dayTime / 24000
        const i64 currentDay = m_world->dayTime() / 24000L;
        const bool newDay = m_lastRestockCheckDay > 0L && currentDay > m_lastRestockCheckDay;

        dayRollover = timeThreshold || newDay;
        m_lastRestockCheckDay = currentDay;
    }

    if (dayRollover) {
        m_lastRestockGameTime = gameTime;
        resetNumberOfRestocks();
    }

    return allowedToRestock() && needsToRestock();
}

void VillagerEntity::restock()
{
    // 补货执行逻辑：
    // 1. 更新所有交易的需求值
    // 2. 重置所有交易的使用次数
    // 3. 记录补货时间
    // 4. 增加今日补货次数
    if (m_offers) {
        m_offers->updateDemandAll();
        m_offers->restockAll();
    }

    m_lastRestockGameTime = m_world ? static_cast<i64>(m_world->currentTick()) : 0LL;
    ++m_numberOfRestocksToday;
}

bool VillagerEntity::needsToRestock() const
{
    // 任意交易被使用过则需要补货
    if (!m_offers) return false;
    return m_offers->needsRestockAny();
}

bool VillagerEntity::allowedToRestock() const
{
    // 补货许可判定：
    // - 今日首次补货总是允许
    // - 第二次补货需要距上次补货至少2400tick（2分钟）
    // - 每日最多补货2次
    if (m_numberOfRestocksToday == 0) {
        return true;
    }
    if (m_numberOfRestocksToday < 2) {
        const i64 gameTime = m_world ? static_cast<i64>(m_world->currentTick()) : 0LL;
        return gameTime > m_lastRestockGameTime + 2400L;
    }
    return false;
}

void VillagerEntity::resetNumberOfRestocks()
{
    _catchUpDemand();
    m_numberOfRestocksToday = 0;
}

void VillagerEntity::_catchUpDemand()
{
    // 需求补偿逻辑：
    // 如果昨日补货少于2次，对每次未补货执行额外的 resetUses + updateDemand
    if (!m_offers) return;

    const i32 missedRestocks = 2 - m_numberOfRestocksToday;
    if (missedRestocks > 0) {
        // 对每次错过的补货，重置使用次数并更新需求
        for (i32 i = 0; i < missedRestocks; ++i) {
            m_offers->restockAll();
        }
    }

    // 对每次错过的补货，更新需求值
    for (i32 i = 0; i < missedRestocks; ++i) {
        m_offers->updateDemandAll();
    }
}

// ========== 睡眠相关 ==========

bool VillagerEntity::isSleeping() const
{
    return pose() == EntityPose::Sleeping;
}

void VillagerEntity::startSleeping(BlockPos pos)
{
    // 如果正在骑乘，先停止骑乘
    if (getVehicle() != INVALID_ENTITY_ID) {
        stopRiding();
    }

    // 设置床为占用状态
    if (m_world) {
        const BlockState* bedState = m_world->getBlockState(pos);
        if (bedState != nullptr && bedState->hasProperty(BlockStateProperties::BED_PART())) {
            // 在 setBlockState 之前提取所需属性值，避免悬挂指针
            bool hasOccupied = bedState->hasProperty(BlockStateProperties::OCCUPIED());
            bool isFoot = bedState->get(BlockStateProperties::BED_PART()) == BlockStateProperties::BedPart::Foot;
            bool hasFacing = bedState->hasProperty(BlockStateProperties::HORIZONTAL_FACING());
            Direction facing = hasFacing ? bedState->get(BlockStateProperties::HORIZONTAL_FACING()) : Direction::None;

            // 设置床头为占用
            if (hasOccupied) {
                BlockState occupiedState = bedState->with(BlockStateProperties::OCCUPIED(), true);
                m_world->setBlockState(pos, &occupiedState, 3);
            }
            // 如果当前是脚部，也设置头部为占用
            if (isFoot && hasFacing) {
                BlockPos headPos = pos.offset(facing);
                const BlockState* headState = m_world->getBlockState(headPos);
                if (headState != nullptr && headState->hasProperty(BlockStateProperties::OCCUPIED())) {
                    BlockState occupiedHeadState = headState->with(BlockStateProperties::OCCUPIED(), true);
                    m_world->setBlockState(headPos, &occupiedHeadState, 3);
                }
            }
        }
    }

    // 设置睡眠姿态
    setPose(EntityPose::Sleeping);

    // 记录睡眠位置
    m_sleepingPos = pos;

    // 设置位置到床的中心（稍微抬高）
    setPosition(pos.x + 0.5, pos.y + 0.6875, pos.z + 0.5);

    // 清除速度
    setVelocity(0.0, 0.0, 0.0);

    // 记录上次睡眠时间到Brain记忆
    if (m_brain && m_world) {
        m_brain->setMemory(ai::brain::memory::MemoryModuleTypes::LAST_SLEPT, static_cast<i64>(m_world->currentTick()));
    }
}

void VillagerEntity::stopSleeping()
{
    // 只有在睡眠时才需要唤醒
    if (!isSleeping()) {
        return;
    }

    // 如果有睡眠位置，根据床的朝向计算唤醒位置
    if (m_sleepingPos.has_value() && m_world) {
        BlockPos bedPos = m_sleepingPos.value();
        const BlockState* bedState = m_world->getBlockState(bedPos);

        // 在 setBlockState 之前提取所需属性值，避免悬挂指针
        bool hasOccupied = (bedState != nullptr && bedState->hasProperty(BlockStateProperties::OCCUPIED()));
        bool hasFacing = (bedState != nullptr && bedState->hasProperty(BlockStateProperties::HORIZONTAL_FACING()));
        Direction bedFacing = hasFacing ? bedState->get(BlockStateProperties::HORIZONTAL_FACING()) : Direction::None;

        // 清除床的占用状态
        if (hasOccupied) {
            BlockState newBedState = bedState->with(BlockStateProperties::OCCUPIED(), false);
            m_world->setBlockState(bedPos, &newBedState, 3);
        }

        // 使用床的朝向计算起床位置
        if (hasFacing) {
            Vector3 wakePos = blocks::BedBlock::findStandUpPosition(*m_world, bedPos, bedFacing, yaw());

            // 计算面向床的方向（yaw）：从起床位置指向床底中心的方向
            Vector3d bedCenter(bedPos.x + 0.5, bedPos.y, bedPos.z + 0.5);
            Vector3d dirToBed = bedCenter - Vector3d(wakePos.x, wakePos.y, wakePos.z);
            f32 dirLen = std::sqrt(dirToBed.x * dirToBed.x + dirToBed.z * dirToBed.z);
            if (dirLen > 0.001) {
                dirToBed.x /= dirLen;
                dirToBed.z /= dirLen;
                f32 yawDeg = static_cast<f32>(math::toDegrees(std::atan2(dirToBed.z, dirToBed.x))) - 90.0f;
                yawDeg = math::wrapDegrees(yawDeg);
                setRotation(yawDeg, 0.0f);
            }

            setPosition(wakePos.x, wakePos.y, wakePos.z);
        } else {
            // 回退：床头正上方
            BlockPos aboveBed = bedPos.up();
            setPosition(aboveBed.x + 0.5, aboveBed.y + 0.1, aboveBed.z + 0.5);
        }
    }

    // 恢复站立姿态
    setPose(EntityPose::Standing);

    // 清除睡眠位置
    m_sleepingPos = std::nullopt;

    // 记录上次醒来时间到Brain记忆
    if (m_brain && m_world) {
        m_brain->setMemory(ai::brain::memory::MemoryModuleTypes::LAST_WOKEN, static_cast<i64>(m_world->currentTick()));
    }
}

bool VillagerEntity::isNightTime() const
{
    if (!m_world) {
        return false;
    }

    i64 tod = m_world->dayTimeOfDay();
    // 夜间时间：12542 - 23459
    return tod >= 12542 && tod <= 23459;
}

bool VillagerEntity::isWorkTime() const
{
    if (!m_world) {
        return false;
    }

    i64 tod = m_world->dayTimeOfDay();
    // 工作时间：2000 - 9000
    return tod >= 2000 && tod <= 9000;
}

void VillagerEntity::updateOffers()
{
    // 根据职业和等级生成交易列表
    using namespace world::village::trade;

    // 傻子村民没有交易
    if (isNitwit()) {
        m_offers = std::make_unique<MerchantOffers>();
        return;
    }

    // 使用世界种子和实体ID生成唯一种子，确保每个村民交易不同
    u64 seed = 0;
    if (m_world) {
        seed = m_world->seed();
    }
    seed = seed * 31 + static_cast<u64>(id());

    // 生成新的交易列表
    m_offers = VillagerTrades::generateOffers(m_villagerData.profession(),
        m_villagerData.type(),
        m_villagerData.level(),
        0, // 需求修正
        seed);
}

void VillagerEntity::rewardTradeXp(MerchantOffer& offer)
{
    // 在增加经验前记录当前等级，用于判断本次交易是否触发了升级
    const i32 prevLevel = m_villagerData.level();

    // 增加村民经验（内部可能触发升级）
    addVillagerExperience(offer.getXp());

    // 记录最后交易的玩家（用于升级时重新补充交易）
    m_lastTradedPlayer = getTradingPlayer();

    // 生成经验球给玩家
    if (offer.shouldRewardExp() && m_world) {
        // 经验球值 = 3 + random(0~3)，即 3~6
        i32 xpOrbCount = 3 + getRandom().nextInt(4);

        // 如果本次交易导致村民升级，额外增加5点经验球值
        // 注意：必须在 addVillagerExperience 之前记录等级，否则升级已发生后
        // 比较会因等级已提升而检测不到升级
        if (m_villagerData.level() > prevLevel) {
            // 设置升级计时器（40 tick = 2秒），在交易界面关闭后递减
            // 计时器到期时升级交易列表并给予再生效果
            m_updateMerchantTimer = 40;
            m_increaseProfessionLevelOnUpdate = true;
            m_prevLevelBeforeTrade = prevLevel; // 记录升级前等级，用于生成中间等级交易
            xpOrbCount += 5;
        }

        // 在村民位置上方0.5格生成经验球
        entity::ExperienceDropHandler::spawnExperienceOrbs(m_world, x(), y() + 0.5, z(), xpOrbCount);
    }
}

void VillagerEntity::_handleTradeReputation()
{
    // 交易完成后更新村庄声望（GossipType::Trading, +1）并播放开心村民粒子
    if (m_lastTradedPlayer == nullptr || m_world == nullptr) {
        return;
    }

    // 更新村庄声望：Trading 类型，每次交易 +1（最大累积100次）
    auto* villageManager = m_world->villageManager();
    if (villageManager != nullptr) {
        world::village::Village* village =
            villageManager->getVillageAt(BlockPos(static_cast<i32>(x()), static_cast<i32>(y()), static_cast<i32>(z())));
        if (village != nullptr) {
            // PlayerId 类型即为 u64，直接使用
            const PlayerId playerId = m_lastTradedPlayer->playerId();
            village->addGossip(playerId, world::village::VillageGossipType::Trading, 1);
        }
    }

    // 播放开心村民粒子效果
    m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::VillagerHappy));
}

void VillagerEntity::_increaseMerchantCareer()
{
    // 升级村民职业等级逻辑：
    // 1. 等级已在 addExperience() 中递增，此处不需要再次升级
    // 2. 为所有新等级生成交易并追加到现有交易列表（不替换）
    //
    // 关键：addExperience() 内部的 while 循环可能一次跳过多个等级
    // （例如从1级直接升到3级），此时需要为2级和3级都生成交易。
    // m_prevLevelBeforeTrade 记录了升级前的等级，用于确定需要补充的等级范围。

    if (isNitwit()) {
        // 傻子村民没有交易
        return;
    }

    using namespace world::village::trade;

    // 使用世界种子和实体ID生成唯一种子，确保每个村民交易不同
    u64 seed = 0;
    if (m_world) {
        seed = m_world->seed();
    }
    seed = seed * 31 + static_cast<u64>(id());

    // 为所有新等级（prevLevel+1 到 currentLevel）生成交易并追加到现有列表
    // 注意：等级已由 addExperience() 正确递增，不需要再调用 setLevel()
    const i32 currentLevel = m_villagerData.level();
    const i32 prevLevel = m_prevLevelBeforeTrade;

    for (i32 level = prevLevel + 1; level <= currentLevel; ++level) {
        auto newOffers = VillagerTrades::generateOffers(m_villagerData.profession(),
            m_villagerData.type(),
            level,
            0,                               // demand
            seed + static_cast<u64>(level)); // 不同等级使用不同种子

        if (newOffers && m_offers) {
            // 将新等级的交易追加到现有交易列表
            for (size_t i = 0; i < newOffers->size(); ++i) {
                MerchantOffer* offer = newOffers->getOffer(i);
                if (offer != nullptr) {
                    m_offers->addOffer(std::make_unique<MerchantOffer>(*offer));
                }
            }
        } else if (newOffers && !m_offers) {
            // 如果现有交易列表不存在（不应该发生，但防御性处理），直接替换
            m_offers = std::move(newOffers);
        }
    }
}

// ============================================================================
// WanderingTraderEntity
// ============================================================================

std::unique_ptr<Entity> WanderingTraderEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<WanderingTraderEntity>(0, registry);
}

WanderingTraderEntity::WanderingTraderEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AbstractVillagerEntity(id, registry)
{
    m_despawnDelay = 48000; // 40分钟 = 48000 ticks
    registerAttributes();
    registerGoals();
}

void WanderingTraderEntity::tick()
{
    AbstractVillagerEntity::tick();

    // 消失倒计时
    if (m_despawnDelay > 0) {
        m_despawnDelay--;
    }

    // 如果没有交易对象且消失时间到，消失
    if (!isTrading() && canDespawn()) {
        remove();
    }
}

void WanderingTraderEntity::rewardTradeXp(MerchantOffer& offer)
{
    // 流浪商人没有升级系统，但交易成功时仍会生成经验球给玩家
    if (offer.shouldRewardExp() && m_world) {
        // 经验球值 = 3 + random(0~3)，即 3~6
        i32 xpOrbCount = 3 + getRandom().nextInt(4);

        // 在流浪商人位置上方0.5格生成经验球
        entity::ExperienceDropHandler::spawnExperienceOrbs(m_world, x(), y() + 0.5, z(), xpOrbCount);
    }
}

// TODO: spawnLlamas 当前为死代码——全代码库无调用者。流浪商人自然生成时应由
// WanderingTraderSpawner（对齐 Java WanderingTraderSpawner）在生成时调 setLlamaCount(2)
// + spawnLlamas() 生成 2 只拴绳贸易羊驼，但本项目 WanderingTraderSpawner 未接入此调用，
// 且 tick() 不触发。故 test.spawn("wandering_trader") 后永远不会出现 trader_llama，
// 商队行为无法端到端测试。待 WanderingTraderSpawner 自然生成链路接入后补全调用。
void WanderingTraderEntity::spawnLlamas()
{
    if (m_hasLlamas || m_llamaCount <= 0 || m_world == nullptr) {
        return;
    }

    // 在流浪商人附近生成贸易羊驼
    // 最多2只羊驼，生成在商人后方
    // ECS 迁移：实体构造需要 registry 句柄（m_world 已判空，此处 registry 必非空）
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return;
    }
    math::Random& rng = m_world->getRandom();

    for (i32 i = 0; i < m_llamaCount && i < 2; ++i) {
        // 计算生成位置：在商人附近随机位置
        f64 offsetX = (rng.nextDouble() - 0.5) * 4.0; // -2 到 +2 格
        f64 offsetZ = (rng.nextDouble() - 0.5) * 4.0;

        f64 spawnX = x() + offsetX;
        f64 spawnZ = z() + offsetZ;
        f64 spawnY = y();

        // 创建商队羊驼
        auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(0), *registry);
        llama->setTypeId(EntityTypeKeys::TRADER_LLAMA); // 工厂绕过补救：直接构造缺 typeId
        llama->setPosition(spawnX, spawnY, spawnZ);
        llama->setDespawnDelay(m_despawnDelay - 1); // 羊驼比商人早消失1 tick

        // 生成羊驼并在生成后将拴绳绑定到流浪商人
        // 注意：setLeashedToEntity 需要实体已拥有有效的 UUID，
        // 因此拴绳绑定必须在 spawnEntity() 之后执行
        EntityInstanceId llamaId = m_world->spawnEntity(std::move(llama));
        Entity* spawnedLlama = m_world->getEntity(llamaId);
        if (spawnedLlama != nullptr) {
            auto* traderLlama = dynamic_cast<TraderLlamaEntity*>(spawnedLlama);
            if (traderLlama != nullptr) {
                traderLlama->setLeashedToEntity(uuid());
            }
        }
    }

    m_hasLlamas = true;
}

void WanderingTraderEntity::registerGoals()
{
    using namespace ai::goal;
    using namespace ai::goal::wandering_trader;

    AgeableEntity::registerGoals();

    // ========== 优先级 0: 基础生存 ==========
    // SwimGoal - 游泳
    m_goalSelector.addGoal(0, std::make_unique<SwimGoal>(this));

    // UseItemGoal - 夜间喝隐身药水
    m_goalSelector.addGoal(0,
        std::make_unique<UseItemGoal>(this,
            potion::PotionUtils::createPotionItem(potion::Potions::INVISIBILITY),
            SoundEvents::ENTITY_WANDERING_TRADER_DISAPPEARED,
            [this](MobEntity* mob) -> bool {
                auto* world = mob->world();
                return world != nullptr && !world->isBrightOutside() &&
                    !mob->hasEffect(effect::EffectType::Invisibility);
            }));

    // UseItemGoal - 白天喝牛奶恢复可见
    m_goalSelector.addGoal(0,
        std::make_unique<UseItemGoal>(this,
            ItemStack(*Items::MILK_BUCKET),
            SoundEvents::ENTITY_WANDERING_TRADER_REAPPEARED,
            [this](MobEntity* mob) -> bool {
                auto* world = mob->world();
                return world != nullptr && world->isBrightOutside() && mob->hasEffect(effect::EffectType::Invisibility);
            }));

    // ========== 优先级 1: 交易相关 ==========
    // TradeWithPlayerGoal - 与玩家交易
    m_goalSelector.addGoal(1, std::make_unique<TradeWithPlayerGoal>(this));

    // ========== 优先级 1: 逃避威胁 ==========
    // AvoidEntityGoal - 躲避僵尸类（对齐 Java VillagerHostilesSensor.ACCEPTABLE_DISTANCE_FROM_HOSTILES，
    //   zombie=8/drowned=8/husk=8/zombie_villager=8；vanilla 白名单**无** ZOMBIFIED_PIGLIN，
    //   此前误列僵尸猪灵已移除——僵尸猪灵虽 extends Zombie，但 vanilla 用精确白名单非 instanceof Zombie）。
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            8.0f, // 躲避距离
            0.5,  // 远距离速度
            0.5,  // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr &&
                    (entity->entityType() == entity::VanillaEntityTypeKeys::ZOMBIE ||
                        entity->entityType() == entity::VanillaEntityTypeKeys::DROWNED ||
                        entity->entityType() == entity::VanillaEntityTypeKeys::HUSK ||
                        entity->entityType() == entity::VanillaEntityTypeKeys::ZOMBIE_VILLAGER);
            }));

    // AvoidEntityGoal - 躲避劫毁兽（vanilla ravager=12，此前缺失）
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            12.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::RAVAGER;
            }));

    // AvoidEntityGoal - 躲避掠夺者
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            15.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PILLAGER;
            }));

    // AvoidEntityGoal - 躲避唤魔者
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            12.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::EVOKER;
            }));

    // AvoidEntityGoal - 躲避卫道士
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            8.0f, // 躲避距离
            0.5,  // 远距离速度
            0.5,  // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::VINDICATOR;
            }));

    // AvoidEntityGoal - 躲避恼鬼
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            8.0f, // 躲避距离
            0.5,  // 远距离速度
            0.5,  // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::VEX;
            }));

    // AvoidEntityGoal - 躲避幻术师
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            12.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::ILLUSIONER;
            }));

    // AvoidEntityGoal - 躲避疣猪兽
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            10.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::ZOGLIN;
            }));

    // PanicGoal - 恐慌逃跑
    m_goalSelector.addGoal(1, std::make_unique<PanicGoal>(this, 0.5));

    // LookAtCustomerGoal - 看向顾客
    m_goalSelector.addGoal(1, std::make_unique<LookAtCustomerGoal>(this));

    // ========== 优先级 2: 移动 ==========
    // MoveToWanderTargetGoal - 向游荡目标移动
    m_goalSelector.addGoal(2, std::make_unique<MoveToWanderTargetGoal>(this, 2.0, 0.35));

    // ========== 优先级 4: 限制范围 ==========
    // MoveTowardsRestrictionGoal - 向限制点移动
    m_goalSelector.addGoal(4, std::make_unique<MoveTowardsRestrictionGoal>(this, 0.35));

    // ========== 优先级 8: 随机移动 ==========
    // WaterAvoidingRandomWalkingGoal - 避水随机行走
    m_goalSelector.addGoal(8, std::make_unique<WaterAvoidingRandomWalkingGoal>(this, 0.35));

    // ========== 优先级 9: 看向 ==========
    // LookAtGoal - 看向玩家
    m_goalSelector.addGoal(
        9, std::make_unique<LookAtGoal>(this, 3.0f, LookAtGoal::DEFAULT_LOOK_CHANCE, TypeFilter<Player>{}));

    // ========== 优先级 10: 看向生物 ==========
    // LookAtGoal - 看向附近生物
    m_goalSelector.addGoal(10, std::make_unique<LookAtGoal>(this, 8.0f));
}

void WanderingTraderEntity::registerAttributes()
{
    AgeableEntity::registerAttributes();

    // 流浪商人属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
}

void WanderingTraderEntity::updateOffers()
{
    using namespace world::village::trade;

    // 确保交易系统已初始化
    if (!WanderingTraderTrades::isInitialized()) {
        WanderingTraderTrades::initialize();
    }

    // 获取世界种子作为交易生成的随机种子
    u64 seed = 0;
    if (m_world) {
        seed = m_world->seed();
    }

    // 使用实体ID混合种子，确保每个流浪商人有独特的交易
    seed = seed * 31 + static_cast<u64>(id());

    // 生成交易列表
    m_offers = WanderingTraderTrades::generateOffers(seed);
}

} // namespace entity
} // namespace mc
