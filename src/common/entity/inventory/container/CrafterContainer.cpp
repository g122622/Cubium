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
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/blockentity/trial/CrafterBlockEntity.hpp"

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
    // TODO: 实现合成预览结果更新逻辑
    // 当合成网格内容变化时，应通过 RecipeManager 查找匹配配方并更新 m_resultInventory 的预览结果。
    // 参考MC原版 CrafterMenu.slotChanged() → refreshRecipeResult() 的实现流程：
    //   1. 检查玩家是否为 ServerPlayer（仅服务端执行）
    //   2. 调用 m_crafterEntity->asCraftInput() 构建合成输入（禁用槽位视为空）
    //   3. 通过 CrafterBlock::getPotentialResults(level, craftingInput) 查找匹配配方
    //   4. 调用 recipe.assemble(craftingInput, registryAccess) 生成预览物品
    //   5. 将结果写入 m_resultInventory->setItem(0, result)
    // 当前 m_resultInventory 已创建并绑定到 CrafterResultSlot，但缺少配方查找和结果填充逻辑。
    // 依赖：RecipeManager/RecipeCache 配方缓存系统、ServerLevel 注册表访问
    AbstractContainerMenu::slotsChanged(inventory);
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
