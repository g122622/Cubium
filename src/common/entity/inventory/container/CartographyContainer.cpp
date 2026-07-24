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

#include "CartographyContainer.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/inventory/Slot.hpp"
#include "item/Items.hpp"
#include "item/items/map/FilledMapItem.hpp"
#include "world/IWorld.hpp"
#include "world/map/MapData.hpp"

namespace mc {

// ============================================================================
// 制图台内部背包
// ============================================================================

namespace {

/**
 * @brief 制图台3格背包
 */
class CartographyInventory : public IInventory {
public:
    static constexpr i32 SIZE = 3; // 地图 + 材料 + 结果

    i32 getContainerSize() const override { return SIZE; }

    bool isEmpty() const override
    {
        for (const auto& item : m_items) {
            if (!item.isEmpty()) {
                return false;
            }
        }
        return true;
    }

    ItemStack getItem(i32 slot) const override
    {
        if (slot >= 0 && slot < SIZE) {
            return m_items[slot];
        }
        return ItemStack();
    }

    void setItem(i32 slot, const ItemStack& stack) override
    {
        if (slot >= 0 && slot < SIZE) {
            m_items[slot] = stack;
            setChanged();
        }
    }

    ItemStack removeItem(i32 slot, i32 count) override
    {
        if (slot >= 0 && slot < SIZE) {
            return m_items[slot].split(count);
        }
        return ItemStack();
    }

    ItemStack removeItemNoUpdate(i32 slot) override
    {
        if (slot >= 0 && slot < SIZE) {
            ItemStack result = std::move(m_items[slot]);
            m_items[slot] = ItemStack();
            return result;
        }
        return ItemStack();
    }

    void clear() override
    {
        for (auto& item : m_items) {
            item = ItemStack();
        }
        setChanged();
    }

    void setChanged() override
    {
        if (m_changeCallback) {
            m_changeCallback();
        }
    }

    void setChangeCallback(std::function<void()> callback) { m_changeCallback = std::move(callback); }

private:
    std::array<ItemStack, SIZE> m_items;
    std::function<void()> m_changeCallback;
};

// ============================================================================
// 自定义槽位
// ============================================================================

/**
 * @brief 地图槽 - 只接受已填充地图
 */
class CartographyMapSlot : public Slot {
public:
    CartographyMapSlot(IInventory* inventory, i32 index, i32 x, i32 y)
        : Slot(inventory, index, x, y)
    {}

    bool mayPlace(const ItemStack& stack) const override { return item::items::FilledMapItem::isFilledMap(stack); }

    i32 getMaxStackSize() const override { return 1; }
};

/**
 * @brief 材料槽 - 接受纸、玻璃板、空地图
 */
class CartographyMaterialSlot : public Slot {
public:
    CartographyMaterialSlot(IInventory* inventory, i32 index, i32 x, i32 y)
        : Slot(inventory, index, x, y)
    {}

    bool mayPlace(const ItemStack& stack) const override
    {
        if (stack.isEmpty() || stack.getItem() == nullptr) {
            return false;
        }
        auto* item = stack.getItem();
        return item == Items::PAPER || item == Items::GLASS_PANE || item == Items::MAP;
    }
};

/**
 * @brief 结果槽 - 只能取出，不能放入
 */
class CartographyResultSlot : public Slot {
public:
    CartographyResultSlot(IInventory* inventory, i32 index, i32 x, i32 y, Player& player)
        : Slot(inventory, index, x, y)
        , m_player(player)
    {}

    bool mayPlace(const ItemStack& /*stack*/) const override { return false; }

    ItemStack onTake(Player& player, ItemStack stack) override
    {
        // 从地图槽和材料槽消耗物品
        auto* inv = getInventory();
        if (inv != nullptr) {
            // 消耗地图槽物品
            ItemStack mapItem = inv->getItem(CartographyContainer::SLOT_MAP);
            if (!mapItem.isEmpty()) {
                mapItem.shrink(1);
                inv->setItem(CartographyContainer::SLOT_MAP, mapItem);
            }

            // 消耗材料槽物品（非创造模式）
            if (!player.isCreative()) {
                ItemStack materialItem = inv->getItem(CartographyContainer::SLOT_MATERIAL);
                if (!materialItem.isEmpty()) {
                    materialItem.shrink(1);
                    inv->setItem(CartographyContainer::SLOT_MATERIAL, materialItem);
                }
            }
        }

        return Slot::onTake(player, stack);
    }

    i32 getMaxStackSize() const override { return 1; }

private:
    Player& m_player;
};

} // anonymous namespace

// ============================================================================
// CartographyContainer
// ============================================================================

CartographyContainer::CartographyContainer(
    ContainerId id, PlayerInventory* playerInventory, const BlockPos& position, IWorld* world)
    : AbstractContainerMenu(id, playerInventory)
    , m_cartographyInventory(std::make_unique<CartographyInventory>())
    , m_position(position)
    , m_world(world)
{
    _initSlots(playerInventory);
}

void CartographyContainer::_initSlots(PlayerInventory* playerInventory)
{
    auto* cartInv = m_cartographyInventory.get();

    // 设置变化回调
    auto* cartInventory = static_cast<CartographyInventory*>(cartInv);
    cartInventory->setChangeCallback([this, cartInv]() { this->slotsChanged(cartInv); });

    // 制图台槽位
    addSlot(std::make_unique<CartographyMapSlot>(cartInv, SLOT_MAP, MAP_SLOT_X, MAP_SLOT_Y));
    addSlot(std::make_unique<CartographyMaterialSlot>(cartInv, SLOT_MATERIAL, MATERIAL_SLOT_X, MATERIAL_SLOT_Y));

    // 结果槽需要Player引用（用于消耗物品）
    Player* player = playerInventory ? playerInventory->getPlayer() : nullptr;
    if (player != nullptr) {
        addSlot(std::make_unique<CartographyResultSlot>(cartInv, SLOT_RESULT, RESULT_SLOT_X, RESULT_SLOT_Y, *player));
    } else {
        addSlot(std::make_unique<Slot>(cartInv, SLOT_RESULT, RESULT_SLOT_X, RESULT_SLOT_Y));
    }

    // 玩家主背包 (3x9)
    addPlayerInventorySlots(PLAYER_INV_X, PLAYER_INV_Y);

    // 玩家快捷栏 (1x9)
    addPlayerHotbarSlots(PLAYER_INV_X, HOTBAR_Y);
}

bool CartographyContainer::stillValid(const Player& player) const
{
    return isWithinDistance(player, m_position);
}

void CartographyContainer::slotsChanged(IInventory* inventory)
{
    if (inventory == m_cartographyInventory.get()) {
        updateResult();
    }
    AbstractContainerMenu::slotsChanged(inventory);
}

void CartographyContainer::updateResult()
{
    if (m_world == nullptr) {
        return;
    }

    ItemStack mapItem = m_cartographyInventory->getItem(SLOT_MAP);
    ItemStack result = ItemStack::EMPTY;

    if (!mapItem.isEmpty() && item::items::FilledMapItem::isFilledMap(mapItem)) {
        if (_hasPaper() && _canExtendMap()) {
            // 纸 + 地图 → 扩展地图
            result = mapItem.copy();
            result.setCount(1);
            auto& tag = result.getOrCreateTag();
            tag["map_scale_direction"] = 1;
        } else if (_hasGlassPane() && _canLockMap()) {
            // 玻璃板 + 地图 → 锁定地图
            result = mapItem.copy();
            result.setCount(1);
            auto& resultTag = result.getOrCreateTag();
            resultTag["map_lock"] = 1;
        } else if (_hasEmptyMap() && _canCopyMap()) {
            // 空地图 + 地图 → 复制地图
            result = mapItem.copy();
            result.setCount(2);
        }
    }

    m_cartographyInventory->setItem(SLOT_RESULT, result);
}

void CartographyContainer::removed(Player& player)
{
    // 将制图台背包中的物品归还给玩家
    clearContainer(player, m_cartographyInventory.get());
    AbstractContainerMenu::removed(player);
}

ItemStack CartographyContainer::quickMoveStack(i32 slotIndex, Player& player)
{
    ItemStack result = ItemStack::EMPTY;
    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || !slot->hasItem()) {
        return result;
    }

    ItemStack stack = slot->getItem();
    result = stack.copy();

    // 制图台槽位 → 玩家背包
    if (slotIndex < CARTOGRAPHY_SLOTS) {
        if (!moveItemToRange(stack, CARTOGRAPHY_SLOTS, CARTOGRAPHY_SLOTS + 36, true)) {
            return ItemStack::EMPTY;
        }
    }
    // 玩家背包 → 制图台槽位
    else {
        if (item::items::FilledMapItem::isFilledMap(stack)) {
            // 已填充地图 → 地图槽
            if (!moveItemToRange(stack, SLOT_MAP, SLOT_MAP + 1, false)) {
                return ItemStack::EMPTY;
            }
        } else if (stack.getItem() == Items::PAPER || stack.getItem() == Items::GLASS_PANE ||
            stack.getItem() == Items::MAP) {
            // 材料 → 材料槽
            if (!moveItemToRange(stack, SLOT_MATERIAL, SLOT_MATERIAL + 1, false)) {
                return ItemStack::EMPTY;
            }
        } else {
            return ItemStack::EMPTY;
        }
    }

    if (stack.isEmpty()) {
        slot->set(ItemStack::EMPTY);
    } else {
        slot->setChanged();
    }

    slot->onTake(player, stack);
    return result;
}

bool CartographyContainer::_hasPaper() const
{
    ItemStack material = m_cartographyInventory->getItem(SLOT_MATERIAL);
    return !material.isEmpty() && material.getItem() == Items::PAPER;
}

bool CartographyContainer::_hasGlassPane() const
{
    ItemStack material = m_cartographyInventory->getItem(SLOT_MATERIAL);
    return !material.isEmpty() && material.getItem() == Items::GLASS_PANE;
}

bool CartographyContainer::_hasEmptyMap() const
{
    ItemStack material = m_cartographyInventory->getItem(SLOT_MATERIAL);
    return !material.isEmpty() && material.getItem() == Items::MAP;
}

bool CartographyContainer::_hasFilledMap() const
{
    ItemStack mapItem = m_cartographyInventory->getItem(SLOT_MAP);
    return item::items::FilledMapItem::isFilledMap(mapItem);
}

bool CartographyContainer::_canExtendMap() const
{
    ItemStack mapItem = m_cartographyInventory->getItem(SLOT_MAP);
    if (!item::items::FilledMapItem::isFilledMap(mapItem)) {
        return false;
    }

    // 探险地图不可扩展
    if (item::items::FilledMapItem::isExplorationMap(mapItem)) {
        return false;
    }

    // 检查缩放级别 < 4
    if (m_world != nullptr) {
        auto* mapData = item::items::FilledMapItem::getMapData(mapItem, *m_world);
        if (mapData != nullptr && mapData->scale() >= world::map::MapData::MAX_SCALE) {
            return false;
        }
    }

    return true;
}

bool CartographyContainer::_canLockMap() const
{
    ItemStack mapItem = m_cartographyInventory->getItem(SLOT_MAP);
    if (!item::items::FilledMapItem::isFilledMap(mapItem)) {
        return false;
    }

    // 已锁定的地图不可再锁定
    if (m_world != nullptr) {
        auto* mapData = item::items::FilledMapItem::getMapData(mapItem, *m_world);
        if (mapData != nullptr && mapData->locked()) {
            return false;
        }
    }

    return true;
}

bool CartographyContainer::_canCopyMap() const
{
    return _hasFilledMap();
}

} // namespace mc
