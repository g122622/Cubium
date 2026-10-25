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

#include "WanderingTraderSpawner.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/passive/horse/TraderLlamaEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include "server/world/ServerWorld.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::spawn {

namespace {

/// 把服务端世界包装为生成放置检查所需的只读视图。
class TraderSpawnWorldAdapter final : public world::spawn::ISpawnWorldReader {
public:
    explicit TraderSpawnWorldAdapter(server::ServerWorld& world)
        : m_world(world)
    {}

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        return m_world.getBlockState(x, y, z);
    }

    [[nodiscard]] bool isInWorldBounds(i32 x, i32 y, i32 z) const override
    {
        return m_world.isWithinWorldBounds(x, y, z);
    }

    [[nodiscard]] i32 getHeight(HeightmapType type, i32 x, i32 z) const override
    {
        const ChunkData* chunk =
            m_world.chunkManager()->tryToGetChunkInMem(world::toChunkCoord(x), world::toChunkCoord(z));
        return chunk != nullptr ? chunk->getTopBlockY(type, world::toLocalCoord(x), world::toLocalCoord(z)) : -1;
    }

    [[nodiscard]] BiomeId getBiome(i32 x, i32 y, i32 z) const override
    {
        const ChunkData* chunk =
            m_world.chunkManager()->tryToGetChunkInMem(world::toChunkCoord(x), world::toChunkCoord(z));
        return chunk != nullptr ? chunk->getBiomeAtBlock(world::toLocalCoord(x), y, world::toLocalCoord(z))
                                : Biomes::Plains;
    }

    [[nodiscard]] u64 seed() const override
    {
        return m_world.seed();
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return m_world.difficulty();
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return m_world.dayTime();
    }

    [[nodiscard]] i32 getMaxLocalRawBrightness(i32 x, i32 y, i32 z) const override
    {
        return m_world.getMaxLocalRawBrightness(BlockPos(x, y, z));
    }

private:
    server::ServerWorld& m_world;
};

} // namespace

WanderingTraderSpawner::WanderingTraderSpawner()
    : m_tickDelay(TICK_DELAY)
    , m_spawnDelay(DEFAULT_SPAWN_DELAY)
    , m_spawnChance(INITIAL_SPAWN_CHANCE)
{}

void WanderingTraderSpawner::tick(server::ServerWorld& world)
{
    if (!world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::SPAWN_WANDERING_TRADERS)) {
        return;
    }

    if (--m_tickDelay > 0) {
        return;
    }
    m_tickDelay = TICK_DELAY;
    m_spawnDelay -= TICK_DELAY;
    if (m_spawnDelay > 0) {
        return;
    }

    m_spawnDelay = DEFAULT_SPAWN_DELAY;
    const i32 previousChance = m_spawnChance;
    m_spawnChance = std::clamp(m_spawnChance + SPAWN_CHANCE_INCREASE, MIN_SPAWN_CHANCE, MAX_SPAWN_CHANCE);
    if (world.getRandom().nextInt(100) > previousChance) {
        return;
    }
    if (trySpawn(world)) {
        m_spawnChance = MIN_SPAWN_CHANCE;
    }
}

bool WanderingTraderSpawner::trySpawn(server::ServerWorld& world)
{
    // 随机挑选一名玩家作为生成中心；无玩家时视为本轮已处理。
    std::vector<Entity*> players = world.getPlayers();
    if (players.empty()) {
        return true;
    }
    Entity* player = players[static_cast<size_t>(world.getRandom().nextInt(static_cast<i32>(players.size())))];
    if (player == nullptr) {
        return true;
    }

    if (world.getRandom().nextInt(SPAWN_ONE_IN_X_CHANCE) != 0) {
        return false;
    }

    const BlockPos center(math::floorTo<BlockCoord>(player->position().x),
        math::floorTo<BlockCoord>(player->position().y),
        math::floorTo<BlockCoord>(player->position().z));

    const std::optional<BlockPos> spawnPos = _findSpawnPositionNear(world, center, SPAWN_RADIUS);
    if (!spawnPos.has_value() || !_hasEnoughSpace(world, *spawnPos)) {
        return false;
    }

    // 群系禁止出生时放弃（如蘑菇岛等 #without_wandering_trader_spawns）。
    const ChunkData* spawnChunk = world.chunkManager()->tryToGetChunkInMem(spawnPos->chunkX(), spawnPos->chunkZ());
    if (spawnChunk != nullptr) {
        const BiomeId biomeId = spawnChunk->getBiomeAtBlock(spawnPos->localX(), spawnPos->y, spawnPos->localZ());
        if (world::biome::BiomeTags::WITHOUT_WANDERING_TRADER_SPAWNS().contains(biomeId)) {
            return false;
        }
    }

    const entity::EntityType* traderType = entity::VanillaEntityTypeKeys::WANDERING_TRADER;
    if (traderType == nullptr) {
        spdlog::warn("WanderingTraderSpawner: wandering trader entity type is not registered");
        return false;
    }

    std::unique_ptr<Entity> traderEntity = traderType->create(&world, *world.entityRegistry());
    if (traderEntity == nullptr) {
        return false;
    }
    traderEntity->setPosition(
        static_cast<f32>(spawnPos->x), static_cast<f32>(spawnPos->y), static_cast<f32>(spawnPos->z));
    auto* trader = dynamic_cast<entity::WanderingTraderEntity*>(traderEntity.get());
    if (trader != nullptr) {
        trader->setDespawnDelay(DESPAWN_DELAY);
        trader->setWanderTarget(center);
    }

    const EntityInstanceId traderId = world.spawnEntity(std::move(traderEntity));
    Entity* spawnedTrader = world.getEntity(traderId);
    if (spawnedTrader == nullptr) {
        return false;
    }

    // 生成 2 只拴绳贸易羊驼：对齐 vanilla tryToSpawnLlamaFor，逐只独立选址，失败则少生成。
    if (dynamic_cast<entity::WanderingTraderEntity*>(spawnedTrader) != nullptr) {
        for (i32 i = 0; i < LLAMA_COUNT; ++i) {
            const BlockPos traderPos(math::floorTo<BlockCoord>(spawnedTrader->position().x),
                math::floorTo<BlockCoord>(spawnedTrader->position().y),
                math::floorTo<BlockCoord>(spawnedTrader->position().z));
            const std::optional<BlockPos> llamaPos = _findSpawnPositionNear(world, traderPos, LLAMA_SPAWN_RADIUS);
            if (!llamaPos.has_value()) {
                continue;
            }
            auto llama = std::make_unique<TraderLlamaEntity>(0, *world.entityRegistry());
            llama->setTypeId(entity::EntityTypeKeys::TRADER_LLAMA);
            llama->setPosition(
                static_cast<f32>(llamaPos->x), static_cast<f32>(llamaPos->y), static_cast<f32>(llamaPos->z));
            const EntityInstanceId llamaId = world.spawnEntity(std::move(llama));
            Entity* spawnedLlama = world.getEntity(llamaId);
            if (auto* traderLlama = dynamic_cast<TraderLlamaEntity*>(spawnedLlama); traderLlama != nullptr) {
                traderLlama->setLeashedToEntity(spawnedTrader->uuid());
            }
        }
    }

    spdlog::info(
        "WanderingTraderSpawner: spawned wandering trader at ({}, {}, {})", spawnPos->x, spawnPos->y, spawnPos->z);
    return true;
}

std::optional<BlockPos> WanderingTraderSpawner::_findSpawnPositionNear(
    server::ServerWorld& world, const BlockPos& center, i32 radius)
{
    TraderSpawnWorldAdapter adapter(world);
    for (i32 attempt = 0; attempt < SPAWN_ATTEMPTS; ++attempt) {
        const i32 x = center.x + world.getRandom().nextInt(radius * 2) - radius;
        const i32 z = center.z + world.getRandom().nextInt(radius * 2) - radius;
        const i32 y = world.getHeight(x, z);
        const BlockPos candidate(x, y, z);
        if (world::spawn::EntitySpawnPlacementRegistry::canSpawnAtLocation(
                world::spawn::PlacementType::OnGround, adapter, Vector3i(x, y, z), "minecraft:wandering_trader")) {
            return candidate;
        }
    }
    return std::nullopt;
}

bool WanderingTraderSpawner::_hasEnoughSpace(IWorld& world, const BlockPos& pos)
{
    // 商人占位区域：pos 到 pos + (1, 2, 1) 的 2x3x2 空间必须无碰撞。
    for (i32 dx = 0; dx <= 1; ++dx) {
        for (i32 dy = 0; dy <= 2; ++dy) {
            for (i32 dz = 0; dz <= 1; ++dz) {
                const BlockState* state = world.getBlockState(pos.x + dx, pos.y + dy, pos.z + dz);
                if (state != nullptr && !state->getCollisionShape().isEmpty()) {
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace mc::server::spawn
