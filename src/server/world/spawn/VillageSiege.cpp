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
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/ChunkData.hpp"
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
    // MC 1.16.5: MonsterEntity.canMonsterSpawnInLight() 中检查难度
    if (!entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        return 0;
    }

    // 条件3: 检查是否为夜晚（不是白天）
    // MC 1.16.5: !world.isDaytime()
    const i64 worldDayTime = world.dayTime();
    const bool isDaytime = (worldDayTime % game::DAY_LENGTH_TICKS) < 12000; // 0-12000 是白天

    if (isDaytime) {
        // 白天重置状态
        m_state = State::Done;
        m_hasSetup = false;
        return 0;
    }

    // 条件4: 检查是否为午夜时刻（天体角度 0.5）
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

    // 条件5: 尚未设置围攻
    if (!m_hasSetup) {
        if (!trySetupSiege(world)) {
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

bool VillageSiege::trySetupSiege(server::ServerWorld& world)
{
    // 更新随机种子
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
        if (!isInValidVillage(world, playerPos)) {
            continue;
        }

        // 条件: 生物群系不能是蘑菇岛
        // MC 1.16.5: getBiome(blockpos).getCategory() != Biome.Category.MUSHROOM
        // 蘑菇岛是安全区域，不会发生僵尸围攻
        if (isMushroomBiome(world, playerPos)) {
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
            auto spawnPos = findRandomSpawnPos(world, searchCenter);

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

        // 找到玩家但未找到有效位置，仍返回 true（已设置）
        return true;
    }

    return false;
}

bool VillageSiege::spawnZombie(server::ServerWorld& world)
{
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

    spdlog::debug("VillageSiege: Spawned zombie {} at ({}, {}, {})", entityId, spawnPos->x, spawnPos->y, spawnPos->z);

    return true;
}

std::optional<BlockPos> VillageSiege::findRandomSpawnPos(IWorld& world, const BlockPos& searchCenter)
{

    for (i32 attempt = 0; attempt < Config::MAX_SPAWN_ATTEMPTS; ++attempt) {
        // 在搜索中心附近随机偏移
        const i32 x = searchCenter.x + m_random.nextInt(Config::SPAWN_OFFSET_RANGE * 2) - Config::SPAWN_OFFSET_RANGE;
        const i32 z = searchCenter.z + m_random.nextInt(Config::SPAWN_OFFSET_RANGE * 2) - Config::SPAWN_OFFSET_RANGE;

        // 获取地表高度（使用 WorldSurface 高度图）
        // IWorld::getHeight 返回最高非空气方块高度
        const i32 y = world.getHeight(x, z);
        const BlockPos pos(x, y, z);

        // 条件1: 生成位置必须在村庄内
        // 由于玩家已在村庄内，且围攻中心围绕玩家设置，因此生成位置通常也在村庄范围内

        // 条件2: 必须满足怪物生成光照条件
        if (canMonsterSpawnAt(world, pos)) {
            return pos;
        }
    }

    return std::nullopt;
}

bool VillageSiege::canMonsterSpawnAt(IWorld& world, const BlockPos& pos)
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

    // MC 1.16.5: 使用 MonsterEntity.isValidLightLevel() 检查光照条件
    // 这是完整的实现，包括天空光照和方块光照的随机阈值检查
    return MonsterEntity::isValidLightLevel(world, pos, m_random);
}

bool VillageSiege::isMidnight(server::ServerWorld& world) const
{
    // MC 1.16.5: celestialAngle == 0.5 表示午夜
    // 参考: VillageSiege.func_230253_a_ 第 33-34 行:
    //   float f = world.func_242415_f(0.0F);  // getCelestialAngle
    //   if ((double)f == 0.5D) { ... }
    //
    // 天体角度计算公式来自 DimensionType.func_236032_b_():
    // 当 dayTime = 18000 时，精确得到 celestialAngle = 0.5
    //
    // 数学推导：
    // d0 = frac(18000/24000 - 0.25) = frac(0.75 - 0.25) = frac(0.5) = 0.5
    // d1 = 0.5 - cos(0.5 * PI) / 2 = 0.5 - cos(90°) / 2 = 0.5 - 0 = 0.5
    // result = (0.5 * 2 + 0.5) / 3 = 1.5 / 3 = 0.5
    //
    // 由于浮点精度问题，直接比较 celestialAngle == 0.5 可能不够稳定，
    // 而 dayTime 是整数，精确检查 dayTime == 18000 更可靠。
    const i64 worldDayTime = world.dayTime() % game::DAY_LENGTH_TICKS;
    return worldDayTime == 18000;
}

bool VillageSiege::isInValidVillage(server::ServerWorld& world, const BlockPos& playerPos)
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

    // MC 1.16.5: VillageSiege.trySetupSiege() 中使用 isVillage() 方法检查
    // 项目的 isVillage() 等价于 VillageManager::getVillageAt() 返回非空
    //
    // 但 MC 原版的村庄定义要求：
    // - 至少有床位（POI 系统 sectionsToVillage 返回距离 <= 6）
    // - 村庄围攻会生成僵尸攻击村民，所以应该有村民存在
    //
    // 项目中使用 Village 类的床位和村民数量来近似判断有效村庄
    // 参考 MC 1.16.5 中村民繁殖的阈值：至少 2 张床才能开始繁殖
    // 村庄围攻需要有意义的村庄目标，这里要求至少 1 张床和至少 1 个村民

    // 检查村庄是否有床位和村民
    // 注意：MC 原版没有这个检查，但这是合理的补充
    // 空村庄不应该触发僵尸围攻
    const i32 bedCount = village->getBedCount();
    const i32 population = village->getPopulation();

    // 至少需要 1 张床和 1 个村民才算是有效村庄
    // 这与项目的 POI 系统和村庄定义一致
    if (bedCount <= 0 || population <= 0) {
        return false;
    }

    return true;
}

bool VillageSiege::isMushroomBiome(server::ServerWorld& world, const BlockPos& pos)
{
    // MC 1.16.5: world.getBiome(blockpos).getCategory() != Biome.Category.MUSHROOM
    // 蘑菇岛生物群系不会发生僵尸围攻

    // 获取玩家所在区块
    const ChunkCoord chunkX = pos.x >> 4;
    const ChunkCoord chunkZ = pos.z >> 4;
    const ChunkData* chunk = world.getChunk(chunkX, chunkZ);

    if (!chunk) {
        // 区块未加载，保守返回 false（不是蘑菇岛）
        return false;
    }

    // 获取玩家位置的生物群系
    const BiomeId biomeId = chunk->getBiomeAtBlock(pos.x, pos.y, pos.z);

    // 检查生物群系类别是否为蘑菇岛
    // 方法1：通过 BiomeRegistry 获取生物群系定义并检查类别
    if (BiomeRegistry::instance().hasBiome(biomeId)) {
        const Biome& biome = BiomeRegistry::instance().get(biomeId);
        if (biome.category() == Biome::Category::Mushroom) {
            return true;
        }
    }

    // 方法2：直接检查蘑菇岛生物群系 ID（作为备用检查）
    // 蘑菇岛 ID: 14 (MushroomFields), 15 (MushroomFieldShore)
    if (biomeId == Biomes::MushroomFields || biomeId == Biomes::MushroomFieldShore) {
        return true;
    }

    return false;
}

} // namespace server::spawn
} // namespace mc
