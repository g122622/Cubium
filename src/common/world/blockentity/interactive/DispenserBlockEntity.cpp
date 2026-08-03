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

#include "DispenserBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "item/core/ItemStack.hpp"
#include <memory>
#include <random>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

DispenserBlockEntity::DispenserBlockEntity(BlockEntityType type, const BlockPos& pos)
    : LootableContainerBlockEntity(type, pos)
    , m_inventory(INVENTORY_SIZE)
    , m_rng(std::random_device{}())
{
    // 注入战利品表延迟填充回调，使 m_inventory 的所有内容访问方法
    // （isEmpty/getItem/setItem/removeItem/removeItemNoUpdate/clear）
    // 都自动触发 _unpackLootTable(nullptr)，与 MC Java 的
    // RandomizableContainerBlockEntity 行为一致。
    m_inventory.setLootUnpackCallback(_makeLootUnpackCallback());
}

bool DispenserBlockEntity::load(const nlohmann::json& data)
{
    if (!LootableContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载库存
    if (data.contains("Items") && data["Items"].is_array()) {
        const auto& items = data["Items"];
        for (const auto& itemData : items) {
            if (itemData.contains("Slot") && itemData["Slot"].is_number()) {
                i32 slot = itemData["Slot"].get<i32>();
                if (slot >= 0 && slot < INVENTORY_SIZE) {
                    auto stackResult = ItemStack::fromJson(itemData);
                    if (stackResult.success()) {
                        m_inventory.setItem(slot, stackResult.value());
                    }
                }
            }
        }
    }

    return true;
}

void DispenserBlockEntity::save(nlohmann::json& data) const
{
    LootableContainerBlockEntity::save(data);

    // 保存库存
    nlohmann::json itemsJson = nlohmann::json::array();
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            nlohmann::json itemJson = stack.toJson();
            itemJson["Slot"] = i;
            itemsJson.push_back(itemJson);
        }
    }
    data["Items"] = itemsJson;
}

// ========== NBT 序列化（结构模板 / 客户端同步）==========

bool DispenserBlockEntity::loadFromNBT(const nbt::CompoundTag& tag)
{
    if (!LootableContainerBlockEntity::loadFromNBT(tag)) {
        return false;
    }

    // 仅在无未解包的战利品表时加载物品，与 MC Java 互斥语义一致
    if (!hasLootTable()) {
        loadItemsFromNBT(tag, m_inventory);
    }

    return true;
}

void DispenserBlockEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    LootableContainerBlockEntity::saveToNBT(tag);

    // 仅在无未解包的战利品表时保存物品，与 MC Java 互斥语义一致
    if (!hasLootTable()) {
        saveItemsToNBT(tag, m_inventory);
    }
}

std::unique_ptr<BlockEntity> DispenserBlockEntity::clone() const
{
    auto cloned = std::make_unique<DispenserBlockEntity>(m_type, m_pos);

    // 复制库存内容
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(i, stack.copy());
        }
    }

    return cloned;
}

void DispenserBlockEntity::clearContainer()
{
    // 先触发战利品表填充，再清空（与 LootableContainerBlockEntity::clearContainer 一致）
    // 注意：m_inventory.clear() 已通过 setLootUnpackCallback 自动触发 _unpackLootTable，
    // 但此处显式调用 _unpackLootTable 以保证语义清晰，并与基类行为保持一致。
    _unpackLootTable(nullptr);
    m_inventory.clear();
    setChanged();
}

i32 DispenserBlockEntity::getRandomSlot()
{
    i32 nonEmptyCount = 0;
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            ++nonEmptyCount;
        }
    }

    if (nonEmptyCount == 0) {
        return -1;
    }

    i32 selectedIndex = m_rng.nextInt(nonEmptyCount);
    i32 currentIndex = 0;
    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            if (currentIndex == selectedIndex) {
                return i;
            }
            ++currentIndex;
        }
    }

    return -1;
}

i32 DispenserBlockEntity::getDispenseSlot()
{
    // 储水池采样算法：每个非空槽位被选中的概率相等
    i32 selectedSlot = -1;
    i32 nonEmptyCount = 0;

    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            ++nonEmptyCount;
            // 以 1/nonEmptyCount 的概率替换当前选择
            if (m_rng.nextInt(nonEmptyCount) == 0) {
                selectedSlot = i;
            }
        }
    }

    return selectedSlot;
}

i32 DispenserBlockEntity::addItemStack(const ItemStack& stack)
{
    // 查找第一个空槽位，将整个物品放入该槽位
    // 不尝试与现有堆叠合并
    if (stack.isEmpty()) {
        return -1;
    }

    for (i32 i = 0; i < INVENTORY_SIZE; ++i) {
        if (m_inventory.getItem(i).isEmpty()) {
            m_inventory.setItem(i, stack.copy());
            setChanged();
            return i;
        }
    }

    return -1;
}

} // namespace blockentity
} // namespace mc
