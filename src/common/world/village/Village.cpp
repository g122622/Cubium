#include "Village.hpp"
#include "../../util/nbt/Nbt.hpp"
#include "../IWorld.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace mc {
namespace world {
namespace village {

namespace {

constexpr std::array<poi::PointOfInterestType, 16> ALL_BED_TYPES = {
    poi::PointOfInterestType::BedRed,
    poi::PointOfInterestType::BedBlack,
    poi::PointOfInterestType::BedBlue,
    poi::PointOfInterestType::BedBrown,
    poi::PointOfInterestType::BedCyan,
    poi::PointOfInterestType::BedGray,
    poi::PointOfInterestType::BedGreen,
    poi::PointOfInterestType::BedLightBlue,
    poi::PointOfInterestType::BedLightGray,
    poi::PointOfInterestType::BedLime,
    poi::PointOfInterestType::BedMagenta,
    poi::PointOfInterestType::BedOrange,
    poi::PointOfInterestType::BedPink,
    poi::PointOfInterestType::BedPurple,
    poi::PointOfInterestType::BedWhite,
    poi::PointOfInterestType::BedYellow,
};

} // namespace

Village::Village(BlockPos center)
    : m_center(center)
    , m_radius(VillageConfig::BASE_RADIUS)
{
}

bool Village::isWithinVillage(BlockPos pos) const {
    f32 dx = static_cast<f32>(pos.x - m_center.x);
    f32 dy = static_cast<f32>(pos.y - m_center.y);
    f32 dz = static_cast<f32>(pos.z - m_center.z);
    f32 distSq = dx * dx + dy * dy + dz * dz;
    return distSq <= m_radius * m_radius;
}

bool Village::isWithinRaidTrigger(BlockPos pos) const {
    f32 dx = static_cast<f32>(pos.x - m_center.x);
    f32 dz = static_cast<f32>(pos.z - m_center.z);
    f32 distSq = dx * dx + dz * dz;
    f32 triggerRadius = VillageConfig::RAID_TRIGGER_RANGE;
    return distSq <= triggerRadius * triggerRadius;
}

void Village::recalculateBounds(const poi::PointOfInterestStorage& poiStorage) {
    // 基于床位和工作站重新计算村庄边界
    // 使用质心作为新中心

    std::vector<const poi::PointOfInterest*> beds;
    for (const auto bedType : ALL_BED_TYPES) {
        auto foundBeds = poiStorage.findAllByType(bedType);
        beds.insert(beds.end(), foundBeds.begin(), foundBeds.end());
    }

    if (beds.empty()) {
        m_bedCount = 0;
        m_radius = VillageConfig::BASE_RADIUS;
        return;
    }

    // 计算所有床位的质心
    i64 sumX = 0, sumY = 0, sumZ = 0;
    for (const auto* poi : beds) {
        BlockPos pos = poi->getPosition();
        sumX += pos.x;
        sumY += pos.y;
        sumZ += pos.z;
    }

    m_center.x = static_cast<i32>(sumX / static_cast<i64>(beds.size()));
    m_center.y = static_cast<i32>(sumY / static_cast<i64>(beds.size()));
    m_center.z = static_cast<i32>(sumZ / static_cast<i64>(beds.size()));

    // 更新床位计数
    m_bedCount = static_cast<i32>(beds.size());

    // 计算半径：基础半径 + 每个床位增加的半径
    m_radius = std::min(
        VillageConfig::BASE_RADIUS + static_cast<f32>(m_bedCount) * VillageConfig::RADIUS_PER_BED,
        VillageConfig::MAX_RADIUS
    );
}

void Village::addVillager(u64 villagerId) {
    m_villagers.insert(villagerId);
}

void Village::removeVillager(u64 villagerId) {
    m_villagers.erase(villagerId);
}

bool Village::hasVillager(u64 villagerId) const {
    return m_villagers.find(villagerId) != m_villagers.end();
}

i32 Village::getAvailableBeds() const {
    return m_bedCount - getPopulation();
}

bool Village::canBreed() const {
    // 村民繁殖条件：有足够的床位
    return getAvailableBeds() > 0 && getPopulation() > 0;
}

void Village::tick(IWorld& world, i64 gameTime) {
    // 更新流言（衰减）
    m_gossipManager.tick(gameTime);

    // TODO: 其他定期更新
    // - 检查村民是否仍在范围内
    // - 更新工作站绑定
    // - 检查袭击结束条件
    (void)world; // 暂时未使用
}

void Village::serialize(nbt::tags::compound_tag& tag) const {
    tag.put("CenterX", static_cast<std::int32_t>(m_center.x));
    tag.put("CenterY", static_cast<std::int32_t>(m_center.y));
    tag.put("CenterZ", static_cast<std::int32_t>(m_center.z));
    tag.put("Radius", static_cast<float>(m_radius));
    tag.put("BedCount", static_cast<std::int32_t>(m_bedCount));
    tag.put("WorkstationCount", static_cast<std::int32_t>(m_workstationCount));
    tag.put("UnderRaid", m_underRaid ? static_cast<std::int8_t>(1) : static_cast<std::int8_t>(0));
    tag.put("LastRaidTime", static_cast<std::int64_t>(m_lastRaidTime));
    tag.put("CreatedTime", static_cast<std::int64_t>(m_createdTime));

    // 序列化村民列表
    auto villagersList = std::make_unique<nbt::tags::long_list_tag>();
    for (u64 villagerId : m_villagers) {
        villagersList->value.push_back(static_cast<i64>(villagerId));
    }
    tag.value["Villagers"] = std::move(villagersList);

    // 序列化聚集点
    if (m_meetingPoint.has_value()) {
        nbt::tags::compound_tag meetingTag;
        meetingTag.put("X", static_cast<std::int32_t>(m_meetingPoint->x));
        meetingTag.put("Y", static_cast<std::int32_t>(m_meetingPoint->y));
        meetingTag.put("Z", static_cast<std::int32_t>(m_meetingPoint->z));
        tag.value["MeetingPoint"] = std::make_unique<nbt::tags::compound_tag>(std::move(meetingTag));
    }

    // 序列化流言
    nbt::tags::compound_tag gossipTag;
    m_gossipManager.serialize(gossipTag);
    tag.value["Gossip"] = std::make_unique<nbt::tags::compound_tag>(std::move(gossipTag));
}

Village Village::deserialize(const nbt::tags::compound_tag& tag) {
    BlockPos center;
    center.x = tag.get<nbt::tags::int_tag>("CenterX");
    center.y = tag.get<nbt::tags::int_tag>("CenterY");
    center.z = tag.get<nbt::tags::int_tag>("CenterZ");

    Village village(center);
    village.m_radius = tag.get<nbt::tags::float_tag>("Radius");
    village.m_bedCount = tag.get<nbt::tags::int_tag>("BedCount");
    village.m_workstationCount = tag.get<nbt::tags::int_tag>("WorkstationCount");
    village.m_underRaid = tag.get<nbt::tags::byte_tag>("UnderRaid") != 0;
    village.m_lastRaidTime = tag.get<nbt::tags::long_tag>("LastRaidTime");
    village.m_createdTime = tag.get<nbt::tags::long_tag>("CreatedTime");

    // 反序列化村民列表
    auto villagersIt = tag.value.find("Villagers");
    if (villagersIt != tag.value.end()) {
        auto* villagersList = dynamic_cast<const nbt::tags::long_list_tag*>(villagersIt->second.get());
        if (villagersList) {
            for (i64 villagerId : villagersList->value) {
                village.m_villagers.insert(static_cast<u64>(villagerId));
            }
        }
    }

    // 反序列化聚集点
    auto meetingIt = tag.value.find("MeetingPoint");
    if (meetingIt != tag.value.end()) {
        auto* meetingTag = dynamic_cast<const nbt::tags::compound_tag*>(meetingIt->second.get());
        if (meetingTag) {
            BlockPos pos;
            pos.x = meetingTag->get<nbt::tags::int_tag>("X");
            pos.y = meetingTag->get<nbt::tags::int_tag>("Y");
            pos.z = meetingTag->get<nbt::tags::int_tag>("Z");
            village.m_meetingPoint = pos;
        }
    }

    // 反序列化流言
    auto gossipIt = tag.value.find("Gossip");
    if (gossipIt != tag.value.end()) {
        auto* gossipTag = dynamic_cast<const nbt::tags::compound_tag*>(gossipIt->second.get());
        if (gossipTag) {
            village.m_gossipManager.deserialize(*gossipTag);
        }
    }

    return village;
}

} // namespace village
} // namespace world
} // namespace mc
