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

// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "Teleporter.hpp"
#include "../DimensionManager.hpp"
#include "../DimensionType.hpp"
#include "PortalSize.hpp"
// Note: ServerWorld is forward declared in Teleporter.hpp
// Implementation of teleport methods is in server module
#include "../../../core/Constants.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../IWorld.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../chunk/ChunkData.hpp"
#include <algorithm>
#include <cmath>

#pragma pop_macro("BYTE_SIZE")

namespace mc {

// ============================================================================
// Teleporter 基类
// ============================================================================

Vector3d Teleporter::transformPosition(const Vector3d& pos, const DimensionType& from, const DimensionType& to)
{
    // 如果两个维度相同，不转换
    if (from.id() == to.id()) {
        return pos;
    }

    // 从源维度转换到主世界
    Vector3d overworldPos = from.scaleToOverworld(pos);

    // 从主世界转换到目标维度
    return to.scaleFromOverworld(overworldPos);
}

std::vector<BlockPos> Teleporter::searchPortalBlocks(IWorld& world, const BlockPos& center, i32 radius)
{
    std::vector<BlockPos> portalBlocks;

    // 获取下界传送门方块
    if (VanillaBlocks::NETHER_PORTAL == nullptr) {
        return portalBlocks;
    }

    const BlockState* portalState = &VanillaBlocks::NETHER_PORTAL->defaultState();

    // 在搜索半径内遍历区块
    ChunkCoord minChunkX = (center.x - radius) >> world::CHUNK_SHIFT;
    ChunkCoord maxChunkX = (center.x + radius) >> world::CHUNK_SHIFT;
    ChunkCoord minChunkZ = (center.z - radius) >> world::CHUNK_SHIFT;
    ChunkCoord maxChunkZ = (center.z + radius) >> world::CHUNK_SHIFT;

    for (ChunkCoord cx = minChunkX; cx <= maxChunkX; ++cx) {
        for (ChunkCoord cz = minChunkZ; cz <= maxChunkZ; ++cz) {
            // 获取区块
            const ChunkData* chunk = world.getChunk(cx, cz);
            if (chunk == nullptr) {
                continue;
            }

            // 遍历区块内的方块
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; ++y) {
                for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
                        const BlockState* state = chunk->getBlockState(x, y, z);
                        if (state == portalState) {
                            // 转换为世界坐标
                            BlockPos worldPos(cx * world::CHUNK_WIDTH + x, y, cz * world::CHUNK_WIDTH + z);

                            // 检查是否在搜索半径内（水平距离）
                            if (worldPos.distanceHorizontalSq(center) <= radius * radius) {
                                portalBlocks.push_back(worldPos);
                            }
                        }
                    }
                }
            }
        }
    }

    return portalBlocks;
}

void Teleporter::placePortalBlocks(IWorld& world, const BlockPos& corner, i32 width, i32 height, Direction axis)
{
    // 检查 NETHER_PORTAL 方块是否已注册
    if (VanillaBlocks::NETHER_PORTAL == nullptr) {
        return;
    }

    // 根据轴向设置传送门方块
    Axis portalAxis = (axis == Direction::East) ? Axis::X : Axis::Z;
    const BlockState* portalState =
        &VanillaBlocks::NETHER_PORTAL->defaultState().with(BlockStateProperties::HORIZONTAL_AXIS(), portalAxis);

    // 放置传送门方块
    for (i32 h = 0; h < height; ++h) {
        for (i32 w = 0; w < width; ++w) {
            BlockPos pos = corner;

            if (axis == Direction::East) {
                // X 轴传送门
                pos.x += w;
                pos.y += h;
            } else {
                // Z 轴传送门
                pos.z += w;
                pos.y += h;
            }

            world.setBlockState(pos, portalState);
        }
    }
}

// ============================================================================
// NetherTeleporter
// ============================================================================

bool NetherTeleporter::teleport(Entity& entity, DimensionId targetDim)
{
    // 传送逻辑：
    // 1. 获取当前位置
    // 2. 根据维度缩放计算目标位置
    // 3. 搜索已存在的传送门
    // 4. 如果没找到，创建新传送门
    // 5. 传送实体

    MC_UNUSED(entity);
    MC_UNUSED(targetDim);

    // TODO: 实现完整的传送逻辑
    // 需要以下基础设施：
    // 1. Entity::getWorld() 返回 ServerWorld
    // 2. Entity::setPosition() 和维度切换
    // 3. ServerWorld::setBlockState() 放置传送门
    // 4. ServerDimensionManager::transferPlayerToDimension()

    return false;
}

std::optional<PortalInfo> NetherTeleporter::findPortal(IWorld& world, const Vector3d& pos)
{
    // 转换为方块坐标
    BlockPos blockPos(
        math::floorTo<BlockCoord>(pos.x), math::floorTo<BlockCoord>(pos.y), math::floorTo<BlockCoord>(pos.z));

    // 根据目标维度确定搜索半径
    // 下界 -> 主世界: 搜索半径 16 格（因为下界坐标 × 8 = 主世界坐标，范围会扩大）
    // 主世界 -> 下界: 搜索半径 128 格（因为主世界坐标 ÷ 8 = 下界坐标，范围会缩小）
    // 注意：此方法在目标世界中执行，所以：
    // - 如果目标世界是主世界，使用 16 格半径
    // - 如果目标世界是下界，使用 128 格半径
    i32 searchRadius = NETHER_TO_OVERWORLD_SEARCH_RADIUS; // 默认 16
    if (world.dimension() == DimensionManager::NETHER) {
        searchRadius = OVERWORLD_TO_NETHER_SEARCH_RADIUS; // 128
    }

    // 搜索传送门方块
    auto portalBlocks = searchPortalBlocks(world, blockPos, searchRadius);

    if (portalBlocks.empty()) {
        return std::nullopt;
    }

    // 找到最近的传送门（使用 3D 距离平方）
    // 次排序键：Y 坐标（距离相同时选择 Y 更小的）
    BlockPos closest = portalBlocks[0];
    i64 closestDistSq = closest.distanceSq(blockPos);

    for (Size i = 1; i < portalBlocks.size(); ++i) {
        i64 distSq = portalBlocks[i].distanceSq(blockPos);
        if (distSq < closestDistSq || (distSq == closestDistSq && portalBlocks[i].y < closest.y)) {
            closestDistSq = distSq;
            closest = portalBlocks[i];
        }
    }

    // 返回传送门信息
    PortalInfo info;
    info.position =
        Vector3d(static_cast<f64>(closest.x) + 0.5, static_cast<f64>(closest.y), static_cast<f64>(closest.z) + 0.5);
    info.yaw = 0.0f;
    info.pitch = 0.0f;
    info.valid = true;
    return info;
}

PortalInfo NetherTeleporter::createPortal(IWorld& world, const Vector3d& pos)
{
    // 在目标位置创建传送门
    BlockPos blockPos(
        math::floorTo<BlockCoord>(pos.x), math::floorTo<BlockCoord>(pos.y), math::floorTo<BlockCoord>(pos.z));

    return createNetherPortal(world, blockPos);
}

std::optional<PortalInfo> NetherTeleporter::findPortalInNether(IWorld& world, const BlockPos& pos)
{
    return findPortal(world, Vector3d(static_cast<f64>(pos.x), static_cast<f64>(pos.y), static_cast<f64>(pos.z)));
}

std::optional<PortalInfo> NetherTeleporter::findPortalInOverworld(IWorld& world, const BlockPos& pos)
{
    return findPortal(world, Vector3d(static_cast<f64>(pos.x), static_cast<f64>(pos.y), static_cast<f64>(pos.z)));
}

PortalInfo NetherTeleporter::createNetherPortal(IWorld& world, const BlockPos& pos)
{
    // 创建下界传送门
    // 1. 寻找合适的位置（足够空间）
    // 2. 放置黑曜石框架
    // 3. 点燃传送门

    // 默认创建 2x3 的传送门框架

    // 确定传送门轴向（随机或基于位置）
    math::Random rng(static_cast<u64>(pos.x) ^ static_cast<u64>(pos.z));
    Direction axis = (rng.nextBoolean()) ? Direction::East : Direction::South;

    // 默认尺寸
    constexpr i32 DEFAULT_WIDTH = 2;
    constexpr i32 DEFAULT_HEIGHT = 3;

    // 放置黑曜石框架
    placeObsidianFrame(world, pos, DEFAULT_WIDTH, DEFAULT_HEIGHT, axis);

    // 放置传送门方块（点燃传送门）
    BlockPos portalCorner = pos;
    if (axis == Direction::East) {
        portalCorner.x += 1;
    } else {
        portalCorner.z += 1;
    }
    portalCorner.y += 1;

    placePortalBlocks(world, portalCorner, DEFAULT_WIDTH, DEFAULT_HEIGHT, axis);

    // 返回传送门信息（站在传送门中心）
    f64 spawnX = static_cast<f64>(pos.x) + 1.5;
    f64 spawnY = static_cast<f64>(pos.y + 1);
    f64 spawnZ = static_cast<f64>(pos.z) + 0.5;

    if (axis == Direction::East) {
        spawnX = static_cast<f64>(pos.x) + 0.5;
        spawnZ = static_cast<f64>(pos.z) + 1.5;
    }

    PortalInfo info;
    info.position = Vector3d(spawnX, spawnY, spawnZ);
    info.yaw = 0.0f;
    info.pitch = 0.0f;
    info.valid = true;
    return info;
}

void NetherTeleporter::placeObsidianFrame(IWorld& world, const BlockPos& corner, i32 width, i32 height, Direction axis)
{
    // 检查黑曜石方块是否已注册
    if (VanillaBlocks::OBSIDIAN == nullptr) {
        return;
    }

    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->defaultState();

    // 确定宽度方向
    Direction widthDir = axis;
    if (axis != Direction::East && axis != Direction::South) {
        widthDir = Direction::East; // 默认 X 轴
    }

    // 底部框架
    for (i32 w = 0; w <= width + 1; ++w) {
        BlockPos pos = corner;
        if (widthDir == Direction::East) {
            pos.x += w;
        } else {
            pos.z += w;
        }
        world.setBlockState(pos, obsidian);
    }

    // 左边框架
    for (i32 h = 1; h <= height; ++h) {
        BlockPos pos = corner;
        pos.y += h;
        world.setBlockState(pos, obsidian);
    }

    // 右边框架
    BlockPos rightBottom = corner;
    if (widthDir == Direction::East) {
        rightBottom.x += width + 1;
    } else {
        rightBottom.z += width + 1;
    }
    for (i32 h = 1; h <= height; ++h) {
        BlockPos pos = rightBottom;
        pos.y += h;
        world.setBlockState(pos, obsidian);
    }

    // 顶部框架
    BlockPos topLeft = corner;
    topLeft.y += height + 1;
    for (i32 w = 0; w <= width + 1; ++w) {
        BlockPos pos = topLeft;
        if (widthDir == Direction::East) {
            pos.x += w;
        } else {
            pos.z += w;
        }
        world.setBlockState(pos, obsidian);
    }
}

// ============================================================================
// EndTeleporter
// ============================================================================

bool EndTeleporter::teleport(Entity& entity, DimensionId targetDim)
{
    // 传送逻辑：
    // 1. 主世界 -> 末地: 传送到固定出生点 (100, 49, 0)
    // 2. 末地 -> 主世界: 返回重生点或床

    MC_UNUSED(entity);
    MC_UNUSED(targetDim);

    // TODO: 实现完整的传送逻辑
    return false;
}

std::optional<PortalInfo> EndTeleporter::findPortal(IWorld& world, const Vector3d& pos)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 末地传送门是固定的，不需要搜索
    // 返回固定的出生位置
    PortalInfo info;
    info.position = getEndSpawnPosition(); // (100.5, 50.0, 0.5)
    info.yaw = 90.0f;                      // 面向末地岛
    info.pitch = 0.0f;
    info.valid = true;
    return info;
}

PortalInfo EndTeleporter::createPortal(IWorld& world, const Vector3d& pos)
{
    MC_UNUSED(pos);
    // 末地传送门是预设的，创建出生平台
    createEndSpawnPlatform(world);

    PortalInfo info;
    info.position = getEndSpawnPosition();
    info.yaw = 90.0f;
    info.pitch = 0.0f;
    info.valid = true;
    return info;
}

void EndTeleporter::createExitPortal(IWorld& world, const BlockPos& pos)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 实现创建末地出口传送门
    // 在击败末影龙后生成
    // 需要放置末地传送门框架和末地传送门方块
}

void EndTeleporter::createEndSpawnPlatform(IWorld& world)
{
    // 创建末地出生平台（黑曜石平台）
    // 出生点 (100, 50, 0)，平台在 y = 48（spawnY - 2）
    // 清空空间 y = 49, 50, 51（spawnY - 1 到 spawnY + 1）

    // 检查黑曜石方块是否已注册
    if (VanillaBlocks::OBSIDIAN == nullptr) {
        return;
    }

    const BlockState* obsidian = &VanillaBlocks::OBSIDIAN->defaultState();

    // 出生点常量
    constexpr i32 SPAWN_X = 100;
    constexpr i32 SPAWN_Y = 50; // 出生点 Y 坐标
    constexpr i32 SPAWN_Z = 0;
    constexpr i32 PLATFORM_Y = SPAWN_Y - 2; // y = 48

    // 放置黑曜石平台 (y = 48)
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 z = -2; z <= 2; ++z) {
            BlockPos pos(SPAWN_X + x, PLATFORM_Y, SPAWN_Z + z);
            world.setBlockState(pos, obsidian);
        }
    }

    // 清空上方空间 (y = 49, 50, 51)
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 z = -2; z <= 2; ++z) {
            for (i32 y = SPAWN_Y - 1; y <= SPAWN_Y + 1; ++y) {
                BlockPos pos(SPAWN_X + x, y, SPAWN_Z + z);
                world.setBlockState(pos, nullptr);
            }
        }
    }
}

void EndTeleporter::placeEndPortalFrame(IWorld& world, const BlockPos& center)
{
    MC_UNUSED(world);
    MC_UNUSED(center);
    // TODO: 实现放置末地传送门框架
    // 需要放置末地传送门框架方块（EndPortalFrameBlock）
}

} // namespace mc
