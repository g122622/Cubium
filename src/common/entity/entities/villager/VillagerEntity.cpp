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
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/village/trade/Merchant.hpp"
#include "../../../world/village/trade/VillagerTrades.hpp"
#include "../../../world/village/trade/WanderingTraderTrades.hpp"
#include "../../ai/brain/memory/MemoryModuleType.hpp"
#include "../../ai/brain/schedule/Activity.hpp"
#include "../../ai/brain/schedule/Schedule.hpp"
#include "../../ai/brain/sensor/Sensors.hpp"
#include "../../ai/goal/goals/villager/VillagerGoals.hpp"
#include "../../attribute/Attributes.hpp"
#include <memory>

namespace mc {
namespace entity {

// ============================================================================
// VillagerEntity
// ============================================================================

std::unique_ptr<Entity> VillagerEntity::create(IWorld* /*world*/)
{
    return std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 0);
}

VillagerEntity::VillagerEntity(LegacyEntityType type, EntityId id)
    : AbstractVillagerEntity(type, id)
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
        i32 dayTime = static_cast<i32>(m_world->dayTime());

        // Brain tick使用 IWorld 和 Random
        math::Random random(ticksExisted());
        m_brain->tick(m_world, this, gameTime, dayTime, random);
    }

    // 更新声音冷却
    if (m_soundCooldown > 0) {
        m_soundCooldown--;
    }

    // 检查工作站点
    // TODO: 检查是否在工作时间且在工作站点附近
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

std::unique_ptr<AgeableEntity> VillagerEntity::createChild()
{
    auto child = std::make_unique<VillagerEntity>(LegacyEntityType::Unknown, 0);
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
    m_working = false;
    m_atWorkstation = false;
    // TODO: 去睡觉
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
    // TODO: 与其他村民互动
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
    return std::make_unique<WanderingTraderEntity>(LegacyEntityType::Unknown, EntityId(0));
}

WanderingTraderEntity::WanderingTraderEntity(LegacyEntityType type, EntityId id)
    : AbstractVillagerEntity(type, id)
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
    // 流浪商人会自动补充交易
    // TODO: 实现交易补充
}

void WanderingTraderEntity::spawnLlamas()
{
    if (m_hasLlamas || m_llamaCount <= 0) {
        return;
    }

    // TODO: 在流浪商人附近生成贸易羊驼
    // 每只羊驼有两个箱子用于存储物品

    m_hasLlamas = true;
}

void WanderingTraderEntity::registerGoals()
{
    AgeableEntity::registerGoals();

    // TODO: 添加流浪商人特有AI目标
    // - 寻找村庄
    // - 补充交易
    // - 逃跑（遇到威胁时）
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
