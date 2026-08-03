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

#include "DropperBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/DispenserBlockEntity.hpp"
#include <memory>

namespace mc {
namespace blockentity {

DropperBlockEntity::DropperBlockEntity(const BlockPos& pos)
    : DispenserBlockEntity(BlockEntityType::Dropper, pos)
{}

std::unique_ptr<BlockEntity> DropperBlockEntity::clone() const
{
    auto cloned = std::make_unique<DropperBlockEntity>(m_pos);
    // 复制库存内容
    for (i32 slot = 0; slot < INVENTORY_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(slot, stack.copy());
        }
    }
    // 复制锁定状态
    if (isLocked()) {
        cloned->setLocked(true);
        cloned->setLockKey(getLockKey());
    }
    return cloned;
}

} // namespace blockentity
} // namespace mc
