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
#include "entity/Player.hpp"
#include "entity/inventory/IInventory.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "item/Items.hpp"
#include "item/items/BannerPatternItem.hpp"
#include "item/items/block/BannerItem.hpp"
#include "sound/SoundEvents.hpp"
#include "world/World.hpp"
#include "world/blockentity/interactive/BannerEntity.hpp"

namespace mc {
namespace entity {
namespace inventory {
namespace container {

// ========== LoomContainer 实现 ==========

LoomContainer::LoomContainer(
    ContainerId id, PlayerInventory* playerInventory, std::unique_ptr<IInventory> inventory, const BlockPos& pos)
    : AbstractContainerMenu(ContainerType::Loom, id)
    , m_inputInventory(std::move(inventory))
    , m_outputInventory(IInventory::create(1))
    , m_pos(pos)
{
    _initSlots(playerInventory);
}

LoomContainer::LoomContainer(ContainerId id, PlayerInventory* playerInventory, const BlockPos& pos)
    : AbstractContainerMenu(ContainerType::Loom, id)
    , m_inputInventory(IInventory::create(3))
    , m_outputInventory(IInventory::create(1))
    , m_pos(pos)
{
    _initSlots(playerInventory);
}

void LoomContainer::_initSlots(PlayerInventory* playerInventory)
{
    // 织布机输入槽位
    addSlot(std::make_unique<LoomBannerSlot>(m_inputInventory.get(), SLOT_BANNER, BANNER_SLOT_X, BANNER_SLOT_Y));
    addSlot(std::make_unique<LoomDyeSlot>(m_inputInventory.get(), SLOT_DYE, DYE_SLOT_X, DYE_SLOT_Y));
    addSlot(std::make_unique<LoomPatternSlot>(m_inputInventory.get(), SLOT_PATTERN, PATTERN_SLOT_X, PATTERN_SLOT_Y));

    // 织布机输出槽位
    addSlot(std::make_unique<LoomResultSlot>(m_outputInventory.get(), SLOT_RESULT, RESULT_SLOT_X, RESULT_SLOT_Y, this));

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
}

bool LoomContainer::stillValid(const Player& player) const
{
    // 检查玩家是否在织布机附近
    // TODO: 实现距离检查
    MC_UNUSED(player);
    return true;
}

void LoomContainer::slotsChanged(IInventory* inventory)
{
    MC_UNUSED(inventory);

    const ItemStack& bannerStack = m_inputInventory->getItem(SLOT_BANNER);
    const ItemStack& dyeStack = m_inputInventory->getItem(SLOT_DYE);
    const ItemStack& patternStack = m_inputInventory->getItem(SLOT_PATTERN);

    // 检查旗帜上的图案数量是否已达上限
    bool maxPatterns = false;
    if (!bannerStack.isEmpty() && bannerStack.getItem() != nullptr) {
        const auto* bannerItem = dynamic_cast<const item::BannerItem*>(bannerStack.getItem());
        if (bannerItem != nullptr) {
            i32 patternCount = blockentity::BannerEntity::getPatternCount(bannerStack);
            maxPatterns = patternCount >= blockentity::BannerEntity::MAX_PATTERNS;
        }
    }

    // 如果放入了图案物品，自动设置对应的图案ID
    if (!patternStack.isEmpty() && patternStack.getItem() != nullptr) {
        const auto* patternItem = dynamic_cast<const item::BannerPatternItem*>(patternStack.getItem());
        if (patternItem != nullptr) {
            if (maxPatterns) {
                m_selectedPattern.set(0);
            } else {
                m_selectedPattern.set(static_cast<i32>(patternItem->getBannerPattern()));
            }
        }
    }

    // 如果旗帜或染料槽为空，重置图案选择
    if (bannerStack.isEmpty() || dyeStack.isEmpty()) {
        m_selectedPattern.set(0);
    }

    _updateResult();
}

bool LoomContainer::clickMenuButton(Player& player, i32 id)
{
    MC_UNUSED(player);

    if (id > 0 && id <= PATTERN_COUNT) {
        // 验证图案选择是否有效
        if (_isValidPattern(id)) {
            m_selectedPattern.set(id);
            _updateResult();
            return true;
        }
    }

    return false;
}

void LoomContainer::_updateResult()
{
    const ItemStack& bannerStack = m_inputInventory->getItem(SLOT_BANNER);
    const ItemStack& dyeStack = m_inputInventory->getItem(SLOT_DYE);
    const ItemStack& patternStack = m_inputInventory->getItem(SLOT_PATTERN);

    i32 selectedPattern = m_selectedPattern.get();

    if (selectedPattern > 0 && !bannerStack.isEmpty() && !dyeStack.isEmpty()) {
        // 验证选中图案的合法性
        if (!_isValidPattern(selectedPattern)) {
            m_outputInventory->setItem(SLOT_RESULT, ItemStack());
            return;
        }

        // 复制原旗帜
        ItemStack result = bannerStack.copy();
        result.setCount(1);

        // 获取图案类型和染料颜色
        blockentity::BannerPatternType patternType = static_cast<blockentity::BannerPatternType>(selectedPattern);
        DyeColor dyeColor = DyeColor::White;

        // 从染料物品获取颜色
        // TODO: 完善DyeItem颜色获取

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
        const ItemStack& currentOutput = m_outputInventory->getItem(SLOT_RESULT);
        if (result != currentOutput) {
            m_outputInventory->setItem(SLOT_RESULT, std::move(result));
        }
    } else {
        m_outputInventory->setItem(SLOT_RESULT, ItemStack());
    }
}

bool LoomContainer::_isValidPattern(i32 patternIndex) const
{
    const ItemStack& patternStack = m_inputInventory->getItem(SLOT_PATTERN);

    // 不需要图案物品的图案（索引1到PATTERN_ITEM_INDEX）
    if (patternIndex > 0 && patternIndex <= PATTERN_ITEM_INDEX) {
        return true;
    }

    // 需要图案物品的图案
    if (patternIndex > PATTERN_ITEM_INDEX && patternIndex <= PATTERN_COUNT) {
        if (!patternStack.isEmpty() && patternStack.getItem() != nullptr) {
            const auto* patternItem = dynamic_cast<const item::BannerPatternItem*>(patternStack.getItem());
            if (patternItem != nullptr) {
                return static_cast<i32>(patternItem->getBannerPattern()) == patternIndex;
            }
        }
        return false;
    }

    return false;
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
            } else if (patternItem != nullptr) {
                // BannerPatternItem → 图案物品槽
                if (!moveItemToRange(stack, SLOT_PATTERN, SLOT_PATTERN + 1, false)) {
                    return ItemStack();
                }
            } else {
                // TODO: 检查是否是DyeItem → 染料槽
                // 否则移到玩家背包
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
    // TODO: 完善DyeItem检查
    return false;
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

void LoomResultSlot::onTake(Player& player, ItemStack stack)
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

    // 图案物品只在需要时消耗（索引 > PATTERN_ITEM_INDEX 时）
    ItemStack patternStack = input->getItem(LoomContainer::SLOT_PATTERN);
    if (!patternStack.isEmpty() && patternStack.getItem() != nullptr) {
        const auto* patternItem = dynamic_cast<const item::BannerPatternItem*>(patternStack.getItem());
        if (patternItem != nullptr) {
            i32 patternIndex = static_cast<i32>(patternItem->getBannerPattern());
            if (patternIndex > LoomContainer::PATTERN_ITEM_INDEX) {
                patternStack.shrink(1);
                input->setItem(LoomContainer::SLOT_PATTERN, std::move(patternStack));
            }
        }
    }

    // 更新输出
    m_container->slotsChanged(input);

    // TODO: 播放织布机音效 UI_LOOM_TAKE_RESULT
}

} // namespace container
} // namespace inventory
} // namespace entity
} // namespace mc
