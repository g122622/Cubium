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

#include "Village.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/poi/PointOfInterest.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include "common/world/village/raid/Raid.hpp"
#include "common/world/village/raid/RaidManager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

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

/// 工作站 POI 类型列表
constexpr std::array<poi::PointOfInterestType, 12> WORKSTATION_TYPES = {
    poi::PointOfInterestType::Smoker,
    poi::PointOfInterestType::BlastFurnace,
    poi::PointOfInterestType::CartographyTable,
    poi::PointOfInterestType::BrewingStand,
    poi::PointOfInterestType::Composter,
    poi::PointOfInterestType::Barrel,
    poi::PointOfInterestType::FletchingTable,
    poi::PointOfInterestType::Cauldron,
    poi::PointOfInterestType::Lectern,
    poi::PointOfInterestType::Stonecutter,
    poi::PointOfInterestType::SmithingTable,
    poi::PointOfInterestType::Loom,
};

} // namespace

Village::Village(BlockPos center)
    : m_center(center)
    , m_radius(VillageConfig::BASE_RADIUS)
{}

bool Village::isWithinVillage(BlockPos pos) const
{
    f32 dx = static_cast<f32>(pos.x - m_center.x);
    f32 dy = static_cast<f32>(pos.y - m_center.y);
    f32 dz = static_cast<f32>(pos.z - m_center.z);
    f32 distSq = dx * dx + dy * dy + dz * dz;
    return distSq <= m_radius * m_radius;
}

bool Village::isWithinRaidTrigger(BlockPos pos) const
{
    f32 dx = static_cast<f32>(pos.x - m_center.x);
    f32 dz = static_cast<f32>(pos.z - m_center.z);
    f32 distSq = dx * dx + dz * dz;
    f32 triggerRadius = VillageConfig::RAID_TRIGGER_RANGE;
    return distSq <= triggerRadius * triggerRadius;
}

void Village::recalculateBounds(const poi::PointOfInterestStorage& poiStorage)
{
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
    m_radius = std::min(VillageConfig::BASE_RADIUS + static_cast<f32>(m_bedCount) * VillageConfig::RADIUS_PER_BED,
        VillageConfig::MAX_RADIUS);
}

void Village::addVillager(u64 villagerId)
{
    m_villagers.insert(villagerId);
}

void Village::removeVillager(u64 villagerId)
{
    m_villagers.erase(villagerId);
}

bool Village::hasVillager(u64 villagerId) const
{
    return m_villagers.find(villagerId) != m_villagers.end();
}

i32 Village::getAvailableBeds() const
{
    return m_bedCount - getPopulation();
}

bool Village::canBreed() const
{
    // 村民繁殖条件：有足够的床位
    return getAvailableBeds() > 0 && getPopulation() > 0;
}

void Village::tick(IWorld& world, i64 gameTime, poi::PointOfInterestStorage* poiStorage)
{
    // 1. 更新流言（衰减）
    m_gossipManager.tick(gameTime);

    // 2. 检查村民是否仍在范围内，释放离开村民的 POI
    _tickVillagerCheck(world, gameTime, poiStorage);

    // 3. 定期更新 POI 统计（如果提供了 POI 存储）
    if (poiStorage != nullptr && gameTime - m_lastPOIStatUpdateTime >= POI_STAT_UPDATE_INTERVAL) {
        _tickPOIStats(*poiStorage);
        m_lastPOIStatUpdateTime = gameTime;
    }

    // 4. 定期验证袭击状态（每 20 tick 一次）
    if (gameTime - m_lastRaidCheckTime >= RAID_CHECK_INTERVAL) {
        _tickRaidCheck(world, gameTime);
        m_lastRaidCheckTime = gameTime;
    }
}

void Village::_tickVillagerCheck(IWorld& world, i64 gameTime, poi::PointOfInterestStorage* poiStorage)
{
    // 村庄村民归属管理：
    // 村民不隶属于某个 Village 对象，而是通过 POI 绑定（HOME/JOB_SITE/MEETING_POINT）
    // 隐式关联到村庄区域。村民不会因远离村庄而被"移除"。
    //
    // 本项目的 Village 类需要维护显式的村民列表用于流言系统和繁殖判定。
    // 村民与村庄的关联通过以下机制管理：
    // 1. VillagerEntity::die() / remove() 中主动调用 releaseAllPois() 和 onVillagerLeave()
    // 2. 本方法定期检查村民实体是否仍存在且在合理范围内

    std::vector<u64> villagersToRemove;

    for (u64 villagerId : m_villagers) {
        // 通过 EntityInstanceId 获取实体
        Entity* entity = world.getEntity(static_cast<EntityInstanceId>(villagerId));

        if (entity == nullptr) {
            // 实体不存在（已卸载或死亡），标记移除
            villagersToRemove.push_back(villagerId);
            continue;
        }

        // 检查实体是否已标记为移除（死亡动画结束等）
        if (entity->isRemoved()) {
            villagersToRemove.push_back(villagerId);
            continue;
        }

        // 检查是否为村民实体
        if (entity->entityType() != entity::VanillaEntityTypeKeys::VILLAGER) {
            // 不是村民（可能是数据错误），移除
            villagersToRemove.push_back(villagerId);
            continue;
        }

        // 获取村民位置
        Vector3 pos = entity->position();
        BlockPos blockPos(static_cast<i32>(std::floor(pos.x)),
            static_cast<i32>(std::floor(pos.y)),
            static_cast<i32>(std::floor(pos.z)));

        // 检查是否在村庄范围内
        if (isWithinVillage(blockPos)) {
            // 村民在范围内，更新最后出现时间
            m_villagerLastSeenTime[villagerId] = gameTime;
        } else {
            // 村民不在村庄范围内
            // POI 记忆只在村民靠近时验证，超出村庄范围的村民给予超时宽限期
            auto it = m_villagerLastSeenTime.find(villagerId);
            if (it != m_villagerLastSeenTime.end()) {
                i64 timeSinceLastSeen = gameTime - it->second;
                if (timeSinceLastSeen > VILLAGER_TIMEOUT) {
                    // 超时，村民长时间不在村庄范围内，视为离开
                    villagersToRemove.push_back(villagerId);
                }
                // 未超时：村民可能暂时离开村庄（如去远处工作），保持关联
            } else {
                // 没有最后出现时间记录，说明是新加入的村民但已经在范围外
                // 记录当前时间作为超时起点，给予宽限期
                m_villagerLastSeenTime[villagerId] = gameTime;
            }
        }
    }

    // 移除离开村庄的村民，并释放其占用的 POI
    for (u64 villagerId : villagersToRemove) {
        // 释放该村民占用的所有 POI（床位、工作站等）
        if (poiStorage != nullptr) {
            poiStorage->releaseAllByOwner(villagerId);
        }

        m_villagers.erase(villagerId);
        m_villagerLastSeenTime.erase(villagerId);
    }
}

void Village::_tickPOIStats(const poi::PointOfInterestStorage& poiStorage)
{
    // 更新床位计数
    i32 bedCount = 0;
    for (const auto bedType : ALL_BED_TYPES) {
        auto beds = poiStorage.findAllInRange(m_center, m_radius, bedType);
        bedCount += static_cast<i32>(beds.size());
    }
    m_bedCount = bedCount;

    // 更新工作站计数
    i32 workstationCount = 0;
    for (const auto wsType : WORKSTATION_TYPES) {
        auto workstations = poiStorage.findAllInRange(m_center, m_radius, wsType);
        workstationCount += static_cast<i32>(workstations.size());
    }
    m_workstationCount = workstationCount;

    // 更新聚集点（钟）
    auto meetingPoint = _findMeetingPoint(poiStorage);
    if (meetingPoint.has_value()) {
        m_meetingPoint = meetingPoint;
    } else {
        m_meetingPoint = std::nullopt;
    }

    // 重新计算村庄边界（基于床位数）
    m_radius = std::min(VillageConfig::BASE_RADIUS + static_cast<f32>(m_bedCount) * VillageConfig::RADIUS_PER_BED,
        VillageConfig::MAX_RADIUS);
}

void Village::_tickRaidCheck(IWorld& world, i64 gameTime)
{
    // 通过 IWorld 获取 RaidManager，验证村庄的袭击状态与实际袭击同步。
    // 本项目中 Village 持有 m_underRaid 标志，由 RaidManager 在 tryStartRaid/onRaidEnd 设置。
    // _tickRaidCheck 作为冗余验证，确保标志与 RaidManager 中的实际袭击状态一致，
    // 防止因异常情况（如 RaidManager 跳过 onRaidEnd）导致标志永远为 true。
    auto* raidManager = world.raidManager();
    if (raidManager == nullptr) {
        // 非 ServerWorld 环境（如客户端）没有 RaidManager，跳过检查
        return;
    }

    raid::Raid* raid = raidManager->getOngoingRaidForVillage(this);

    if (m_underRaid) {
        // 村庄标记为袭击中，但 RaidManager 中没有对应的进行中袭击
        // 说明袭击已经结束但 onRaidEnd 未正确同步（防御性检查）
        if (raid == nullptr) {
            m_underRaid = false;
            m_lastRaidTime = gameTime;
        }
    } else {
        // 村庄标记为非袭击中，但 RaidManager 中存在针对此村庄的进行中袭击
        // 可能是外部直接创建了袭击而未设置 village 标志（防御性检查）
        if (raid != nullptr) {
            m_underRaid = true;
        }
    }
}

std::optional<BlockPos> Village::_findMeetingPoint(const poi::PointOfInterestStorage& poiStorage) const
{
    // 在村庄范围内搜索钟 POI
    auto bells = poiStorage.findAllInRange(m_center, m_radius, poi::PointOfInterestType::Bell);

    if (bells.empty()) {
        return std::nullopt;
    }

    // 找到最近的钟
    const poi::PointOfInterest* nearestBell = nullptr;
    i64 minDistSq = std::numeric_limits<i64>::max();

    for (const auto* poi : bells) {
        BlockPos bellPos = poi->getPosition();
        i64 distSq = bellPos.distanceSq(m_center);
        if (distSq < minDistSq) {
            minDistSq = distSq;
            nearestBell = poi;
        }
    }

    if (nearestBell != nullptr) {
        return nearestBell->getPosition();
    }

    return std::nullopt;
}

void Village::serialize(nbt::tags::compound_tag& tag) const
{
    tag.put("Id", static_cast<std::int64_t>(m_id));
    tag.put("CenterX", static_cast<std::int32_t>(m_center.x));
    tag.put("CenterY", static_cast<std::int32_t>(m_center.y));
    tag.put("CenterZ", static_cast<std::int32_t>(m_center.z));
    tag.put("Radius", static_cast<float>(m_radius));
    tag.put("BedCount", static_cast<std::int32_t>(m_bedCount));
    tag.put("WorkstationCount", static_cast<std::int32_t>(m_workstationCount));
    tag.put("UnderRaid", m_underRaid ? static_cast<std::int8_t>(1) : static_cast<std::int8_t>(0));
    tag.put("LastRaidTime", static_cast<std::int64_t>(m_lastRaidTime));
    tag.put("CreatedTime", static_cast<std::int64_t>(m_createdTime));
    tag.put("LastPOIStatUpdateTime", static_cast<std::int64_t>(m_lastPOIStatUpdateTime));

    // 序列化村民列表
    auto villagersList = std::make_unique<nbt::tags::long_list_tag>();
    for (u64 villagerId : m_villagers) {
        villagersList->value.push_back(static_cast<i64>(villagerId));
    }
    tag.value["Villagers"] = std::move(villagersList);

    // 序列化村民最后出现时间
    auto villagerTimesList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& [villagerId, time] : m_villagerLastSeenTime) {
        nbt::tags::compound_tag entry;
        entry.put("VillagerId", static_cast<std::int64_t>(villagerId));
        entry.put("LastSeenTime", static_cast<std::int64_t>(time));
        villagerTimesList->value.push_back(std::move(entry));
    }
    tag.value["VillagerLastSeenTimes"] = std::move(villagerTimesList);

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

Village Village::deserialize(const nbt::tags::compound_tag& tag)
{
    BlockPos center;
    center.x = tag.get<nbt::tags::int_tag>("CenterX");
    center.y = tag.get<nbt::tags::int_tag>("CenterY");
    center.z = tag.get<nbt::tags::int_tag>("CenterZ");

    Village village(center);

    // 反序列化 ID（向后兼容）
    if (tag.value.find("Id") != tag.value.end()) {
        village.m_id = static_cast<VillageId>(tag.get<nbt::tags::long_tag>("Id"));
    }

    village.m_radius = tag.get<nbt::tags::float_tag>("Radius");
    village.m_bedCount = tag.get<nbt::tags::int_tag>("BedCount");
    village.m_workstationCount = tag.get<nbt::tags::int_tag>("WorkstationCount");
    village.m_underRaid = tag.get<nbt::tags::byte_tag>("UnderRaid") != 0;
    village.m_lastRaidTime = tag.get<nbt::tags::long_tag>("LastRaidTime");
    village.m_createdTime = tag.get<nbt::tags::long_tag>("CreatedTime");

    // 可选字段：POI 统计更新时间（向后兼容）
    if (tag.value.find("LastPOIStatUpdateTime") != tag.value.end()) {
        village.m_lastPOIStatUpdateTime = tag.get<nbt::tags::long_tag>("LastPOIStatUpdateTime");
    }

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

    // 反序列化村民最后出现时间（向后兼容）
    auto villagerTimesIt = tag.value.find("VillagerLastSeenTimes");
    if (villagerTimesIt != tag.value.end()) {
        auto* villagerTimesList = dynamic_cast<const nbt::tags::compound_list_tag*>(villagerTimesIt->second.get());
        if (villagerTimesList) {
            for (const auto& entry : villagerTimesList->value) {
                u64 villagerId = static_cast<u64>(entry.get<nbt::tags::long_tag>("VillagerId"));
                i64 lastSeenTime = entry.get<nbt::tags::long_tag>("LastSeenTime");
                village.m_villagerLastSeenTime[villagerId] = lastSeenTime;
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
