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
#include "sound/SoundCategory.hpp"
#include "util/Direction.hpp"
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
{
    // 注入战利品表延迟填充回调，使 SimpleInventory 的所有内容访问方法
    // （isEmpty/getItem/setItem/removeItem/removeItemNoUpdate/clear）
    // 都自动触发 _unpackLootTable(nullptr)，与 MC Java 的
    // RandomizableContainerBlockEntity 行为一致。
    m_inventory.setLootUnpackCallback(_makeLootUnpackCallback());
}

BarrelEntity::~BarrelEntity() noexcept = default;

void BarrelEntity::openContainer(Player* player)
{
    // 触发战利品表填充
    fillWithLoot(player);

    // MC原版：仅在首个玩家打开（openCount从0变为1）时播放音效和更新方块状态
    const bool wasEmpty = (m_openCount == 0);

    // 基类已处理观察者检查和负数保护
    LootableContainerBlockEntity::openContainer(player);

    if (wasEmpty && m_world != nullptr) {
        _updateBlockState(*m_world, true);
        _playSound(true);
    }

    setChanged();
}

void BarrelEntity::closeContainer(Player* player)
{
    // 基类已处理观察者检查
    LootableContainerBlockEntity::closeContainer(player);

    if (m_world != nullptr) {
        _updateBlockState(*m_world, m_openCount > 0);
        if (m_openCount == 0) {
            _playSound(false);
        }
    }

    setChanged();
}

i32 BarrelEntity::getComparatorSignal(IWorld& /*world*/) const
{
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

        // 通知客户端方块实体数据更新
        // 参考 MC: BarrelBlockEntity.tick() 中定期调用 level.sendBlockUpdated()
        // notifyBlockUpdate 即使方块状态未改变也会触发客户端同步
        world.notifyBlockUpdate(m_pos);
    }
}

void BarrelEntity::_updateBlockState(IWorld& world, bool open)
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

void BarrelEntity::_playSound(bool isOpen)
{
    if (m_world == nullptr) {
        return;
    }

    // 获取方块朝向，根据朝向偏移音效位置
    const BlockState* state = m_world->getBlockState(m_pos);
    if (state == nullptr) {
        return;
    }

    Direction facing = state->hasProperty(BlockStateProperties::FACING()) ? state->get(BlockStateProperties::FACING())
                                                                          : Direction::North;

    // 音效位置：方块中心偏移朝向方向 0.5 格
    f32 offsetX = 0.5f + 0.5f * static_cast<f32>(Directions::xOffset(facing));
    f32 offsetY = 0.5f + 0.5f * static_cast<f32>(Directions::yOffset(facing));
    f32 offsetZ = 0.5f + 0.5f * static_cast<f32>(Directions::zOffset(facing));

    const char* soundId = isOpen ? "minecraft:block.barrel.open" : "minecraft:block.barrel.close";
    m_world->playSound(ResourceLocation(soundId),
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f64>(m_pos.x) + offsetX,
            static_cast<f64>(m_pos.y) + offsetY,
            static_cast<f64>(m_pos.z) + offsetZ),
        0.5f,
        1.0f);
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

// ========== NBT 序列化（结构模板 / 客户端同步）==========

bool BarrelEntity::loadFromNBT(const nbt::CompoundTag& tag)
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

void BarrelEntity::saveToNBT(nbt::CompoundTag& tag) const
{
    LootableContainerBlockEntity::saveToNBT(tag);

    // 仅在无未解包的战利品表时保存物品，与 MC Java 互斥语义一致
    if (!hasLootTable()) {
        saveItemsToNBT(tag, m_inventory);
    }
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
