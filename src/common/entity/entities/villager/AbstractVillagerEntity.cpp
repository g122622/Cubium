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

#include "AbstractVillagerEntity.hpp"
#include "../../entities/player/Player.hpp"
#include "../../inventory/AbstractContainerMenu.hpp"
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
{}

void VillagerData::setProfession(VillagerProfession profession)
{
    m_profession = profession;
    // 改变职业时重置等级和经验
    m_level = 1;
    m_experience = 0;
}

void VillagerData::setLevel(i32 level)
{
    m_level = std::clamp(level, 1, getMaxLevel());
}

void VillagerData::addExperience(i32 amount)
{
    m_experience += amount;

    // 检查升级
    while (m_level < getMaxLevel() && m_experience >= getExperienceForLevel(m_level)) {
        m_experience -= getExperienceForLevel(m_level);
        m_level++;
    }
}

i32 VillagerData::getExperienceForLevel(i32 level)
{
    // 参考 MC 1.16.5 升级经验表
    switch (level) {
        case 1:
            return 10;
        case 2:
            return 70;
        case 3:
            return 150;
        case 4:
            return 250;
        default:
            return 0;
    }
}

// ============================================================================
// AbstractVillagerEntity
// ============================================================================

AbstractVillagerEntity::AbstractVillagerEntity(EntityId id)
    : AgeableEntity(id)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(8)) // 8格库存
{}

void AbstractVillagerEntity::tick()
{
    AgeableEntity::tick();

    // 更新交易状态
    if (m_tradingPlayer && !m_tradingPlayer->isAlive()) {
        stopTrading();
    }
}

void AbstractVillagerEntity::setOffers(std::unique_ptr<MerchantOffers> offers)
{
    m_offers = std::move(offers);
}

void AbstractVillagerEntity::updateOffers()
{
    // 子类实现
}

void AbstractVillagerEntity::startTrading(Player* player)
{
    m_tradingPlayer = player;
    // TODO: 打开交易界面
}

void AbstractVillagerEntity::stopTrading()
{
    m_tradingPlayer = nullptr;
}

// ============================================================================
// INamedContainerProvider 接口实现
// ============================================================================

std::unique_ptr<AbstractContainerMenu> AbstractVillagerEntity::createMenu(i32 containerId, Player& player)
{
    // TODO: 创建村民交易菜单
    // 需要实现 MerchantContainer 类
    // 参考 MC 1.16.5: AbstractVillagerEntity.createMenu()
    (void)containerId;
    (void)player;
    return nullptr;
}

std::string AbstractVillagerEntity::getDisplayName() const
{
    // 返回村民的自定义名称或默认名称
    // 参考 MC 1.16.5: AbstractVillagerEntity.getDisplayName()
    if (hasCustomName()) {
        return customNameText();
    }
    // 默认返回实体类型名称
    return "Villager";
}

void AbstractVillagerEntity::addExperience(i32 amount)
{
    m_experience += amount;
}

f32 AbstractVillagerEntity::experienceProgress() const
{
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

void AbstractVillagerEntity::resetBreedWillingness()
{
    // 繁殖后重置繁殖意愿
    m_willingToBreed = false;
}

} // namespace entity
} // namespace mc
