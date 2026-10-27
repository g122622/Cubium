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

#include "PlayerSpawnFinder.hpp"

#include "common/entity/entities/player/GameModeUtils.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "server/application/IServer.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace mc::server {

namespace {

/// 玩家碰撞箱尺寸（对齐原版 EntityType.PLAYER 的 0.6 x 1.8）
constexpr f32 PLAYER_WIDTH = 0.6f;
constexpr f32 PLAYER_HEIGHT = 1.8f;

constexpr i32 RADIUS_MULTIPLIER = 2;
constexpr i32 STEP_COPRIME_THRESHOLD = 16;
constexpr i32 STEP_COPRIME_LARGE = 17;
constexpr i32 BORDER_MIN_RADIUS = 1;

/// 玩家碰撞箱底面中心在方块柱 pos 上的位置
[[nodiscard]] Vector3 _bottomCenter(const BlockPos& pos)
{
    return Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f);
}

} // namespace

i32 PlayerSpawnFinder::candidateCount(i32 radius) noexcept
{
    const i64 side = static_cast<i64>(radius) * RADIUS_MULTIPLIER + 1;
    return static_cast<i32>(std::min<i64>(ABSOLUTE_MAX_ATTEMPTS, side * side));
}

i32 PlayerSpawnFinder::coprime(i32 count) noexcept
{
    return count <= STEP_COPRIME_THRESHOLD ? count - 1 : STEP_COPRIME_LARGE;
}

bool PlayerSpawnFinder::noCollisionNoLiquid(ServerWorld& world, const BlockPos& pos)
{
    const AxisAlignedBB box = AxisAlignedBB::fromPosition(_bottomCenter(pos), PLAYER_WIDTH, PLAYER_HEIGHT);
    return !world.hasBlockCollision(box) && !world.containsAnyLiquid(box);
}

std::optional<Vector3> PlayerSpawnFinder::fixupSpawnHeight(ServerWorld& world, const BlockPos& suggestedPos)
{
    const i32 maxY = world.getMaxBuildHeight();
    const i32 minY = world.getMinBuildHeight();
    i32 y = suggestedPos.y;
    while (!noCollisionNoLiquid(world, BlockPos(suggestedPos.x, y, suggestedPos.z)) && y < maxY) {
        ++y;
    }
    --y;
    while (noCollisionNoLiquid(world, BlockPos(suggestedPos.x, y, suggestedPos.z)) && y > minY) {
        --y;
    }
    ++y;
    return _bottomCenter(BlockPos(suggestedPos.x, y, suggestedPos.z));
}

std::optional<Vector3> PlayerSpawnFinder::findSpawn(ServerWorld& world, const BlockPos& suggestedPos)
{
    // 冒险模式世界不做半径搜索（只做高度修正）。
    const bool isAdventureMode =
        world.server() != nullptr && entity::GameModeUtils::isAdventure(world.server()->defaultGameMode());
    if (!world.hasSkyLight() || isAdventureMode) {
        return fixupSpawnHeight(world, suggestedPos);
    }

    i32 radius = std::max(0, world.getGameRules().getInt(world::gamerule::GameRuleKeys::RESPAWN_RADIUS));

    // 半径不得超出世界边界：取建议位置到最近边界的距离，边界距离不足 1 时强制为 1。
    const i32 borderDistance =
        static_cast<i32>(std::floor(world.worldBorder().getClosestDistance(suggestedPos.x, suggestedPos.z)));
    if (borderDistance < radius) {
        radius = borderDistance;
    }
    if (borderDistance <= BORDER_MIN_RADIUS) {
        radius = BORDER_MIN_RADIUS;
    }

    const i32 count = candidateCount(radius);
    const i32 step = coprime(count);
    const i32 side = radius * RADIUS_MULTIPLIER + 1;

    // 候选起点按维度与建议位置派生，保证同一建议位置的搜索结果稳定可复现；
    // 原版使用无种子随机源，Cubium 有意不引入不可复现性（存档重载后落点应一致）。
    const i64 randomSeed = static_cast<i64>(suggestedPos.x) * 341873128712LL ^
        static_cast<i64>(suggestedPos.z) * 132897987541LL ^ static_cast<i64>(world.dimension());
    mc::math::Random random(randomSeed);
    const i32 offset = random.nextInt(count);

    for (i32 i = 0; i < count; ++i) {
        const i32 index = (offset + step * i) % count;
        const i32 k = index % side;
        const i32 l = index / side;
        const i32 candidateX = suggestedPos.x + k - radius;
        const i32 candidateZ = suggestedPos.z + l - radius;

        const ChunkCoord chunkX = static_cast<ChunkCoord>(candidateX >> world::CHUNK_SHIFT);
        const ChunkCoord chunkZ = static_cast<ChunkCoord>(candidateZ >> world::CHUNK_SHIFT);
        if (world.chunkManager()->requestFullChunkSync(chunkX, chunkZ) == nullptr) {
            continue;
        }

        // getHeight 返回首个可达空气格（原版 getHeightmapPos(MOTION_BLOCKING) 语义）。
        const i32 topY = world.getHeight(candidateX, candidateZ);
        const BlockPos candidate(candidateX, topY, candidateZ);
        if (noCollisionNoLiquid(world, candidate)) {
            return _bottomCenter(candidate);
        }
    }

    // 全部候选失败：回退到建议列的高度修正。
    return fixupSpawnHeight(world, suggestedPos);
}

} // namespace mc::server
