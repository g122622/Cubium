#include "VillageManager.hpp"
#include "../../util/nbt/Nbt.hpp"
#include "../IWorld.hpp"
#include <array>
#include <algorithm>
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

} // anonymous namespace

VillageManager::VillageManager(IWorld& world)
    : m_world(world)
{
}

// ========== 村庄查询 ==========

Village* VillageManager::getVillageAt(BlockPos pos) {
    for (auto& village : m_villages) {
        if (village->isWithinVillage(pos)) {
            return village.get();
        }
    }
    return nullptr;
}

const Village* VillageManager::getVillageAt(BlockPos pos) const {
    for (const auto& village : m_villages) {
        if (village->isWithinVillage(pos)) {
            return village.get();
        }
    }
    return nullptr;
}

Village* VillageManager::getOrCreateVillage(BlockPos pos) {
    // 首先检查是否已有村庄
    Village* existing = getVillageAt(pos);
    if (existing) {
        return existing;
    }

    // 检查附近是否有床位 POI（包含所有床颜色）
    std::vector<const poi::PointOfInterest*> beds;
    for (const auto bedType : ALL_BED_TYPES) {
        auto found = m_poiStorage.findAllInRange(pos, VillageConfig::BASE_RADIUS, bedType);
        beds.insert(beds.end(), found.begin(), found.end());
    }

    if (beds.empty()) {
        // 没有POI，不创建村庄
        return nullptr;
    }

    // 计算POI的中心作为村庄中心
    i64 sumX = 0, sumY = 0, sumZ = 0;
    for (const auto* poi : beds) {
        sumX += poi->getPosition().x;
        sumY += poi->getPosition().y;
        sumZ += poi->getPosition().z;
    }

    BlockPos center;
    center.x = static_cast<i32>(sumX / static_cast<i64>(beds.size()));
    center.y = static_cast<i32>(sumY / static_cast<i64>(beds.size()));
    center.z = static_cast<i32>(sumZ / static_cast<i64>(beds.size()));

    return createVillage(center);
}

Village* VillageManager::getVillageById(VillageId id) {
    auto it = m_villageById.find(id);
    return it != m_villageById.end() ? it->second : nullptr;
}

// ========== 村民管理 ==========

void VillageManager::onVillagerJoin(u64 villagerId, BlockPos pos) {
    // 如果村民已在其他村庄，先离开
    onVillagerLeave(villagerId);

    // 查找或创建村庄
    Village* village = getOrCreateVillage(pos);
    if (!village) {
        return;
    }

    // 加入村庄
    village->addVillager(villagerId);
    m_villagerToVillage[villagerId] = village;

    // 更新区块映射
    u64 chunkKey = getChunkKey(
        static_cast<ChunkCoord>(pos.x >> 4),
        static_cast<ChunkCoord>(pos.z >> 4)
    );
    // 注意：这里需要知道村庄ID，暂时跳过
}

void VillageManager::onVillagerLeave(u64 villagerId) {
    auto it = m_villagerToVillage.find(villagerId);
    if (it == m_villagerToVillage.end()) {
        return;
    }

    Village* village = it->second;
    village->removeVillager(villagerId);
    m_villagerToVillage.erase(it);
}

Village* VillageManager::getVillageForVillager(u64 villagerId) {
    auto it = m_villagerToVillage.find(villagerId);
    return it != m_villagerToVillage.end() ? it->second : nullptr;
}

std::vector<u64> VillageManager::getVillagersInVillage(VillageId villageId) const {
    std::vector<u64> result;
    Village* village = const_cast<VillageManager*>(this)->getVillageById(villageId);
    if (village) {
        const auto& villagers = village->getVillagers();
        result.reserve(villagers.size());
        for (u64 id : villagers) {
            result.push_back(id);
        }
    }
    return result;
}

// ========== POI管理 ==========

void VillageManager::onBlockPlaced(BlockPos pos, u32 blockId) {
    // 检查是否为POI类型
    poi::PointOfInterestType poiType = poi::POITypeHelper::fromBlockId(blockId);
    if (poiType == poi::PointOfInterestType::None) {
        return;
    }

    // 注册POI
    m_poiStorage.registerPOI(pos, poiType);

    // 检查是否需要创建新村庄或更新现有村庄
    Village* village = getVillageAt(pos);
    if (village) {
        // 更新村庄边界
        village->recalculateBounds(m_poiStorage);
    } else if (poi::POITypeHelper::isBed(poiType)) {
        // 床可能创建新村庄
        getOrCreateVillage(pos);
    }
}

void VillageManager::onBlockRemoved(BlockPos pos) {
    // 尝试注销POI
    if (m_poiStorage.hasPOI(pos)) {
        m_poiStorage.unregisterPOI(pos);

        // 更新附近的村庄边界
        for (auto& village : m_villages) {
            if (village->isWithinVillage(pos)) {
                village->recalculateBounds(m_poiStorage);
            }
        }
    }
}

// ========== 袭击管理 ==========

bool VillageManager::isInRaidRange(BlockPos pos) const {
    for (const auto& village : m_villages) {
        if (village->isUnderRaid() && village->isWithinRaidTrigger(pos)) {
            return true;
        }
    }
    return false;
}

Village* VillageManager::getVillageUnderRaid(BlockPos pos) {
    for (auto& village : m_villages) {
        if (village->isUnderRaid() && village->isWithinRaidTrigger(pos)) {
            return village.get();
        }
    }
    return nullptr;
}

// ========== Tick更新 ==========

void VillageManager::tick(i64 gameTime) {
    // 更新所有村庄，传递可修改的 POI 存储
    for (auto& village : m_villages) {
        village->tick(m_world, gameTime, &m_poiStorage);
    }

    // 定期移除空村庄
    if (gameTime % 1200 == 0) { // 每分钟检查一次
        removeEmptyVillages();
    }

    // 定期更新村庄边界
    if (gameTime % 6000 == 0) { // 每5分钟更新一次
        updateVillageBounds();
    }
}

// ========== 区块回调 ==========

void VillageManager::onChunkLoaded(ChunkCoord x, ChunkCoord z) {
    m_poiStorage.onChunkLoaded(x, z);
}

void VillageManager::onChunkUnloaded(ChunkCoord x, ChunkCoord z) {
    m_poiStorage.onChunkUnloaded(x, z);

    // 更新区块到村庄的映射
    u64 chunkKey = getChunkKey(x, z);
    m_chunkToVillages.erase(chunkKey);
}

// ========== 序列化 ==========

void VillageManager::serialize(nbt::tags::compound_tag& tag) const {
    // 序列化村庄
    auto villagesList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& village : m_villages) {
        nbt::tags::compound_tag villageTag;
        village->serialize(villageTag);
        villagesList->value.push_back(std::move(villageTag));
    }
    tag.value["Villages"] = std::move(villagesList);

    // 序列化POI
    nbt::tags::compound_tag poiTag;
    m_poiStorage.serialize(poiTag);
    tag.value["POI"] = std::make_unique<nbt::tags::compound_tag>(std::move(poiTag));

    tag.put("NextVillageId", static_cast<std::int64_t>(m_nextVillageId));
}

void VillageManager::deserialize(const nbt::tags::compound_tag& tag) {
    // 清空现有数据
    m_villages.clear();
    m_villageById.clear();
    m_villagerToVillage.clear();
    m_chunkToVillages.clear();

    // 反序列化村庄
    auto villagesIt = tag.value.find("Villages");
    if (villagesIt != tag.value.end()) {
        auto* villagesList = dynamic_cast<const nbt::tags::compound_list_tag*>(villagesIt->second.get());
        if (villagesList) {
            for (const auto& villageTag : villagesList->value) {
                auto village = std::make_unique<Village>(Village::deserialize(villageTag));

                VillageId id = m_nextVillageId++;
                Village* ptr = village.get();
                m_villageById[id] = ptr;
                m_villages.push_back(std::move(village));
            }
        }
    }

    // 反序列化POI
    auto poiIt = tag.value.find("POI");
    if (poiIt != tag.value.end()) {
        auto* poiTag = dynamic_cast<const nbt::tags::compound_tag*>(poiIt->second.get());
        if (poiTag) {
            m_poiStorage.deserialize(*poiTag);
        }
    }

    m_nextVillageId = static_cast<VillageId>(tag.get<nbt::tags::long_tag>("NextVillageId"));
}

// ========== 私有方法 ==========

Village* VillageManager::createVillage(BlockPos center) {
    auto village = std::make_unique<Village>(center);

    VillageId id = m_nextVillageId++;
    Village* ptr = village.get();
    m_villageById[id] = ptr;

    // 初始化村庄边界
    village->recalculateBounds(m_poiStorage);

    // 触发回调
    if (m_onVillageCreated) {
        m_onVillageCreated(*village);
    }

    m_villages.push_back(std::move(village));
    return ptr;
}

void VillageManager::removeEmptyVillages() {
    for (auto it = m_villages.begin(); it != m_villages.end(); ) {
        if ((*it)->getPopulation() == 0 && (*it)->getBedCount() == 0) {
            // 移除村庄ID映射
            for (auto& [id, ptr] : m_villageById) {
                if (ptr == it->get()) {
                    m_villageById.erase(id);
                    break;
                }
            }
            it = m_villages.erase(it);
        } else {
            ++it;
        }
    }
}

void VillageManager::updateVillageBounds() {
    for (auto& village : m_villages) {
        village->recalculateBounds(m_poiStorage);
    }
}

Village* VillageManager::checkPlayerEnterVillage(BlockPos playerPos, BlockPos prevPos) {
    // 检查当前位置是否在村庄内
    Village* currentVillage = getVillageAt(playerPos);

    // 如果之前位置无效（默认构造），只检查当前位置
    if (prevPos.x == 0 && prevPos.y == 0 && prevPos.z == 0) {
        return currentVillage;
    }

    // 检查之前位置是否在村庄内
    Village* prevVillage = getVillageAt(prevPos);

    // 如果从村庄外进入村庄内，返回该村庄
    if (currentVillage != nullptr && prevVillage == nullptr) {
        return currentVillage;
    }

    // 如果从一个村庄移动到另一个村庄
    if (currentVillage != nullptr && prevVillage != nullptr && currentVillage != prevVillage) {
        return currentVillage;
    }

    // 没有进入新村庄
    return nullptr;
}

u64 VillageManager::getChunkKey(ChunkCoord x, ChunkCoord z) {
    return (static_cast<u64>(static_cast<u32>(x)) << 32) |
           static_cast<u64>(static_cast<u32>(z));
}

} // namespace village
} // namespace world
} // namespace mc
