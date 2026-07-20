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

#include "VillageSiege.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "common/world/village/Village.hpp"
#include "common/world/village/VillageManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc {
namespace server::spawn {

// ============================================================================
// 构造函数
// ============================================================================

VillageSiege::VillageSiege()
    : m_random(0) // 种子会在使用时设置
{}

// ============================================================================
// 主要接口
// ============================================================================

i32 VillageSiege::tick(server::ServerWorld& world, bool spawnHostiles)
{
    // 条件1: 必须允许生成敌对生物
    if (!spawnHostiles) {
        return 0;
    }

    // 条件2: 检查游戏难度（非和平模式才能发生僵尸围攻）
    if (!entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        return 0;
    }

    // 条件3: 检查是否为夜晚（不是白天）
    const i64 worldDayTime = world.dayTimeOfDay();
    constexpr i64 DAYTIME_END = 12000; // 白天结束时间（ticks）
    const bool isDaytime = worldDayTime < DAYTIME_END;

    if (isDaytime) {
        // 白天重置状态
        m_state = State::Done;
        m_hasSetup = false;
        return 0;
    }

    // 条件4: 检查是否为午夜时刻（天体角度 0.5）
    if (_isMidnight(world)) {
        // 10% 概率触发今晚围攻
        if (m_random.nextInt(Config::TRIGGER_CHANCE) == 0) {
            m_state = State::Tonight;
        } else {
            m_state = State::Done;
        }
    }

    // 如果状态不是 Tonight，直接返回
    if (m_state != State::Tonight) {
        return 0;
    }

    // 条件5: 尚未设置围攻
    if (!m_hasSetup) {
        if (!_trySetupSiege(world)) {
            return 0;
        }
        m_hasSetup = true;
    }

    // 条件6: 等待生成延迟
    if (m_nextSpawnDelay > 0) {
        --m_nextSpawnDelay;
        return 0;
    }

    // 设置下一次生成延迟
    m_nextSpawnDelay = Config::SPAWN_DELAY;

    // 生成僵尸
    if (m_siegeCount > 0) {
        if (_spawnZombie(world)) {
            --m_siegeCount;
            return 1;
        }
    }

    // 围攻结束
    if (m_siegeCount <= 0) {
        m_state = State::Done;
    }

    return 0;
}

// ============================================================================
// 内部方法
// ============================================================================

bool VillageSiege::_trySetupSiege(server::ServerWorld& world)
{
    // 更新随机种子 - 使用累积的 dayTime 作为种子
    m_random.setSeed(world.dayTime());

    // 获取所有玩家
    const auto& players = world.getPlayers();

    for (const auto* entity : players) {
        if (!entity) {
            continue;
        }

        // 只处理玩家实体
        const auto* player = dynamic_cast<const Player*>(entity);
        if (!player || player->isSpectator()) {
            continue;
        }

        const BlockPos playerPos(static_cast<i32>(player->position().x),
            static_cast<i32>(player->position().y),
            static_cast<i32>(player->position().z));

        // 条件: 玩家必须在村庄内
        if (!_isInValidVillage(world, playerPos)) {
            continue;
        }

        // 条件: 生物群系不能是蘑菇岛
        // 蘑菇岛是安全区域，不会发生僵尸围攻
        if (_isMushroomBiome(world, playerPos)) {
            continue;
        }

        // 尝试在玩家周围找到有效的生成位置
        for (i32 attempt = 0; attempt < Config::MAX_SETUP_ATTEMPTS; ++attempt) {
            // 在玩家周围32格圆周上随机选择方向
            const f32 angle = m_random.nextFloat() * math::TWO_PI;
            const i32 spawnX = playerPos.x + static_cast<i32>(std::cos(angle) * Config::SPAWN_DISTANCE);
            const i32 spawnY = playerPos.y;
            const i32 spawnZ = playerPos.z + static_cast<i32>(std::sin(angle) * Config::SPAWN_DISTANCE);

            const BlockPos searchCenter(spawnX, spawnY, spawnZ);
            auto spawnPos = _findRandomSpawnPos(world, searchCenter);

            if (spawnPos.has_value()) {
                m_spawnCenter = spawnPos.value();
                m_nextSpawnDelay = 0;
                m_siegeCount = Config::TOTAL_ZOMBIES;

                spdlog::info("VillageSiege: Setup complete at ({}, {}, {}), {} zombies to spawn",
                    m_spawnCenter.x,
                    m_spawnCenter.y,
                    m_spawnCenter.z,
                    m_siegeCount);

                return true;
            }
        }

        // 找到符合条件的玩家（在村庄内且不在蘑菇岛），即使未找到生成位置也返回 true
        // 这样围攻状态会被设置为 hasSetupSiege = true，但 siegeCount = 0（因为未找到位置）
        // 实际效果是围攻立即结束（因为 siegeCount <= 0）
        return true;
    }

    return false;
}

bool VillageSiege::_spawnZombie(server::ServerWorld& world)
{
    auto spawnPos = _findRandomSpawnPos(world, m_spawnCenter);

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

    // 调用 finalizeSpawn 进行完整初始化
    // 包括基于难度的装备设置、破门能力、附魔等
    auto* mobEntity = dynamic_cast<MobEntity*>(entity.get());
    if (mobEntity != nullptr) {
        // 创建区域难度实例
        entity::combat::DifficultyInstance difficultyInstance =
            entity::combat::DifficultyInstance::at(world, *spawnPos);
        // 调用 finalizeSpawn（由 MobEntity 提供，ZombieEntity 重写以添加僵尸特有逻辑）
        mobEntity->finalizeSpawn(world, difficultyInstance, world::spawn::SpawnReason::Event);
    }

    // 生成实体到世界
    const EntityId entityId = world.spawnEntity(std::move(entity));

    if (entityId == 0) {
        spdlog::warn("VillageSiege: Failed to spawn zombie entity");
        return false;
    }

    return true;
}

std::optional<BlockPos> VillageSiege::_findRandomSpawnPos(IWorld& world, const BlockPos& searchCenter)
{
    for (i32 attempt = 0; attempt < Config::MAX_SPAWN_ATTEMPTS; ++attempt) {
        // 在搜索中心附近随机偏移
        const i32 x = searchCenter.x + m_random.nextInt(Config::SPAWN_OFFSET_RANGE * 2) - Config::SPAWN_OFFSET_RANGE;
        const i32 z = searchCenter.z + m_random.nextInt(Config::SPAWN_OFFSET_RANGE * 2) - Config::SPAWN_OFFSET_RANGE;

        // 获取地表高度（使用 WorldSurface 高度图）
        const i32 y = world.getHeight(x, z);
        const BlockPos pos(x, y, z);

        // 检查是否在村庄内
        auto* villageManager = world.villageManager();
        if (villageManager == nullptr || villageManager->getVillageAt(pos) == nullptr) {
            continue; // 不在村庄内，跳过此位置
        }

        // 检查生成位置是否满足怪物生成条件
        if (_canMonsterSpawnAt(world, pos)) {
            return pos;
        }
    }

    return std::nullopt;
}

bool VillageSiege::_canMonsterSpawnAt(IWorld& world, const BlockPos& pos)
{
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

    // 检查光照条件
    return MonsterEntity::isValidLightLevel(world, pos, m_random);
}

bool VillageSiege::_isMidnight(server::ServerWorld& world) const
{
    // 午夜时刻：天体角度 0.5 对应 dayTime == 18000
    // 天体角度计算公式：
    // d0 = frac(dayTime/24000 - 0.25) = frac(18000/24000 - 0.25) = frac(0.5) = 0.5
    // d1 = 0.5 - cos(0.5 * PI) / 2 = 0.5
    // result = (0.5 * 2 + 0.5) / 3 = 0.5
    //
    // 由于浮点精度问题，直接比较 celestialAngle == 0.5 可能不够稳定，
    // 而 dayTime 是整数，精确检查 dayTime == 18000 更可靠。
    constexpr i64 MIDNIGHT_DAYTIME = 18000;
    const i64 worldDayTime = world.dayTimeOfDay();
    return worldDayTime == MIDNIGHT_DAYTIME;
}

bool VillageSiege::_isInValidVillage(server::ServerWorld& world, const BlockPos& playerPos)
{
    // 获取村庄管理器
    auto* villageManager = world.villageManager();
    if (!villageManager) {
        return false;
    }

    // 检查玩家是否在村庄内
    const world::village::Village* village = villageManager->getVillageAt(playerPos);
    if (!village) {
        return false;
    }

    // 检查村庄是否有床位和村民
    // 空村庄不应该触发僵尸围攻
    const i32 bedCount = village->getBedCount();
    const i32 population = village->getPopulation();

    // 至少需要 1 张床和 1 个村民才算是有效村庄
    if (bedCount <= 0 || population <= 0) {
        return false;
    }

    return true;
}

bool VillageSiege::_isMushroomBiome(server::ServerWorld& world, const BlockPos& pos)
{
    // 蘑菇岛生物群系不会发生僵尸围攻

    // 获取玩家所在区块
    const ChunkCoord chunkX = pos.x >> world::CHUNK_SHIFT;
    const ChunkCoord chunkZ = pos.z >> world::CHUNK_SHIFT;
    const ChunkData* chunk = world.getChunk(chunkX, chunkZ);

    if (!chunk) {
        // 区块未加载，保守返回 false（不是蘑菇岛）
        return false;
    }

    // 获取玩家位置的生物群系
    const BiomeId biomeId = chunk->getBiomeAtBlock(pos.x, pos.y, pos.z);

    // 检查是否为蘑菇岛生物群系（蘑菇岛不会发生僵尸围攻）
    if (biomeId == Biomes::MushroomFields || biomeId == Biomes::MushroomFieldShore) {
        return true;
    }

    return false;
}

} // namespace server::spawn
} // namespace mc
