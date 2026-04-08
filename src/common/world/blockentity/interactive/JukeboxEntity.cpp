#include "world/blockentity/interactive/JukeboxEntity.hpp"
#include "world/IWorld.hpp"
#include "item/core/ItemStack.hpp"
#include "item/core/Item.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== JukeboxEntity 实现 ==========

JukeboxEntity::JukeboxEntity(const BlockPos& pos)
    : ContainerBlockEntity(BlockEntityType::Jukebox, pos)
    , m_inventory(1) {
}

JukeboxEntity::~JukeboxEntity() = default;

ItemStack JukeboxEntity::getRecord() const {
    return m_inventory.getItem(SLOT_RECORD);
}

void JukeboxEntity::setRecord(const ItemStack& record) {
    m_inventory.setItem(SLOT_RECORD, record);
    setChanged();
}

bool JukeboxEntity::hasRecord() const {
    return !m_inventory.getItem(SLOT_RECORD).isEmpty();
}

void JukeboxEntity::startPlaying(IWorld& world) {
    if (!hasRecord()) {
        return;
    }

    const ItemStack& record = getRecord();
    if (record.isEmpty()) {
        return;
    }

    // 获取唱片ID
    // TODO: 从物品获取唱片类型和音乐
    m_isPlaying = true;
    // 使用物品登记ID作为唱片ID
    const Item* item = record.getItem();
    m_recordId = item != nullptr ? static_cast<i32>(item->itemId() % 16u) : 0;

    // TODO: 播放音乐
    // world.playEvent(pos, 1010, m_recordId);

    setChanged();
    MC_UNUSED(world);
}

void JukeboxEntity::stopPlaying(IWorld& world) {
    if (m_isPlaying) {
        m_isPlaying = false;
        m_recordId = 0;

        // TODO: 停止音乐
        // world.playEvent(pos, 1011, 0);

        setChanged();
    }
    MC_UNUSED(world);
}

i32 JukeboxEntity::getComparatorSignal() const {
    if (!hasRecord()) {
        return 0;
    }

    // 唱片机的比较器信号 = 唱片ID
    // 不同唱片有不同的信号强度（1-15）
    const ItemStack& record = getRecord();
    if (record.isEmpty()) {
        return 0;
    }

    // TODO: 映射唱片ID到信号强度
    const Item* item = record.getItem();
    return item != nullptr ? static_cast<i32>(item->itemId() % 15u) + 1 : 0;
}

void JukeboxEntity::tick(IWorld& world) {
    // 检查唱片是否被移除
    if (m_isPlaying && !hasRecord()) {
        stopPlaying(world);
    }
}

bool JukeboxEntity::load(const nlohmann::json& data) {
    if (!ContainerBlockEntity::load(data)) {
        return false;
    }

    // 加载唱片
    if (data.contains("RecordItem")) {
        // TODO: 加载ItemStack
        // m_inventory.load(data["RecordItem"]);
    }

    if (data.contains("IsPlaying")) {
        m_isPlaying = data["IsPlaying"].get<bool>();
    }

    if (data.contains("RecordId")) {
        m_recordId = data["RecordId"].get<i32>();
    }

    return true;
}

void JukeboxEntity::save(nlohmann::json& data) const {
    ContainerBlockEntity::save(data);

    // 保存唱片
    if (!m_inventory.getItem(SLOT_RECORD).isEmpty()) {
        // TODO: 保存ItemStack
        nlohmann::json recordJson;
        m_inventory.save(recordJson);
        data["RecordItem"] = recordJson;
    }

    data["IsPlaying"] = m_isPlaying;
    data["RecordId"] = m_recordId;
}

std::unique_ptr<BlockEntity> JukeboxEntity::clone() const {
    auto clone = std::make_unique<JukeboxEntity>(m_pos);
    clone->m_isPlaying = m_isPlaying;
    clone->m_recordId = m_recordId;
    // TODO: 复制物品
    return clone;
}

} // namespace blockentity
} // namespace mc
