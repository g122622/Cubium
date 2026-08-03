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

#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/MusicDiscItem.hpp"
#include "common/sound/jukebox/JukeboxSong.hpp"
#include "common/sound/jukebox/JukeboxSongs.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

JukeboxEntity::JukeboxEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::Jukebox, pos)
    , m_inventory(1)
    , m_songPlayer([this]() { onSongChanged(); }, pos)
{}

JukeboxEntity::~JukeboxEntity() noexcept = default;

ItemStack JukeboxEntity::getRecord() const
{
    return m_inventory.getItem(SLOT_RECORD);
}

void JukeboxEntity::setRecord(const ItemStack& record, IWorld& world)
{
    m_inventory.setItem(SLOT_RECORD, record);
    setChanged();

    // 根据唱片内容开始或停止播放
    if (!record.isEmpty()) {
        startPlaying(world);
    } else {
        stopPlaying(world);
    }
}

bool JukeboxEntity::hasRecord() const
{
    return !m_inventory.getItem(SLOT_RECORD).isEmpty();
}

void JukeboxEntity::startPlaying(IWorld& world)
{
    const ItemStack record = getRecord();
    if (record.isEmpty()) {
        stopPlaying(world);
        return;
    }

    // 通过 MusicDiscItem 的声音事件ID查找对应的 JukeboxSong
    const Item* item = record.getItem();
    const auto* discItem = dynamic_cast<const item::items::MusicDiscItem*>(item);
    if (discItem == nullptr) {
        // 非 MusicDiscItem 类型的物品，停止播放
        stopPlaying(world);
        return;
    }

    // 从 JukeboxSongs 注册表查找歌曲
    const JukeboxSong* song = JukeboxSongs::getSongBySoundEvent(discItem->getSoundEventId());
    if (song == nullptr) {
        // 未找到对应歌曲，停止播放
        // 这种情况理论上不应该发生，因为所有唱片都有对应的歌曲定义
        stopPlaying(world);
        return;
    }

    m_songPlayer.play(world, *song);
}

void JukeboxEntity::stopPlaying(IWorld& world)
{
    m_songPlayer.stop(world);
}

i32 JukeboxEntity::getComparatorSignal() const
{
    if (!hasRecord()) {
        return 0;
    }

    // 从 MusicDiscItem 获取比较器信号强度
    const ItemStack record = getRecord();
    const Item* item = record.getItem();
    const auto* discItem = dynamic_cast<const item::items::MusicDiscItem*>(item);
    if (discItem != nullptr) {
        return discItem->getComparatorOutput();
    }

    return 0;
}

void JukeboxEntity::tick(IWorld& world)
{
    // 检查唱片是否被移除（如漏斗提取）
    if (!hasRecord() && m_songPlayer.isPlaying()) {
        stopPlaying(world);
        return;
    }

    m_songPlayer.tick(world);
}

bool JukeboxEntity::needsTick() const noexcept
{
    // 只有正在播放时才需要 tick
    // 参考 MC 1.21.11: JukeboxBlock.getTicker() 仅在 HAS_RECORD 为 true 时返回 ticker
    return m_songPlayer.isPlaying();
}

void JukeboxEntity::onSongChanged()
{
    setChanged();
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

    // 恢复播放进度
    const bool wasPlaying = data.value("IsPlaying", false);
    i64 ticksSinceSongStarted = data.value("TicksSinceSongStarted", static_cast<i64>(0));

    if (wasPlaying && hasRecord()) {
        const ItemStack record = getRecord();
        const Item* item = record.getItem();
        const auto* discItem = dynamic_cast<const item::items::MusicDiscItem*>(item);
        if (discItem != nullptr) {
            const JukeboxSong* song = JukeboxSongs::getSongBySoundEvent(discItem->getSoundEventId());
            if (song != nullptr) {
                // 从存档恢复播放状态，但不重新播放声音
                m_songPlayer.setSongWithoutPlaying(*song, ticksSinceSongStarted);
            }
        }
    }

    return true;
}

void JukeboxEntity::save(nlohmann::json& data) const
{
    ContainerBlockEntity::save(data);

    const ItemStack record = m_inventory.getItem(SLOT_RECORD);
    if (!record.isEmpty()) {
        data["RecordItem"] = record.toJson();
    }

    data["IsPlaying"] = m_songPlayer.isPlaying();
    if (m_songPlayer.isPlaying()) {
        data["TicksSinceSongStarted"] = m_songPlayer.getTicksSinceSongStarted();
    }
}

std::unique_ptr<BlockEntity> JukeboxEntity::clone() const
{
    auto cloned = std::make_unique<JukeboxEntity>(m_pos);
    cloned->m_inventory.setItem(SLOT_RECORD, m_inventory.getItem(SLOT_RECORD));
    // 注意：克隆时不恢复播放状态，因为新实体不在世界中
    return cloned;
}

} // namespace blockentity
} // namespace mc
