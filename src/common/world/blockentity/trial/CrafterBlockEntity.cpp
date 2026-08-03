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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN NO ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CrafterBlockEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "item/core/ItemStack.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/BlockState.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {

CrafterBlockEntity::CrafterBlockEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::Crafter, pos)
    , m_inventory(CONTAINER_SIZE, [this]() { _onInventoryChanged(); })
{
    m_slotStates.fill(SLOT_ENABLED);
}

bool CrafterBlockEntity::load(const nlohmann::json& data)
{
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载槽位禁用状态
    if (data.contains("disabled_slots") && data["disabled_slots"].is_array()) {
        for (const auto& slotVal : data["disabled_slots"]) {
            if (slotVal.is_number()) {
                i32 slot = slotVal.get<i32>();
                if (slotCanBeDisabled(slot)) {
                    m_slotStates[slot] = SLOT_DISABLED;
                }
            }
        }
    } else {
        // 兼容旧格式
        m_slotStates.fill(SLOT_ENABLED);
    }

    // 加载触发状态
    if (data.contains("triggered") && data["triggered"].is_number()) {
        m_triggered = data["triggered"].get<i32>() != 0;
    }

    // 加载合成动画剩余tick
    if (data.contains("crafting_ticks_remaining") && data["crafting_ticks_remaining"].is_number()) {
        m_craftingTicksRemaining = data["crafting_ticks_remaining"].get<i32>();
    }

    return true;
}

void CrafterBlockEntity::save(nlohmann::json& data) const
{
    ContainerBlockEntity::save(data);

    // 保存槽位禁用状态
    nlohmann::json disabledSlots = nlohmann::json::array();
    for (i32 i = 0; i < CONTAINER_SIZE; ++i) {
        if (m_slotStates[i] == SLOT_DISABLED) {
            disabledSlots.push_back(i);
        }
    }
    data["disabled_slots"] = disabledSlots;

    // 保存触发状态
    data["triggered"] = m_triggered ? 1 : 0;

    // 保存合成动画剩余tick
    data["crafting_ticks_remaining"] = m_craftingTicksRemaining;
}

std::unique_ptr<BlockEntity> CrafterBlockEntity::clone() const
{
    auto cloned = std::make_unique<CrafterBlockEntity>(m_pos);

    // 复制物品
    for (i32 i = 0; i < CONTAINER_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(i, stack.copy());
        }
    }

    // 复制槽位状态
    cloned->m_slotStates = m_slotStates;
    cloned->m_triggered = m_triggered;
    cloned->m_craftingTicksRemaining = m_craftingTicksRemaining;
    cloned->m_customName = m_customName;

    return cloned;
}

void CrafterBlockEntity::tick(IWorld& world)
{
    // 合成动画倒计时
    if (m_craftingTicksRemaining > 0) {
        --m_craftingTicksRemaining;
        setChanged();
        if (m_craftingTicksRemaining == 0) {
            // 动画结束，将 CRAFTING 状态重置为 false
            const BlockState* state = world.getBlockState(m_pos);
            if (state != nullptr && state->hasProperty(BlockStateProperties::CRAFTING()) &&
                state->get(BlockStateProperties::CRAFTING())) {
                BlockState newState = state->with(BlockStateProperties::CRAFTING(), false);
                world.setBlockState(m_pos, &newState, 3);
            }
        }
    }
}

bool CrafterBlockEntity::needsTick() const noexcept
{
    // 有合成动画倒计时时需要 tick
    return m_craftingTicksRemaining > 0;
}

void CrafterBlockEntity::clearContainer()
{
    m_inventory.clear();
    setChanged();
}

bool CrafterBlockEntity::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    if (m_slotStates[slot] == SLOT_DISABLED) {
        return false;
    }
    return m_inventory.canPlaceItem(slot, stack);
}

void CrafterBlockEntity::setSlotState(i32 slot, bool enabled)
{
    if (slotCanBeDisabled(slot) || enabled) {
        m_slotStates[slot] = enabled ? SLOT_ENABLED : SLOT_DISABLED;
        setChanged();
    }
}

bool CrafterBlockEntity::isSlotDisabled(i32 slot) const
{
    if (slot < 0 || slot >= CONTAINER_SIZE) {
        return false;
    }
    return m_slotStates[slot] == SLOT_DISABLED;
}

void CrafterBlockEntity::setTriggered(bool triggered)
{
    m_triggered = triggered;
    setChanged();
}

CraftingInventory CrafterBlockEntity::asCraftInput() const
{
    CraftingInventory input(3, 3);
    for (i32 i = 0; i < CONTAINER_SIZE; ++i) {
        // 禁用槽位视为空
        if (m_slotStates[i] == SLOT_DISABLED) {
            continue;
        }
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            i32 x = i % 3;
            i32 y = i / 3;
            input.setItemAt(x, y, stack.copy());
        }
    }
    return input;
}

i32 CrafterBlockEntity::getRedstoneSignal() const
{
    i32 signal = 0;
    for (i32 i = 0; i < CONTAINER_SIZE; ++i) {
        if (!m_inventory.getItem(i).isEmpty() || m_slotStates[i] == SLOT_DISABLED) {
            ++signal;
        }
    }
    return signal;
}

bool CrafterBlockEntity::slotCanBeDisabled(i32 slot) const
{
    return slot >= 0 && slot < CONTAINER_SIZE && m_inventory.getItem(slot).isEmpty();
}

void CrafterBlockEntity::_onInventoryChanged()
{
    // 当物品被放入禁用槽位时，自动重新启用该槽位
    // 对应MC原版CrafterBlockEntity.setItem()中的行为：
    // if (this.isSlotDisabled(p_307195_)) { this.setSlotState(p_307195_, true); }
    for (i32 i = 0; i < CONTAINER_SIZE; ++i) {
        if (m_slotStates[i] == SLOT_DISABLED && !m_inventory.getItem(i).isEmpty()) {
            m_slotStates[i] = SLOT_ENABLED;
        }
    }
}

} // namespace mc
