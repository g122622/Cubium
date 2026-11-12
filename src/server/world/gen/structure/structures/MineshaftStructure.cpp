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

#include "MineshaftStructure.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/base/FallingBlock.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::world::gen::structure {

namespace {

/**
 * @brief 起点房间的 genDepth
 *
 * 原版 `MineshaftStructure.generatePiecesAndAdjust` 用 `new MineShaftRoom(0, ...)` 建起始
 * 房间：那个 0 是 `StructurePiece.genDepth`，与包围盒起点 Y（恒为 50）无关。子构件取
 * `父深度 + 1`，`generateAndAddPiece` 的截断判据是 `> 8`，因此矿井从 0 起算、最多九层。
 */
constexpr i32 kStartRoomDepth = 0;

/// 展开层数上限（原版 `MineshaftPieces.MAX_DEPTH`）。判据比较的是父构件的深度。
constexpr i32 kMaxMineshaftDepth = 8;

/**
 * @brief 矿井片段的方向编码（0=北 1=南 2=西 3=东）转 Direction
 *
 * 走廊与楼梯的 generate 用局部坐标（x 0..2、z 沿通道伸）书写方块，必须靠
 * `setCoordBaseMode` 才能映射到世界坐标；房间与交叉点则直接使用世界坐标。
 */
Direction directionFromIndex(i32 direction)
{
    switch (direction) {
        case 1:
            return Direction::South;
        case 2:
            return Direction::West;
        case 3:
            return Direction::East;
        case 0:
        default:
            return Direction::North;
    }
}

} // namespace

// ============================================================================
// 辅助函数实现
// ============================================================================

std::unique_ptr<MineshaftPiece> createMineshaftPiece(std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 depth,
    MineshaftType type)
{
    i32 chance = rng.nextInt(100);

    std::unique_ptr<MineshaftPiece> created;

    if (chance >= 80) {
        // 交叉点：高度由一次 nextInt(4) 决定（0 表示双层，高 6；否则高 2），
        // 水平范围按朝向取固定的偏移表。
        //
        // 原版 MineShaftCrossing.findCrossing：
        //   int i = nextInt(4) == 0 ? 6 : 2;
        //   北 -> (-1, 0, -4) .. (3, i, 0)
        //   南 -> (-1, 0,  0) .. (3, i, 4)
        //   西 -> (-4, 0, -1) .. (0, i, 3)
        //   东 -> ( 0, 0, -1) .. (4, i, 3)
        // 此前既没有那次 nextInt(4)，偏移也与原版不同。
        const i32 crossHeight = (rng.nextInt(4) == 0) ? 6 : 2;
        i32 minX = x - 1, maxX = x + 3;
        i32 minZ = z - 4, maxZ = z;
        switch (direction) {
            case 1: // 南
                minZ = z;
                maxZ = z + 4;
                break;
            case 2: // 西
                minX = x - 4;
                maxX = x;
                minZ = z - 1;
                maxZ = z + 3;
                break;
            case 3: // 东
                minX = x;
                maxX = x + 4;
                minZ = z - 1;
                maxZ = z + 3;
                break;
            case 0: // 北
            default:
                break;
        }
        const i32 minY = y;
        const i32 maxY = y + crossHeight;

        created = std::make_unique<MineshaftCross>(
            MineshaftPieceTypes::CROSS, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    } else if (chance >= 70) {
        // 楼梯：固定下降 5 格、水平延伸 8 格，按朝向取偏移表。
        //
        // 原版 MineshaftStairs.findStairs：
        //   北 -> (0, -5, -8) .. (2, 2, 0)
        //   南 -> (0, -5,  0) .. (2, 2, 8)
        //   西 -> (-8, -5, 0) .. (0, 2, 2)
        //   东 -> ( 0, -5, 0) .. (8, 2, 2)
        // 此前 Y 方向只降了 4 格（length/2），与原版差 1。
        constexpr i32 kStairsDrop = 5;
        constexpr i32 kStairsReach = 8;
        i32 minX = x, maxX = x + 2;
        i32 minZ = z, maxZ = z + 2;
        switch (direction) {
            case 0: // 北
                minZ = z - kStairsReach;
                maxZ = z;
                break;
            case 1: // 南
                minZ = z;
                maxZ = z + kStairsReach;
                break;
            case 2: // 西
                minX = x - kStairsReach;
                maxX = x;
                break;
            case 3: // 东
                minX = x;
                maxX = x + kStairsReach;
                break;
        }
        const i32 minY = y - kStairsDrop;
        const i32 maxY = y + 2;

        created = std::make_unique<MineshaftStairs>(
            MineshaftPieceTypes::STAIRS, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    } else {
        // 走廊长度从 nextInt(3)+2 起逐个尝试缩短，第一个不与已有构件相交的长度胜出。
        //
        // 原版 findCorridorSize 的写法是
        //   for (int i = nextInt(3) + 2; i > 0; i--) { int j = i * 5; ... 相交就继续缩短 }
        // 因此走廊跨度恒为 5 的倍数（10 / 15 / 20）。此前用固定长度减去后直接构造，
        // 边界为此偏移 1 格。
        constexpr i32 kCorridorLengthStep = 5;
        i32 minX = x, maxX = x + 2;
        i32 minZ = z, maxZ = z + 2;
        i32 minY = y, maxY = y + 2;
        bool found = false;
        for (i32 attempt = rng.nextInt(3) + 2; attempt > 0; --attempt) {
            const i32 span = attempt * kCorridorLengthStep - 1;
            i32 trialMinX = x, trialMaxX = x + 2;
            i32 trialMinZ = z, trialMaxZ = z + 2;
            switch (direction) {
                case 0: // 北
                    trialMinZ = z - span;
                    trialMaxZ = z;
                    break;
                case 1: // 南
                    trialMinZ = z;
                    trialMaxZ = z + span;
                    break;
                case 2: // 西
                    trialMinX = x - span;
                    trialMaxX = x;
                    break;
                case 3: // 东
                default:
                    trialMinX = x;
                    trialMaxX = x + span;
                    break;
            }
            const StructureBoundingBox trial(trialMinX, minY, trialMinZ, trialMaxX, maxY, trialMaxZ);
            bool collides = false;
            for (const auto& existing : pieces) {
                if (existing->boundingBox().intersects(trial)) {
                    collides = true;
                    break;
                }
            }
            if (!collides) {
                minX = trialMinX;
                maxX = trialMaxX;
                minZ = trialMinZ;
                maxZ = trialMaxZ;
                found = true;
                break;
            }
        }
        if (!found) {
            return nullptr;
        }

        created = std::make_unique<MineshaftCorridor>(
            MineshaftPieceTypes::CORRIDOR, rng, minX, minY, minZ, maxX, maxY, maxZ, direction, type);
    }

    // 【必须设置生成深度】构件类型 ID（ROOM=60/CORRIDOR=61/CROSS=62/STAIRS=63）与生成深度
    // 是两个不同的量。addMineshaftPiece 用 `depth > 8` 限制展开层数；此前实现把类型 ID
    // 当深度传入，首次展开即 61 > 8 直接失败，矿井永远只剩起点房间一个构件。
    if (created) {
        created->setGenDepth(depth);

        // 原版只有走廊与楼梯在构造里调用 setOrientation；交叉点与房间不调用，
        // 它们的 rotation 与 mirror 保持默认的 NONE。
        //
        // setOrientation 的映射：
        //   NORTH -> mirror NONE,        rotation NONE
        //   SOUTH -> mirror LEFT_RIGHT,  rotation NONE
        //   WEST  -> mirror LEFT_RIGHT,  rotation CLOCKWISE_90
        //   EAST  -> mirror NONE,        rotation CLOCKWISE_90
        //
        // 朝向同时决定构件内部的方块布局，缺了它整个矿井的构件全部按朝北绘制。
        const i32 pieceType = created->getComponentType();
        if (pieceType == MineshaftPieceTypes::CORRIDOR || pieceType == MineshaftPieceTypes::STAIRS) {
            switch (direction) {
                case 1: // 南
                    created->setMirror(Mirror::LeftRight);
                    created->setRotation(Rotation::None);
                    break;
                case 2: // 西
                    created->setMirror(Mirror::LeftRight);
                    created->setRotation(Rotation::Clockwise90);
                    break;
                case 3: // 东
                    created->setMirror(Mirror::None);
                    created->setRotation(Rotation::Clockwise90);
                    break;
                case 0: // 北
                default:
                    created->setMirror(Mirror::None);
                    created->setRotation(Rotation::None);
                    break;
            }
        }

        // 新构件不得与已有构件重叠。
        //
        // 原版 findCorridorSize / findCrossing / findStairs 在返回包围盒前都会调用
        // findCollisionPiece，一旦相交就返回 null、该分支不再生成。这一步是把
        // 「每段三分支、深度 8 层」的指数展开收敛到实际几十个构件的关键——
        // 缺了它矿井会膨胀到数千个构件（实测 5681）。
        const StructureBoundingBox box = created->boundingBox();
        for (const auto& existing : pieces) {
            if (existing->boundingBox().intersects(box)) {
                return nullptr;
            }
        }
    }

    return created;
}

MineshaftPiece* addMineshaftPiece(MineshaftPiece* parent,
    const MineshaftPiece* origin,
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    i32 direction,
    i32 parentDepth,
    MineshaftType type)
{
    // 深度限制。原版 generateAndAddPiece 的判据是 `p_227714_ > 8`，其中 p_227714_
    // 是**父构件的深度**：主口段传父的 `this.getGenDepth()`，走廊第 2 段传 `i + 1`。
    // 子构件的深度随后取 `p_227714_ + 1`。
    MC_ASSERT_RELEASE_MSG(
        parentDepth >= kStartRoomDepth, "mineshaft piece depth must not go below the start room depth");
    if (parentDepth > kStartRoomDepth + kMaxMineshaftDepth) {
        return nullptr;
    }

    // 距离限制：原版以**起点房间**的包围盒为基准（`p_227707_` 始终是起始构件），
    // 而不是发起这次开口的父构件。用父构件作基准会让远处分支被过早截断，
    // 整棵树的长短与原版不同。
    if (origin != nullptr) {
        if (std::abs(x - origin->minX()) > 80 || std::abs(z - origin->minZ()) > 80) {
            return nullptr;
        }
    }

    // 创建新片段
    auto piece = createMineshaftPiece(pieces, rng, x, y, z, direction, parentDepth + 1, type);
    if (!piece) {
        return nullptr;
    }

    // 必须**先登记再展开子构件**。原版 generateAndAddPiece 的顺序是
    //   accessor.addPiece(piece); piece.addChildren(...);
    // 此前这里先展开、由调用方随后才 push，结果是子构件排在父构件之前，
    // 整座矿井的 pieces 顺序完全倒置。
    MineshaftPiece* created = piece.get();
    pieces.push_back(std::move(piece));
    created->buildComponent(pieces, rng, 8);
    return created;
}

// ============================================================================
// MineshaftPiece 实现
// ============================================================================

MineshaftPiece::MineshaftPiece(
    i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, MineshaftType mineshaftType)
    : StructurePiece(type, minX, minY, minZ, maxX, maxY, maxZ)
    , m_mineshaftType(mineshaftType)
{}

bool MineshaftPiece::_canPlaceAt(i32 /*x*/, i32 y, i32 /*z*/)
{
    return world::isValidY(y) && y > world::MIN_BUILD_HEIGHT + 4;
}

const BlockState* MineshaftPiece::planksState() const noexcept
{
    return m_mineshaftType == MineshaftType::Mesa ? VanillaBlocks::getState(VanillaBlocks::DARK_OAK_PLANKS)
                                                  : VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);
}

const BlockState* MineshaftPiece::woodState() const noexcept
{
    return m_mineshaftType == MineshaftType::Mesa ? VanillaBlocks::getState(VanillaBlocks::DARK_OAK_LOG)
                                                  : VanillaBlocks::getState(VanillaBlocks::OAK_LOG);
}

const BlockState* MineshaftPiece::fenceState() const noexcept
{
    return m_mineshaftType == MineshaftType::Mesa ? VanillaBlocks::getState(VanillaBlocks::DARK_OAK_FENCE)
                                                  : VanillaBlocks::getState(VanillaBlocks::OAK_FENCE);
}

namespace {

/// 空世界的「读不到方块」在结构逻辑里一律按空气处理（原版 `getBlock` 越界返回 AIR）
const BlockState& airState()
{
    return VanillaBlocks::AIR->defaultState();
}

} // namespace

const BlockState* MineshaftPiece::_getBlock(
    IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const
{
    const BlockPos pos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (!bounds.isInside(pos.x, pos.y, pos.z)) {
        return &airState();
    }
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr ? state : &airState();
}

bool MineshaftPiece::_isInterior(IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const
{
    const i32 worldX = getXWithOffset(x, z);
    const i32 worldY = getYWithOffset(y + 1);
    const i32 worldZ = getZWithOffset(x, z);
    if (!bounds.isInside(worldX, worldY, worldZ)) {
        return false;
    }
    // 原版用 OCEAN_FLOOR_WG 高度图，且 `LevelReader.getHeight` 返回首个可放置 Y
    // （最高方块 Y + 1）；本项目的 `IWorld::getHeight` 返回最高方块 Y，需要 +1。
    return worldY < world.getHeight(worldX, worldZ, HeightmapType::OceanFloorWG) + 1;
}

bool MineshaftPiece::_isSupportingBox(
    IWorld& world, const StructureBoundingBox& bounds, i32 x1, i32 x2, i32 y, i32 z) const
{
    for (i32 x = x1; x <= x2; ++x) {
        if (_getBlock(world, x, y + 1, z, bounds)->isAir()) {
            return false;
        }
    }
    return true;
}

bool MineshaftPiece::_isInInvalidLocation(IWorld& world, const StructureBoundingBox& bounds) const
{
    const i32 minX = std::max(m_minX - 1, bounds.minX());
    const i32 minY = std::max(m_minY - 1, bounds.minY());
    const i32 minZ = std::max(m_minZ - 1, bounds.minZ());
    const i32 maxX = std::min(m_maxX + 1, bounds.maxX());
    const i32 maxY = std::min(m_maxY + 1, bounds.maxY());
    const i32 maxZ = std::min(m_maxZ + 1, bounds.maxZ());

    // 原版 `level.getBiome(center).is(BiomeTags.MINESHAFT_BLOCKING)`：群系取自世界本身，
    // 不走生成器。世界侧按 vanilla 的 Voronoi 抖动选格，空世界桩固定返回非阻挡群系。
    if (biome::BiomeTags::MINESHAFT_BLOCKING().contains(
            world.getBiome((minX + maxX) / 2, (minY + maxY) / 2, (minZ + maxZ) / 2))) {
        return true;
    }

    auto isLiquidAt = [&world](i32 x, i32 y, i32 z) {
        const BlockState* state = world.getBlockState(x, y, z);
        return state != nullptr && state->isLiquid();
    };

    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 z = minZ; z <= maxZ; ++z) {
            if (isLiquidAt(x, minY, z) || isLiquidAt(x, maxY, z)) {
                return true;
            }
        }
    }
    for (i32 x = minX; x <= maxX; ++x) {
        for (i32 y = minY; y <= maxY; ++y) {
            if (isLiquidAt(x, y, minZ) || isLiquidAt(x, y, maxZ)) {
                return true;
            }
        }
    }
    for (i32 z = minZ; z <= maxZ; ++z) {
        for (i32 y = minY; y <= maxY; ++y) {
            if (isLiquidAt(minX, y, z) || isLiquidAt(maxX, y, z)) {
                return true;
            }
        }
    }
    return false;
}

bool MineshaftPiece::_canBeReplaced(IWorld& world, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds) const
{
    const BlockState* state = _getBlock(world, x, y, z, bounds);
    const Block* block = &state->getBlock();
    if (planksState() != nullptr && block == &planksState()->getBlock()) {
        return false;
    }
    if (woodState() != nullptr && block == &woodState()->getBlock()) {
        return false;
    }
    if (fenceState() != nullptr && block == &fenceState()->getBlock()) {
        return false;
    }
    if (VanillaBlocks::CHAIN != nullptr && block == VanillaBlocks::CHAIN) {
        return false;
    }
    return true;
}

void MineshaftPiece::setBlockState(
    IWorldWriter& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");
    if (!_canBeReplaced(*reader, x, y, z, bounds)) {
        return;
    }
    StructurePiece::setBlockState(world, state, x, y, z, bounds);
}

void MineshaftPiece::_generateMaybeBox(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::IRandom& rng,
    f32 probability,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    const BlockState* boundaryBlock,
    const BlockState* insideBlock,
    bool requireNonAir,
    bool requireInterior)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");
    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (rng.nextFloat() > probability) {
                    continue;
                }
                if (requireNonAir && _getBlock(*reader, x, y, z, bounds)->isAir()) {
                    continue;
                }
                if (requireInterior && !_isInterior(*reader, x, y, z, bounds)) {
                    continue;
                }
                const bool isBoundary = x == minX || x == maxX || y == minY || y == maxY || z == minZ || z == maxZ;
                setBlockState(world, isBoundary ? boundaryBlock : insideBlock, x, y, z, bounds);
            }
        }
    }
}

void MineshaftPiece::_maybeGenerateBlock(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    math::IRandom& rng,
    f32 probability,
    i32 x,
    i32 y,
    i32 z,
    const BlockState* state)
{
    if (rng.nextFloat() < probability) {
        setBlockState(world, state, x, y, z, bounds);
    }
}

void MineshaftPiece::_generateUpperHalfSphere(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    const BlockState* state,
    bool requireNonAir)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");
    const f32 spanX = static_cast<f32>(maxX - minX + 1);
    const f32 spanY = static_cast<f32>(maxY - minY + 1);
    const f32 spanZ = static_cast<f32>(maxZ - minZ + 1);
    const f32 centerX = static_cast<f32>(minX) + spanX / 2.0F;
    const f32 centerZ = static_cast<f32>(minZ) + spanZ / 2.0F;

    for (i32 y = minY; y <= maxY; ++y) {
        const f32 dy = static_cast<f32>(y - minY) / spanY;
        for (i32 x = minX; x <= maxX; ++x) {
            const f32 dx = (static_cast<f32>(x) - centerX) / (spanX * 0.5F);
            for (i32 z = minZ; z <= maxZ; ++z) {
                const f32 dz = (static_cast<f32>(z) - centerZ) / (spanZ * 0.5F);
                if (requireNonAir && _getBlock(*reader, x, y, z, bounds)->isAir()) {
                    continue;
                }
                if (dx * dx + dy * dy + dz * dz <= 1.05F) {
                    setBlockState(world, state, x, y, z, bounds);
                }
            }
        }
    }
}

void MineshaftPiece::_setPlanksBlock(
    IWorldWriter& world, const StructureBoundingBox& bounds, const BlockState& planks, i32 x, i32 y, i32 z)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");
    if (!_isInterior(*reader, x, y, z, bounds)) {
        return;
    }
    const BlockPos pos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    const BlockState* existing = reader->getBlockState(pos);
    if (existing == nullptr) {
        existing = &airState();
    }
    if (!existing->isFaceSturdy(*reader, pos, Direction::Up, SupportType::Full)) {
        setBlockState(world, &planks, x, y, z, bounds);
    }
}

void MineshaftPiece::_fillColumnDown(
    IWorldWriter& world, const BlockState& state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");
    BlockPos pos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (!bounds.isInside(pos.x, pos.y, pos.z)) {
        return;
    }
    const i32 topY = pos.y;
    while (true) {
        const BlockState* current = reader->getBlockState(pos);
        if (current == nullptr || !isReplaceableByStructures(*current) || pos.y <= reader->getMinBuildHeight() + 1) {
            break;
        }
        pos.y -= 1;
    }
    const BlockState* below = reader->getBlockState(pos);
    if (below != nullptr && _canPlaceColumnOnTopOf(*reader, pos)) {
        while (pos.y < topY) {
            pos.y += 1;
            world.setBlockState(pos.x, pos.y, pos.z, &state, world::BlockUpdateFlags::UPDATE_CLIENTS);
        }
    }
}

bool MineshaftPiece::_canPlaceColumnOnTopOf(IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && state->isFaceSturdy(world, pos, Direction::Up, SupportType::Full);
}

bool MineshaftPiece::_canHangChainBelow(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    return Block::canSupportCenter(const_cast<IWorld&>(world), pos, Direction::Down) &&
        dynamic_cast<const blocks::FallingBlock*>(&state.getBlock()) == nullptr;
}

void MineshaftPiece::_fillColumnBetween(
    IWorldWriter& world, const BlockState& state, i32 x, i32 fromY, i32 toY, i32 z) const
{
    for (i32 y = fromY; y < toY; ++y) {
        world.setBlockState(x, y, z, &state, world::BlockUpdateFlags::UPDATE_CLIENTS);
    }
}

void MineshaftPiece::_fillPillarDownOrChainUp(
    IWorldWriter& world, const BlockState& state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");
    const BlockPos base(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    if (!bounds.isInside(base.x, base.y, base.z)) {
        return;
    }

    const i32 startY = base.y;
    bool down = true;
    bool up = true;
    for (i32 step = 1; down || up; ++step) {
        if (down) {
            const i32 probeY = startY - step;
            const BlockState* probe = reader->getBlockState(base.x, probeY, base.z);
            const bool replaceable =
                probe != nullptr && isReplaceableByStructures(*probe) && !probe->is(VanillaBlocks::LAVA);
            if (!replaceable && probe != nullptr && _canPlaceColumnOnTopOf(*reader, BlockPos(base.x, probeY, base.z))) {
                _fillColumnBetween(world, state, base.x, probeY + 1, startY, base.z);
                return;
            }
            down = step <= 20 && replaceable && probeY > reader->getMinBuildHeight() + 1;
        }

        if (up) {
            const i32 probeY = startY + step;
            const BlockState* probe = reader->getBlockState(base.x, probeY, base.z);
            const bool replaceable = probe != nullptr && isReplaceableByStructures(*probe);
            if (!replaceable && probe != nullptr &&
                _canHangChainBelow(*reader, BlockPos(base.x, probeY, base.z), *probe)) {
                world.setBlockState(base.x, startY + 1, base.z, fenceState(), world::BlockUpdateFlags::UPDATE_CLIENTS);
                _fillColumnBetween(
                    world, VanillaBlocks::CHAIN->defaultState(), base.x, startY + 2, startY + step, base.z);
                return;
            }
            up = step <= 50 && replaceable && probeY < reader->getMaxBuildHeight() - 1;
        }
    }
}

void MineshaftPiece::_placeSupport(
    IWorldWriter& world, const StructureBoundingBox& bounds, i32 x1, i32 y, i32 z, i32 x2, i32 topY, math::IRandom& rng)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");
    if (!_isSupportingBox(*reader, bounds, x1, x2, topY, z)) {
        return;
    }
    const BlockState* planks = planksState();
    const BlockState* fence = fenceState();
    // 两侧竖向栅栏（朝内的连接位），顶上一根横梁把两根栅栏连起来。
    const BlockState& westFence = fence->with(BlockStateProperties::WEST(), true);
    const BlockState& eastFence = fence->with(BlockStateProperties::EAST(), true);
    fillWithBlocks(world, bounds, x1, y, z, x1, topY - 1, z, &westFence, nullptr, false);
    fillWithBlocks(world, bounds, x2, y, z, x2, topY - 1, z, &eastFence, nullptr, false);
    if (rng.nextInt(4) == 0) {
        fillWithBlocks(world, bounds, x1, topY, z, x1, topY, z, planks, nullptr, false);
        fillWithBlocks(world, bounds, x2, topY, z, x2, topY, z, planks, nullptr, false);
    } else {
        fillWithBlocks(world, bounds, x1, topY, z, x2, topY, z, planks, nullptr, false);
        const BlockState& southTorch =
            VanillaBlocks::WALL_TORCH->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::South);
        const BlockState& northTorch =
            VanillaBlocks::WALL_TORCH->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North);
        _maybeGenerateBlock(world, bounds, rng, 0.05F, x1 + 1, topY, z - 1, &southTorch);
        _maybeGenerateBlock(world, bounds, rng, 0.05F, x1 + 1, topY, z + 1, &northTorch);
    }
}

void MineshaftPiece::_maybePlaceCobWeb(
    IWorldWriter& world, const StructureBoundingBox& bounds, math::IRandom& rng, f32 probability, i32 x, i32 y, i32 z)
{
    IWorld& reader = *dynamic_cast<IWorld*>(&world);
    if (_isInterior(reader, x, y, z, bounds) && rng.nextFloat() < probability &&
        _hasSturdyNeighbours(reader, bounds, x, y, z, 2)) {
        setBlockState(world, &VanillaBlocks::COBWEB->defaultState(), x, y, z, bounds);
    }
}

bool MineshaftPiece::_hasSturdyNeighbours(
    IWorld& world, const StructureBoundingBox& bounds, i32 x, i32 y, i32 z, i32 count) const
{
    BlockPos pos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    i32 sturdy = 0;
    for (const Direction direction : Directions::all()) {
        pos.x += Directions::xOffset(direction);
        pos.y += Directions::yOffset(direction);
        pos.z += Directions::zOffset(direction);
        if (bounds.isInside(pos.x, pos.y, pos.z)) {
            const BlockState* state = world.getBlockState(pos);
            if (state != nullptr &&
                state->isFaceSturdy(world, pos, Directions::opposite(direction), SupportType::Full)) {
                if (++sturdy >= count) {
                    return true;
                }
            }
        }
        pos.x -= Directions::xOffset(direction);
        pos.y -= Directions::yOffset(direction);
        pos.z -= Directions::zOffset(direction);
    }
    return false;
}

// ============================================================================
// MineshaftRoom 实现
// ============================================================================

namespace {

/**
 * 按 X -> Y -> Z 的顺序抽取房间的三个跨度。
 *
 * 原版是 new BoundingBox(x, 50, z, x+7+nextInt(6), 54+nextInt(6), z+7+nextInt(6))，
 * Java 保证实参从左到右求值；而 C++ 的实参求值顺序未指定，
 * 把三个 nextInt(6) 写在同一构造实参里会让随机序列错位
 * （实测房间高度变成 5，而原版是 10）。
 */
std::array<i32, 3> drawRoomSpans(math::IRandom& rng)
{
    const i32 a = rng.nextInt(6);
    const i32 b = rng.nextInt(6);
    const i32 c2 = rng.nextInt(6);
    return {a, b, c2};
}

} // namespace

MineshaftRoom::MineshaftRoom(i32 componentType, math::IRandom& rng, i32 x, i32 y, i32 z, MineshaftType type)
    : MineshaftRoom(componentType, rng, x, y, z, type, drawRoomSpans(rng))
{}

MineshaftRoom::MineshaftRoom(
    i32 componentType, math::IRandom& rng, i32 x, i32 y, i32 z, MineshaftType type, std::array<i32, 3> spans)
    : MineshaftPiece(componentType, x, 50, z, x + 7 + spans[0], 54 + spans[1], z + 7 + spans[2], type)
{
    // 房间的起点 Y 恒为 50，X/Z 跨度各为 7..12、顶面为 54..59。
    // 传入的 y 在原版里不参与房间包围盒。
    //
    // 深度从 0 起算（见 kStartRoomDepth 的说明）：包围盒起点 Y 恒为 50，与深度无关。
    setGenDepth(kStartRoomDepth);
    MC_UNUSED(y);
    MC_UNUSED(rng);
    MC_UNUSED(type);
}

void MineshaftRoom::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");

    if (_isInInvalidLocation(*reader, chunkBounds)) {
        return;
    }

    // 起点房间是一个实心壳：下方一层洞穴空气掏空、上方半球掏空，四边各留一条
    // 通到子构件入口的隧道。房间没有朝向（coordBaseMode == None），这里的坐标
    // 就是世界坐标。
    const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    fillWithBlocks(world,
        chunkBounds,
        m_minX,
        m_minY + 1,
        m_minZ,
        m_maxX,
        std::min(m_minY + 3, m_maxY),
        m_maxZ,
        caveAir,
        caveAir,
        false);

    for (const StructureBoundingBox& entrance : m_childEntranceBoxes) {
        fillWithBlocks(world,
            chunkBounds,
            entrance.minX(),
            entrance.maxY() - 2,
            entrance.minZ(),
            entrance.maxX(),
            entrance.maxY(),
            entrance.maxZ(),
            caveAir,
            caveAir,
            false);
    }

    _generateUpperHalfSphere(world, chunkBounds, m_minX, m_minY + 4, m_minZ, m_maxX, m_maxY, m_maxZ, caveAir, false);
}

void MineshaftRoom::offset(i32 dx, i32 dy, i32 dz)
{
    MineshaftPiece::offset(dx, dy, dz);
    for (auto& box : m_childEntranceBoxes) {
        box.offset(dx, dy, dz);
    }
}

void MineshaftRoom::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    MC_UNUSED(maxDepth);

    // 房间沿四条边各开一串出口，每条边都用「随机步进 + 固定 4 格间隔」推进：
    //
    //   j = 0;
    //   while (j < span) {
    //       j += nextInt(span);
    //       if (j + 3 > span) break;      // 剩余跨度不足 3
    //       ...在 j 处开口，y = minY + nextInt(k) + 1...
    //       j += 4;
    //   }
    //
    // 四个循环的顺序固定为 北、南、西、东，各自独立重置 j。
    //
    // 子构件的深度是父深度 + 1，理由同走廊。
    // 原版 addChildren 不做深度提前返回：截断只发生在 generateAndAddPiece
    // 与走廊第 2 段的 `i < 8` 里。这里多一层返回会少掉深度 8 构件开出的分支。
    const i32 depth = genDepth();
    i32 k = (maxY() - minY() + 1) - 3 - 1;
    if (k <= 0) {
        k = 1;
    }

    auto addChild = [&](i32 cx, i32 cy, i32 cz, i32 dir) -> MineshaftPiece* {
        return addMineshaftPiece(this, pieces.front().get(), pieces, rng, cx, cy, cz, dir, depth, m_mineshaftType);
    };

    const i32 xSpan = maxX() - minX() + 1;
    const i32 zSpan = maxZ() - minZ() + 1;

    // 北侧：沿 X 步进，出口在 minZ - 1
    for (i32 offset = 0; offset < xSpan;) {
        offset += rng.nextInt(xSpan);
        if (offset + 3 > xSpan) {
            break;
        }
        const i32 cx = minX() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1; // 独立语句：实参求值顺序未指定
        if (MineshaftPiece* child = addChild(cx, cy, minZ() - 1, 0)) {
            const StructureBoundingBox box = child->boundingBox();
            m_childEntranceBoxes.emplace_back(box.minX(), box.minY(), minZ(), box.maxX(), box.maxY(), minZ() + 1);
        }
        offset += 4;
    }

    // 南侧：沿 X 步进，出口在 maxZ + 1
    for (i32 offset = 0; offset < xSpan;) {
        offset += rng.nextInt(xSpan);
        if (offset + 3 > xSpan) {
            break;
        }
        const i32 cx = minX() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1;
        if (MineshaftPiece* child = addChild(cx, cy, maxZ() + 1, 1)) {
            const StructureBoundingBox box = child->boundingBox();
            m_childEntranceBoxes.emplace_back(box.minX(), box.minY(), maxZ() - 1, box.maxX(), box.maxY(), maxZ());
        }
        offset += 4;
    }

    // 西侧：沿 Z 步进，出口在 minX - 1
    for (i32 offset = 0; offset < zSpan;) {
        offset += rng.nextInt(zSpan);
        if (offset + 3 > zSpan) {
            break;
        }
        const i32 cz = minZ() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1;
        if (MineshaftPiece* child = addChild(minX() - 1, cy, cz, 2)) {
            const StructureBoundingBox box = child->boundingBox();
            m_childEntranceBoxes.emplace_back(minX(), box.minY(), box.minZ(), minX() + 1, box.maxY(), box.maxZ());
        }
        offset += 4;
    }

    // 东侧：沿 Z 步进，出口在 maxX + 1
    for (i32 offset = 0; offset < zSpan;) {
        offset += rng.nextInt(zSpan);
        if (offset + 3 > zSpan) {
            break;
        }
        const i32 cz = minZ() + offset;
        const i32 cy = minY() + rng.nextInt(k) + 1;
        if (MineshaftPiece* child = addChild(maxX() + 1, cy, cz, 3)) {
            const StructureBoundingBox box = child->boundingBox();
            m_childEntranceBoxes.emplace_back(maxX() - 1, box.minY(), box.minZ(), maxX(), box.maxY(), box.maxZ());
        }
        offset += 4;
    }
}

// ============================================================================
// MineshaftCorridor 实现
// ============================================================================

MineshaftCorridor::MineshaftCorridor(i32 componentType,
    math::IRandom& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    i32 direction,
    MineshaftType type)
    : MineshaftPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ, type)

{
    m_direction = direction;
    m_hasRails = rng.nextInt(3) == 0;                   // 33% 概率有铁轨
    m_hasSpiders = !m_hasRails && rng.nextInt(23) == 0; // ~4% 概率有蜘蛛刷怪笼

    // 走廊的方块用局部坐标书写（x 0..2、z 沿通道），必须记录朝向才能映射到世界坐标。
    setCoordBaseMode(directionFromIndex(direction));

    // 段数按**跨度**算（max - min + 1），与原版 `boundingBox.getZSpan() / 5` 一致。
    // 此前漏了 +1：长度为 10 的走廊会少算一段，通道因此短 5 格。
    if (direction == 0 || direction == 1) {
        m_sectionCount = (maxZ - minZ + 1) / 5;
    } else {
        m_sectionCount = (maxX - minX + 1) / 5;
    }
}

void MineshaftCorridor::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* chunk,
    IChunkGenerator* /*generator*/)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");

    if (_isInInvalidLocation(*reader, chunkBounds)) {
        return;
    }

    const i32 sectionLength = m_sectionCount * 5 - 1;
    const BlockState* planks = planksState();
    const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);

    // 走廊主体：先掏出一条 3 格宽、2 格高的通行空间，再按 0.8 的概率把上半层
    // （y=2）也掏空——这样天花板高低起伏。
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 2, 1, sectionLength, caveAir, caveAir, false);
    _generateMaybeBox(world, chunkBounds, rng, 0.8F, 0, 2, 0, 2, 2, sectionLength, caveAir, caveAir, false, false);

    if (m_hasSpiders) {
        // 蜘蛛走廊：上半层再按 0.6 概率铺蜘蛛网（只在内部、且该格本身非空气时）。
        _generateMaybeBox(world,
            chunkBounds,
            rng,
            0.6F,
            0,
            0,
            0,
            2,
            1,
            sectionLength,
            &VanillaBlocks::COBWEB->defaultState(),
            caveAir,
            false,
            true);
    }

    for (i32 section = 0; section < m_sectionCount; ++section) {
        const i32 sectionZ = 2 + section * 5;

        _placeSupport(world, chunkBounds, 0, 0, sectionZ, 2, 2, rng);

        _maybePlaceCobWeb(world, chunkBounds, rng, 0.1F, 0, 2, sectionZ - 1);
        _maybePlaceCobWeb(world, chunkBounds, rng, 0.1F, 2, 2, sectionZ - 1);
        _maybePlaceCobWeb(world, chunkBounds, rng, 0.1F, 0, 2, sectionZ + 1);
        _maybePlaceCobWeb(world, chunkBounds, rng, 0.1F, 2, 2, sectionZ + 1);
        _maybePlaceCobWeb(world, chunkBounds, rng, 0.05F, 0, 2, sectionZ - 2);
        _maybePlaceCobWeb(world, chunkBounds, rng, 0.05F, 2, 2, sectionZ - 2);
        _maybePlaceCobWeb(world, chunkBounds, rng, 0.05F, 0, 2, sectionZ + 2);
        _maybePlaceCobWeb(world, chunkBounds, rng, 0.05F, 2, 2, sectionZ + 2);

        if (rng.nextInt(100) == 0) {
            _createChest(world, *reader, rng, 2, 0, sectionZ - 1, chunkBounds, chunk);
        }
        if (rng.nextInt(100) == 0) {
            _createChest(world, *reader, rng, 0, 0, sectionZ + 1, chunkBounds, chunk);
        }

        if (m_hasSpiders && !m_spawnerPlaced) {
            const i32 zOffset = sectionZ - 1 + rng.nextInt(3);
            const BlockPos spawnerPos(getXWithOffset(1, zOffset), getYWithOffset(0), getZWithOffset(1, zOffset));
            if (chunkBounds.isInside(spawnerPos.x, spawnerPos.y, spawnerPos.z) &&
                _isInterior(*reader, 1, 0, zOffset, chunkBounds)) {
                m_spawnerPlaced = true;
                setBlockState(world, &VanillaBlocks::SPAWNER->defaultState(), 1, 0, zOffset, chunkBounds);
                IWorld* readableWorld = dynamic_cast<IWorld*>(&world);
                if (readableWorld != nullptr) {
                    BlockEntity* blockEntity = readableWorld->getBlockEntity(spawnerPos);
                    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::MobSpawner) {
                        static_cast<blockentity::MobSpawnerBlockEntity*>(blockEntity)
                            ->setEntityId(ResourceLocation(entity::EntityTypeKeys::CAVE_SPIDER), rng);
                    }
                }
            }
        }
    }

    // 底部木板：只有当该位置「在内部」（上方一格仍在地表之下）且脚下不是满格
    // 支撑面时才铺。
    for (i32 x = 0; x <= 2; ++x) {
        for (i32 z = 0; z <= sectionLength; ++z) {
            _setPlanksBlock(world, chunkBounds, *planks, x, -1, z);
        }
    }

    _placeDoubleLowerOrUpperSupport(world, *reader, chunkBounds, 0, -1, 2);
    if (m_sectionCount > 1) {
        _placeDoubleLowerOrUpperSupport(world, *reader, chunkBounds, 0, -1, sectionLength - 2);
    }

    if (m_hasRails) {
        const BlockState& railSouthNorth = VanillaBlocks::RAIL->defaultState().with(
            BlockStateProperties::RAIL_SHAPE(), BlockStateProperties::RailShape::NorthSouth);
        for (i32 z = 0; z <= sectionLength; ++z) {
            const BlockState* below = _getBlock(*reader, 1, -1, z, chunkBounds);
            if (!below->isAir() && below->isSolidRender()) {
                const f32 probability = _isInterior(*reader, 1, 0, z, chunkBounds) ? 0.7F : 0.9F;
                _maybeGenerateBlock(world, chunkBounds, rng, probability, 1, 0, z, &railSouthNorth);
            }
        }
    }
}

void MineshaftCorridor::_placeDoubleLowerOrUpperSupport(
    IWorldWriter& world, IWorld& reader, const StructureBoundingBox& chunkBounds, i32 x, i32 y, i32 z)
{
    const BlockState* planks = planksState();
    if (_getBlock(reader, x, y, z, chunkBounds)->is(&planks->getBlock())) {
        _fillPillarDownOrChainUp(world, *woodState(), x, y, z, chunkBounds);
    }
    if (_getBlock(reader, x + 2, y, z, chunkBounds)->is(&planks->getBlock())) {
        _fillPillarDownOrChainUp(world, *woodState(), x + 2, y, z, chunkBounds);
    }
}

void MineshaftCorridor::_createChest(IWorldWriter& world,
    IWorld& reader,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* chunk)
{
    const BlockPos pos(getXWithOffset(x, z), getYWithOffset(y), getZWithOffset(x, z));
    const BlockState* atPos = reader.getBlockState(pos);
    const BlockState* below = reader.getBlockState(pos.x, pos.y - 1, pos.z);
    const bool posIsAir = atPos == nullptr || atPos->isAir();
    const bool belowIsAir = below == nullptr || below->isAir();
    if (!chunkBounds.isInside(pos.x, pos.y, pos.z) || !posIsAir || belowIsAir) {
        return;
    }

    const BlockState& rail = VanillaBlocks::RAIL->defaultState().with(BlockStateProperties::RAIL_SHAPE(),
        rng.nextBoolean() ? BlockStateProperties::RailShape::NorthSouth : BlockStateProperties::RailShape::EastWest);
    setBlockState(world, &rail, x, y, z, chunkBounds);

    // 战利品表种子必须在任何可能提前返回之前抽出来：原版先抽 nextLong() 播种战利品表，
    // 再创建实体；种子若因缺容器而跳过抽取，后续所有抽样都会错位。
    const i64 lootTableSeed = rng.nextLong();

    // 宝箱矿车是实体，世界生成阶段没有实体容器，按项目既有的延迟生成机制记进
    // ChunkPrimer 的待生成实体表，区块发布后由 ServerWorld 在主线程真正创建。
    if (chunk == nullptr) {
        // 缺少 ChunkPrimer（结构探针等不带区块的调用方）：随机流已按原版消费，只是不产生实体。
        return;
    }

    SpawnedEntityData minecart(std::string("minecraft:chest_minecart"),
        static_cast<f32>(pos.x) + 0.5f,
        static_cast<f32>(pos.y) + 0.5f,
        static_cast<f32>(pos.z) + 0.5f,
        world::spawn::SpawnReason::ChunkGeneration);
    // 战利品表的懒解包依赖 NBT：ChestMinecart 在 readFromNBT 里读到 LootTable/LootTableSeed
    // 后，玩家首次打开时才填充物品。
    nbt::tags::compound_tag entityNbt;
    entityNbt.put(
        ::mc::entity::serialization::nbt_keys::LOOT_TABLE, std::string("minecraft:chests/abandoned_mineshaft"));
    entityNbt.put(::mc::entity::serialization::nbt_keys::LOOT_TABLE_SEED, lootTableSeed);
    minecart.nbt = std::move(entityNbt);
    chunk->addSpawnedEntity(std::move(minecart));
}

void MineshaftCorridor::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    MC_UNUSED(maxDepth);

    // 走廊的展开分两段，顺序不能变：
    //
    // 1) 先抽一次 nextInt(4)，按走廊朝向决定从哪条边开一个「主口」。
    //    走廊的朝向恒为四个水平方向之一，所以整段必然执行。
    // 2) 再在 genDepth < 8 时沿一边逐点步进，每点抽一次 nextInt(5)，命中 0 或 1 才开口。
    //
    // 第 2 段的条件是 `direction != NORTH && direction != SOUTH` 时沿 X 扫描、
    // 否则沿 Z 扫描。走廊朝向不会是 null，因此这里直接按方向判断。
    //
    // 原版 addChildren 传的是 this.getGenDepth()，generateAndAddPiece 内部再 +1；
    // 本项目的 addMineshaftPiece 直接把这个值当新构件的深度，所以要显式 +1。
    //
    // 注意这里**不能**加 `if (depth > maxDepth) return;`：原版 addChildren 的主口段
    // 是无条件执行的，深度截断只发生在 generateAndAddPiece（即 addMineshaftPiece）
    // 里，以及第 2 段的 `i < 8`。多这一层提前返回会让深度 8 的走廊不再开主口，
    // 实测少掉一整条分支（原版 [9] 就是深度 8 的构件开出来的深度 9 子构件）。
    //
    // 这里的 depth 是**本构件自己的深度**（原版 addChildren 里的 `int i = getGenDepth()`），
    // 主口段直接把它传给 generateAndAddPiece，子构件取 i + 1；第 2 段传 i + 1。
    const i32 depth = genDepth();
    MC_UNUSED(maxDepth);

    auto addChild = [&](i32 cx, i32 cy, i32 cz, i32 dir, i32 childDepth) {
        (void)addMineshaftPiece(this, pieces.front().get(), pieces, rng, cx, cy, cz, dir, childDepth, m_mineshaftType);
    };

    // ---- 第 1 段：主口 ----
    const i32 j = rng.nextInt(4);
    if (j <= 1) {
        const i32 y = minY() - 1 + rng.nextInt(3);
        switch (m_direction) {
            case 1: // 南
                addChild(minX(), y, maxZ() + 1, 1, depth);
                break;
            case 2: // 西
                addChild(minX() - 1, y, minZ(), 2, depth);
                break;
            case 3: // 东
                addChild(maxX() + 1, y, minZ(), 3, depth);
                break;
            case 0: // 北
            default:
                addChild(minX(), y, minZ() - 1, 0, depth);
                break;
        }
    } else if (j == 2) {
        const i32 y = minY() - 1 + rng.nextInt(3);
        switch (m_direction) {
            case 1: // 南
                addChild(minX() - 1, y, maxZ() - 3, 2, depth);
                break;
            case 2: // 西
                addChild(minX(), y, minZ() - 1, 0, depth);
                break;
            case 3: // 东
                addChild(maxX() - 3, y, minZ() - 1, 0, depth);
                break;
            case 0: // 北
            default:
                addChild(minX() - 1, y, minZ(), 2, depth);
                break;
        }
    } else {
        const i32 y = minY() - 1 + rng.nextInt(3);
        switch (m_direction) {
            case 1: // 南
                addChild(maxX() + 1, y, maxZ() - 3, 3, depth);
                break;
            case 2: // 西
                addChild(minX(), y, maxZ() + 1, 1, depth);
                break;
            case 3: // 东
                addChild(maxX() - 3, y, maxZ() + 1, 1, depth);
                break;
            case 0: // 北
            default:
                addChild(maxX() + 1, y, minZ(), 3, depth);
                break;
        }
    }

    // ---- 第 2 段：沿边逐点步进 ----
    //
    // 原版判据是 `if (i < 8)`，i 为走廊自己的 genDepth；起点房间的深度是 kStartRoomDepth。
    //
    // 关键：第 2 段传给 addMineshaftPiece 的是**父深度本身**，主口段传的是同一语义的
    // 父深度。子构件最终取 parentDepth + 1。第 2 段因此比主口段深一层，这一点很容易漏
    // ——传错会让子构件的深度少 1，后续所有截断点跟着偏移。
    if (genDepth() < kStartRoomDepth + kMaxMineshaftDepth) {
        if (m_direction != 0 && m_direction != 1) {
            // 朝东西：沿 X 扫描，南北两侧开口。
            for (i32 x = minX() + 3; x + 3 <= maxX(); x += 5) {
                const i32 pick = rng.nextInt(5);
                if (pick == 0) {
                    addChild(x, minY(), minZ() - 1, 0, depth + 1);
                } else if (pick == 1) {
                    addChild(x, minY(), maxZ() + 1, 1, depth + 1);
                }
            }
        } else {
            // 朝南北：沿 Z 扫描，东西两侧开口。
            for (i32 z = minZ() + 3; z + 3 <= maxZ(); z += 5) {
                const i32 pick = rng.nextInt(5);
                if (pick == 0) {
                    addChild(minX() - 1, minY(), z, 2, depth + 1);
                } else if (pick == 1) {
                    addChild(maxX() + 1, minY(), z, 3, depth + 1);
                }
            }
        }
    }
}

// ============================================================================
// MineshaftCross 实现
// ============================================================================

MineshaftCross::MineshaftCross(
    i32 componentType, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 direction, MineshaftType type)
    : MineshaftPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ, type)

{
    m_direction = direction;
    m_isTwoFloored = (maxY - minY + 1) > 3;
}

void MineshaftCross::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");

    if (_isInInvalidLocation(*reader, chunkBounds)) {
        return;
    }

    // 交叉点没有朝向（coordBaseMode == None），下面用的都是世界坐标。
    const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    if (m_isTwoFloored) {
        // 双层交叉点：下层 + 上层各掏一条十字，中间留一层楼板。
        fillWithBlocks(
            world, chunkBounds, m_minX + 1, m_minY, m_minZ, m_maxX - 1, m_minY + 2, m_maxZ, caveAir, caveAir, false);
        fillWithBlocks(
            world, chunkBounds, m_minX, m_minY, m_minZ + 1, m_maxX, m_minY + 2, m_maxZ - 1, caveAir, caveAir, false);
        fillWithBlocks(
            world, chunkBounds, m_minX + 1, m_maxY - 2, m_minZ, m_maxX - 1, m_maxY, m_maxZ, caveAir, caveAir, false);
        fillWithBlocks(
            world, chunkBounds, m_minX, m_maxY - 2, m_minZ + 1, m_maxX, m_maxY, m_maxZ - 1, caveAir, caveAir, false);
        fillWithBlocks(world,
            chunkBounds,
            m_minX + 1,
            m_minY + 3,
            m_minZ + 1,
            m_maxX - 1,
            m_minY + 3,
            m_maxZ - 1,
            caveAir,
            caveAir,
            false);
    } else {
        fillWithBlocks(
            world, chunkBounds, m_minX + 1, m_minY, m_minZ, m_maxX - 1, m_maxY, m_maxZ, caveAir, caveAir, false);
        fillWithBlocks(
            world, chunkBounds, m_minX, m_minY, m_minZ + 1, m_maxX, m_maxY, m_maxZ - 1, caveAir, caveAir, false);
    }

    // 四个内角各立一根柱子（顶端若已有方块才立，避免顶穿地面）。
    const BlockState* planks = planksState();
    auto placeSupportPillar = [&](i32 x, i32 z) {
        if (!_getBlock(*reader, x, m_maxY + 1, z, chunkBounds)->isAir()) {
            fillWithBlocks(world, chunkBounds, x, m_minY, z, x, m_maxY, z, planks, caveAir, false);
        }
    };
    placeSupportPillar(m_minX + 1, m_minZ + 1);
    placeSupportPillar(m_minX + 1, m_maxZ - 1);
    placeSupportPillar(m_maxX - 1, m_minZ + 1);
    placeSupportPillar(m_maxX - 1, m_maxZ - 1);

    const i32 floorY = m_minY - 1;
    for (i32 x = m_minX; x <= m_maxX; ++x) {
        for (i32 z = m_minZ; z <= m_maxZ; ++z) {
            _setPlanksBlock(world, chunkBounds, *planks, x, floorY, z);
        }
    }
}

void MineshaftCross::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    // 原版 addChildren 不做深度提前返回：截断只发生在 generateAndAddPiece
    // 与走廊第 2 段的 `i < 8` 里。这里多一层返回会少掉深度 8 构件开出的分支。
    const i32 depth = genDepth();
    MC_UNUSED(maxDepth);

    // 交叉点按朝向固定开出三个方向，坐标由包围盒直接算出，不抽随机数。
    //
    // 原版 MineShaftCrossing.addChildren 的四个分支：
    //   北 -> (minX+1, minY, minZ-1) 北 | (minX-1, minY, minZ+1) 西 | (maxX+1, minY, minZ+1) 东
    //   南 -> (minX+1, minY, maxZ+1) 南 | (minX-1, minY, minZ+1) 西 | (maxX+1, minY, minZ+1) 东
    //   西 -> (minX+1, minY, minZ-1) 北 | (minX+1, minY, maxZ+1) 南 | (minX-1, minY, minZ+1) 西
    //   东 -> (minX+1, minY, minZ-1) 北 | (minX+1, minY, maxZ+1) 南 | (maxX+1, minY, minZ+1) 东
    //
    // 此前这里遍历四个方向、每个方向以 nextInt(3) 决定是否跳过，既不匹配原版的
    // 分支表，又多吃了最多四个随机数。
    auto addChild = [&](i32 nx, i32 ny, i32 nz, i32 dir) {
        (void)addMineshaftPiece(this, pieces.front().get(), pieces, rng, nx, ny, nz, dir, depth, m_mineshaftType);
    };

    switch (m_direction) {
        case 1: // 南
            addChild(minX() + 1, minY(), maxZ() + 1, 1);
            addChild(minX() - 1, minY(), minZ() + 1, 2);
            addChild(maxX() + 1, minY(), minZ() + 1, 3);
            break;
        case 2: // 西
            addChild(minX() + 1, minY(), minZ() - 1, 0);
            addChild(minX() + 1, minY(), maxZ() + 1, 1);
            addChild(minX() - 1, minY(), minZ() + 1, 2);
            break;
        case 3: // 东
            addChild(minX() + 1, minY(), minZ() - 1, 0);
            addChild(minX() + 1, minY(), maxZ() + 1, 1);
            addChild(maxX() + 1, minY(), minZ() + 1, 3);
            break;
        case 0: // 北
        default:
            addChild(minX() + 1, minY(), minZ() - 1, 0);
            addChild(minX() - 1, minY(), minZ() + 1, 2);
            addChild(maxX() + 1, minY(), minZ() + 1, 3);
            break;
    }

    // 双层交叉点（包围盒高度大于 3）还要在上一层再开三个方向，
    // 每个方向由一次 nextBoolean 决定是否生成，共四个：
    //   1) 北 (minX+1, minY+4, minZ-1)
    //   2) 西 (minX-1, minY+4, minZ+1)
    //   3) 东 (maxX+1, minY+4, minZ+1)
    //   4) 南 (minX+1, minY+4, maxZ+1)
    //
    // 此前这里漏了第 4 个（南向）分支。少一次 nextBoolean 会让随机序列从这里
    // 开始整体错位，同时少掉南向的整棵子树。
    if (maxY() - minY() + 1 > 3) {
        const i32 upperY = minY() + 4;
        if (rng.nextBoolean()) {
            addChild(minX() + 1, upperY, minZ() - 1, 0);
        }
        if (rng.nextBoolean()) {
            addChild(minX() - 1, upperY, minZ() + 1, 2);
        }
        if (rng.nextBoolean()) {
            addChild(maxX() + 1, upperY, minZ() + 1, 3);
        }
        if (rng.nextBoolean()) {
            addChild(minX() + 1, upperY, maxZ() + 1, 1);
        }
    }
}

// ============================================================================
// MineshaftStairs 实现
// ============================================================================

MineshaftStairs::MineshaftStairs(
    i32 componentType, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, i32 direction, MineshaftType type)
    : MineshaftPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ, type)

{
    m_direction = direction;
    // 楼梯同样用局部坐标书写方块，朝向必须记录。
    setCoordBaseMode(directionFromIndex(direction));
}

void MineshaftStairs::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "mineshaft placement requires an IWorld");

    if (_isInInvalidLocation(*reader, chunkBounds)) {
        return;
    }

    // 楼梯片段：顶部平台 + 底部过渡 + 五级台阶，逐级下沉。
    const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    fillWithBlocks(world, chunkBounds, 0, 5, 0, 2, 7, 1, caveAir, caveAir, false);
    fillWithBlocks(world, chunkBounds, 0, 0, 7, 2, 2, 8, caveAir, caveAir, false);
    for (i32 i = 0; i < 5; ++i) {
        fillWithBlocks(world, chunkBounds, 0, 5 - i - (i < 4 ? 1 : 0), 2 + i, 2, 7 - i, 2 + i, caveAir, caveAir, false);
    }
}

void MineshaftStairs::buildComponent(
    std::vector<std::unique_ptr<MineshaftPiece>>& pieces, math::IRandom& rng, i32 maxDepth)
{
    MC_UNUSED(maxDepth);

    // 楼梯按朝向在正前方开一个口，坐标用包围盒的 minX / minY 直接算出，不抽随机数：
    //
    //   北 -> (minX, minY, minZ - 1) 北
    //   南 -> (minX, minY, maxZ + 1) 南
    //   西 -> (minX - 1, minY, minZ) 西
    //   东 -> (maxX + 1, minY, minZ) 东
    //
    // 此前这里用的是包围盒中点 (minX+maxX)/2 与下沉后的 minY - length/2，
    // 两处都与原版不符，导致楼梯之后的整棵子树坐标全部偏移。
    //
    // 原版 addChildren 不做深度提前返回：截断只发生在 generateAndAddPiece 里。
    const i32 depth = genDepth();

    i32 x, z;
    switch (m_direction) {
        case 1: // 南
            x = minX();
            z = maxZ() + 1;
            break;
        case 2: // 西
            x = minX() - 1;
            z = minZ();
            break;
        case 3: // 东
            x = maxX() + 1;
            z = minZ();
            break;
        case 0: // 北
        default:
            x = minX();
            z = minZ() - 1;
            break;
    }

    (void)addMineshaftPiece(this, pieces.front().get(), pieces, rng, x, minY(), z, m_direction, depth, m_mineshaftType);
}

// ============================================================================
// MineshaftStructure 实现
// ============================================================================

const std::string MineshaftStructure::m_name = "mineshaft";

const std::vector<BiomeId> MineshaftStructure::m_mesaBiomes = {
    Biomes::Badlands, Biomes::BadlandsPlateau, Biomes::ErodedBadlands, Biomes::WoodedBadlandsPlateau};

MineshaftStructure::MineshaftStructure(ResourceLocation id, MineshaftType type)
    : Structure(std::move(id))
{
    m_config.type = type;
    m_config.probability = (type == MineshaftType::Mesa) ? 0.004f : 0.004f;
}

const biome::BiomeTag* MineshaftStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_MINESHAFT();
}

std::unique_ptr<StructureStart> MineshaftStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    // 原版 MineshaftStructure.findGenerationPoint 的第一句就是
    //     context.random().nextDouble();
    // 这个值被直接丢弃，作用是推进随机序列——矿井的房间跨度、走廊长度、
    // 分支选择全部建立在它之后的状态上。漏掉它会让整座矿井的随机序列错一位，
    // 实测房间高度变成 5 而原版是 10。
    (void)rng.nextDouble();

    auto start = makeStructureStart(chunkX, chunkZ);

    // 起点固定在区块内偏移 2 的位置，且这里不消耗任何随机数。
    //
    // 原版 MineshaftStructure.generatePiecesAndAdjust 直接用
    // chunkPos.getBlockX(2) / getBlockZ(2) 建起始房间，房间的 Y 由房间自己定成 50。
    // 此前这里先 rng.nextInt(CHUNK_WIDTH) 抽两次起点、再用
    // getHeight + rng.nextInt 抽一次高度，白吃了三个随机数，
    // 使后续所有构件的方向、类型、尺寸整体错位。
    const i32 baseX = (chunkX << world::CHUNK_SHIFT) + 2;
    const i32 baseZ = (chunkZ << world::CHUNK_SHIFT) + 2;
    const i32 baseY = 50;

    // 创建起始房间
    auto room = std::make_unique<MineshaftRoom>(MineshaftPieceTypes::ROOM, rng, baseX, baseY, baseZ, m_config.type);

    // 构建矿井片段
    std::vector<std::unique_ptr<MineshaftPiece>> pieces;
    pieces.push_back(std::move(room));

    // 递归生成矿井结构
    pieces[0]->buildComponent(pieces, rng, 8); // 最大深度8

    // 将片段添加到起点
    for (auto& piece : pieces) {
        start->addPiece(std::move(piece));
    }

    start->recalculateStructureSize();

    // 把整座矿井下移到海平面以下。
    //
    // 注意 offset 只移动单个构件、不传播到子节点，所以必须先把全部构件收集起来，
    // 再统一平移。只移 start 会让房间下移、其余构件留在原处，整座矿井撕裂。
    //
    // 原版 MineshaftStructure.generatePiecesAndAdjust 末尾对非恶地类型调用
    //   moveBelowSeaLevel(seaLevel, minY, random, 10)
    // 其算法是：
    //   int i = seaLevel - padding;                       // 可用的顶面高度上限
    //   BoundingBox bb = 全部构件的包围盒;
    //   int j = bb.getYSpan() + minY + 1;                 // 贴着世界底部的顶面
    //   if (j < i) j += random.nextInt(i - j);            // 剩余空间内随机下移
    //   offsetPiecesVertically(j - bb.maxY());
    // 缺了这一步，矿井会停在房间构造时写死的 Y=50 附近，
    // 而原版的实际房间顶面在 -10 左右（整座结构被下移了 65 格）。
    // 收集全部构件（start 自己 + 它的后续链）。
    std::vector<world::gen::structure::StructurePiece*> allPieces;
    for (const auto& piece : start->pieces()) {
        allPieces.push_back(piece.get());
    }

    const i32 seaLevel = ::mc::world::SEA_LEVEL;
    const StructureBoundingBox overall = start->piecesBoundingBox();

    if (m_config.type == MineshaftType::Mesa) {
        // 恶地矿井贴着地表：以整体包围盒中心的地表高度为准，
        // 地表不高于海平面时对齐到海平面，否则在海平面与地表之间随机取一个高度。
        // 原版 generatePiecesAndAdjust 的 MESA 分支走的就是这条路径，
        // 不走 moveBelowSeaLevel。
        const i32 centerX = (overall.minX() + overall.maxX()) / 2;
        const i32 centerZ = (overall.minZ() + overall.maxZ()) / 2;
        const i32 centerY = (overall.minY() + overall.maxY()) / 2;
        const i32 surfaceY = generator.getHeight(centerX, centerZ, HeightmapType::WorldSurfaceWG);
        i32 targetY = seaLevel;
        if (surfaceY > seaLevel) {
            // Mth.randomBetweenInclusive：闭区间随机。
            targetY = seaLevel + rng.nextInt(surfaceY - seaLevel + 1);
        }
        const i32 mesaOffset = targetY - centerY;
        for (auto* piece : allPieces) {
            piece->offset(0, mesaOffset, 0);
        }
        start->recalculateStructureSize();

        // 生物群系校验（恶地分支同样要做）：原版候选点是
        // `(区块中心 X, 50 + 下移量, 区块最小 Z)`，恶地的下移量来自上面的地表对齐。
        // 漏掉这一步会让恶地矿井在任意群系都被接受。
        const i32 mesaCandidateX = chunkX * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2;
        if (!isValidBiomeAt(generator, BlockPos(mesaCandidateX, baseY + mesaOffset, chunkZ * world::CHUNK_WIDTH))) {
            start->pieces().clear();
            start->recalculateStructureSize();
        }
        return start;
    }

    // 其余类型整座下移到海平面以下。
    //
    // 原版 StructurePiecesBuilder.moveBelowSeaLevel(seaLevel, minY, random, 10)：
    //   int i = seaLevel - padding;                  // 顶面上限
    //   int j = bb.getYSpan() + minY + 1;            // 贴着世界底部的顶面
    //   if (j < i) j += random.nextInt(i - j);       // 剩余空间内随机上移
    //   offsetPiecesVertically(j - bb.maxY());
    const i32 padding = 10;
    const i32 topLimit = seaLevel - padding;
    i32 topY = (overall.maxY() - overall.minY() + 1) + world::MIN_BUILD_HEIGHT + 1;
    if (topY < topLimit) {
        topY += rng.nextInt(topLimit - topY);
    }
    const i32 seaOffset = topY - overall.maxY();
    for (auto* piece : allPieces) {
        piece->offset(0, seaOffset, 0);
    }

    // 起点包围盒必须按平移后的构件重算：`StructureStart::offset` 不传播到片段，
    // 只平移集合盒会让 STRUCTURE_STARTS 阶段报出一个悬在下移前高度的盒子
    // （构件都在 -25..-4，起点的盒子却是 40..61）。原版的 start 盒永远是片段的并集。
    start->recalculateStructureSize();

    // 生物群系校验：原版在 `findGenerationPoint` 把候选点取成
    // `(区块中心 X, 50 + 下移量, 区块最小 Z)` 之后校验。下移量必须用本次实际下移的结果。
    const i32 candidateX = chunkX * world::CHUNK_WIDTH + world::CHUNK_WIDTH / 2;
    const i32 candidateZ = chunkZ * world::CHUNK_WIDTH;
    if (!isValidBiomeAt(generator, BlockPos(candidateX, baseY + seaOffset, candidateZ))) {
        start->pieces().clear();
        start->recalculateStructureSize();
        return start;
    }

    return start;
}

} // namespace mc::world::gen::structure
