#include "AbstractVillagerEntity.hpp"
#include "../../entities/player/Player.hpp"
#include <algorithm>

namespace mc {
namespace entity {

// ============================================================================
// VillagerData
// ============================================================================

VillagerData::VillagerData(VillagerType type, VillagerProfession profession, i32 level)
    : m_type(type)
    , m_profession(profession)
    , m_level(level)
{
}

void VillagerData::setProfession(VillagerProfession profession) {
    m_profession = profession;
    // 改变职业时重置等级和经验
    m_level = 1;
    m_experience = 0;
}

void VillagerData::setLevel(i32 level) {
    m_level = std::clamp(level, 1, getMaxLevel());
}

void VillagerData::addExperience(i32 amount) {
    m_experience += amount;

    // 检查升级
    while (m_level < getMaxLevel() && m_experience >= getExperienceForLevel(m_level)) {
        m_experience -= getExperienceForLevel(m_level);
        m_level++;
    }
}

i32 VillagerData::getExperienceForLevel(i32 level) {
    // 参考 MC 1.16.5 升级经验表
    switch (level) {
        case 1: return 10;
        case 2: return 70;
        case 3: return 150;
        case 4: return 250;
        default: return 0;
    }
}

// ============================================================================
// AbstractVillagerEntity
// ============================================================================

AbstractVillagerEntity::AbstractVillagerEntity(LegacyEntityType type, EntityId id)
    : AgeableEntity(type, id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(8))  // 8格库存
{
}

void AbstractVillagerEntity::tick() {
    AgeableEntity::tick();

    // 更新交易状态
    if (m_tradingPlayer && !m_tradingPlayer->isAlive()) {
        stopTrading();
    }
}

void AbstractVillagerEntity::setOffers(std::unique_ptr<MerchantOffers> offers) {
    m_offers = std::move(offers);
}

void AbstractVillagerEntity::updateOffers() {
    // 子类实现
}

void AbstractVillagerEntity::startTrading(Player* player) {
    m_tradingPlayer = player;
    // TODO: 打开交易界面
}

void AbstractVillagerEntity::stopTrading() {
    m_tradingPlayer = nullptr;
}

void AbstractVillagerEntity::addExperience(i32 amount) {
    m_experience += amount;
}

f32 AbstractVillagerEntity::experienceProgress() const {
    // 村民最高等级为 5，已满级则进度为 0
    if (m_experience <= 0) {
        return 0.0f;
    }

    // 获取当前等级（子类可能有自己的等级系统）
    i32 currentLevel = getTradingLevel();
    if (currentLevel >= VillagerData::getMaxLevel()) {
        // 已满级，进度为 0（不再显示经验条）
        return 0.0f;
    }

    // 获取升到下一级所需的经验
    i32 requiredXp = VillagerData::getExperienceForLevel(currentLevel);
    if (requiredXp <= 0) {
        return 0.0f;
    }

    // 计算进度（0-1）
    return static_cast<f32>(m_experience) / static_cast<f32>(requiredXp);
}

void AbstractVillagerEntity::resetBreedWillingness() {
    // 繁殖后重置繁殖意愿
    m_willingToBreed = false;
}

} // namespace entity
} // namespace mc
