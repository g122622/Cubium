/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "entity/inventory/container/CrafterContainer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/world/block/BlockPos.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/trial/CrafterBlockEntity.hpp"
#include <memory>

namespace mc {

namespace {

/// 合成器槽位：禁用槽位不允许放置物品
class CrafterInputSlot : public Slot {
public:
    CrafterInputSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, CrafterBlockEntity* crafterEntity)
        : Slot(inventory, slotIndex, x, y)
        , m_crafterEntity(crafterEntity)
    {}

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override
    {
        // 禁用槽位不允许放置物品
        if (m_crafterEntity != nullptr && m_crafterEntity->isSlotDisabled(getIndex())) {
            return false;
        }
        return Slot::mayPlace(stack);
    }

private:
    CrafterBlockEntity* m_crafterEntity;
};

/// 预览结果槽位：不可交互，仅展示合成结果
class CrafterResultSlot : public Slot {
public:
    CrafterResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y)
        : Slot(inventory, slotIndex, x, y)
    {}

    /// 预览结果槽位不允许放置物品
    [[nodiscard]] bool mayPlace(const ItemStack& stack) const noexcept override
    {
        (void)stack;
        return false;
    }

    /// 预览结果槽位不允许取出物品（自动合成器通过红石触发合成，非手动提取）
    [[nodiscard]] bool mayPickup(Player& player) const override
    {
        (void)player;
        return false;
    }

    [[nodiscard]] bool isValid() const noexcept override { return true; }
};

} // anonymous namespace

// ========== 构造函数 ==========

CrafterContainer::CrafterContainer(
    ContainerId id, PlayerInventory* playerInventory, IInventory* crafterInventory, CrafterBlockEntity* crafterEntity)
    : AbstractContainerMenu(id, playerInventory)
    , m_crafterInventory(crafterInventory)
    , m_crafterEntity(crafterEntity)
    , m_resultInventory(std::make_unique<CraftResultInventory>())
{
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(crafterInventory != nullptr);
    MC_ASSERT(crafterInventory->getContainerSize() == CRAFT_SLOTS);

    // 通知合成器背包已打开
    crafterInventory->openInventory(*playerInventory->getPlayer());

    _initSlots(playerInventory);

    // 注册合成器背包内容变更回调，当物品变化时触发 slotsChanged 以更新预览结果
    // CrafterBlockEntity 使用 SimpleInventory，其 setOnChanged 回调在 setChanged() 时触发
    // 但 SimpleInventory 的 setOnChanged 是单一回调（被 CrafterBlockEntity 的 _onInventoryChanged 占用），
    // 因此我们通过 IInventory 的监听器机制来监听变化。
    // 这里我们直接在 slotsChanged 中处理更新，因为 AbstractContainerMenu 的
    // broadcastChanges/detectAndSendChanges 机制会在物品变化时调用 slotsChanged。

    // 初始更新预览结果
    updateResult();
}

// ========== 容器接口 ==========

bool CrafterContainer::stillValid(const Player& player) const
{
    // 如果没有关联的方块实体，背包可访问
    if (m_crafterEntity == nullptr) {
        return true;
    }

    // 检查玩家是否在合成器附近（8格范围内）
    const BlockPos pos = m_crafterEntity->getPos();
    return player.distanceSqTo(static_cast<f32>(pos.x) + 0.5f,
               static_cast<f32>(pos.y) + 0.5f,
               static_cast<f32>(pos.z) + 0.5f) <= 64.0f; // 8^2 = 64
}

void CrafterContainer::slotsChanged(IInventory* inventory)
{
    // 当合成器背包内容变化时，更新预览结果
    if (inventory == m_crafterInventory) {
        updateResult();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

void CrafterContainer::updateResult()
{
    // 通过 CrafterBlockEntity::asCraftInput() 构建合成输入，
    // 禁用槽位在 asCraftInput() 中被视为空槽位，符合 MC 原版行为
    if (m_crafterEntity != nullptr) {
        CraftingInventory craftingInput = m_crafterEntity->asCraftInput();
        m_currentRecipe = crafting::RecipeManager::instance().findMatchingRecipe(craftingInput);

        if (m_currentRecipe != nullptr) {
            m_resultInventory->setResultItem(m_currentRecipe->assemble(craftingInput));
            m_resultInventory->setCraftingRecipeUsed(m_currentRecipe);
        } else {
            m_resultInventory->clear();
            m_resultInventory->setCraftingRecipeUsed(nullptr);
        }
    } else {
        // 无 CrafterBlockEntity 时（不应发生，但做防御处理），
        // 直接使用合成器背包构建输入
        // 注意：此时禁用槽位的物品仍会被包含在配方匹配中，
        // 因为 IInventory 接口无法区分禁用槽位
        CraftingInventory craftingInput(3, 3);
        for (i32 i = 0; i < CRAFT_SLOTS; ++i) {
            const ItemStack& stack = m_crafterInventory->getItem(i);
            if (!stack.isEmpty()) {
                craftingInput.setItemAt(i % 3, i / 3, stack.copy());
            }
        }

        m_currentRecipe = crafting::RecipeManager::instance().findMatchingRecipe(craftingInput);

        if (m_currentRecipe != nullptr) {
            m_resultInventory->setResultItem(m_currentRecipe->assemble(craftingInput));
            m_resultInventory->setCraftingRecipeUsed(m_currentRecipe);
        } else {
            m_resultInventory->clear();
            m_resultInventory->setCraftingRecipeUsed(nullptr);
        }
    }

    broadcastChanges();
}

void CrafterContainer::removed(Player& player)
{
    // 配对构造函数中的 openInventory 调用，通知方块实体玩家已关闭容器
    // 即使当前 IInventory::closeInventory 默认为空操作，也必须配对调用，
    // 以防子类（如未来的查看者计数实现）重写该方法
    m_crafterInventory->closeInventory(player);
    AbstractContainerMenu::removed(player);
}

bool CrafterContainer::isSlotDisabled(i32 slot) const
{
    if (slot < 0 || slot >= CRAFT_SLOTS) {
        return false;
    }
    return m_crafterEntity != nullptr && m_crafterEntity->isSlotDisabled(slot);
}

void CrafterContainer::setSlotState(i32 slot, bool enabled)
{
    if (m_crafterEntity != nullptr && slot >= 0 && slot < CRAFT_SLOTS) {
        m_crafterEntity->setSlotState(slot, enabled);
        // 禁用/启用槽位改变了合成输入，需要更新预览结果
        updateResult();
    }
}

ItemStack CrafterContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (!slot || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack slotStack = slot->getItem();
    ItemStack result = slotStack.copy();

    // 合成器槽位范围：0-8（合成网格），9（预览结果）
    // 玩家背包槽位范围：10-45
    if (slotIndex < CRAFTER_SLOTS) {
        // 预览结果槽位不允许快速移动
        if (slotIndex == RESULT_SLOT) {
            return ItemStack();
        }
        // 从合成网格移到玩家背包
        if (!moveItemToRange(slotStack, CRAFTER_SLOTS, getSlotCount(), true)) {
            return ItemStack();
        }
    } else {
        // 从玩家背包移到合成网格（跳过禁用槽位）
        if (!moveItemToRange(slotStack, 0, CRAFT_SLOTS, false)) {
            return ItemStack();
        }
    }

    if (slotStack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->setChanged();
    }

    return result;
}

// ========== 私有方法 ==========

void CrafterContainer::_initSlots(PlayerInventory* playerInventory)
{
    // ========== 合成网格（3x3）==========

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 3; ++col) {
            i32 slotIndex = row * 3 + col;
            i32 x = CRAFT_SLOT_START_X + col * SLOT_SIZE;
            i32 y = CRAFT_SLOT_START_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<CrafterInputSlot>(m_crafterInventory, slotIndex, x, y, m_crafterEntity));
        }
    }

    // ========== 预览结果槽位 ==========

    addSlot(std::make_unique<CrafterResultSlot>(m_resultInventory.get(), 0, RESULT_SLOT_X, RESULT_SLOT_Y));

    // ========== 玩家主背包（3行9列）==========

    for (i32 row = 0; row < 3; ++row) {
        for (i32 col = 0; col < 9; ++col) {
            i32 slotIndex = 9 + row * 9 + col;
            i32 x = 8 + col * SLOT_SIZE;
            i32 y = PLAYER_INV_Y + row * SLOT_SIZE;

            addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
        }
    }

    // ========== 玩家快捷栏（1行9列）==========

    for (i32 col = 0; col < 9; ++col) {
        i32 slotIndex = col;
        i32 x = 8 + col * SLOT_SIZE;
        i32 y = HOTBAR_Y;

        addSlot(std::make_unique<Slot>(playerInventory, slotIndex, x, y));
    }
}

} // namespace mc
