#include "RaidManager.hpp"
#include "Raid.hpp"
#include "../Village.hpp"
#include "../VillageManager.hpp"
#include "../../IWorld.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/effect/EffectInstance.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include <algorithm>

namespace mc {
namespace world {
namespace village {
namespace raid {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 袭击触发距离
constexpr f32 RAID_TRIGGER_DISTANCE = 64.0f;

} // namespace

// ============================================================================
// RaidManager 实现
// ============================================================================

RaidManager::RaidManager(IWorld& world, village::VillageManager& villageManager)
    : m_world(world)
    , m_villageManager(villageManager)
{
}

bool RaidManager::isWithinRaidRange(BlockPos pos, BlockPos center) const {
    f32 distSq = static_cast<f32>(
        (pos.x - center.x) * (pos.x - center.x) +
        (pos.z - center.z) * (pos.z - center.z)
    );
    return distSq <= RAID_TRIGGER_DISTANCE * RAID_TRIGGER_DISTANCE;
}

Raid* RaidManager::getRaidAt(BlockPos pos) {
    for (auto& raid : m_raids) {
        if (raid && raid->status() == RaidStatus::Ongoing) {
            if (isWithinRaidRange(pos, raid->center())) {
                return raid.get();
            }
        }
    }
    return nullptr;
}

const Raid* RaidManager::getRaidAt(BlockPos pos) const {
    for (const auto& raid : m_raids) {
        if (raid && raid->status() == RaidStatus::Ongoing) {
            if (isWithinRaidRange(pos, raid->center())) {
                return raid.get();
            }
        }
    }
    return nullptr;
}

bool RaidManager::hasRaidAt(BlockPos pos) const {
    return getRaidAt(pos) != nullptr;
}

Raid* RaidManager::getRaidForVillage(village::Village* village) {
    if (village == nullptr) return nullptr;

    for (auto& raid : m_raids) {
        if (raid && raid->village() == village) {
            return raid.get();
        }
    }
    return nullptr;
}

size_t RaidManager::getActiveRaidCount() const {
    size_t count = 0;
    for (const auto& raid : m_raids) {
        if (raid && raid->status() == RaidStatus::Ongoing) {
            ++count;
        }
    }
    return count;
}

Raid* RaidManager::tryStartRaid(BlockPos pos, i32 badOmenLevel) {
    // 检查是否可以开始袭击
    if (!canStartRaidAt(pos)) {
        return nullptr;
    }

    // 查找附近的村庄
    Village* village = findNearbyVillage(pos);
    if (village == nullptr) {
        return nullptr;
    }

    // 检查村庄是否已有袭击
    if (getRaidForVillage(village) != nullptr) {
        return nullptr;
    }

    // 创建新袭击
    RaidId id = generateRaidId();
    auto raid = std::make_unique<Raid>(id, village);
    raid->setBadOmenLevel(badOmenLevel);
    raid->setCenter(pos);

    Raid* raidPtr = raid.get();
    m_raids.push_back(std::move(raid));

    // 标记村庄处于袭击中
    village->setUnderRaid(true);

    // 触发袭击开始回调
    if (m_callbacks.onRaidStarted) {
        m_callbacks.onRaidStarted(*raidPtr, village->getCenter());
    }

    return raidPtr;
}

void RaidManager::onPlayerEnterVillage(Player* player, village::Village* village) {
    if (player == nullptr || village == nullptr) return;

    // 检查玩家是否有不祥之兆效果
    // Player 类已添加独立的效果管理接口
    if (player->hasEffect(entity::effect::EffectType::BadOmen)) {
        const entity::effect::EffectInstance* effect = player->getEffect(entity::effect::EffectType::BadOmen);
        if (effect != nullptr) {
            i32 badOmenLevel = effect->getEffectLevel();

            // 移除玩家的不祥之兆效果
            player->removeEffect(entity::effect::EffectType::BadOmen);

            // 尝试开始袭击
            Raid* raid = tryStartRaid(village->getCenter(), badOmenLevel);
            if (raid != nullptr) {
                // 袭击已开始
                // TODO: 发送袭击开始通知
            }
        }
    }
}

void RaidManager::onPlayerEnterVillageWithCallback(const BadOmenCheckCallback& checkBadOmen, village::Village* village) {
    if (!checkBadOmen || village == nullptr) return;

    // 使用回调检查不祥之兆
    i32 badOmenLevel = checkBadOmen(village->getCenter());
    if (badOmenLevel > 0) {
        // 尝试开始袭击
        Raid* raid = tryStartRaid(village->getCenter(), badOmenLevel);
        if (raid != nullptr) {
            // 袭击已开始
            // TODO: 发送袭击开始通知
        }
    }
}

void RaidManager::tick() {
    // 更新所有袭击
    for (auto& raid : m_raids) {
        if (raid) {
            raid->tick(m_world);
        }
    }

    // 移除已完成的袭击
    removeCompletedRaids();
}

void RaidManager::onRaidEnd(Raid* raid) {
    if (raid == nullptr) return;

    // 清除村庄的袭击状态
    village::Village* village = raid->village();
    if (village != nullptr) {
        village->setUnderRaid(false);
        village->setLastRaidTime(static_cast<i64>(m_world.currentTick()));
    }

    // 根据袭击结果执行不同逻辑
    switch (raid->status()) {
        case RaidStatus::Victory:
            // 玩家胜利，给予英雄效果
            if (m_callbacks.onRaidVictory) {
                // 收集英雄 UUID 列表
                std::vector<Uuid> heroUuids;
                for (const auto& uuid : raid->heroes()) {
                    heroUuids.push_back(uuid);
                }
                m_callbacks.onRaidVictory(*raid, heroUuids, raid->badOmenLevel());
            }
            break;
        case RaidStatus::Loss:
            // 掠夺者胜利，村庄可能被摧毁
            if (m_callbacks.onRaidLoss) {
                m_callbacks.onRaidLoss(*raid);
            }
            break;
        case RaidStatus::Stopped:
            // 袭击被停止
            break;
        default:
            break;
    }
}

void RaidManager::removeCompletedRaids() {
    // 收集已完成的袭击
    std::vector<Raid*> completedRaids;
    for (auto& raid : m_raids) {
        if (raid && raid->status() != RaidStatus::Ongoing) {
            completedRaids.push_back(raid.get());
        }
    }

    // 通知袭击结束
    for (Raid* raid : completedRaids) {
        onRaidEnd(raid);
    }

    // 移除已完成的袭击
    m_raids.erase(
        std::remove_if(m_raids.begin(), m_raids.end(),
            [](const std::unique_ptr<Raid>& raid) {
                return raid && raid->status() != RaidStatus::Ongoing;
            }),
        m_raids.end()
    );
}

RaidId RaidManager::generateRaidId() {
    return m_nextRaidId++;
}

village::Village* RaidManager::findNearbyVillage(BlockPos pos) const {
    // 使用村庄管理器查找村庄
    return m_villageManager.getVillageAt(pos);
}

bool RaidManager::canStartRaidAt(BlockPos pos) const {
    // 检查位置是否在世界边界内
    if (!m_world.isWithinWorldBounds(pos)) {
        return false;
    }

    // 检查是否已有袭击
    if (hasRaidAt(pos)) {
        return false;
    }

    return true;
}

} // namespace raid
} // namespace village
} // namespace world
} // namespace mc
