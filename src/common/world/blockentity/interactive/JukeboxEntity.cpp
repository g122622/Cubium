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

#include "world/blockentity/interactive/JukeboxEntity.hpp"

#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"

#include <unordered_map>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 获取唱片对应的比较器信号强度。
 *
 * MC 1.16.5: 每种唱片有固定的比较器输出值（1-15）。
 * 参考: net.minecraft.item.MusicDiscItem.comparatorValue
 *
 * @param item 唱片物品。
 * @return 范围 [1, 15] 的信号强度，如果不是有效唱片返回 0。
 */
[[nodiscard]] i32 getRecordComparatorSignal(const Item* item)
{
    if (item == nullptr) {
        return 0;
    }

    // MC 1.16.5 固定映射表
    // 参考: net.minecraft.item.Items 中的 MusicDiscItem 构造
    static const std::unordered_map<std::string, i32> s_discSignals = {
        {"minecraft:music_disc_13", 1},
        {"minecraft:music_disc_cat", 2},
        {"minecraft:music_disc_blocks", 3},
        {"minecraft:music_disc_chirp", 4},
        {"minecraft:music_disc_far", 5},
        {"minecraft:music_disc_mall", 6},
        {"minecraft:music_disc_mellohi", 7},
        {"minecraft:music_disc_stal", 8},
        {"minecraft:music_disc_strad", 9},
        {"minecraft:music_disc_ward", 10},
        {"minecraft:music_disc_11", 11},
        {"minecraft:music_disc_wait", 12},
        {"minecraft:music_disc_pigstep", 13},
    };

    const std::string& location = item->itemLocation().toString();
    const auto it = s_discSignals.find(location);
    return it != s_discSignals.end() ? it->second : 0;
}

} // namespace

JukeboxEntity::JukeboxEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::Jukebox, pos)
    , m_inventory(1)
{}

JukeboxEntity::~JukeboxEntity() = default;

ItemStack JukeboxEntity::getRecord() const
{
    return m_inventory.getItem(SLOT_RECORD);
}

void JukeboxEntity::setRecord(const ItemStack& record)
{
    m_inventory.setItem(SLOT_RECORD, record);
    setChanged();
}

bool JukeboxEntity::hasRecord() const
{
    return !m_inventory.getItem(SLOT_RECORD).isEmpty();
}

void JukeboxEntity::startPlaying(IWorld& world)
{
    MC_UNUSED(world);

    const ItemStack record = getRecord();
    if (record.isEmpty()) {
        m_isPlaying = false;
        m_recordId = 0;
        return;
    }

    const Item* item = record.getItem();
    m_recordId = getRecordComparatorSignal(item);
    m_isPlaying = m_recordId > 0;
    setChanged();
}

void JukeboxEntity::stopPlaying(IWorld& world)
{
    MC_UNUSED(world);

    if (!m_isPlaying && m_recordId == 0) {
        return;
    }

    m_isPlaying = false;
    m_recordId = 0;
    setChanged();
}

i32 JukeboxEntity::getComparatorSignal() const
{
    if (!hasRecord()) {
        return 0;
    }

    if (m_recordId > 0) {
        return m_recordId;
    }

    const ItemStack record = getRecord();
    return getRecordComparatorSignal(record.getItem());
}

void JukeboxEntity::tick(IWorld& world)
{
    if (m_isPlaying && !hasRecord()) {
        stopPlaying(world);
    }
}

bool JukeboxEntity::load(const nlohmann::json& data)
{
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    m_inventory.clear();
    if (data.contains("RecordItem") && data["RecordItem"].is_object()) {
        const auto recordResult = ItemStack::fromJson(data["RecordItem"]);
        if (recordResult.success()) {
            m_inventory.setItem(SLOT_RECORD, recordResult.value());
        }
    }

    m_isPlaying = data.value("IsPlaying", false);
    m_recordId = data.value("RecordId", 0);
    return true;
}

void JukeboxEntity::save(nlohmann::json& data) const
{
    ContainerBlockEntity::save(data);

    const ItemStack record = m_inventory.getItem(SLOT_RECORD);
    if (!record.isEmpty()) {
        data["RecordItem"] = record.toJson();
    }

    data["IsPlaying"] = m_isPlaying;
    data["RecordId"] = m_recordId;
}

std::unique_ptr<BlockEntity> JukeboxEntity::clone() const
{
    auto clone = std::make_unique<JukeboxEntity>(m_pos);
    clone->m_inventory.setItem(SLOT_RECORD, m_inventory.getItem(SLOT_RECORD));
    clone->m_isPlaying = m_isPlaying;
    clone->m_recordId = m_recordId;
    return clone;
}

} // namespace blockentity
} // namespace mc