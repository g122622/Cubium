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
#include "common/entity/ai/brain/memory/MemoryModuleStatus.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/brain/schedule/Activity.hpp"
#include "common/entity/ai/brain/schedule/Schedule.hpp"
#include "common/entity/ai/brain/sensor/Sensors.hpp"
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
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/effect/EffectType.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceDropHandler.hpp"
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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/VillageGossipType.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "common/world/village/trade/Merchant.hpp"
#include "common/world/village/trade/VillagerTrades.hpp"
#include "common/world/village/trade/WanderingTraderTrades.hpp"
#include <cmath>
#include <memory>

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

std::unique_ptr<Entity> VillagerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<VillagerEntity>(0);
}

VillagerEntity::VillagerEntity(EntityInstanceId id)
    : AbstractVillagerEntity(id)
    , m_brain(std::make_unique<VillagerBrain>())
{
    registerAttributes();
    registerGoals();
    initializeBrain();
}

void VillagerEntity::tick()
{
    AbstractVillagerEntity::tick();

    // 更新Brain系统
    if (m_brain && m_world) {
        // 获取游戏时间和白天时间
        i64 gameTime = m_world->currentTick();
        i32 dayTime = static_cast<i32>(m_world->dayTimeOfDay());

        // Brain tick使用实体的持久化随机数生成器
        m_brain->tick(m_world, this, gameTime, dayTime, getRandom());
    }

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

void VillagerEntity::remove()
{
    // 村民被移除时释放POI并通知村庄。村民不会因距离远而消失，
    // 但当确实被移除时（死亡动画结束、区块卸载等）需要清理POI占用。

    releaseAllPois();

    // 调用父类 remove()
    AbstractVillagerEntity::remove();
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

std::unique_ptr<AgeableEntity> VillagerEntity::createChild()
{
    auto child = std::make_unique<VillagerEntity>(0);
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

    // 从 Brain 获取交互目标
    auto targetMemory = m_brain->getMemory<LivingEntity*>(ai::brain::memory::MemoryModuleTypes::INTERACTION_TARGET);
    if (!targetMemory.has_value() || !*targetMemory) {
        return;
    }

    LivingEntity* target = *targetMemory;
    if (!target->isAlive()) {
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
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
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

std::unique_ptr<Entity> WanderingTraderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WanderingTraderEntity>(0);
}

WanderingTraderEntity::WanderingTraderEntity(EntityInstanceId id)
    : AbstractVillagerEntity(id)
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

void WanderingTraderEntity::spawnLlamas()
{
    if (m_hasLlamas || m_llamaCount <= 0 || m_world == nullptr) {
        return;
    }

    // 在流浪商人附近生成贸易羊驼
    // 最多2只羊驼，生成在商人后方
    math::Random& rng = m_world->getRandom();

    for (i32 i = 0; i < m_llamaCount && i < 2; ++i) {
        // 计算生成位置：在商人附近随机位置
        f64 offsetX = (rng.nextDouble() - 0.5) * 4.0; // -2 到 +2 格
        f64 offsetZ = (rng.nextDouble() - 0.5) * 4.0;

        f64 spawnX = x() + offsetX;
        f64 spawnZ = z() + offsetZ;
        f64 spawnY = y();

        // 创建商队羊驼
        auto llama = std::make_unique<TraderLlamaEntity>(EntityInstanceId(0));
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
    // AvoidEntityGoal - 躲避僵尸
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
                        entity->entityType() == entity::VanillaEntityTypeKeys::ZOMBIFIED_PIGLIN);
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
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
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
