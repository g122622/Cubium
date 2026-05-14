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

#include "world/blockentity/storage/BarrelEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/Block.hpp"

namespace mc {
namespace blockentity {

// ========== BarrelEntity 实现 ==========

BarrelEntity::BarrelEntity(const BlockPos& pos)
    : LootableContainerBlockEntity(BlockEntityType::Barrel, pos)
    , m_inventory(BARREL_SIZE)
{}

BarrelEntity::~BarrelEntity() = default;

void BarrelEntity::openContainer(Player* player)
{
    // 触发战利品表填充
    fillWithLoot(player);

    // 基类已处理观察者检查和负数保护
    LootableContainerBlockEntity::openContainer(player);

    if (m_world != nullptr) {
        updateBlockState(*m_world, true);
    }

    setChanged();
}

void BarrelEntity::closeContainer(Player* player)
{
    // 基类已处理观察者检查
    LootableContainerBlockEntity::closeContainer(player);

    if (m_world != nullptr) {
        updateBlockState(*m_world, m_openCount > 0);
    }

    setChanged();
}

i32 BarrelEntity::getComparatorSignal(IWorld& world) const
{
    MC_UNUSED(world);

    i32 filledSlots = 0;
    i32 totalCount = 0;

    for (i32 i = 0; i < BARREL_SIZE; ++i) {
        const ItemStack& stack = m_inventory.getItem(i);
        if (!stack.isEmpty()) {
            filledSlots++;
            totalCount += stack.getCount();
        }
    }

    if (filledSlots == 0) {
        return 0;
    }

    const f32 fillRatio = static_cast<f32>(filledSlots) / static_cast<f32>(BARREL_SIZE);
    return static_cast<i32>(fillRatio * 14.0f) + (totalCount > 0 ? 1 : 0);
}

void BarrelEntity::tick(IWorld& world)
{
    if (m_world == nullptr) {
        m_world = &world;
    }

    m_ticksSinceSync++;

    if (m_ticksSinceSync >= 10) {
        m_ticksSinceSync = 0;

        // 通过 setBlockState 触发方块更新与客户端状态同步。
        const BlockState* state = world.getBlockState(m_pos);
        if (state != nullptr) {
            world.setBlockState(m_pos, state, 3);
        }
    }

    MC_UNUSED(world);
}

void BarrelEntity::updateBlockState(IWorld& world, bool open)
{
    const BlockState* state = world.getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }

    if (!state->hasProperty(BlockStateProperties::OPEN())) {
        return;
    }

    const BlockState& updated = state->with(BlockStateProperties::OPEN(), open);
    world.setBlockState(m_pos, &updated, 3);
}

bool BarrelEntity::load(const nlohmann::json& data)
{
    if (!LootableContainerBlockEntity::load(data)) {
        return false;
    }

    if (data.contains("items")) {
        m_inventory.load(data["items"]);
    }

    if (data.contains("open_count")) {
        m_openCount = data["open_count"].get<i32>();
    }

    return true;
}

void BarrelEntity::save(nlohmann::json& data) const
{
    LootableContainerBlockEntity::save(data);

    nlohmann::json itemsJson;
    m_inventory.save(itemsJson);
    data["items"] = itemsJson;
    data["open_count"] = m_openCount;
}

std::unique_ptr<BlockEntity> BarrelEntity::clone() const
{
    auto cloned = std::make_unique<BarrelEntity>(m_pos);
    cloned->m_openCount = m_openCount;
    for (i32 slot = 0; slot < BARREL_SIZE; ++slot) {
        const ItemStack stack = m_inventory.getItem(slot);
        if (!stack.isEmpty()) {
            cloned->m_inventory.setItem(slot, stack.copy());
        }
    }
    return cloned;
}

} // namespace blockentity
} // namespace mc
