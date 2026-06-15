/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or/or sell
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

#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/MusicDiscItem.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"

namespace mc {
namespace blockentity {

JukeboxEntity::JukeboxEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::Jukebox, pos)
    , m_inventory(1)
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
        m_isPlaying = false;
        m_ticksSinceSongStarted = 0;
        m_songLengthTicks = 0;
        return;
    }

    // 从 MusicDiscItem 获取比较器信号强度
    const Item* item = record.getItem();
    const auto* discItem = dynamic_cast<const item::items::MusicDiscItem*>(item);
    if (discItem == nullptr) {
        // 非 MusicDiscItem 类型的唱片物品（不应该发生，但做防御性处理）
        m_isPlaying = false;
        m_ticksSinceSongStarted = 0;
        m_songLengthTicks = 0;
        return;
    }

    // 通过 world.playEvent 广播播放事件给所有客户端
    // data 参数为唱片对应的比较器信号强度，客户端根据此值确定播放哪首曲目
    // 参考 MC 1.21.11: JukeboxSongPlayer.play() 使用注册表 ID，但我们的简化实现使用信号强度
    world.playEvent(world::WorldEvents::PLAY_RECORD_SOUND, m_pos, discItem->getComparatorOutput());

    m_isPlaying = true;
    m_ticksSinceSongStarted = 0;
    m_songLengthTicks = 0; // TODO: 歌曲长度未知，播放直到唱片被移除。需要实现JukeboxSong注册表，
                           // 存储每首唱片的lengthInSeconds，在startPlaying()中计算lengthInTicks
                           // = ceil(lengthInSeconds * 20)，并在tick()中检测hasFinished()自动停止

    setChanged();
}

void JukeboxEntity::stopPlaying(IWorld& world)
{
    if (!m_isPlaying) {
        return;
    }

    // 广播停止事件：data=0 表示停止播放
    world.playEvent(world::WorldEvents::PLAY_RECORD_SOUND, m_pos, 0);

    m_isPlaying = false;
    m_ticksSinceSongStarted = 0;
    m_songLengthTicks = 0;

    setChanged();
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
    if (!m_isPlaying) {
        return;
    }

    // 检查唱片是否被移除（如漏斗提取）
    if (!hasRecord()) {
        stopPlaying(world);
        return;
    }

    ++m_ticksSinceSongStarted;

    // TODO: 歌曲自动结束检测。原版MC中JukeboxSongPlayer.tick()会在
    // ticksSinceSongStarted >= lengthInTicks + 20时自动调用stop()。
    // 当前m_songLengthTicks始终为0（未知），需要实现JukeboxSong注册表后补充。

    // 每20tick（1秒）触发一次音符粒子效果和游戏事件
    // 参考 MC 1.21.11: JukeboxSongPlayer.tick() 中 shouldEmitJukeboxPlayingEvent()
    if (m_ticksSinceSongStarted % 20 == 0) {
        // TODO: 触发 GameEvent.JUKEBOX_PLAY 和音符粒子效果
        // 暂时跳过，等游戏事件系统和粒子系统完善后补充
    }
}

bool JukeboxEntity::needsTick() const noexcept
{
    // 只有正在播放时才需要 tick
    return m_isPlaying;
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
    m_ticksSinceSongStarted = data.value("TicksSinceSongStarted", static_cast<i64>(0));
    m_songLengthTicks = 0;

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
    data["TicksSinceSongStarted"] = m_ticksSinceSongStarted;
}

std::unique_ptr<BlockEntity> JukeboxEntity::clone() const
{
    auto cloned = std::make_unique<JukeboxEntity>(m_pos);
    cloned->m_inventory.setItem(SLOT_RECORD, m_inventory.getItem(SLOT_RECORD));
    cloned->m_isPlaying = m_isPlaying;
    cloned->m_ticksSinceSongStarted = m_ticksSinceSongStarted;
    cloned->m_songLengthTicks = m_songLengthTicks;
    return cloned;
}

} // namespace blockentity
} // namespace mc
