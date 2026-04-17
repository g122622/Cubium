#include "world/blockentity/interactive/JukeboxEntity.hpp"

#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 判断物品是否为唱片。
 * @param item 待检查物品。
 * @return true 表示是唱片。
 * @note 目前按 `music_disc_` 前缀识别，后续接入物品标签系统时可替换为标签判定。
 */
[[nodiscard]] bool isMusicDisc(const Item* item) {
    if (item == nullptr) {
        return false;
    }

    const String& path = item->itemLocation().path();
    return path.rfind("music_disc_", 0) == 0;
}

/**
 * @brief 将唱片映射到比较器信号强度。
 * @param item 唱片物品。
 * @return 范围 [1, 15] 的信号强度。
 */
[[nodiscard]] i32 mapRecordToSignal(const Item* item) {
    if (!isMusicDisc(item)) {
        return 0;
    }

    const String& path = item->itemLocation().path();
    u32 hash = 2166136261u;
    for (char c : path) {
        hash ^= static_cast<u8>(c);
        hash *= 16777619u;
    }

    return static_cast<i32>(hash % 15u) + 1;
}

} // namespace

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
    MC_UNUSED(world);

    const ItemStack record = getRecord();
    if (record.isEmpty()) {
        m_isPlaying = false;
        m_recordId = 0;
        return;
    }

    const Item* item = record.getItem();
    m_recordId = mapRecordToSignal(item);
    m_isPlaying = m_recordId > 0;
    setChanged();
}

void JukeboxEntity::stopPlaying(IWorld& world) {
    MC_UNUSED(world);

    if (!m_isPlaying && m_recordId == 0) {
        return;
    }

    m_isPlaying = false;
    m_recordId = 0;
    setChanged();
}

i32 JukeboxEntity::getComparatorSignal() const {
    if (!hasRecord()) {
        return 0;
    }

    if (m_recordId > 0) {
        return m_recordId;
    }

    const ItemStack record = getRecord();
    return mapRecordToSignal(record.getItem());
}

void JukeboxEntity::tick(IWorld& world) {
    if (m_isPlaying && !hasRecord()) {
        stopPlaying(world);
    }
}

bool JukeboxEntity::load(const nlohmann::json& data) {
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

void JukeboxEntity::save(nlohmann::json& data) const {
    ContainerBlockEntity::save(data);

    const ItemStack record = m_inventory.getItem(SLOT_RECORD);
    if (!record.isEmpty()) {
        data["RecordItem"] = record.toJson();
    }

    data["IsPlaying"] = m_isPlaying;
    data["RecordId"] = m_recordId;
}

std::unique_ptr<BlockEntity> JukeboxEntity::clone() const {
    auto clone = std::make_unique<JukeboxEntity>(m_pos);
    clone->m_inventory.setItem(SLOT_RECORD, m_inventory.getItem(SLOT_RECORD));
    clone->m_isPlaying = m_isPlaying;
    clone->m_recordId = m_recordId;
    return clone;
}

} // namespace blockentity
} // namespace mc