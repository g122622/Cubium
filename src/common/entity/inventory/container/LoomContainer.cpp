/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software", to deal
 * in the Software without restriction, without without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "entity/inventory/container/LoomContainer.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/BannerPattern.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/IInventory.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "item/items/BannerPatternItem.hpp"
#include "item/items/DyeItemHelper.hpp"
#include "item/items/block/BannerItem.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/interactive/BannerEntity.hpp"
#include <algorithm>
#include <memory>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace entity {
namespace inventory {
namespace container {

// ========== 染料物品工具函数 ==========

namespace {

/// 检查物品是否为染料（委托 DyeItemHelper，避免各子系统重复维护染料集合）
bool _isDyeItem(const Item* item)
{
    return item::DyeItemHelper::isDyeItem(item);
}

/// 获取染料物品对应的颜色（调用方须先经 _isDyeItem 判定）
DyeColor _dyeColorFromItem(const Item* item)
{
    const auto color = item::DyeItemHelper::dyeColorOfItem(item);
    MC_ASSERT_RELEASE(color.has_value());
    return *color;
}

// ========== LoomContainer 实现 ==========

} // namespace

LoomContainer::LoomContainer(
    ContainerId id, PlayerInventory* playerInventory, std::unique_ptr<IInventory> inventory, const BlockPos& pos)
    : AbstractContainerMenu(id, playerInventory)
    , m_inputInventory(std::move(inventory))
    , m_outputInventory(std::make_unique<blockentity::SimpleInventory>(1))
    , m_pos(pos)
{
    _initSlots(playerInventory);
}

LoomContainer::LoomContainer(ContainerId id, PlayerInventory* playerInventory, const BlockPos& pos)
    : AbstractContainerMenu(id, playerInventory)
    , m_inputInventory(std::make_unique<blockentity::SimpleInventory>(3))
    , m_outputInventory(std::make_unique<blockentity::SimpleInventory>(1))
    , m_pos(pos)
{
    _initSlots(playerInventory);
}

LoomContainer::LoomContainer(ContainerId id, PlayerInventory* playerInventory, const BlockPos& pos, IWorld* world)
    : LoomContainer(id, playerInventory, pos)
{
    MC_ASSERT_RELEASE(world != nullptr);
    m_world = world;
}

void LoomContainer::_initSlots(PlayerInventory* playerInventory)
{
    // 织布机输入槽位
    addSlot(std::make_unique<LoomBannerSlot>(m_inputInventory.get(), SLOT_BANNER, BANNER_SLOT_X, BANNER_SLOT_Y));
    addSlot(std::make_unique<LoomDyeSlot>(m_inputInventory.get(), SLOT_DYE, DYE_SLOT_X, DYE_SLOT_Y));
    addSlot(std::make_unique<LoomPatternSlot>(m_inputInventory.get(), SLOT_PATTERN, PATTERN_SLOT_X, PATTERN_SLOT_Y));

    // 织布机输出槽位
    addSlot(std::make_unique<LoomResultSlot>(m_outputInventory.get(), 0, RESULT_SLOT_X, RESULT_SLOT_Y, this));

    // 玩家主背包（3行9列）
    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            addSlot(std::make_unique<Slot>(playerInventory, 9 + row * 9 + col, 8 + col * 18, 84 + row * 18));
        }
    }

    // 玩家快捷栏
    for (i32 col = 0; col < 9; ++col) {
        addSlot(std::make_unique<Slot>(playerInventory, col, 8 + col * 18, 142));
    }
    trackInt(
        [this] {
            return getSelectedPatternIndex();
        },
        [this](i32 index) {
            m_selectedPattern = index >= 0 && index < static_cast<i32>(m_selectablePatterns.size())
                ? static_cast<i32>(m_selectablePatterns[static_cast<std::size_t>(index)])
                : 0;
        });
}

bool LoomContainer::stillValid(const Player& player) const
{
    if (!isWithinDistance(player, m_pos)) {
        return false;
    }
    if (m_world == nullptr) {
        return true;
    }
    const auto* state = m_world->getBlockState(m_pos);
    return player.world() == m_world && state != nullptr && &state->getBlock() == VanillaBlocks::LOOM;
}

void LoomContainer::_updateSelectablePatterns()
{
    m_selectablePatterns.clear();
    const auto banner = m_inputInventory->getItem(SLOT_BANNER);
    const auto dye = m_inputInventory->getItem(SLOT_DYE);
    const auto pattern = m_inputInventory->getItem(SLOT_PATTERN);
    if (banner.isEmpty() || dye.isEmpty() || dynamic_cast<const item::BannerItem*>(banner.getItem()) == nullptr ||
        !_isDyeItem(dye.getItem()) ||
        blockentity::BannerEntity::getPatternCount(banner) >= blockentity::BannerEntity::MAX_PATTERNS) {
        return;
    }
    if (pattern.isEmpty()) {
        const auto basic = blockentity::BannerPatterns::getLoomPatterns();
        m_selectablePatterns.assign(basic.begin(), basic.end());
    } else if (const auto* item = dynamic_cast<const item::BannerPatternItem*>(pattern.getItem())) {
        const auto type = item->getBannerPattern();
        if (blockentity::BannerPatterns::hasPatternItem(type)) {
            m_selectablePatterns.push_back(type);
        }
    }
}

void LoomContainer::slotsChanged(IInventory* inventory)
{
    MC_UNUSED(inventory);
    _updateSelectablePatterns();
    if (!_isValidPattern(m_selectedPattern)) {
        m_selectedPattern = m_selectablePatterns.size() == 1 ? static_cast<i32>(m_selectablePatterns.front()) : 0;
    }
    _updateResult();
}

i32 LoomContainer::getSelectedPatternIndex() const noexcept
{
    const auto it = std::find(m_selectablePatterns.begin(),
        m_selectablePatterns.end(),
        static_cast<blockentity::BannerPatternType>(m_selectedPattern));
    return it == m_selectablePatterns.end() ? -1 : static_cast<i32>(std::distance(m_selectablePatterns.begin(), it));
}

bool LoomContainer::clickMenuButton(Player& player, i32 index)
{
    MC_UNUSED(player);
    if (index < 0 || index >= static_cast<i32>(m_selectablePatterns.size())) {
        return false;
    }
    m_selectedPattern = static_cast<i32>(m_selectablePatterns[static_cast<std::size_t>(index)]);
    _updateResult();
    return true;
}

void LoomContainer::_updateResult()
{
    const ItemStack& bannerStack = m_inputInventory->getItem(SLOT_BANNER);
    const ItemStack& dyeStack = m_inputInventory->getItem(SLOT_DYE);
    const ItemStack& patternStack = m_inputInventory->getItem(SLOT_PATTERN);

    i32 selectedPattern = m_selectedPattern;

    if (selectedPattern > 0 && !bannerStack.isEmpty() && !dyeStack.isEmpty()) {
        // 验证选中图案的合法性
        if (!_isValidPattern(selectedPattern)) {
            m_outputInventory->setItem(0, ItemStack());
            return;
        }

        // 复制原旗帜
        ItemStack result = bannerStack.copy();
        result.setCount(1);

        // 获取图案类型和染料颜色
        blockentity::BannerPatternType patternType = static_cast<blockentity::BannerPatternType>(selectedPattern);
        DyeColor dyeColor = DyeColor::White;

        // 从染料物品获取颜色
        if (dyeStack.getItem() != nullptr) {
            dyeColor = _dyeColorFromItem(dyeStack.getItem());
        }

        // 添加新图案到BlockEntityTag.Patterns
        nlohmann::json& blockEntityTag = result.getOrCreateChildTag("BlockEntityTag");

        nlohmann::json patternsArray;
        if (blockEntityTag.contains("Patterns") && blockEntityTag["Patterns"].is_array()) {
            patternsArray = blockEntityTag["Patterns"];
        } else {
            patternsArray = nlohmann::json::array();
        }

        nlohmann::json patternEntry;
        patternEntry["Pattern"] = blockentity::BannerPatterns::getHashName(patternType);
        patternEntry["Color"] = static_cast<i32>(dyeColor);
        patternsArray.push_back(patternEntry);

        blockEntityTag["Patterns"] = patternsArray;

        // 检查输出是否与当前输出不同
        const ItemStack& currentOutput = m_outputInventory->getItem(0);
        if (result != currentOutput) {
            m_outputInventory->setItem(0, std::move(result));
        }
    } else {
        m_outputInventory->setItem(0, ItemStack());
    }
}

bool LoomContainer::_isValidPattern(i32 patternIndex) const
{
    if (patternIndex <= 0 || patternIndex >= static_cast<i32>(blockentity::BannerPatternType::Count)) {
        return false;
    }
    return std::find(m_selectablePatterns.begin(),
               m_selectablePatterns.end(),
               static_cast<blockentity::BannerPatternType>(patternIndex)) != m_selectablePatterns.end();
}

ItemStack LoomContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    MC_UNUSED(player);

    ItemStack result;
    Slot* slot = getSlot(slotIndex);

    if (slot == nullptr || !slot->hasItem()) {
        return result;
    }

    ItemStack stack = slot->getItem();
    result = stack.copy();

    if (slotIndex == SLOT_RESULT) {
        // 输出槽 → 玩家背包
        if (!moveItemToRange(stack, LOOM_SLOTS, LOOM_SLOTS + 36, true)) {
            return ItemStack();
        }
    } else if (slotIndex >= LOOM_SLOTS) {
        // 玩家背包 → 输入槽
        const auto* item = stack.getItem();
        if (item != nullptr) {
            const auto* bannerItem = dynamic_cast<const item::BannerItem*>(item);
            const auto* patternItem = dynamic_cast<const item::BannerPatternItem*>(item);

            if (bannerItem != nullptr) {
                // BannerItem → 旗帜槽
                if (!moveItemToRange(stack, SLOT_BANNER, SLOT_BANNER + 1, false)) {
                    return ItemStack();
                }
            } else if (_isDyeItem(item)) {
                // 染料 → 染料槽
                if (!moveItemToRange(stack, SLOT_DYE, SLOT_DYE + 1, false)) {
                    return ItemStack();
                }
            } else if (patternItem != nullptr) {
                // BannerPatternItem → 图案物品槽
                if (!moveItemToRange(stack, SLOT_PATTERN, SLOT_PATTERN + 1, false)) {
                    return ItemStack();
                }
            } else {
                if (!moveItemToRange(stack, LOOM_SLOTS, LOOM_SLOTS + 27, false)) {
                    return ItemStack();
                }
            }
        }
    } else {
        // 输入槽 → 玩家背包
        if (!moveItemToRange(stack, LOOM_SLOTS, LOOM_SLOTS + 36, false)) {
            return ItemStack();
        }
    }

    if (stack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->setChanged();
    }

    return result;
}

void LoomContainer::removed(Player& player)
{
    // 将输入槽位的物品返回给玩家
    clearContainer(player, m_inputInventory.get());
}

// ========== LoomBannerSlot 实现 ==========

bool LoomBannerSlot::mayPlace(const ItemStack& stack) const
{
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        return false;
    }
    return dynamic_cast<const item::BannerItem*>(stack.getItem()) != nullptr;
}

// ========== LoomDyeSlot 实现 ==========

bool LoomDyeSlot::mayPlace(const ItemStack& stack) const
{
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        return false;
    }
    return _isDyeItem(stack.getItem());
}

// ========== LoomPatternSlot 实现 ==========

bool LoomPatternSlot::mayPlace(const ItemStack& stack) const
{
    if (stack.isEmpty() || stack.getItem() == nullptr) {
        return false;
    }
    return dynamic_cast<const item::BannerPatternItem*>(stack.getItem()) != nullptr;
}

// ========== LoomResultSlot 实现 ==========

LoomResultSlot::LoomResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, LoomContainer* container)
    : Slot(inventory, slotIndex, x, y)
    , m_container(container)
{}

bool LoomResultSlot::mayPlace(const ItemStack& stack) const
{
    MC_UNUSED(stack);
    return false;
}

ItemStack LoomResultSlot::onTake(Player& player, ItemStack stack)
{
    MC_UNUSED(player);
    MC_UNUSED(stack);

    // 消耗输入物品
    IInventory* input = &m_container->getInputInventory();

    ItemStack bannerStack = input->getItem(LoomContainer::SLOT_BANNER);
    if (!bannerStack.isEmpty()) {
        bannerStack.shrink(1);
        input->setItem(LoomContainer::SLOT_BANNER, std::move(bannerStack));
    }

    ItemStack dyeStack = input->getItem(LoomContainer::SLOT_DYE);
    if (!dyeStack.isEmpty()) {
        dyeStack.shrink(1);
        input->setItem(LoomContainer::SLOT_DYE, std::move(dyeStack));
    }

    // 图案卷可重复使用，只消耗旗帜和染料。

    // 更新输出
    m_container->slotsChanged(input);

    return stack;
}

} // namespace container
} // namespace inventory
} // namespace entity
} // namespace mc
