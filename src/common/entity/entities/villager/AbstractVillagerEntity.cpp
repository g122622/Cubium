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
#include "common/core/Types.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/container/MerchantContainerMenu.hpp"
#include "common/item/Items.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <utility>

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
    // 升级经验表
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

AbstractVillagerEntity::AbstractVillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AgeableEntity(id, registry)
    , m_inventory(std::make_unique<blockentity::SimpleInventory>(8)) // 8格库存
{}

void AbstractVillagerEntity::tick()
{
    AgeableEntity::tick();

    // 不高兴计数器每 tick 递减（对齐 vanilla AbstractVillager.tick）。
    if (m_unhappyCounter > 0) {
        --m_unhappyCounter;
    }

    // 更新交易状态
    if (m_tradingPlayer && !m_tradingPlayer->isAlive()) {
        stopTrading();
    }
}

ActionResultType AbstractVillagerEntity::interactMob(Player& player, Hand hand)
{
    // 对齐 vanilla AbstractVillager#mobInteract：
    //   手持刷怪蛋 / 已死 / 交易中 / 睡觉 → 交基类；
    //   幼体 → 只表现不高兴；
    //   其余 → 服务端 startTrading + 打开交易界面，主手交互计入统计。
    const ItemStack& heldItem = player.getHeldItem(hand);
    const bool holdsSpawnEgg = heldItem.getItem() == Items::VILLAGER_SPAWN_EGG;

    if (holdsSpawnEgg || !isAlive() || isTrading() || pose() == EntityPose::Sleeping) {
        return MobEntity::interactMob(player, hand);
    }

    if (isChild()) {
        setUnhappy();
        return ActionResultType::Success;
    }

    if (!isClientSide()) {
        const bool noOffers = getOffers().empty();
        if (hand == Hand::MainHand) {
            if (noOffers) {
                setUnhappy();
            }
            player.awardCustomStat(ResourceLocation("minecraft:talked_to_villager"), 1);
        }

        if (noOffers) {
            return ActionResultType::Consume;
        }

        startTrading(&player);
        // 经 IWorld::openEntityContainer 打开交易界面（与箱子船共用同一条链路）。
        // 返回 false 表示服务端未接线/提供者拒绝打开——界面不开，但不影响本次交互成功语义。
        (void)player.openContainer(*this);
    }

    return ActionResultType::Success;
}

void AbstractVillagerEntity::setUnhappy()
{
    // 对齐 vanilla AbstractVillager#setUnhappy：置 40 tick 计数器，服务端播放否定音效。
    m_unhappyCounter = 40;
    if (!isClientSide()) {
        playSound(SoundEvents::ENTITY_VILLAGER_NO, 1.0f, 1.0f);
    }
}

void AbstractVillagerEntity::updateOffers()
{
    // 子类实现
}

// ============================================================================
// INamedContainerProvider 接口实现
// ============================================================================

std::unique_ptr<AbstractContainerMenu> AbstractVillagerEntity::createMenu(i32 containerId, Player& player)
{
    auto menu = std::make_unique<MerchantContainerMenu>(containerId, &player.inventory(), *this);

    // 设置交易界面的属性
    menu->setMerchantLevel(getTradingLevel());
    menu->setShowProgressBar(showProgressBar());
    menu->setCanRestock(canRestock());

    return menu;
}

std::string AbstractVillagerEntity::getDisplayName() const
{
    // 返回村民的自定义名称或默认名称
    if (hasCustomName()) {
        return customNameText();
    }
    // 默认返回实体类型名称
    return "Villager";
}

ContainerType AbstractVillagerEntity::getMenuType() const
{
    // 村民交易界面固定为 Merchant（对齐 vanilla MenuType.MERCHANT）。
    return ContainerType::Merchant;
}

// ============================================================================
// IMerchant 接口实现
// ============================================================================

MerchantOffers& AbstractVillagerEntity::getOffers()
{
    if (!m_offers) {
        m_offers = std::make_unique<MerchantOffers>();
        updateOffers();
    }
    return *m_offers;
}

const MerchantOffers& AbstractVillagerEntity::getOffers() const
{
    // 注意：const 版本不会懒加载，调用方应确保 offers 已存在
    MC_ASSERT_RELEASE(m_offers != nullptr);
    return *m_offers;
}

void AbstractVillagerEntity::setOffers(MerchantOffers offers)
{
    m_offers = std::make_unique<MerchantOffers>(std::move(offers));
}

void AbstractVillagerEntity::startTrading(Player* player)
{
    m_tradingPlayer = player;
}

void AbstractVillagerEntity::stopTrading()
{
    m_tradingPlayer = nullptr;
}

void AbstractVillagerEntity::addExperience(i32 amount)
{
    m_experience += amount;
}

void AbstractVillagerEntity::restock()
{
    if (m_offers) {
        m_offers->restockAll();
    }
}

void AbstractVillagerEntity::notifyTrade(MerchantOffer& offer)
{
    // 增加使用次数
    offer.increaseUses();

    // 重置环境音效计时器（村民发出"成交"音效）
    m_livingSoundTime = -getTalkInterval();

    // 给予交易经验奖励
    rewardTradeXp(offer);

    // 触发交易成就/进度
    if (m_tradingPlayer != nullptr && m_world != nullptr) {
        // resultItem = 玩家获得的物品（交易结果），paymentItem = 玩家付出的物品（支付物品）
        m_world->onVillagerTrade(m_tradingPlayer->id(), this, offer.getSell(), offer.getBuyA());
    }
}

void AbstractVillagerEntity::notifyTradeUpdated(const ItemStack& resultStack)
{
    // 在非客户端环境下，当交易输入变化时播放确认/否定音效
    if (!isClientSide()) {
        // 只有距离上次环境音效超过20 tick时才播放，避免频繁播放
        if (m_livingSoundTime > -getTalkInterval() + 20) {
            m_livingSoundTime = -getTalkInterval();

            if (!resultStack.isEmpty()) {
                playTradeSound(true);
            } else {
                playTradeSound(false);
            }
        }
    }
}

void AbstractVillagerEntity::overrideOffers(MerchantOffers offers)
{
    m_offers = std::make_unique<MerchantOffers>(std::move(offers));
}

bool AbstractVillagerEntity::isClientSide() const
{
    return m_world != nullptr && m_world->isClientSide();
}

bool AbstractVillagerEntity::stillValid(const Player& player) const
{
    if (m_tradingPlayer != &player || !isAlive()) {
        return false;
    }

    // 原版 MC 使用 AABB.distanceToSqr(eyePosition) 计算玩家眼睛到实体碰撞箱的距离，
    // 阈值为 entityInteractionRange + 4.0（默认 7.0 格）。
    // 当前使用实体脚底距离平方近似，阈值为 8 格（64 平方）。
    constexpr f32 MAX_TRADE_DISTANCE_SQ = 64.0f;
    return distanceSqTo(player) <= MAX_TRADE_DISTANCE_SQ;
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

void AbstractVillagerEntity::playTradeSound(bool success)
{
    const auto& soundId = success ? SoundEvents::ENTITY_VILLAGER_YES : SoundEvents::ENTITY_VILLAGER_NO;
    playSound(soundId, 1.0f, 1.0f);
}

} // namespace entity
} // namespace mc
