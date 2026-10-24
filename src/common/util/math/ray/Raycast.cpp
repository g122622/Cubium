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

#include "Raycast.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace mc {

namespace {

/**
 * @brief 计算浮点数的小数部分
 */
inline f32 fract(f32 x)
{
    return x - std::floor(x);
}

/**
 * @brief 获取符号
 */
inline i32 signum(f32 x)
{
    if (x > 0.0f) {
        return 1;
    }
    if (x < 0.0f) {
        return -1;
    }
    return 0;
}

struct RayAabbHit {
    f32 t = 0.0f;
    Direction face = Direction::None;
};

[[nodiscard]] std::optional<RayAabbHit> intersectSegmentAabb(
    const Vector3& origin, const Vector3& delta, const AxisAlignedBB& box)
{
    constexpr f32 EPSILON = 1.0e-7f;

    f32 tMin = 0.0f;
    f32 tMax = 1.0f;
    Direction enterFace = Direction::None;

    const auto updateAxis =
        [&](f32 axisOrigin, f32 axisDelta, f32 axisMin, f32 axisMax, Direction minFace, Direction maxFace) -> bool {
        if (std::abs(axisDelta) < EPSILON) {
            return axisOrigin >= axisMin && axisOrigin <= axisMax;
        }

        f32 t1 = (axisMin - axisOrigin) / axisDelta;
        f32 t2 = (axisMax - axisOrigin) / axisDelta;
        Direction nearFace = minFace;
        Direction farFace = maxFace;

        if (t1 > t2) {
            std::swap(t1, t2);
            std::swap(nearFace, farFace);
        }

        if (t1 > tMin) {
            tMin = t1;
            enterFace = nearFace;
        }

        tMax = std::min(tMax, t2);
        return tMin <= tMax;
    };

    if (!updateAxis(origin.x, delta.x, box.minX, box.maxX, Direction::West, Direction::East)) {
        return std::nullopt;
    }
    if (!updateAxis(origin.y, delta.y, box.minY, box.maxY, Direction::Down, Direction::Up)) {
        return std::nullopt;
    }
    if (!updateAxis(origin.z, delta.z, box.minZ, box.maxZ, Direction::North, Direction::South)) {
        return std::nullopt;
    }

    if (tMax < 0.0f || tMin > 1.0f) {
        return std::nullopt;
    }

    RayAabbHit hit;
    hit.t = std::clamp(tMin, 0.0f, 1.0f);
    if (tMin < 0.0f) {
        // 射线起点位于包围盒内部时，沿射线反方向给出命中面。
        hit.face = Directions::fromVector(-delta.x, -delta.y, -delta.z);
    } else {
        hit.face = enterFace;
    }
    return hit;
}

bool traceShapeBoxes(const CollisionShape& shape,
    const BlockPos& blockPos,
    const Vector3& adjustedStart,
    const Vector3& delta,
    const Vector3& originalStart,
    Vector3& outHitPos,
    Direction& outFace,
    f32& outDistance)
{
    f32 bestT = std::numeric_limits<f32>::max();
    Direction bestFace = Direction::None;
    bool hitAny = false;

    for (const auto& localBox : shape.boxes()) {
        const AxisAlignedBB worldBox(static_cast<f32>(blockPos.x) + localBox.minX,
            static_cast<f32>(blockPos.y) + localBox.minY,
            static_cast<f32>(blockPos.z) + localBox.minZ,
            static_cast<f32>(blockPos.x) + localBox.maxX,
            static_cast<f32>(blockPos.y) + localBox.maxY,
            static_cast<f32>(blockPos.z) + localBox.maxZ);

        auto hit = intersectSegmentAabb(adjustedStart, delta, worldBox);
        if (!hit.has_value()) {
            continue;
        }

        if (hit->t < bestT) {
            bestT = hit->t;
            bestFace = hit->face;
            hitAny = true;
        }
    }

    if (!hitAny) {
        return false;
    }

    outHitPos = Vector3(
        adjustedStart.x + delta.x * bestT, adjustedStart.y + delta.y * bestT, adjustedStart.z + delta.z * bestT);
    outFace = bestFace;
    outDistance = outHitPos.distance(originalStart);
    if (outDistance < 1.0e-5f) {
        outDistance = 0.0f;
    }
    return true;
}

[[nodiscard]] bool traceBlockShape(const BlockState& state,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    const Vector3& adjustedStart,
    const Vector3& delta,
    const Vector3& originalStart,
    Vector3& outHitPos,
    Direction& outFace,
    f32& outDistance)
{
    const CollisionShape& shape = state.getShape();
    if (shape.isEmpty()) {
        return false;
    }
    return traceShapeBoxes(
        shape, BlockPos(blockX, blockY, blockZ), adjustedStart, delta, originalStart, outHitPos, outFace, outDistance);
}

} // anonymous namespace

BlockRaycastResult raycastBlocks(const RaycastContext& context, const IWorld& world)
{
    const Vector3& start = context.ray.origin;
    const Vector3& dir = context.ray.direction;

    if (dir.lengthSquared() < 0.0001f) {
        // 方向为零向量，返回miss
        return BlockRaycastResult::miss();
    }

    // 计算终点
    const Vector3 end = context.endPosition();

    if (start.distanceSquared(end) < 0.0001f) {
        // 起点和终点重合，返回miss
        return BlockRaycastResult::miss();
    }

    // 使用MC风格的端点偏移，避免在边界处产生精度问题
    // 参考MC IBlockReader.doRayTrace
    // lerp(-1e-7, a, b) = a + (b - a) * (-1e-7) = a - (b - a) * 1e-7
    const f32 eps = 1.0e-7f;

    // 偏移后的终点：从终点向起点方向微移
    const Vector3 adjustedEnd(
        end.x + (start.x - end.x) * eps, end.y + (start.y - end.y) * eps, end.z + (start.z - end.z) * eps);

    // 偏移后的起点：从起点向终点方向微移
    const Vector3 adjustedStart(
        start.x + (end.x - start.x) * eps, start.y + (end.y - start.y) * eps, start.z + (end.z - start.z) * eps);

    // DDA方向向量：从偏移后终点到偏移后起点（负方向）
    // 这与MC的实现一致：d6 = d0 - d3 = adjustedEnd - adjustedStart
    const f32 dx = adjustedEnd.x - adjustedStart.x;
    const f32 dy = adjustedEnd.y - adjustedStart.y;
    const f32 dz = adjustedEnd.z - adjustedStart.z;
    const Vector3 ddaDelta(dx, dy, dz);

    // 当前方块坐标：从偏移后的起点开始
    i32 currentX = static_cast<i32>(std::floor(adjustedStart.x));
    i32 currentY = static_cast<i32>(std::floor(adjustedStart.y));
    i32 currentZ = static_cast<i32>(std::floor(adjustedStart.z));

    // per-raycast 区块缓存：DDA 步进通常在少数几个区块内移动，复用上一次 getChunk
    // 的 const ChunkData* 直读可避免每步都对 m_chunksMutex 加锁 + unordered_map 哈希查找。
    // 复刻 ServerWorld::getBlockState 的直读模式（getChunk + 本地坐标 + chunk->getBlockState），
    // 仅在跨区块边界时刷新缓存指针。raycast 在主线程同步执行，区块卸载由主线程 tick 驱动，
    // 故裸指针在一次 raycast（微秒级）期间稳定安全。
    const ChunkData* cachedChunk = nullptr;
    ChunkCoord cachedChunkX = 0;
    ChunkCoord cachedChunkZ = 0;
    // 标记当前缓存坐标是否已确认 getChunk 返回 nullptr（避免对同一未加载区块反复 getChunk）。
    // 同时用于回退判定：getChunk 返回 nullptr 时（区块未加载，或测试桩世界不实现 getChunk）
    // 回退到 world.getBlockState，保持与原 raycast 行为一致。
    bool cachedChunkIsNull = false;

    // 缓存式 getBlockState：命中缓存则直读，否则刷新缓存；getChunk 返回 nullptr 时回退到
    // world.getBlockState（测试桩世界的 getBlockState 可能有独立于 getChunk 的实现）。
    const auto getCachedBlockState = [&](i32 x, i32 y, i32 z) -> const BlockState* {
        const ChunkCoord chunkX = world::toChunkCoord(x);
        const ChunkCoord chunkZ = world::toChunkCoord(z);
        if (cachedChunk == nullptr || chunkX != cachedChunkX || chunkZ != cachedChunkZ) {
            cachedChunkX = chunkX;
            cachedChunkZ = chunkZ;
            cachedChunk = world.chunkManager()->tryToGetChunkInMem(chunkX, chunkZ);
            cachedChunkIsNull = (cachedChunk == nullptr);
        }
        if (cachedChunkIsNull) {
            // 区块未加载或测试桩不实现 getChunk：回退到 world.getBlockState，行为与原 raycast 一致
            return world.getBlockState(x, y, z);
        }
        // ChunkData::getBlockState 要求 X/Z 为区块内本地坐标 [0,15]、Y 为世界坐标，越界自处理
        const BlockCoord localX = x - chunkX * world::CHUNK_WIDTH;
        const BlockCoord localZ = z - chunkZ * world::CHUNK_WIDTH;
        return cachedChunk->getBlockState(localX, y, localZ);
    };

    // 先检查起点位置的方块（MC的重要步骤！）
    if (world.isWithinWorldBounds(currentX, currentY, currentZ)) {
        const BlockState* state = getCachedBlockState(currentX, currentY, currentZ);
        if (state != nullptr && !state->isAir() && !state->isLiquid()) {
            Vector3 hitPos;
            Direction hitFace = Direction::None;
            f32 hitDistance = 0.0f;
            const bool hit = traceBlockShape(
                *state, currentX, currentY, currentZ, adjustedStart, ddaDelta, start, hitPos, hitFace, hitDistance);
            if (hit) {
                return BlockRaycastResult::hit(start, BlockPos(currentX, currentY, currentZ), hitFace, 0.0f);
            }
        }
    }

    // 计算步进方向
    const i32 stepX = signum(dx);
    const i32 stepY = signum(dy);
    const i32 stepZ = signum(dz);

    // 计算每单位距离穿过方块边界的次数
    // tDelta = 1 / |direction component|
    const f32 tDeltaX = (stepX == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepX) / dx;
    const f32 tDeltaY = (stepY == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepY) / dy;
    const f32 tDeltaZ = (stepZ == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepZ) / dz;

    // 计算到下一个边界的初始t值
    // 使用偏移后的起点计算
    f32 tMaxX = (stepX == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaX * (stepX > 0 ? (1.0f - fract(adjustedStart.x)) : fract(adjustedStart.x));
    f32 tMaxY = (stepY == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaY * (stepY > 0 ? (1.0f - fract(adjustedStart.y)) : fract(adjustedStart.y));
    f32 tMaxZ = (stepZ == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaZ * (stepZ > 0 ? (1.0f - fract(adjustedStart.z)) : fract(adjustedStart.z));

    // DDA步进
    // MC使用 1.0 作为循环条件（代表遍历完整的射线）
    while (tMaxX <= 1.0f || tMaxY <= 1.0f || tMaxZ <= 1.0f) {
        // 选择最小的t值前进
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                // X方向步进
                currentX += stepX;
                tMaxX += tDeltaX;
            } else {
                // Z方向步进
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                // Y方向步进
                currentY += stepY;
                tMaxY += tDeltaY;
            } else {
                // Z方向步进
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        }

        // 检查世界边界
        if (!world.isWithinWorldBounds(currentX, currentY, currentZ)) {
            // 超出世界边界，返回miss
            return BlockRaycastResult::miss();
        }

        // 获取方块状态（命中区块缓存则直读，避免重复加锁查找）
        const BlockState* state = getCachedBlockState(currentX, currentY, currentZ);

        // 区块未加载，视为空气继续
        if (state == nullptr) {
            continue;
        }

        // 空气和液体不参与该射线选中
        if (state->isAir() || state->isLiquid()) {
            continue;
        }

        Vector3 hitPos;
        Direction hitFace = Direction::None;
        f32 hitDistance = 0.0f;
        const bool hit = traceBlockShape(
            *state, currentX, currentY, currentZ, adjustedStart, ddaDelta, start, hitPos, hitFace, hitDistance);
        if (!hit) {
            continue;
        }

        return BlockRaycastResult::hit(hitPos, BlockPos(currentX, currentY, currentZ), hitFace, hitDistance);
    }

    // 未击中任何方块
    return BlockRaycastResult::miss();
}

BlockRaycastResult raycastFluidSource(const RaycastContext& context, const IWorld& world)
{
    const Vector3& start = context.ray.origin;
    const Vector3& dir = context.ray.direction;
    if (dir.lengthSquared() < 0.0001f) {
        return BlockRaycastResult::miss();
    }

    const Vector3 end = context.endPosition();
    if (start.distanceSquared(end) < 0.0001f) {
        return BlockRaycastResult::miss();
    }

    // 与 raycastBlocks 相同的端点微偏移，避免起点/终点恰在方块边界时的精度问题。
    const f32 eps = 1.0e-7f;
    const Vector3 adjustedEnd(
        end.x + (start.x - end.x) * eps, end.y + (start.y - end.y) * eps, end.z + (start.z - end.z) * eps);
    const Vector3 adjustedStart(
        start.x + (end.x - start.x) * eps, start.y + (end.y - start.y) * eps, start.z + (end.z - start.z) * eps);
    const Vector3 ddaDelta(
        adjustedEnd.x - adjustedStart.x, adjustedEnd.y - adjustedStart.y, adjustedEnd.z - adjustedStart.z);

    // 方块检测结果：None = 无流体（不阻挡），Blocked = 流动流体（阻挡但不命中），Hit = 命中流体源。
    enum class CellResult : u8 { None, Blocked, Hit };

    // 射线命中信息：仅 CellResult::Hit 时有效。
    struct CellHit {
        BlockPos pos;
        Vector3 hitPos;
        Direction face = Direction::None;
        f32 distance = 0.0f;
    };

    const auto traceFluidCell = [&](const BlockPos& pos, CellHit& out) -> CellResult {
        const fluid::FluidState* fluidState = world.getFluidState(pos);
        if (fluidState == nullptr || fluidState->isEmpty()) {
            // 无流体：不阻挡（本检测不关心实心方块）
            return CellResult::None;
        }
        if (!fluidState->isSource()) {
            // 流动流体在 SOURCE_ONLY 语义下挡住射线但不产生命中
            return CellResult::Blocked;
        }
        // 流体碰撞形状：水平铺满整格，高度按液体量缩放
        CollisionShape shape;
        shape.addBox(0.0f, 0.0f, 0.0f, 1.0f, fluidState->getHeight(), 1.0f);
        if (!traceShapeBoxes(shape, pos, adjustedStart, ddaDelta, start, out.hitPos, out.face, out.distance)) {
            // 射线几何上未穿过该流体形状（例如擦过未满方块液面的上沿），继续前进
            return CellResult::None;
        }
        out.pos = pos;
        return CellResult::Hit;
    };

    // 起点所在方块单独检测；DDA 首轮步进会停在起点格，需要跳过以免重复判定。
    const BlockPos startPos(static_cast<i32>(std::floor(adjustedStart.x)),
        static_cast<i32>(std::floor(adjustedStart.y)),
        static_cast<i32>(std::floor(adjustedStart.z)));

    CellHit hit;
    if (world.isWithinWorldBounds(startPos.x, startPos.y, startPos.z)) {
        switch (traceFluidCell(startPos, hit)) {
            case CellResult::Hit:
                return BlockRaycastResult::hit(hit.hitPos, hit.pos, hit.face, hit.distance);
            case CellResult::Blocked:
                return BlockRaycastResult::miss();
            case CellResult::None:
                break;
        }
    }

    i32 currentX = startPos.x;
    i32 currentY = startPos.y;
    i32 currentZ = startPos.z;

    const i32 stepX = signum(ddaDelta.x);
    const i32 stepY = signum(ddaDelta.y);
    const i32 stepZ = signum(ddaDelta.z);
    const f32 tDeltaX = (stepX == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepX) / ddaDelta.x;
    const f32 tDeltaY = (stepY == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepY) / ddaDelta.y;
    const f32 tDeltaZ = (stepZ == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepZ) / ddaDelta.z;
    f32 tMaxX = (stepX == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaX * (stepX > 0 ? (1.0f - fract(adjustedStart.x)) : fract(adjustedStart.x));
    f32 tMaxY = (stepY == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaY * (stepY > 0 ? (1.0f - fract(adjustedStart.y)) : fract(adjustedStart.y));
    f32 tMaxZ = (stepZ == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaZ * (stepZ > 0 ? (1.0f - fract(adjustedStart.z)) : fract(adjustedStart.z));

    while (tMaxX <= 1.0f || tMaxY <= 1.0f || tMaxZ <= 1.0f) {
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                currentX += stepX;
                tMaxX += tDeltaX;
            } else {
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                currentY += stepY;
                tMaxY += tDeltaY;
            } else {
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        }

        if (!world.isWithinWorldBounds(currentX, currentY, currentZ)) {
            return BlockRaycastResult::miss();
        }

        // 首轮步进仍停在起点格（已在上面判定过），跳过。
        if (currentX == startPos.x && currentY == startPos.y && currentZ == startPos.z) {
            continue;
        }

        switch (traceFluidCell(BlockPos(currentX, currentY, currentZ), hit)) {
            case CellResult::Hit:
                return BlockRaycastResult::hit(hit.hitPos, hit.pos, hit.face, hit.distance);
            case CellResult::Blocked:
                return BlockRaycastResult::miss();
            case CellResult::None:
                break;
        }
    }

    return BlockRaycastResult::miss();
}

} // namespace mc
