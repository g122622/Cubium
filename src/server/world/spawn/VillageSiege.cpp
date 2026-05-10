#include "VillageSiege.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/Village.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc {
namespace server::spawn {

// ============================================================================
// 构造函数
// ============================================================================

VillageSiege::VillageSiege()
    : m_random(0)  // 种子会在使用时设置
{
}

// ============================================================================
// 主要接口
// ============================================================================

i32 VillageSiege::tick(ServerWorld& world, bool spawnHostiles) {
    // 条件1: 必须允许生成敌对生物
    if (!spawnHostiles) {
        return 0;
    }

    // 条件2: 检查是否为夜晚（不是白天）
    // MC 1.16.5: !world.isDaytime()
    const i64 worldDayTime = world.dayTime();
    const bool isDaytime = (worldDayTime % game::DAY_LENGTH_TICKS) < 12000;  // 0-12000 是白天

    if (isDaytime) {
        // 白天重置状态
        m_state = State::Done;
        m_hasSetup = false;
        return 0;
    }

    // 条件3: 检查是否为午夜时刻（天体角度 0.5）
    // MC 1.16.5: celestialAngle == 0.5
    if (isMidnight(world)) {
        // 10% 概率触发今晚围攻
        if (m_random.nextInt(Config::TRIGGER_CHANCE) == 0) {
            m_state = State::Tonight;
            spdlog::debug("VillageSiege: Siege triggered for tonight");
        } else {
            m_state = State::Done;
        }
    }

    // 如果状态不是 Tonight，直接返回
    if (m_state != State::Tonight) {
        return 0;
    }

    // 条件4: 尚未设置围攻
    if (!m_hasSetup) {
        if (!trySetupSiege(world)) {
            return 0;
        }
        m_hasSetup = true;
    }

    // 条件5: 等待生成延迟
    if (m_nextSpawnDelay > 0) {
        --m_nextSpawnDelay;
        return 0;
    }

    // 设置下一次生成延迟
    m_nextSpawnDelay = Config::SPAWN_DELAY;

    // 生成僵尸
    if (m_siegeCount > 0) {
        if (spawnZombie(world)) {
            --m_siegeCount;
            return 1;
        }
    }

    // 围攻结束
    if (m_siegeCount <= 0) {
        m_state = State::Done;
        spdlog::debug("VillageSiege: Siege completed");
    }

    return 0;
}

// ============================================================================
// 内部方法
// ============================================================================

bool VillageSiege::trySetupSiege(ServerWorld& world) {
    // 更新随机种子
    m_random.setSeed(world.dayTime());

    // 获取所有玩家
    const auto& players = world.getPlayers();

    for (const auto* player : players) {
        if (!player || player->isSpectator()) {
            continue;
        }

        const BlockPos playerPos(
            static_cast<i32>(player->getPosition().x),
            static_cast<i32>(player->getPosition().y),
            static_cast<i32>(player->getPosition().z)
        );

        // 条件: 玩家必须在村庄内
        if (!isInValidVillage(world, playerPos)) {
            continue;
        }

        // 条件: 生物群系不能是蘑菇岛（暂不检查，因为生物群系系统尚未完全实现）
        // TODO: 检查生物群系是否为蘑菇岛

        // 尝试在玩家周围找到有效的生成位置
        for (i32 attempt = 0; attempt < Config::MAX_SETUP_ATTEMPTS; ++attempt) {
            // 在玩家周围32格圆周上随机选择方向
            const f32 angle = m_random.nextFloat() * math::TWO_PI;
            const i32 spawnX = playerPos.x + static_cast<i32>(std::cos(angle) * Config::SPAWN_DISTANCE);
            const i32 spawnY = playerPos.y;
            const i32 spawnZ = playerPos.z + static_cast<i32>(std::sin(angle) * Config::SPAWN_DISTANCE);

            const BlockPos searchCenter(spawnX, spawnY, spawnZ);
            auto spawnPos = findRandomSpawnPos(world, searchCenter);

            if (spawnPos.has_value()) {
                m_spawnCenter = spawnPos.value();
                m_nextSpawnDelay = 0;
                m_siegeCount = Config::TOTAL_ZOMBIES;

                spdlog::info("VillageSiege: Setup complete at ({}, {}, {}), {} zombies to spawn",
                    m_spawnCenter.x, m_spawnCenter.y, m_spawnCenter.z, m_siegeCount);

                return true;
            }
        }

        // 找到玩家但未找到有效位置，仍返回 true（已设置）
        return true;
    }

    return false;
}

bool VillageSiege::spawnZombie(ServerWorld& world) {
    auto spawnPos = findRandomSpawnPos(world, m_spawnCenter);

    if (!spawnPos.has_value()) {
        return false;
    }

    // 使用实体注册表创建僵尸
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* zombieType = registry.getType(entity::EntityTypes::ZOMBIE);

    if (!zombieType || !zombieType->canSummon()) {
        spdlog::warn("VillageSiege: Zombie entity type not found or not summonable");
        return false;
    }

    std::unique_ptr<Entity> entity = zombieType->create(&world);
    if (!entity) {
        spdlog::warn("VillageSiege: Failed to create zombie entity");
        return false;
    }

    // 设置位置和朝向
    const f32 posX = static_cast<f32>(spawnPos->x) + 0.5f;
    const f32 posY = static_cast<f32>(spawnPos->y);
    const f32 posZ = static_cast<f32>(spawnPos->z) + 0.5f;
    const f32 yaw = m_random.nextFloat() * 360.0f;

    entity->setPosition(posX, posY, posZ);
    entity->setRotation(yaw, 0.0f);

    // 生成实体到世界
    const EntityId entityId = world.spawnEntity(std::move(entity));

    if (entityId == 0) {
        spdlog::warn("VillageSiege: Failed to spawn zombie entity");
        return false;
    }

    spdlog::debug("VillageSiege: Spawned zombie {} at ({}, {}, {})",
        entityId, spawnPos->x, spawnPos->y, spawnPos->z);

    return true;
}

std::optional<BlockPos> VillageSiege::findRandomSpawnPos(
    IWorld& world,
    const BlockPos& searchCenter) {

    for (i32 attempt = 0; attempt < Config::MAX_SPAWN_ATTEMPTS; ++attempt) {
        // 在搜索中心附近随机偏移
        const i32 x = searchCenter.x + m_random.nextInt(Config::SPAWN_OFFSET_RANGE * 2) - Config::SPAWN_OFFSET_RANGE;
        const i32 z = searchCenter.z + m_random.nextInt(Config::SPAWN_OFFSET_RANGE * 2) - Config::SPAWN_OFFSET_RANGE;

        // 获取地表高度（使用 WorldSurface 高度图）
        // IWorld::getHeight 返回最高非空气方块高度
        const i32 y = world.getHeight(x, z);
        const BlockPos pos(x, y, z);

        // 条件1: 必须在村庄内
        // TODO: 当 VillageManager 可用时检查

        // 条件2: 必须满足怪物生成光照条件
        if (canMonsterSpawnAt(world, pos)) {
            return pos;
        }
    }

    return std::nullopt;
}

bool VillageSiege::canMonsterSpawnAt(IWorld& world, const BlockPos& pos) {
    // 检查位置是否有固体方块作为地面
    const BlockState* groundState = world.getBlockState(pos.x, pos.y - 1, pos.z);
    if (!groundState || !groundState->isSolid()) {
        return false;
    }

    // 检查生成位置是否有足够的碰撞空间（2格高）
    const BlockState* feetState = world.getBlockState(pos.x, pos.y, pos.z);
    if (feetState && !feetState->isAir() && !feetState->isLiquid()) {
        return false;
    }

    const BlockState* headState = world.getBlockState(pos.x, pos.y + 1, pos.z);
    if (headState && !headState->isAir() && !headState->isLiquid()) {
        return false;
    }

    // 检查光照条件（怪物需要在暗处生成）
    // MC 1.16.5: 天空光照 > random.nextInt(32) 则不能生成
    // 方块光照 <= random.nextInt(8) 才能生成
    // 简化实现：检查方块光照 <= 7
    const u8 blockLight = world.getBlockLight(pos);
    const u8 skyLight = world.getSkyLight(pos);

    // 使用随机值检查（参考 MC 的 isValidLightLevel）
    const i32 lightThreshold = m_random.nextInt(8);
    const bool validLightLevel = (blockLight <= static_cast<u8>(lightThreshold));

    // 如果天空光照太亮，也不能生成
    const i32 skyLightThreshold = m_random.nextInt(32);
    const bool skyLightTooBright = (skyLight > static_cast<u8>(skyLightThreshold));

    if (skyLightTooBright && !validLightLevel) {
        return false;
    }

    return true;
}

bool VillageSiege::isMidnight(ServerWorld& world) const {
    // MC 1.16.5: celestialAngle == 0.5 表示午夜
    // 天体角度计算：((dayTime % 24000) / 24000 - 0.25) 经过余弦变换后约为 0.5
    // 简化检查：游戏时间在 18000-18200 范围内视为午夜
    const i64 worldDayTime = world.dayTime() % game::DAY_LENGTH_TICKS;
    return worldDayTime >= 18000 && worldDayTime <= 18200;
}

bool VillageSiege::isInValidVillage(ServerWorld& world, const BlockPos& playerPos) {
    // 获取村庄管理器
    auto* villageManager = world.villageManager();
    if (!villageManager) {
        return false;
    }

    // 检查玩家是否在村庄内
    const Village* village = villageManager->getVillageAt(playerPos);
    if (!village) {
        return false;
    }

    // 检查村庄是否有足够的床位和村民
    // MC 1.16.5 要求：至少 10 张床和 20 个村民
    // 简化实现：只检查村庄是否存在
    (void)village;

    return true;
}

} // namespace server::spawn
} // namespace mc
