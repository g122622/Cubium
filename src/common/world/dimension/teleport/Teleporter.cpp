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
#include "../../../util/assert/AssertAll.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/property/Properties.hpp"
#include "../../IWorld.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <optional>
#include <vector>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

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
    // TODO: 此方法返回 false，因为下界传送器的完整传送逻辑由 ServerPlayer::changeDimension() 协调，
    // 维度切换需要 ServerDimensionManager 处理区块加载/卸载和数据包发送。
    // 此方法提供传送门搜索和创建逻辑，供 ServerPlayer 调用。
    // 未来如有非玩家实体穿越下界传送门的需求，需在此方法中实现完整的传送流程。
    //
    // 传送流程：
    // 1. ServerPlayer::onPortalTriggered() 确定目标维度
    // 2. ServerPlayer::changeDimension() 使用 transformPosition() 计算目标坐标
    // 3. 调用 findPortal() 搜索已有传送门，或 createPortal() 创建新传送门
    // 4. 调用 ServerDimensionManager::transferPlayerToDimension() 执行维度切换
    MC_UNUSED(entity);
    MC_UNUSED(targetDim);
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
    // TODO: 此方法返回 false，因为末地传送器的完整传送逻辑由 ServerPlayer::changeDimension() 协调，
    // 维度切换需要 ServerDimensionManager 处理区块加载/卸载和数据包发送。
    // 未来如有非玩家实体进入末地传送门的需求，需在此方法中实现完整的传送流程。
    //
    // 传送流程：
    // 1. EndPortalBlock::onEntityCollision() 检测实体进入末地传送门方块
    // 2. 对于主世界 -> 末地：调用 createEndSpawnPlatform() 创建出生平台，
    //    传送到固定位置 (100.5, 50.0, 0.5)
    // 3. 对于末地 -> 主世界：使用玩家的重生点
    // 4. 调用 ServerDimensionManager::transferPlayerToDimension() 执行维度切换
    MC_UNUSED(entity);
    MC_UNUSED(targetDim);
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
    info.position = getEndSpawnPosition(); // (100.5, 50.0, 0.5)
    info.yaw = 90.0f;
    info.pitch = 0.0f;
    info.valid = true;
    return info;
}

void EndTeleporter::createExitPortal(IWorld& world, const BlockPos& pos, bool active)
{
    // 创建末地出口传送门讲台
    // 参考 MC Java EndPodiumFeature.place()
    //
    // 讲台结构（以 pos 为中心，Y 方向展开）：
    // - Y = pos.y - 1（底部层）：内圆基岩（半径 < 2.5），外环末地石（半径 < 3.5）
    // - Y = pos.y（传送门层）：内圆传送门方块（active）或空气（inactive），外环基岩
    // - Y > pos.y（上方空间）：全部空气（清除讲台区域内）
    // - 中心柱：pos 向上 4 格基岩（覆盖传送门层中心）
    // - 4 个墙上火把：中心柱 Y+2 的四个水平方向
    //
    // 常量：
    // - PODIUM_RADIUS = 4（水平扫描范围）
    // - PODIUM_PILLAR_HEIGHT = 4（中心柱高度）

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    const BlockState* endStone = VanillaBlocks::getState(VanillaBlocks::END_STONE);
    const BlockState* endPortal = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* wallTorch = VanillaBlocks::getState(VanillaBlocks::WALL_TORCH);

    if (!bedrock || !endStone || !air) {
        return;
    }

    // 传送门方块在激活时才需要
    if (active && !endPortal) {
        return;
    }

    constexpr f64 INNER_RADIUS_SQ = 2.5 * 2.5; // 内圆半径 2.5 的平方
    constexpr f64 OUTER_RADIUS_SQ = 3.5 * 3.5; // 外圆半径 3.5 的平方

    for (i32 dx = -PODIUM_RADIUS; dx <= PODIUM_RADIUS; ++dx) {
        for (i32 dz = -PODIUM_RADIUS; dz <= PODIUM_RADIUS; ++dz) {
            f64 distSq = static_cast<f64>(dx * dx + dz * dz);
            bool inInner = distSq <= INNER_RADIUS_SQ;
            bool inOuter = distSq <= OUTER_RADIUS_SQ;

            // Y = pos.y - 1（底部层）
            BlockPos basePos(pos.x + dx, pos.y - 1, pos.z + dz);
            if (inInner) {
                // 内圆：基岩
                world.setBlockState(basePos, bedrock);
            } else if (inOuter) {
                // 外环：末地石
                world.setBlockState(basePos, endStone);
            }

            // Y = pos.y（传送门层）
            if (inInner) {
                // 内圆：传送门方块（active）或空气（inactive）
                BlockPos portalPos(pos.x + dx, pos.y, pos.z + dz);
                if (active) {
                    world.setBlockState(portalPos, endPortal);
                } else {
                    world.setBlockState(portalPos, air);
                }
            } else if (inOuter) {
                // 外环：基岩
                BlockPos rimPos(pos.x + dx, pos.y, pos.z + dz);
                world.setBlockState(rimPos, bedrock);
            }

            // Y > pos.y（清除上方空间，只清除外环范围内的）
            if (inOuter) {
                for (i32 dy = 1; dy <= PODIUM_PILLAR_HEIGHT; ++dy) {
                    BlockPos clearPos(pos.x + dx, pos.y + dy, pos.z + dz);
                    world.setBlockState(clearPos, air);
                }
            }
        }
    }

    // 中心柱：4 格基岩（从 pos.y 到 pos.y+3），覆盖传送门层中心方块
    for (i32 dy = 0; dy < PODIUM_PILLAR_HEIGHT; ++dy) {
        BlockPos pillarPos(pos.x, pos.y + dy, pos.z);
        world.setBlockState(pillarPos, bedrock);
    }

    // 墙上火把：中心柱 Y+2 的四个水平方向
    if (wallTorch) {
        // 火把朝向：朝向外侧，即附着在中心柱上
        // 北面位置(z-1)的火把朝北，南面位置(z+1)的火把朝南，等等
        const BlockState* torchNorth = &wallTorch->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
        world.setBlockState(BlockPos(pos.x, pos.y + 2, pos.z - 1), torchNorth);

        const BlockState* torchSouth = &wallTorch->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
        world.setBlockState(BlockPos(pos.x, pos.y + 2, pos.z + 1), torchSouth);

        const BlockState* torchWest = &wallTorch->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West);
        world.setBlockState(BlockPos(pos.x - 1, pos.y + 2, pos.z), torchWest);

        const BlockState* torchEast = &wallTorch->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East);
        world.setBlockState(BlockPos(pos.x + 1, pos.y + 2, pos.z), torchEast);
    }
}

void EndTeleporter::createEndSpawnPlatform(IWorld& world)
{
    // 创建末地出生平台（黑曜石平台）
    // 参考 MC Java EndPlatformFeature.createEndPlatform()
    //
    // 出生点 (100, 50, 0)，平台在 y = 48（spawnY - 2）
    // 清空空间 y = 49, 50, 51, 52（spawnY - 1 到 spawnY + 2，共 4 层）
    //
    // MC Java 中传入的位置是 BlockPos(100, 49, 0)（即 spawnY - 1），
    // 然后 k==-1 时放黑曜石，k==0,1,2,3 时放空气。
    // 对应到我们的坐标：黑曜石在 Y=48，空气在 Y=49,50,51,52。

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

    // 清空上方空间 (y = 49, 50, 51, 52，共 4 层)
    for (i32 x = -2; x <= 2; ++x) {
        for (i32 z = -2; z <= 2; ++z) {
            for (i32 y = SPAWN_Y - 1; y <= SPAWN_Y + 2; ++y) {
                BlockPos pos(SPAWN_X + x, y, SPAWN_Z + z);
                world.setBlockState(pos, nullptr);
            }
        }
    }
}

void EndTeleporter::placeEndPortalFrame(IWorld& world, const BlockPos& center)
{
    // 放置末地传送门框架方块环
    // 参考 MC Java EndPortalFrameBlock.getOrCreatePortalShape() 中的图案定义
    //
    // 传送门框架图案（5×5，从上往下看，北 = -Z，南 = +Z）：
    //   ? v v v ?      v = FACING=NORTH（北边框架的凸起朝北，背离中心）
    //   > P P P <      > = FACING=WEST（西边框架的凸起朝西，背离中心）
    //   > P P P <      P = 末地传送门方块（3×3 内部区域）
    //   > P P P <      < = FACING=EAST（东边框架的凸起朝东，背离中心）
    //   ? ^ ^ ^ ?      ^ = FACING=SOUTH（南边框架的凸起朝南，背离中心）
    //   ? = 角落，不放置任何方块
    //
    // 每个框架方块都带有末影之眼（EYE=true），框架凸起朝外（背离传送门中心）。
    // 框架位于内部 3×3 传送门区域外侧一格，形成 5×5 的外环。
    // 框架放置后，在内部 3×3 区域填充末地传送门方块。
    //
    // center 参数表示传送门内部 3×3 区域的中心底部位置，
    // 框架放置在 center 周围 ±1 格的一圈上（不含角落）。

    if (VanillaBlocks::END_PORTAL_FRAME == nullptr || VanillaBlocks::END_PORTAL == nullptr) {
        return;
    }

    const BlockState* frameState = &VanillaBlocks::END_PORTAL_FRAME->defaultState();
    const BlockState* portalState = &VanillaBlocks::END_PORTAL->defaultState();

    // 北边（z = center.z - 2）：3 个框架，凸起朝北（FACING=NORTH）
    const BlockState* frameNorth = &frameState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
                                        .with(BlockStateProperties::EYE(), true);
    for (i32 dx = -1; dx <= 1; ++dx) {
        BlockPos pos(center.x + dx, center.y, center.z - 2);
        world.setBlockState(pos, frameNorth);
    }

    // 南边（z = center.z + 2）：3 个框架，凸起朝南（FACING=SOUTH）
    const BlockState* frameSouth = &frameState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South)
                                        .with(BlockStateProperties::EYE(), true);
    for (i32 dx = -1; dx <= 1; ++dx) {
        BlockPos pos(center.x + dx, center.y, center.z + 2);
        world.setBlockState(pos, frameSouth);
    }

    // 西边（x = center.x - 2）：3 个框架，凸起朝西（FACING=WEST）
    const BlockState* frameWest = &frameState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::West)
                                       .with(BlockStateProperties::EYE(), true);
    for (i32 dz = -1; dz <= 1; ++dz) {
        BlockPos pos(center.x - 2, center.y, center.z + dz);
        world.setBlockState(pos, frameWest);
    }

    // 东边（x = center.x + 2）：3 个框架，凸起朝东（FACING=EAST）
    const BlockState* frameEast = &frameState->with(BlockStateProperties::HORIZONTAL_FACING(), Direction::East)
                                       .with(BlockStateProperties::EYE(), true);
    for (i32 dz = -1; dz <= 1; ++dz) {
        BlockPos pos(center.x + 2, center.y, center.z + dz);
        world.setBlockState(pos, frameEast);
    }

    // 内部 3×3 区域放置末地传送门方块
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            BlockPos pos(center.x + dx, center.y, center.z + dz);
            world.setBlockState(pos, portalState);
        }
    }
}

} // namespace mc

#pragma pop_macro("BYTE_SIZE")
