#include "VillagerEntity.hpp"
#include "../../../item/ItemStack.hpp"
#include "../../../item/Items.hpp"
#include "../../../world/IWorld.hpp"
#include "../../attribute/Attributes.hpp"
#include <memory>

namespace mc {
namespace entity {

// ============================================================================
// VillagerEntity
// ============================================================================

std::unique_ptr<Entity> VillagerEntity::create(IWorld* /*world*/) {
    return std::make_unique<VillagerEntity>(LegacyEntityType::Unknown, 0);
}

VillagerEntity::VillagerEntity(LegacyEntityType type, EntityId id)
    : AbstractVillagerEntity(type, id)
{
    registerAttributes();
    registerGoals();
}

void VillagerEntity::tick() {
    AbstractVillagerEntity::tick();

    // 更新声音冷却
    if (m_soundCooldown > 0) {
        m_soundCooldown--;
    }

    // 检查工作站点
    // TODO: 检查是否在工作时间且在工作站点附近
}

void VillagerEntity::setProfession(VillagerProfession profession) {
    m_villagerData.setProfession(profession);

    // 根据职业更新交易列表
    updateOffers();
}

bool VillagerEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 村民用食物繁殖：面包、土豆、胡萝卜、甜菜根
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;

    return item == Items::BREAD ||
           item == Items::POTATO ||
           item == Items::CARROT ||
           item == Items::BEETROOT;
}

std::unique_ptr<AgeableEntity> VillagerEntity::createChild() {
    auto child = std::make_unique<VillagerEntity>(LegacyEntityType::Unknown, 0);
    child->setChild(true);

    // 继承村民类型
    child->setVillagerType(m_villagerData.type());

    // 设置位置
    child->setPosition(x(), y(), z());

    return child;
}

bool VillagerEntity::canWork() const {
    // 不是傻子且有工作站点
    return !isNitwit() &&
           m_workStation.x != 0 || m_workStation.y != 0 || m_workStation.z != 0;
}

void VillagerEntity::rest() {
    m_working = false;
    m_atWorkstation = false;
    // TODO: 去睡觉
}

void VillagerEntity::work() {
    m_working = true;
    m_workTime++;

    // 检查是否需要补货
    if (m_needsRestock || m_workTime % 24000 == 0) {
        restockTrades();
        m_needsRestock = false;
    }
}

void VillagerEntity::play() {
    // TODO: 与其他村民互动
}

void VillagerEntity::registerGoals() {
    AgeableEntity::registerGoals();

    // TODO: 添加村民特有AI目标
    // - 寻找工作站点
    // - 工作
    // - 睡觉
    // - 闲逛
    // - 与玩家交易
    // - 繁殖
}

void VillagerEntity::registerAttributes() {
    AgeableEntity::registerAttributes();

    // 村民属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
}

void VillagerEntity::restockTrades() {
    // TODO: 补充交易物品
    m_lastRestock = m_workTime;
}

// ============================================================================
// WanderingTraderEntity
// ============================================================================

std::unique_ptr<Entity> WanderingTraderEntity::create(IWorld* /*world*/) {
    return std::make_unique<WanderingTraderEntity>(LegacyEntityType::Unknown, 0);
}

WanderingTraderEntity::WanderingTraderEntity(LegacyEntityType type, EntityId id)
    : AbstractVillagerEntity(type, id)
{
    m_despawnDelay = 48000;  // 40分钟 = 48000 ticks
    registerAttributes();
    registerGoals();
}

void WanderingTraderEntity::tick() {
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

void WanderingTraderEntity::restockTrades() {
    // 流浪商人会自动补充交易
    // TODO: 实现交易补充
}

void WanderingTraderEntity::spawnLlamas() {
    if (m_hasLlamas || m_llamaCount <= 0) {
        return;
    }

    // TODO: 在流浪商人附近生成贸易羊驼
    // 每只羊驼有两个箱子用于存储物品

    m_hasLlamas = true;
}

void WanderingTraderEntity::registerGoals() {
    AgeableEntity::registerGoals();

    // TODO: 添加流浪商人特有AI目标
    // - 寻找村庄
    // - 补充交易
    // - 逃跑（遇到威胁时）
}

void WanderingTraderEntity::registerAttributes() {
    AgeableEntity::registerAttributes();

    // 流浪商人属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.5);
}

void WanderingTraderEntity::updateOffers() {
    // 流浪商人的交易列表是固定的
    // TODO: 根据世界种子生成交易列表
}

} // namespace entity
} // namespace mc
