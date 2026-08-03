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

#include "world/blockentity/interactive/ShelfBlockEntity.hpp"

#include "common/core/Types.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "item/core/ItemStack.hpp"
#include "world/IWorld.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

// ========== ShelfBlockEntity 实现 ==========

ShelfBlockEntity::ShelfBlockEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::Shelf, pos)
    , m_inventory(SHELF_SIZE)
{}

ShelfBlockEntity::~ShelfBlockEntity() noexcept = default;

ItemStack ShelfBlockEntity::swapItemNoUpdate(i32 slot, const ItemStack& newItem)
{
    ItemStack oldItem = m_inventory.removeItemNoUpdate(slot);
    m_inventory.setItem(slot, newItem);
    return oldItem;
}

void ShelfBlockEntity::markChanged()
{
    ContainerBlockEntity::setChanged();
    // 通知客户端方块实体数据已更新
    // notifyBlockUpdate 即使方块状态未改变也会触发客户端同步
    if (m_world != nullptr && !m_world->isClientSide()) {
        m_world->notifyBlockUpdate(m_pos);
    }
}

i32 ShelfBlockEntity::getAnalogOutputSignal() const
{
    // 3位二进制编码：每个槽位是否占用对应1位
    // 槽位0: bit 0 (值1), 槽位1: bit 1 (值2), 槽位2: bit 2 (值4)
    i32 signal = 0;
    for (i32 i = 0; i < SHELF_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty()) {
            signal |= (1 << i);
        }
    }
    return signal;
}

bool ShelfBlockEntity::load(const nlohmann::json& data)
{
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    return true;
}

void ShelfBlockEntity::save(nlohmann::json& data) const
{
    ContainerBlockEntity::save(data);

    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
}

std::unique_ptr<BlockEntity> ShelfBlockEntity::clone() const
{
    auto cloned = std::make_unique<ShelfBlockEntity>(m_pos);
    for (i32 slot = 0; slot < SHELF_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(slot, stack.copy());
        }
    }
    return cloned;
}

} // namespace blockentity
} // namespace mc
