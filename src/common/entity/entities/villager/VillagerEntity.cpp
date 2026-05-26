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
#include "../../../item/Items.hpp"
#include "../../../item/core/ItemStack.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/village/VillageManager.hpp"
#include "../../../world/village/poi/PointOfInterestStorage.hpp"
#include "../../../world/village/trade/Merchant.hpp"
#include "../../../world/village/trade/VillagerTrades.hpp"
#include "../../../world/village/trade/WanderingTraderTrades.hpp"
#include "../../ai/brain/memory/MemoryModuleType.hpp"
#include "../../ai/brain/schedule/Activity.hpp"
#include "../../ai/brain/schedule/Schedule.hpp"
#include "../../ai/brain/sensor/Sensors.hpp"
#include "../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../ai/goal/goals/PanicGoal.hpp"
#include "../../ai/goal/goals/SwimGoal.hpp"
#include "../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../ai/goal/goals/special/WanderingTraderGoals.hpp"
#include "../../ai/goal/goals/villager/VillagerGoals.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../core/EntityPose.hpp"
#include "../../entities/passive/horse/TraderLlamaEntity.hpp"
#include <memory>

namespace mc {
namespace entity {

// ============================================================================
// VillagerEntity
// ============================================================================

std::unique_ptr<Entity> VillagerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<VillagerEntity>(0);
}

VillagerEntity::VillagerEntity(EntityId id)
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

        // Brain tick使用 IWorld 和 Random
        math::Random random(ticksExisted());
        m_brain->tick(m_world, this, gameTime, dayTime, random);
    }

    // 更新声音冷却
    if (m_soundCooldown > 0) {
        m_soundCooldown--;
    }

    // 工作站点检查由 WorkAtJobSiteGoal 自动处理
    // - shouldExecute() 检查是否是工作时间 (2000-9000 ticks) 和是否有工作站点
    // - tick() 中使用 isWithinDistance() 检查是否在工作站点附近
    // - Schedule 系统在 2000 ticks 时自动切换到 WORK 活动
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

    // 注册传感器
    m_brain->registerSensor(std::make_unique<NearestPlayersSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<NearestVisibleLivingEntitySensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<HurtBySensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<MobSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<WorkStationSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<VillagePoiSensor<VillagerEntity>>());
    m_brain->registerSensor(std::make_unique<BabySensor<VillagerEntity>>());

    // 设置日程
    m_brain->setSchedule(&ai::brain::schedule::Schedule::VILLAGER_DEFAULT);

    // 设置默认活动
    m_brain->setDefaultActivities({ai::brain::schedule::Activity::IDLE});
    m_brain->setFallbackActivity(ai::brain::schedule::Activity::IDLE);
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

bool VillagerEntity::canPickUpItem(const ItemStack& itemStack) const
{
    // 参考 MC 1.16.5 VillagerEntity.func_230293_i_()
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

    // 农民职业额外可拾取：小麦、小麦种子、甜菜根种子、骨粉
    // 注意：小麦、小麦种子、甜菜根种子已在上面检查
    // 农民特有物品已在 MC 1.16.5 中通过 VillagerProfession.getSpecificItems() 实现
    // 但本项目中农民职业特有物品就是上面列出的物品，所以无需额外检查

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
    // MC 1.16.5: 村民的睡眠由Brain系统自动管理，不需要在此主动触发
    // - Schedule::VILLAGER_DEFAULT 在游戏时间12000 ticks时切换到 Activity::REST
    // - SleepAtNightGoal 在REST活动期间自动检查睡眠条件并执行睡眠
    // - 参考: SleepAtNightGoal::trySleep() -> VillagerEntity::startSleeping()
    m_working = false;
    m_atWorkstation = false;
}

void VillagerEntity::work()
{
    m_working = true;
    m_workTime++;

    // 检查是否需要补货
    if (m_needsRestock || m_workTime % 24000 == 0) {
        restockTrades();
        m_needsRestock = false;
    }
}

void VillagerEntity::play()
{
    // 参考 MC 1.16.5 VillagerEntity.play() 方法
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

    // 参考 MC 1.16.5 VillagerEntity.func_242368_a()
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

    // 参考 MC 1.16.5: 流言传播时会减少衰减值
    // transferFrom() 方法会在传播时减少流言值
    // 这里简化实现，实际需要 VillageGossipManager::transferFrom()

    // 尝试生成铁傀儡（如果村民足够多且声誉足够高）
    // 参考 MC 1.16.5 VillagerEntity.func_242367_a()
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

void VillagerEntity::restockTrades()
{
    // 补充交易物品
    if (m_offers) {
        m_offers->restockAll();
    }
    m_lastRestock = m_workTime;
}

// ========== 睡眠相关 ==========

bool VillagerEntity::isSleeping() const
{
    return pose() == EntityPose::Sleeping;
}

void VillagerEntity::startSleeping(BlockPos pos)
{
    // 参考 MC 1.16.5 LivingEntity.startSleeping()

    // 如果正在骑乘，先停止骑乘
    if (getVehicle() != INVALID_ENTITY_ID) {
        stopRiding();
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
    // 参考 MC 1.16.5 LivingEntity.wakeUp()

    // 只有在睡眠时才需要唤醒
    if (!isSleeping()) {
        return;
    }

    // 如果有睡眠位置，计算唤醒位置
    if (m_sleepingPos.has_value() && m_world) {
        BlockPos bedPos = m_sleepingPos.value();

        // 计算唤醒位置（床旁边）
        // 参考 MC 1.16.5 BedBlock.getWakeUpPosition()
        // 简化实现：在床的朝向方向找一个空位
        // 这里暂时使用床上方位置
        Vector3d wakeUpPos(bedPos.x + 0.5, bedPos.y + 1.0, bedPos.z + 0.5);

        // 设置位置
        setPosition(wakeUpPos.x, wakeUpPos.y, wakeUpPos.z);
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

void VillagerEntity::updateOffers()
{
    // 根据职业和等级生成交易列表
    using namespace world::village::trade;

    // 傻子村民没有交易
    if (isNitwit()) {
        m_offers = std::make_unique<MerchantOffers>();
        return;
    }

    // 生成新的交易列表
    m_offers = VillagerTrades::generateOffers(m_villagerData.profession(),
        m_villagerData.type(),
        m_villagerData.level(),
        0, // 需求修正
        0  // 种子
    );
}

// ============================================================================
// WanderingTraderEntity
// ============================================================================

std::unique_ptr<Entity> WanderingTraderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<WanderingTraderEntity>(0);
}

WanderingTraderEntity::WanderingTraderEntity(EntityId id)
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

void WanderingTraderEntity::restockTrades()
{
    // MC 1.16.5 WanderingTraderEntity.restock()
    // 流浪商人会自动补充已用完的交易

    if (m_offers == nullptr) {
        return;
    }

    // 遍历所有交易，补充已用完的
    for (size_t i = 0; i < m_offers->size(); ++i) {
        MerchantOffer* offer = m_offers->getOffer(i);
        if (offer != nullptr && offer->isOutOfStock()) {
            // 重置交易次数
            offer->restock();
        }
    }
}

void WanderingTraderEntity::spawnLlamas()
{
    // MC 1.16.5 WanderingTraderEntity.spawnLlamas()
    if (m_hasLlamas || m_llamaCount <= 0 || m_world == nullptr) {
        return;
    }

    // 在流浪商人附近生成贸易羊驼
    // MC 1.16.5: 最多2只羊驼，生成在商人后方
    math::Random& rng = m_world->getRandom();

    for (i32 i = 0; i < m_llamaCount && i < 2; ++i) {
        // 计算生成位置：在商人附近随机位置
        f64 offsetX = (rng.nextDouble() - 0.5) * 4.0; // -2 到 +2 格
        f64 offsetZ = (rng.nextDouble() - 0.5) * 4.0;

        f64 spawnX = x() + offsetX;
        f64 spawnZ = z() + offsetZ;
        f64 spawnY = y();

        // 创建商队羊驼
        auto llama = std::make_unique<TraderLlamaEntity>(EntityId(0));
        llama->setPosition(spawnX, spawnY, spawnZ);
        llama->setDespawnDelay(m_despawnDelay - 1); // 羊驼比商人早消失1 tick

        // TODO: 当拴绳系统实现后，将羊驼拴在商人身上
        // llama->attachToEntity(this, ...);

        m_world->spawnEntity(std::move(llama));
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
                    (entity->typeId() == entity::EntityTypeIdNumber::ZOMBIE ||
                        entity->typeId() == entity::EntityTypeIdNumber::DROWNED ||
                        entity->typeId() == entity::EntityTypeIdNumber::HUSK ||
                        entity->typeId() == entity::EntityTypeIdNumber::ZOMBIFIED_PIGLIN);
            }));

    // AvoidEntityGoal - 躲避掠夺者
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            15.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::PILLAGER;
            }));

    // AvoidEntityGoal - 躲避唤魔者
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            12.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::EVOKER;
            }));

    // AvoidEntityGoal - 躲避卫道士
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            8.0f, // 躲避距离
            0.5,  // 远距离速度
            0.5,  // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::VINDICATOR;
            }));

    // AvoidEntityGoal - 躲避恼鬼
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            8.0f, // 躲避距离
            0.5,  // 远距离速度
            0.5,  // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::VEX;
            }));

    // AvoidEntityGoal - 躲避幻术师
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            12.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::ILLUSIONER;
            }));

    // AvoidEntityGoal - 躲避疣猪兽
    m_goalSelector.addGoal(1,
        std::make_unique<AvoidEntityGoal>(this,
            10.0f, // 躲避距离
            0.5,   // 远距离速度
            0.5,   // 近距离速度
            [](const LivingEntity* entity) -> bool {
                return entity != nullptr && entity->typeId() == entity::EntityTypeIdNumber::ZOGLIN;
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
