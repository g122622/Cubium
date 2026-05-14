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

#include "Slot.hpp"
#include "../../item/Items.hpp"
#include "../../item/enchantment/EnchantmentHelper.hpp"
#include "../../item/items/armor/ArmorItem.hpp"
#include "../../world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "../entities/player/Player.hpp"
#include "IInventory.hpp"
#include "IRecipeHolder.hpp"

namespace mc {

Slot::Slot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
    : m_inventory(inventory)
    , m_slotIndex(slotIndex)
    , m_x(x)
    , m_y(y)
{}

ItemStack Slot::getItem() const
{
    if (m_inventory == nullptr) {
        return ItemStack::EMPTY;
    }
    return m_inventory->getItem(m_slotIndex);
}

void Slot::set(const ItemStack& stack)
{
    if (m_inventory != nullptr) {
        m_inventory->setItem(m_slotIndex, stack);
        setChanged();
    }
}

bool Slot::hasItem() const
{
    return !isEmpty();
}

bool Slot::isEmpty() const
{
    return getItem().isEmpty();
}

ItemStack Slot::remove(i32 amount)
{
    if (m_inventory == nullptr) {
        return ItemStack::EMPTY;
    }
    return m_inventory->removeItem(m_slotIndex, amount);
}

bool Slot::mayPlace(const ItemStack& stack) const
{
    if (m_inventory == nullptr) {
        return false;
    }
    return m_inventory->canPlaceItem(m_slotIndex, stack);
}

bool Slot::mayPickup(Player& player) const
{
    (void)player;
    // 默认允许拾取，子类可重写此方法
    return true;
}

void Slot::setChanged()
{
    if (m_inventory != nullptr) {
        m_inventory->setChanged();
    }
}

i32 Slot::getMaxStackSize() const
{
    if (m_inventory == nullptr) {
        return 64;
    }
    return m_inventory->getMaxStackSize();
}

i32 Slot::getMaxStackSize(const ItemStack& stack) const
{
    if (stack.isEmpty()) {
        return getMaxStackSize();
    }
    return std::min(stack.getMaxStackSize(), getMaxStackSize());
}

void Slot::onSlotChange(const ItemStack& oldStack, const ItemStack& newStack)
{
    // MC 1.16.5: 如果数量增加，调用 onCrafting
    i32 countDiff = newStack.getCount() - oldStack.getCount();
    if (countDiff > 0) {
        onCrafting(newStack, countDiff);
    }
}

void Slot::onCrafting(const ItemStack& stack, i32 amount)
{
    // 默认空实现，子类可重写
    (void)stack;
    (void)amount;
}

void Slot::onSwapCraft(i32 numItemsCrafted)
{
    // 默认空实现，子类可重写
    (void)numItemsCrafted;
}

void Slot::onCrafting(const ItemStack& stack)
{
    // 默认空实现，子类可重写
    (void)stack;
}

ItemStack Slot::onTake(Player& player, ItemStack stack)
{
    // MC 1.16.5: 默认只调用 setChanged
    (void)player;
    setChanged();
    return stack;
}

Slot& Slot::setBackground(const ResourceLocation& atlas, const ResourceLocation& sprite)
{
    m_background.atlas = atlas;
    m_background.sprite = sprite;
    return *this;
}

bool Slot::isSameInventory(const Slot& other) const
{
    return m_inventory == other.m_inventory;
}

// ============================================================================
// ArmorSlot
// ============================================================================

ArmorSlot::ArmorSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, ArmorType armorType)
    : Slot(inventory, slotIndex, x, y)
    , m_armorType(armorType)
{}

bool ArmorSlot::mayPlace(const ItemStack& stack) const
{
    if (!Slot::mayPlace(stack)) {
        return false;
    }

    const auto* armorItem = dynamic_cast<const item::items::ArmorItem*>(stack.getItem());
    if (armorItem == nullptr) {
        return false;
    }

    switch (m_armorType) {
        case ArmorType::Head:
            return armorItem->isHelmet();
        case ArmorType::Chest:
            return armorItem->isChestplate();
        case ArmorType::Legs:
            return armorItem->isLeggings();
        case ArmorType::Feet:
            return armorItem->isBoots();
    }

    return false;
}

bool ArmorSlot::mayPickup(Player& player) const
{
    // MC 1.16.5: 有绑定诅咒的护甲无法取下（除非创造模式）
    const ItemStack& stack = getItem();
    if (stack.isEmpty()) {
        return true;
    }

    // 创造模式可以取下任何护甲
    if (player.isCreative()) {
        return true;
    }

    // 绑定诅咒的护甲无法取下
    if (item::enchant::EnchantmentHelper::hasBindingCurse(stack)) {
        return false;
    }

    return true;
}

// ============================================================================
// ResultSlot
// ============================================================================

ResultSlot::ResultSlot(
    IInventory* inventory, i32 slotIndex, i32 x, i32 y, CraftingInventory* craftingGrid, Player* player)
    : Slot(inventory, slotIndex, x, y)
    , m_craftingGrid(craftingGrid)
    , m_player(player)
{}

void ResultSlot::onCrafting(const ItemStack& stack, i32 amount)
{
    // MC 1.16.5: 追踪合成数量
    m_amountCrafted += amount;
    onCrafting(stack);
}

void ResultSlot::onSwapCraft(i32 numItemsCrafted)
{
    // MC 1.16.5: 数字键交换时追踪数量
    m_amountCrafted += numItemsCrafted;
}

void ResultSlot::onCrafting(const ItemStack& stack)
{
    // MC 1.16.5: 触发统计和成就
    // 参考: net.minecraft.inventory.container.CraftingResultSlot.onCrafting
    if (m_amountCrafted > 0 && m_player != nullptr) {
        // 调用玩家的合成回调（统计 + 成就）
        // ServerPlayer 会重写此方法来更新统计和触发成就
        m_player->onItemCrafted(const_cast<ItemStack&>(stack), m_amountCrafted);
    }
    m_amountCrafted = 0;

    // MC 1.16.5: 通知 IRecipeHolder（用于解锁配方到配方书）
    // 参考: net.minecraft.inventory.container.CraftingResultSlot.onCrafting
    IInventory* inventory = getInventory();
    if (inventory != nullptr) {
        IRecipeHolder* recipeHolder = dynamic_cast<IRecipeHolder*>(inventory);
        if (recipeHolder != nullptr && m_player != nullptr) {
            recipeHolder->onCrafting(*m_player);
        }
    }

    (void)stack;
}

ItemStack ResultSlot::onTake(Player& player, ItemStack stack)
{
    // MC 1.16.5: 触发合成完成事件
    onCrafting(stack);

    // 注意：材料消耗由 CraftingMenu.handleResultSlotClick() 和 quickMoveStack() 处理
    // 它们调用 consumeIngredients() -> shrinkCraftingGrid() 消耗材料
    // 并通过 recipe->getRemainingItems() 处理剩余物品（如水桶->空桶）
    // 参考: net.minecraft.inventory.container.WorkbenchContainer.slotChangedCraftingGrid

    (void)player;
    setChanged();
    return stack;
}

// ============================================================================
// FurnaceFuelSlot
// ============================================================================

FurnaceFuelSlot::FurnaceFuelSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
    : Slot(inventory, slotIndex, x, y)
{}

bool FurnaceFuelSlot::mayPlace(const ItemStack& stack) const
{
    // MC 1.16.5: 只接受燃料或空桶
    return isFuel(stack) || isBucket(stack);
}

i32 FurnaceFuelSlot::getMaxStackSize(const ItemStack& stack) const
{
    // MC 1.16.5: 桶只能放1个
    if (isBucket(stack)) {
        return 1;
    }
    return Slot::getMaxStackSize(stack);
}

bool FurnaceFuelSlot::isFuel(const ItemStack& stack)
{
    // MC 1.16.5: 委托给 AbstractFurnaceEntity::isFuel()
    return blockentity::AbstractFurnaceEntity::isFuel(stack);
}

bool FurnaceFuelSlot::isBucket(const ItemStack& stack)
{
    // MC 1.16.5: 检查物品是否是任何类型的桶
    // 参考: FurnaceFuelSlot.isBucket() - 只检查空桶，但这里我们需要检查所有桶
    // 因为岩浆桶也可以作为燃料放入燃料槽
    const Item* item = stack.getItem();
    return item == Items::BUCKET || item == Items::WATER_BUCKET || item == Items::LAVA_BUCKET ||
        item == Items::COD_BUCKET || item == Items::SALMON_BUCKET || item == Items::PUFFERFISH_BUCKET ||
        item == Items::TROPICAL_FISH_BUCKET || item == Items::MILK_BUCKET;
}

// ============================================================================
// FurnaceResultSlot
// ============================================================================

// 注意：这是 mc::FurnaceResultSlot，与 mc::ResultSlot（合成结果槽）是不同的类

mc::FurnaceResultSlot::FurnaceResultSlot(Player* player,
    IInventory* inventory,
    i32 slotIndex,
    i32 x,
    i32 y,
    blockentity::AbstractFurnaceEntity* furnaceEntity)
    : Slot(inventory, slotIndex, x, y)
    , m_player(player)
    , m_furnaceEntity(furnaceEntity)
{}

ItemStack mc::FurnaceResultSlot::remove(i32 amount)
{
    // MC 1.16.5: 追踪取出数量
    if (hasItem()) {
        m_removeCount += std::min(amount, getItem().getCount());
    }
    return Slot::remove(amount);
}

void mc::FurnaceResultSlot::onCrafting(const ItemStack& stack, i32 amount)
{
    m_removeCount += amount;
    onCrafting(stack);
}

void mc::FurnaceResultSlot::onCrafting(const ItemStack& stack)
{
    // MC 1.16.5: 触发熔炼统计和经验发放
    // 参考: net.minecraft.inventory.container.FurnaceResultSlot.onCrafting
    if (m_removeCount > 0 && m_player != nullptr) {
        // 调用玩家的合成回调（统计 + 成就）
        // 注意：这里使用 const_cast 是因为 onItemCrafted 需要非 const ItemStack
        m_player->onItemCrafted(const_cast<ItemStack&>(stack), m_removeCount);

        // 从熔炉方块实体发放累积的经验
        // 参考 MC 1.16.5: AbstractFurnaceTileEntity.func_235645_d_
        if (m_furnaceEntity != nullptr) {
            f32 storedXp = m_furnaceEntity->getStoredExperience();
            if (storedXp > 0.0f) {
                // 提取并清空累积经验
                f32 xpToGrant = m_furnaceEntity->extractStoredExperience();
                if (xpToGrant > 0.0f) {
                    // 向玩家发放经验
                    m_player->addExperience(static_cast<i32>(std::floor(xpToGrant)));
                }
            }
        }
    }
    m_removeCount = 0;
    (void)stack;
}

ItemStack mc::FurnaceResultSlot::onTake(Player& player, ItemStack stack)
{
    // MC 1.16.5: 如果 m_removeCount 为 0（快速移动场景），使用 stack 的数量
    if (m_removeCount == 0 && !stack.isEmpty()) {
        m_removeCount = stack.getCount();
    }
    onCrafting(stack);
    setChanged();
    (void)player;
    return stack;
}

} // namespace mc
