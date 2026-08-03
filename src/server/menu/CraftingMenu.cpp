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

#include "server/menu/CraftingMenu.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/screen/ScreenType.hpp"
#include "common/world/block/BlockPos.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "world/blockentity/CraftingTableEntity.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace mc {

namespace {

bool canStackResultWithCarried(const ItemStack& carried, const ItemStack& result)
{
    if (result.isEmpty()) {
        return false;
    }

    if (carried.isEmpty()) {
        return true;
    }

    if (!carried.isSameItem(result)) {
        return false;
    }

    return carried.getCount() + result.getCount() <= carried.getMaxStackSize();
}

void processCraftingGrid(CraftingInventory& grid, const crafting::CraftingRecipe* recipe)
{
    if (recipe == nullptr) {
        return;
    }

    // 使用 getRemainingItems() 获取剩余物品
    // 剩余物品包括：水桶->空桶、玻璃瓶->玻璃瓶、碗->碗等
    std::vector<ItemStack> remaining = recipe->getRemainingItems(grid);

    for (i32 slot = 0; slot < grid.getContainerSize(); ++slot) {
        ItemStack stack = grid.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }

        // 减少物品数量
        i32 count = std::max(1, recipe->getIngredientCount(slot));
        stack.shrink(count);

        // 如果有剩余物品（如空桶），替换原物品
        if (slot < static_cast<i32>(remaining.size()) && !remaining[slot].isEmpty()) {
            grid.setItem(slot, remaining[slot]);
        } else {
            grid.setItem(slot, stack.isEmpty() ? ItemStack() : stack);
        }
    }
}

void shrinkCraftingGrid(CraftingInventory& grid, const crafting::CraftingRecipe* recipe)
{
    // 兼容旧接口，调用新的处理函数
    processCraftingGrid(grid, recipe);
}

} // namespace

// ========== CraftingMenu 实现 ==========

CraftingMenu::CraftingMenu(ContainerId id, PlayerInventory* playerInventory, CraftingTableEntity* blockEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_craftingGrid(GRID_WIDTH, GRID_HEIGHT)
    , m_blockEntity(blockEntity)
    , m_screenType(ScreenType::CraftingTable)
{

    // 添加合成网格槽位 (槽位 0-8)
    addCraftingGridSlots(98, 18);

    // 添加结果槽位 (槽位 9)
    addResultSlot(154, 28);

    // 添加玩家主背包 (槽位 10-36)
    addPlayerInventorySlots(8, 84);

    // 添加玩家快捷栏 (槽位 37-45)
    addPlayerHotbarSlots(8, 142);

    m_craftingGrid.setContentChangedCallback([this]() { slotsChanged(&m_craftingGrid); });

    updateResult();
}

CraftingMenu::CraftingMenu(ContainerId id, PlayerInventory* playerInventory, i32 width, i32 height)
    : AbstractContainerMenu(id, playerInventory)
    , m_craftingGrid(width, height)
    , m_blockEntity(nullptr)
    , m_screenType(ScreenType::Inventory)
{

    addCraftingGridSlots(98, 18);
    addResultSlot(154, 28);
    addPlayerInventorySlots(8, 84);
    addPlayerHotbarSlots(8, 142);

    m_craftingGrid.setContentChangedCallback([this]() { slotsChanged(&m_craftingGrid); });

    updateResult();
}

void CraftingMenu::addCraftingGridSlots(i32 startX, i32 startY)
{
    for (i32 y = 0; y < GRID_HEIGHT; ++y) {
        for (i32 x = 0; x < GRID_WIDTH; ++x) {
            i32 index = y * GRID_WIDTH + x;
            i32 posX = startX + x * 18;
            i32 posY = startY + y * 18;
            addSlot(std::make_unique<Slot>(&m_craftingGrid, index, posX, posY));
        }
    }
}

void CraftingMenu::addResultSlot(i32 x, i32 y)
{
    // 结果槽位：不能放入物品，只能取出
    // 传入玩家指针用于触发配方解锁和成就
    Player* player = m_playerInventory != nullptr ? m_playerInventory->getPlayer() : nullptr;
    addSlot(std::make_unique<ResultSlot>(&m_result, 0, x, y, &m_craftingGrid, player));
}

void CraftingMenu::slotsChanged(IInventory* inventory)
{
    if (inventory == &m_craftingGrid) {
        updateResult();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack CraftingMenu::clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player)
{
    if (slotIndex == RESULT_SLOT && clickType != ClickType::QuickMove) {
        if (_handleResultSlotClick() != nullptr) {
            broadcastChanges();
        }
        return getCarriedItem();
    }

    return AbstractContainerMenu::clicked(slotIndex, button, clickType, player);
}

bool CraftingMenu::stillValid(const Player& player) const
{
    if (m_blockEntity == nullptr) {
        return true;
    }

    const BlockPos pos = m_blockEntity->getPos();
    return player.distanceSqTo(
               static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y) + 0.5f, static_cast<f32>(pos.z) + 0.5f) <= 64.0f;
}

ItemStack CraftingMenu::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack originalStack = slot->getItem();
    ItemStack resultStack = originalStack.copy();

    // 结果槽位（槽位9）：Shift+点击移动到玩家背包
    if (slotIndex == RESULT_SLOT) {
        const crafting::CraftingRecipe* recipe = crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);
        if (recipe == nullptr) {
            return ItemStack();
        }

        ItemStack crafted = recipe->assemble(m_craftingGrid);
        ItemStack remaining = crafted.copy();
        if (!moveItemToRange(remaining, PLAYER_INV_START, getSlotCount() - 1, true) || !remaining.isEmpty()) {
            return ItemStack();
        }

        _consumeIngredients(recipe);
        updateResult();
        return crafted;
    }
    // 合成网格槽位（槽位0-8）：Shift+点击移动到玩家背包
    else if (isGridSlot(slotIndex)) {
        if (!moveItemToRange(resultStack, PLAYER_INV_START, getSlotCount() - 1)) {
            return ItemStack();
        }
    }
    // 玩家背包槽位：Shift+点击移动到合成网格
    else {
        // 优先移动到合成网格
        if (!moveItemToRange(resultStack, GRID_SLOT_START, GRID_SLOT_START + GRID_SLOT_COUNT - 1)) {
            return ItemStack();
        }
    }

    // 更新槽位
    if (resultStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->set(resultStack);
    }

    return originalStack;
}

void CraftingMenu::removed(Player& player)
{
    (void)player;

    // 将持有物品返回玩家背包
    if (!m_carried.isEmpty()) {
        m_playerInventory->add(m_carried);
        m_carried = ItemStack();
    }

    // 将合成网格中的物品返回玩家
    for (i32 i = 0; i < m_craftingGrid.getContainerSize(); ++i) {
        ItemStack stack = m_craftingGrid.removeItemNoUpdate(i);
        if (!stack.isEmpty()) {
            m_playerInventory->add(stack);
        }
    }

    AbstractContainerMenu::removed(player);
}

void CraftingMenu::updateResult()
{
    m_currentRecipe = crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);

    if (m_currentRecipe != nullptr) {
        m_result.setResultItem(m_currentRecipe->assemble(m_craftingGrid));
        // 设置当前使用的配方，用于配方解锁
        m_result.setCraftingRecipeUsed(m_currentRecipe);
    } else {
        m_result.clear();
        m_result.setCraftingRecipeUsed(nullptr);
    }

    broadcastChanges();
}

const crafting::CraftingRecipe* CraftingMenu::_handleResultSlotClick()
{
    const crafting::CraftingRecipe* recipe = crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);
    if (recipe == nullptr) {
        return nullptr;
    }

    ItemStack result = recipe->assemble(m_craftingGrid);
    if (!canStackResultWithCarried(m_carried, result)) {
        return nullptr;
    }

    if (m_carried.isEmpty()) {
        m_carried = result;
    } else {
        m_carried.grow(result.getCount());
    }

    _consumeIngredients(recipe);

    // 更新结果（需要重新查找配方，因为原料已变化）
    updateResult();
    return recipe;
}

void CraftingMenu::_consumeIngredients(const crafting::CraftingRecipe* recipe)
{
    shrinkCraftingGrid(m_craftingGrid, recipe);
}

// ========== InventoryCraftingMenu 实现 ==========

InventoryCraftingMenu::InventoryCraftingMenu(ContainerId id, PlayerInventory* playerInventory)
    : AbstractContainerMenu(id, playerInventory)
    , m_craftingGrid(2, 2)
{

    // 槽位布局：
    // 槽位 0: 合成结果 (154, 28)
    // 槽位 1-4: 合成网格 (2x2) (98, 18) 到 (116, 36)
    // 槽位 5-8: 护甲 (8, 8), (8, 26), (8, 44), (8, 62)
    // 槽位 9-35: 主背包 (3x9) (8, 84)
    // 槽位 36-44: 快捷栏 (1x9) (8, 142)
    // 槽位 45: 副手 (77, 62)

    // 添加结果槽位 (槽位 0)
    // 传入玩家指针用于触发配方解锁和成就
    Player* player = m_playerInventory != nullptr ? m_playerInventory->getPlayer() : nullptr;
    addSlot(std::make_unique<ResultSlot>(&m_result, 0, 154, 28, &m_craftingGrid, player));

    // 添加合成网格槽位 (槽位 1-4)
    for (i32 y = 0; y < 2; ++y) {
        for (i32 x = 0; x < 2; ++x) {
            const i32 index = y * 2 + x;
            addSlot(std::make_unique<Slot>(&m_craftingGrid, index, 98 + x * 18, 18 + y * 18));
        }
    }

    // 添加护甲槽位 (槽位 5-8)
    addPlayerArmorSlots(8, 8);

    // 添加玩家主背包 (槽位 9-35)
    addPlayerInventorySlots(8, 84);

    // 添加玩家快捷栏 (槽位 36-44)
    addPlayerHotbarSlots(8, 142);

    // 添加副手槽位 (槽位 45)
    addPlayerOffhandSlot(77, 62);

    // 设置合成网格变化回调
    m_craftingGrid.setContentChangedCallback([this]() { slotsChanged(&m_craftingGrid); });

    updateResult();
}

void InventoryCraftingMenu::slotsChanged(IInventory* inventory)
{
    if (inventory == &m_craftingGrid) {
        updateResult();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack InventoryCraftingMenu::clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player)
{
    if (slotIndex == RESULT_SLOT && clickType != ClickType::QuickMove) {
        if (_handleResultSlotClick() != nullptr) {
            broadcastChanges();
        }
        return getCarriedItem();
    }

    return AbstractContainerMenu::clicked(slotIndex, button, clickType, player);
}

ItemStack InventoryCraftingMenu::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack originalStack = slot->getItem();
    ItemStack movingStack = originalStack.copy();

    // 槽位布局：
    // 0: 合成结果
    // 1-4: 合成网格
    // 5-8: 护甲
    // 9-35: 主背包
    // 36-44: 快捷栏
    // 45: 副手

    if (slotIndex == RESULT_SLOT) {
        // 结果槽位：Shift+点击移动到玩家背包
        const crafting::CraftingRecipe* recipe = crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);
        if (recipe == nullptr) {
            return ItemStack();
        }

        ItemStack crafted = recipe->assemble(m_craftingGrid);
        ItemStack remaining = crafted.copy();

        // 尝试移动到主背包，然后是快捷栏
        if (!moveItemToRange(remaining, PLAYER_INV_START, PLAYER_INV_END, true)) {
            if (!moveItemToRange(remaining, HOTBAR_START, HOTBAR_END, true)) {
                return ItemStack();
            }
        }

        _consumeIngredients(recipe);
        updateResult();
        return crafted;
    }

    // 合成网格槽位：Shift+点击移动到玩家背包
    if (slotIndex >= GRID_SLOT_START && slotIndex <= GRID_SLOT_END) {
        if (!moveItemToRange(movingStack, PLAYER_INV_START, HOTBAR_END, true)) {
            return ItemStack();
        }
    }
    // 护甲槽位：Shift+点击移动到主背包
    else if (slotIndex >= ARMOR_SLOT_START && slotIndex < ARMOR_SLOT_START + ARMOR_SLOT_COUNT) {
        if (!moveItemToRange(movingStack, PLAYER_INV_START, PLAYER_INV_END, true)) {
            if (!moveItemToRange(movingStack, HOTBAR_START, HOTBAR_END, true)) {
                return ItemStack();
            }
        }
    }
    // 主背包：Shift+点击移动到合成网格或护甲
    else if (slotIndex >= PLAYER_INV_START && slotIndex <= PLAYER_INV_END) {
        // 优先尝试移动到护甲槽
        if (!moveItemToRange(movingStack, ARMOR_SLOT_START, ARMOR_SLOT_START + ARMOR_SLOT_COUNT - 1)) {
            // 然后尝试移动到合成网格
            if (!moveItemToRange(movingStack, GRID_SLOT_START, GRID_SLOT_END)) {
                return ItemStack();
            }
        }
    }
    // 快捷栏：Shift+点击移动到主背包或合成网格
    else if (slotIndex >= HOTBAR_START && slotIndex <= HOTBAR_END) {
        // 优先尝试移动到护甲槽
        if (!moveItemToRange(movingStack, ARMOR_SLOT_START, ARMOR_SLOT_START + ARMOR_SLOT_COUNT - 1)) {
            // 然后尝试移动到主背包
            if (!moveItemToRange(movingStack, PLAYER_INV_START, PLAYER_INV_END)) {
                // 最后尝试移动到合成网格
                if (!moveItemToRange(movingStack, GRID_SLOT_START, GRID_SLOT_END)) {
                    return ItemStack();
                }
            }
        }
    }
    // 副手槽：Shift+点击移动到主背包
    else if (slotIndex == OFFHAND_SLOT) {
        if (!moveItemToRange(movingStack, PLAYER_INV_START, PLAYER_INV_END, true)) {
            if (!moveItemToRange(movingStack, HOTBAR_START, HOTBAR_END, true)) {
                return ItemStack();
            }
        }
    }

    // 更新槽位
    if (movingStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->set(movingStack);
    }

    updateResult();
    return originalStack;
}

void InventoryCraftingMenu::updateResult()
{
    m_currentRecipe = crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);

    if (m_currentRecipe != nullptr) {
        m_result.setResultItem(m_currentRecipe->assemble(m_craftingGrid));
        // 设置当前使用的配方，用于配方解锁
        m_result.setCraftingRecipeUsed(m_currentRecipe);
    } else {
        m_result.clear();
        m_result.setCraftingRecipeUsed(nullptr);
    }

    broadcastChanges();
}

const crafting::CraftingRecipe* InventoryCraftingMenu::_handleResultSlotClick()
{
    const crafting::CraftingRecipe* recipe = crafting::RecipeManager::instance().findMatchingRecipe(m_craftingGrid);
    if (recipe == nullptr) {
        return nullptr;
    }

    ItemStack result = recipe->assemble(m_craftingGrid);
    if (!canStackResultWithCarried(m_carried, result)) {
        return nullptr;
    }

    if (m_carried.isEmpty()) {
        m_carried = result;
    } else {
        m_carried.grow(result.getCount());
    }

    _consumeIngredients(recipe);
    updateResult();
    return recipe;
}

void InventoryCraftingMenu::_consumeIngredients(const crafting::CraftingRecipe* recipe)
{
    shrinkCraftingGrid(m_craftingGrid, recipe);
}

} // namespace mc
