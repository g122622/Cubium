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

#include "NetherFortressPieces.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::structure::nether_fortress {

namespace {

// ============================================================================
// 方块状态与位置工具
// ============================================================================

const BlockState* netherBricks()
{
    return &VanillaBlocks::NETHER_BRICKS->defaultState();
}

const BlockState* airState()
{
    return &VanillaBlocks::AIR->defaultState();
}

const BlockState* lavaState()
{
    return &VanillaBlocks::LAVA->defaultState();
}

const BlockState* soulSandState()
{
    return &VanillaBlocks::SOUL_SAND->defaultState();
}

const BlockState* netherWartState()
{
    return &VanillaBlocks::NETHER_WART->defaultState();
}

const BlockState* spawnerState()
{
    return &VanillaBlocks::SPAWNER->defaultState();
}

/**
 * @brief 下界砖栅栏的指定连接状态
 *
 * 与原版的 `setValue(FenceBlock.NORTH/SOUTH/EAST/WEST, true)` 链等价；未打开的方向
 * 保持默认值 false。状态对象在方块的状态表里预计算，返回的是稳定引用。
 */
const BlockState* netherBrickFence(bool north, bool south, bool east, bool west)
{
    const BlockState* state = &VanillaBlocks::NETHER_BRICK_FENCE->defaultState();
    if (north) {
        state = &state->with(BlockStateProperties::NORTH(), true);
    }
    if (south) {
        state = &state->with(BlockStateProperties::SOUTH(), true);
    }
    if (east) {
        state = &state->with(BlockStateProperties::EAST(), true);
    }
    if (west) {
        state = &state->with(BlockStateProperties::WEST(), true);
    }
    return state;
}

/// 下界砖楼梯的朝向状态（原版 `setValue(StairBlock.FACING, direction)`）。
const BlockState* netherBrickStairs(Direction facing)
{
    return &VanillaBlocks::NETHER_BRICK_STAIRS->defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

/// 原版 `Util.getRandom(Direction.Plane.HORIZONTAL.faces, random)`：顺序是 N/E/S/W。
Direction randomHorizontalDirection(math::IRandom& rng)
{
    static constexpr Direction kHorizontal[] = {Direction::North, Direction::East, Direction::South, Direction::West};
    return kHorizontal[rng.nextInt(4)];
}

BlockPos worldPosOf(const StructurePiece& piece, i32 x, i32 y, i32 z)
{
    return BlockPos(piece.getXWithOffset(x, z), piece.getYWithOffset(y), piece.getZWithOffset(x, z));
}

// ============================================================================
// 权重表
// ============================================================================

std::unique_ptr<PieceWeight> makeWeight(PieceKind kind, i32 weight, i32 maxPlaceCount, bool allowInRow)
{
    return std::make_unique<PieceWeight>(PieceWeight{kind, weight, 0, maxPlaceCount, allowInRow});
}

/// 桥段权重表（原版 `BRIDGE_PIECE_WEIGHTS`）。
std::vector<std::unique_ptr<PieceWeight>> makeBridgeWeights()
{
    std::vector<std::unique_ptr<PieceWeight>> weights;
    weights.push_back(makeWeight(PieceKind::BridgeStraight, 30, 0, true));
    weights.push_back(makeWeight(PieceKind::BridgeCrossing, 10, 4, false));
    weights.push_back(makeWeight(PieceKind::RoomCrossing, 10, 4, false));
    weights.push_back(makeWeight(PieceKind::StairsRoom, 10, 3, false));
    weights.push_back(makeWeight(PieceKind::MonsterThrone, 5, 2, false));
    weights.push_back(makeWeight(PieceKind::CastleEntrance, 5, 1, false));
    return weights;
}

/// 城堡段权重表（原版 `CASTLE_PIECE_WEIGHTS`）。
std::vector<std::unique_ptr<PieceWeight>> makeCastleWeights()
{
    std::vector<std::unique_ptr<PieceWeight>> weights;
    weights.push_back(makeWeight(PieceKind::CastleSmallCorridor, 25, 0, true));
    weights.push_back(makeWeight(PieceKind::CastleSmallCorridorCrossing, 15, 5, false));
    weights.push_back(makeWeight(PieceKind::CastleSmallCorridorRightTurn, 5, 10, false));
    weights.push_back(makeWeight(PieceKind::CastleSmallCorridorLeftTurn, 5, 10, false));
    weights.push_back(makeWeight(PieceKind::CastleCorridorStairs, 10, 3, true));
    weights.push_back(makeWeight(PieceKind::CastleCorridorTBalcony, 7, 2, false));
    weights.push_back(makeWeight(PieceKind::CastleStalkRoom, 5, 2, false));
    return weights;
}

} // namespace

// ============================================================================
// 片段创建分派
// ============================================================================

std::unique_ptr<NetherBridgePiece> createBridgePiece(
    PieceKind kind, PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    std::unique_ptr<NetherBridgePiece> piece;
    switch (kind) {
        case PieceKind::BridgeStraight:
            piece = BridgeStraight::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::BridgeCrossing:
            piece = BridgeCrossing::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::RoomCrossing:
            piece = RoomCrossing::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::StairsRoom:
            piece = StairsRoom::createPiece(accessor, x, y, z, genDepth, direction);
            break;
        case PieceKind::MonsterThrone:
            piece = MonsterThrone::createPiece(accessor, x, y, z, genDepth, direction);
            break;
        case PieceKind::CastleEntrance:
            piece = CastleEntrance::createPiece(accessor, rng, x, y, z, direction, genDepth);
            break;
        case PieceKind::CastleSmallCorridor:
            piece = CastleSmallCorridorPiece::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::CastleSmallCorridorCrossing:
            piece = CastleSmallCorridorCrossingPiece::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::CastleSmallCorridorRightTurn:
            piece = CastleSmallCorridorRightTurnPiece::createPiece(accessor, rng, x, y, z, direction, genDepth);
            break;
        case PieceKind::CastleSmallCorridorLeftTurn:
            piece = CastleSmallCorridorLeftTurnPiece::createPiece(accessor, rng, x, y, z, direction, genDepth);
            break;
        case PieceKind::CastleCorridorStairs:
            piece = CastleCorridorStairsPiece::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::CastleCorridorTBalcony:
            piece = CastleCorridorTBalconyPiece::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::CastleStalkRoom:
            piece = CastleStalkRoom::createPiece(accessor, x, y, z, direction, genDepth);
            break;
        case PieceKind::BridgeEndFiller:
            piece = BridgeEndFiller::createPiece(accessor, rng, x, y, z, direction, genDepth);
            break;
    }
    return piece;
}

// ============================================================================
// NetherBridgePiece
// ============================================================================

NetherBridgePiece::NetherBridgePiece(i32 type, i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : StructurePiece(type, box.minX(), box.minY(), box.minZ(), box.maxX(), box.maxY(), box.maxZ())
{
    setCoordBaseMode(orientation);
    setGenDepth(genDepth);
}

i32 NetherBridgePiece::_updatePieceWeight(const std::vector<std::unique_ptr<PieceWeight>>& weights) const
{
    bool hasAvailable = false;
    i32 total = 0;
    for (const auto& weight : weights) {
        if (weight->maxPlaceCount > 0 && weight->placeCount < weight->maxPlaceCount) {
            hasAvailable = true;
        }
        total += weight->weight;
    }
    return hasAvailable ? total : -1;
}

std::unique_ptr<NetherBridgePiece> NetherBridgePiece::_generatePiece(StartPiece& start,
    std::vector<std::unique_ptr<PieceWeight>>& weights,
    PieceAccessor& accessor,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{
    const i32 total = _updatePieceWeight(weights);
    bool canContinue = total > 0 && depth <= MAX_DEPTH;
    i32 attempts = 0;

    while (attempts < 5 && canContinue) {
        ++attempts;
        i32 roll = rng.nextInt(total);
        for (size_t index = 0; index < weights.size(); ++index) {
            PieceWeight& weight = *weights[index];
            roll -= weight.weight;
            if (roll >= 0) {
                continue;
            }
            // 同一片段不允许连续出现（除非该种类显式允许），或该种类已达上限：本轮作废，
            // 回到外层重新抽一次（外层最多 5 次）。
            if (!weight.doPlace() || (&weight == start.previousPiece() && !weight.allowInRow)) {
                break;
            }
            auto piece = createBridgePiece(weight.kind, accessor, rng, x, y, z, direction, depth);
            if (piece != nullptr) {
                ++weight.placeCount;
                start.setPreviousPiece(&weight);
                if (!weight.isValid()) {
                    weights.erase(weights.begin() + static_cast<ptrdiff_t>(index));
                }
                return piece;
            }
            // 包围盒越界/与已有构件碰撞：继续试下一个候选种类（不再消耗随机数）。
        }
    }

    return BridgeEndFiller::createPiece(accessor, rng, x, y, z, direction, depth);
}

void NetherBridgePiece::_generateAndAddPiece(StartPiece& start,
    PieceAccessor& accessor,
    math::IRandom& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth,
    bool isCastle)
{
    const StructureBoundingBox startBox = start.boundingBox();
    if (std::abs(x - startBox.minX()) <= MAX_HORIZONTAL_RANGE &&
        std::abs(z - startBox.minZ()) <= MAX_HORIZONTAL_RANGE) {
        auto& weights = isCastle ? start.availableCastlePieces() : start.availableBridgePieces();
        auto piece = _generatePiece(start, weights, accessor, rng, x, y, z, direction, depth + 1);
        if (piece != nullptr) {
            StructurePiece* raw = piece.get();
            accessor.pieces.push_back(std::move(piece));
            accessor.pendingChildren.push_back(raw);
        }
        return;
    }

    // 超出 112 格半径：原版仍会构造一个封口片段（因此照样消耗随机数），但**不登记**它，
    // 调用方也丢弃返回值。这里保留同样的随机数消耗，不放入构件表。
    auto discarded = BridgeEndFiller::createPiece(accessor, rng, x, y, z, direction, depth);
    MC_UNUSED(discarded);
}

void NetherBridgePiece::_generateChildForward(
    StartPiece& start, PieceAccessor& accessor, math::IRandom& rng, i32 offsetX, i32 offsetY, bool isCastle)
{
    switch (orientation()) {
        case Direction::North:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                minX() + offsetX,
                minY() + offsetY,
                minZ() - 1,
                Direction::North,
                genDepth(),
                isCastle);
        case Direction::South:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                minX() + offsetX,
                minY() + offsetY,
                maxZ() + 1,
                Direction::South,
                genDepth(),
                isCastle);
        case Direction::West:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                minX() - 1,
                minY() + offsetY,
                minZ() + offsetX,
                Direction::West,
                genDepth(),
                isCastle);
        case Direction::East:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                maxX() + 1,
                minY() + offsetY,
                minZ() + offsetX,
                Direction::East,
                genDepth(),
                isCastle);
        default:
            return;
    }
}

void NetherBridgePiece::_generateChildLeft(
    StartPiece& start, PieceAccessor& accessor, math::IRandom& rng, i32 offsetY, i32 offsetZ, bool isCastle)
{
    switch (orientation()) {
        case Direction::North:
        case Direction::South:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                minX() - 1,
                minY() + offsetY,
                minZ() + offsetZ,
                Direction::West,
                genDepth(),
                isCastle);
        case Direction::West:
        case Direction::East:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                minX() + offsetZ,
                minY() + offsetY,
                minZ() - 1,
                Direction::North,
                genDepth(),
                isCastle);
        default:
            return;
    }
}

void NetherBridgePiece::_generateChildRight(
    StartPiece& start, PieceAccessor& accessor, math::IRandom& rng, i32 offsetY, i32 offsetZ, bool isCastle)
{
    switch (orientation()) {
        case Direction::North:
        case Direction::South:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                maxX() + 1,
                minY() + offsetY,
                minZ() + offsetZ,
                Direction::East,
                genDepth(),
                isCastle);
        case Direction::West:
        case Direction::East:
            return _generateAndAddPiece(start,
                accessor,
                rng,
                minX() + offsetZ,
                minY() + offsetY,
                maxZ() + 1,
                Direction::South,
                genDepth(),
                isCastle);
        default:
            return;
    }
}

void NetherBridgePiece::_fillColumnDown(
    IWorldWriter& world, const BlockState* state, i32 x, i32 y, i32 z, const StructureBoundingBox& bounds)
{
    const i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    const i32 worldZ = getZWithOffset(x, z);
    if (!bounds.isInside(worldX, worldY, worldZ)) {
        return;
    }
    IWorld* readableWorld = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(readableWorld != nullptr, "nether fortress fillColumnDown requires an IWorld-capable writer");
    fillColumnDown(*readableWorld, state, x, y, z, bounds);
}

// ============================================================================
// BridgeCrossing
// ============================================================================

BridgeCrossing::BridgeCrossing(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_BRIDGE_CROSSING, genDepth, box, orientation)
{}

BridgeCrossing::BridgeCrossing(i32 type, i32 x, i32 z, Direction orientation)
    : NetherBridgePiece(type,
          /*genDepth=*/0,
          // 起始片段的盒子来自 `StructurePiece.makeBoundingBox`，不是 `orientBox`：它只按
          // 朝向的轴交换宽/深，**不做镜像翻转**（南北向朝向后盒子仍从原点向 +Z 展开）。
          // 19×10×19 两个水平尺寸相同，因此四个朝向得到同一个盒子。
          StructureBoundingBox(x, MAGIC_START_Y, z, x + 19 - 1, MAGIC_START_Y + 10 - 1, z + 19 - 1),
          orientation)
{}

BridgeCrossing::BridgeCrossing(i32 x, i32 z, Direction orientation)
    : BridgeCrossing(PIECE_BRIDGE_CROSSING, x, z, orientation)
{}

std::unique_ptr<BridgeCrossing> BridgeCrossing::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -8, -3, 0, 19, 10, 19, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<BridgeCrossing>(genDepth, box, direction);
}

void BridgeCrossing::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 8, 3, false);
    _generateChildLeft(start, accessor, rng, 3, 8, false);
    _generateChildRight(start, accessor, rng, 3, 8, false);
}

void BridgeCrossing::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    fillWithBlocks(world, chunkBounds, 7, 3, 0, 11, 4, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 7, 18, 4, 11, bricks, bricks, false);
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 8, 5, 0, 10, 7, 18, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 8, 18, 7, 10, air, air, false);
    fillWithBlocks(world, chunkBounds, 7, 5, 0, 7, 5, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 7, 5, 11, 7, 5, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 11, 5, 0, 11, 5, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 11, 5, 11, 11, 5, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 7, 7, 5, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 11, 5, 7, 18, 5, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 11, 7, 5, 11, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 11, 5, 11, 18, 5, 11, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 7, 2, 0, 11, 2, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 7, 2, 13, 11, 2, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 7, 0, 0, 11, 1, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 7, 0, 15, 11, 1, 18, bricks, bricks, false);

    for (i32 i = 7; i <= 11; ++i) {
        for (i32 j = 0; j <= 2; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
            _fillColumnDown(world, bricks, i, -1, 18 - j, chunkBounds);
        }
    }

    fillWithBlocks(world, chunkBounds, 0, 2, 7, 5, 2, 11, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 13, 2, 7, 18, 2, 11, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 0, 7, 3, 1, 11, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 15, 0, 7, 18, 1, 11, bricks, bricks, false);

    for (i32 k = 0; k <= 2; ++k) {
        for (i32 l = 7; l <= 11; ++l) {
            _fillColumnDown(world, bricks, k, -1, l, chunkBounds);
            _fillColumnDown(world, bricks, 18 - k, -1, l, chunkBounds);
        }
    }
}

// ============================================================================
// BridgeStraight
// ============================================================================

BridgeStraight::BridgeStraight(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_BRIDGE_STRAIGHT, genDepth, box, orientation)
{}

std::unique_ptr<BridgeStraight> BridgeStraight::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -1, -3, 0, 5, 10, 19, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<BridgeStraight>(genDepth, box, direction);
}

void BridgeStraight::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 1, 3, false);
}

void BridgeStraight::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 3, 0, 4, 4, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 5, 0, 3, 7, 18, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 0, 0, 5, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 5, 0, 4, 5, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 4, 2, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 13, 4, 2, 18, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 4, 1, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 0, 15, 4, 1, 18, bricks, bricks, false);

    for (i32 i = 0; i <= 4; ++i) {
        for (i32 j = 0; j <= 2; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
            _fillColumnDown(world, bricks, i, -1, 18 - j, chunkBounds);
        }
    }

    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    const BlockState* fenceNse = netherBrickFence(true, true, true, false);
    const BlockState* fenceNsw = netherBrickFence(true, true, false, true);
    fillWithBlocks(world, chunkBounds, 0, 1, 1, 0, 4, 1, fenceNse, fenceNse, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 4, 0, 4, 4, fenceNse, fenceNse, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 14, 0, 4, 14, fenceNse, fenceNse, false);
    fillWithBlocks(world, chunkBounds, 0, 1, 17, 0, 4, 17, fenceNse, fenceNse, false);
    fillWithBlocks(world, chunkBounds, 4, 1, 1, 4, 4, 1, fenceNsw, fenceNsw, false);
    fillWithBlocks(world, chunkBounds, 4, 3, 4, 4, 4, 4, fenceNsw, fenceNsw, false);
    fillWithBlocks(world, chunkBounds, 4, 3, 14, 4, 4, 14, fenceNsw, fenceNsw, false);
    fillWithBlocks(world, chunkBounds, 4, 1, 17, 4, 4, 17, fenceNsw, fenceNsw, false);
    MC_UNUSED(fenceNs);
}

// ============================================================================
// BridgeEndFiller
// ============================================================================

BridgeEndFiller::BridgeEndFiller(
    i32 genDepth, math::IRandom& rng, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_BRIDGE_END_FILLER, genDepth, box, orientation)
    , m_selfSeed(rng.nextInt())
{}

std::unique_ptr<BridgeEndFiller> BridgeEndFiller::createPiece(
    PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -1, -3, 0, 5, 10, 8, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<BridgeEndFiller>(genDepth, rng, box, direction);
}

void BridgeEndFiller::addChildren(StartPiece& /*start*/, PieceAccessor& /*accessor*/, math::IRandom& /*rng*/)
{
    // 封口片段不再展开子分支。
}

void BridgeEndFiller::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    // 封口片段用自己保存的种子重建随机流（原版 `RandomSource.create(this.selfSeed)`），
    // 与结构生成共享的主随机流无关。
    math::JavaLegacyRandom localRng(static_cast<u64>(static_cast<i64>(m_selfSeed)));
    const BlockState* bricks = netherBricks();

    for (i32 i = 0; i <= 4; ++i) {
        for (i32 j = 3; j <= 4; ++j) {
            const i32 k = localRng.nextInt(8);
            fillWithBlocks(world, chunkBounds, i, j, 0, i, j, k, bricks, bricks, false);
        }
    }

    i32 l = localRng.nextInt(8);
    fillWithBlocks(world, chunkBounds, 0, 5, 0, 0, 5, l, bricks, bricks, false);
    l = localRng.nextInt(8);
    fillWithBlocks(world, chunkBounds, 4, 5, 0, 4, 5, l, bricks, bricks, false);

    for (i32 i1 = 0; i1 <= 4; ++i1) {
        const i32 k1 = localRng.nextInt(5);
        fillWithBlocks(world, chunkBounds, i1, 2, 0, i1, 2, k1, bricks, bricks, false);
    }

    for (i32 j1 = 0; j1 <= 4; ++j1) {
        for (i32 l1 = 0; l1 <= 1; ++l1) {
            const i32 i2 = localRng.nextInt(3);
            fillWithBlocks(world, chunkBounds, j1, l1, 0, j1, l1, i2, bricks, bricks, false);
        }
    }
}

// ============================================================================
// RoomCrossing
// ============================================================================

RoomCrossing::RoomCrossing(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_ROOM_CROSSING, genDepth, box, orientation)
{}

std::unique_ptr<RoomCrossing> RoomCrossing::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -2, 0, 0, 7, 9, 7, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<RoomCrossing>(genDepth, box, direction);
}

void RoomCrossing::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 2, 0, false);
    _generateChildLeft(start, accessor, rng, 0, 2, false);
    _generateChildRight(start, accessor, rng, 0, 2, false);
}

void RoomCrossing::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 6, 1, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 6, 7, 6, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 1, 6, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 6, 1, 6, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 2, 0, 6, 6, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 2, 6, 6, 6, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 6, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 5, 0, 6, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 2, 0, 6, 6, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 2, 5, 6, 6, 6, bricks, bricks, false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    fillWithBlocks(world, chunkBounds, 2, 6, 0, 4, 6, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 0, 4, 5, 0, fenceEw, fenceEw, false);
    fillWithBlocks(world, chunkBounds, 2, 6, 6, 4, 6, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 6, 4, 5, 6, fenceEw, fenceEw, false);
    fillWithBlocks(world, chunkBounds, 0, 6, 2, 0, 6, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 2, 0, 5, 4, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 6, 6, 2, 6, 6, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 5, 2, 6, 5, 4, fenceNs, fenceNs, false);

    for (i32 i = 0; i <= 6; ++i) {
        for (i32 j = 0; j <= 6; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
        }
    }
}

// ============================================================================
// StairsRoom
// ============================================================================

StairsRoom::StairsRoom(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_STAIRS_ROOM, genDepth, box, orientation)
{}

std::unique_ptr<StairsRoom> StairsRoom::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, i32 genDepth, Direction direction)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -2, 0, 0, 7, 11, 7, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<StairsRoom>(genDepth, box, direction);
}

void StairsRoom::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildRight(start, accessor, rng, 6, 2, false);
}

void StairsRoom::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 6, 1, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 6, 10, 6, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 1, 8, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 2, 0, 6, 8, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 1, 0, 8, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 2, 1, 6, 8, 6, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 6, 5, 8, 6, bricks, bricks, false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 2, 0, 5, 4, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 6, 3, 2, 6, 5, 2, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 6, 3, 4, 6, 5, 4, fenceNs, fenceNs, false);
    setBlockState(world, bricks, 5, 2, 5, chunkBounds);
    fillWithBlocks(world, chunkBounds, 4, 2, 5, 4, 3, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 3, 2, 5, 3, 4, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 2, 5, 2, 5, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 5, 1, 6, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 7, 1, 5, 7, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 8, 2, 6, 8, 4, air, air, false);
    fillWithBlocks(world, chunkBounds, 2, 6, 0, 4, 8, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 0, 4, 5, 0, fenceEw, fenceEw, false);

    for (i32 i = 0; i <= 6; ++i) {
        for (i32 j = 0; j <= 6; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
        }
    }
}

// ============================================================================
// MonsterThrone
// ============================================================================

MonsterThrone::MonsterThrone(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_MONSTER_THRONE, genDepth, box, orientation)
{}

std::unique_ptr<MonsterThrone> MonsterThrone::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, i32 genDepth, Direction direction)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -2, 0, 0, 7, 8, 9, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<MonsterThrone>(genDepth, box, direction);
}

void MonsterThrone::addChildren(StartPiece& /*start*/, PieceAccessor& /*accessor*/, math::IRandom& /*rng*/)
{
    // 王座房间是终点，不再展开。
}

void MonsterThrone::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 6, 7, 7, air, air, false);
    fillWithBlocks(world, chunkBounds, 1, 0, 0, 5, 1, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 1, 5, 2, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 2, 5, 3, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 4, 3, 5, 4, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 0, 1, 4, 2, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 2, 0, 5, 4, 2, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 5, 2, 1, 5, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 5, 2, 5, 5, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 3, 0, 5, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 5, 3, 6, 5, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 5, 8, 5, 5, 8, bricks, bricks, false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    setBlockState(world, netherBrickFence(false, false, false, true), 1, 6, 3, chunkBounds);
    setBlockState(world, netherBrickFence(false, false, true, false), 5, 6, 3, chunkBounds);
    setBlockState(world, netherBrickFence(true, false, true, false), 0, 6, 3, chunkBounds);
    setBlockState(world, netherBrickFence(true, false, false, true), 6, 6, 3, chunkBounds);
    fillWithBlocks(world, chunkBounds, 0, 6, 4, 0, 6, 7, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 6, 6, 4, 6, 6, 7, fenceNs, fenceNs, false);
    setBlockState(world, netherBrickFence(false, true, true, false), 0, 6, 8, chunkBounds);
    setBlockState(world, netherBrickFence(false, true, false, true), 6, 6, 8, chunkBounds);
    fillWithBlocks(world, chunkBounds, 1, 6, 8, 5, 6, 8, fenceEw, fenceEw, false);
    setBlockState(world, netherBrickFence(false, false, true, false), 1, 7, 8, chunkBounds);
    fillWithBlocks(world, chunkBounds, 2, 7, 8, 4, 7, 8, fenceEw, fenceEw, false);
    setBlockState(world, netherBrickFence(false, false, false, true), 5, 7, 8, chunkBounds);
    setBlockState(world, netherBrickFence(false, false, true, false), 2, 8, 8, chunkBounds);
    setBlockState(world, fenceEw, 3, 8, 8, chunkBounds);
    setBlockState(world, netherBrickFence(false, false, false, true), 4, 8, 8, chunkBounds);

    if (!m_hasPlacedSpawner) {
        const BlockPos spawnerPos = worldPosOf(*this, 3, 5, 5);
        if (chunkBounds.isInside(spawnerPos.x, spawnerPos.y, spawnerPos.z)) {
            m_hasPlacedSpawner = true;
            setBlockState(world, spawnerState(), 3, 5, 5, chunkBounds);
            IWorld* readableWorld = dynamic_cast<IWorld*>(&world);
            if (readableWorld != nullptr) {
                BlockEntity* blockEntity = readableWorld->getBlockEntity(spawnerPos);
                if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::MobSpawner) {
                    static_cast<blockentity::MobSpawnerBlockEntity*>(blockEntity)
                        ->setEntityId(ResourceLocation(entity::EntityTypeKeys::BLAZE), rng);
                }
            }
        }
    }

    for (i32 i = 0; i <= 6; ++i) {
        for (i32 j = 0; j <= 6; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
        }
    }
}

// ============================================================================
// CastleCorridorStairsPiece
// ============================================================================

CastleCorridorStairsPiece::CastleCorridorStairsPiece(
    i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_CORRIDOR_STAIRS, genDepth, box, orientation)
{}

std::unique_ptr<CastleCorridorStairsPiece> CastleCorridorStairsPiece::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -1, -7, 0, 5, 14, 10, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<CastleCorridorStairsPiece>(genDepth, box, direction);
}

void CastleCorridorStairsPiece::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 1, 0, true);
}

void CastleCorridorStairsPiece::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    // 楼梯与栅栏的朝向使用「未旋转」的世界朝向，由 setBlockState 按片段旋转/镜像整体变换。
    const BlockState* stairs = netherBrickStairs(Direction::South);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);

    for (i32 i = 0; i <= 9; ++i) {
        const i32 j = std::max(1, 7 - i);
        const i32 k = std::min(std::max(j + 5, 14 - i), 13);
        const i32 l = i;
        fillWithBlocks(world, chunkBounds, 0, 0, i, 4, j, i, bricks, bricks, false);
        fillWithBlocks(world, chunkBounds, 1, j + 1, i, 3, k - 1, i, air, air, false);
        if (i <= 6) {
            setBlockState(world, stairs, 1, j + 1, i, chunkBounds);
            setBlockState(world, stairs, 2, j + 1, i, chunkBounds);
            setBlockState(world, stairs, 3, j + 1, i, chunkBounds);
        }
        fillWithBlocks(world, chunkBounds, 0, k, i, 4, k, i, bricks, bricks, false);
        fillWithBlocks(world, chunkBounds, 0, j + 1, i, 0, k - 1, i, bricks, bricks, false);
        fillWithBlocks(world, chunkBounds, 4, j + 1, i, 4, k - 1, i, bricks, bricks, false);
        if ((i & 1) == 0) {
            fillWithBlocks(world, chunkBounds, 0, j + 2, i, 0, j + 3, i, fenceNs, fenceNs, false);
            fillWithBlocks(world, chunkBounds, 4, j + 2, i, 4, j + 3, i, fenceNs, fenceNs, false);
        }
        for (i32 i1 = 0; i1 <= 4; ++i1) {
            _fillColumnDown(world, bricks, i1, -1, l, chunkBounds);
        }
    }
}

// ============================================================================
// CastleCorridorTBalconyPiece
// ============================================================================

CastleCorridorTBalconyPiece::CastleCorridorTBalconyPiece(
    i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_CORRIDOR_T_BALCONY, genDepth, box, orientation)
{}

std::unique_ptr<CastleCorridorTBalconyPiece> CastleCorridorTBalconyPiece::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -3, 0, 0, 9, 7, 9, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<CastleCorridorTBalconyPiece>(genDepth, box, direction);
}

void CastleCorridorTBalconyPiece::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    i32 offsetZ = 1;
    if (orientation() == Direction::West || orientation() == Direction::North) {
        offsetZ = 5;
    }
    // 两次 nextInt(8) 分别作为实参在各自调用处求值：左侧标志抽出后，先展开左分支
    // （可能消耗多次抽签），再抽右侧标志。合成两个局部变量会打乱随机流顺序。
    _generateChildLeft(start, accessor, rng, 0, offsetZ, rng.nextInt(8) > 0);
    _generateChildRight(start, accessor, rng, 0, offsetZ, rng.nextInt(8) > 0);
}

void CastleCorridorTBalconyPiece::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 8, 1, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 8, 5, 8, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 6, 0, 8, 6, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 2, 5, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 2, 0, 8, 5, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 0, 1, 4, 0, fenceEw, fenceEw, false);
    fillWithBlocks(world, chunkBounds, 7, 3, 0, 7, 4, 0, fenceEw, fenceEw, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 4, 8, 2, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 4, 2, 2, 4, air, air, false);
    fillWithBlocks(world, chunkBounds, 6, 1, 4, 7, 2, 4, air, air, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 8, 7, 3, 8, fenceEw, fenceEw, false);
    setBlockState(world, netherBrickFence(false, true, true, false), 0, 3, 8, chunkBounds);
    setBlockState(world, netherBrickFence(false, true, false, true), 8, 3, 8, chunkBounds);
    fillWithBlocks(world, chunkBounds, 0, 3, 6, 0, 3, 7, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 8, 3, 6, 8, 3, 7, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 4, 0, 5, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 8, 3, 4, 8, 5, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 5, 2, 5, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 3, 5, 7, 5, 5, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 4, 5, 1, 5, 5, fenceEw, fenceEw, false);
    fillWithBlocks(world, chunkBounds, 7, 4, 5, 7, 5, 5, fenceEw, fenceEw, false);

    for (i32 i = 0; i <= 5; ++i) {
        for (i32 j = 0; j <= 8; ++j) {
            _fillColumnDown(world, bricks, j, -1, i, chunkBounds);
        }
    }
}

// ============================================================================
// CastleEntrance
// ============================================================================

CastleEntrance::CastleEntrance(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_ENTRANCE, genDepth, box, orientation)
{}

std::unique_ptr<CastleEntrance> CastleEntrance::createPiece(
    PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -5, -3, 0, 13, 14, 13, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    // 原版构造函数形参带随机源但未使用，这里保持签名但同样不取数。
    MC_UNUSED(rng);
    return std::make_unique<CastleEntrance>(genDepth, box, direction);
}

void CastleEntrance::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 5, 3, true);
}

void CastleEntrance::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* chunk,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 3, 0, 12, 4, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 0, 12, 13, 12, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 0, 1, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 11, 5, 0, 12, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 11, 4, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 8, 5, 11, 10, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 9, 11, 7, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 0, 4, 12, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 8, 5, 0, 10, 12, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 9, 0, 7, 12, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 11, 2, 10, 12, 10, bricks, bricks, false);
    fillWithBlocks(world,
        chunkBounds,
        5,
        8,
        0,
        7,
        8,
        0,
        &VanillaBlocks::NETHER_BRICK_FENCE->defaultState(),
        &VanillaBlocks::NETHER_BRICK_FENCE->defaultState(),
        false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);

    for (i32 i = 1; i <= 11; i += 2) {
        fillWithBlocks(world, chunkBounds, i, 10, 0, i, 11, 0, fenceEw, fenceEw, false);
        fillWithBlocks(world, chunkBounds, i, 10, 12, i, 11, 12, fenceEw, fenceEw, false);
        fillWithBlocks(world, chunkBounds, 0, 10, i, 0, 11, i, fenceNs, fenceNs, false);
        fillWithBlocks(world, chunkBounds, 12, 10, i, 12, 11, i, fenceNs, fenceNs, false);
        setBlockState(world, bricks, i, 13, 0, chunkBounds);
        setBlockState(world, bricks, i, 13, 12, chunkBounds);
        setBlockState(world, bricks, 0, 13, i, chunkBounds);
        setBlockState(world, bricks, 12, 13, i, chunkBounds);
        if (i != 11) {
            setBlockState(world, fenceEw, i + 1, 13, 0, chunkBounds);
            setBlockState(world, fenceEw, i + 1, 13, 12, chunkBounds);
            setBlockState(world, fenceNs, 0, 13, i + 1, chunkBounds);
            setBlockState(world, fenceNs, 12, 13, i + 1, chunkBounds);
        }
    }

    setBlockState(world, netherBrickFence(true, false, true, false), 0, 13, 0, chunkBounds);
    setBlockState(world, netherBrickFence(false, true, true, false), 0, 13, 12, chunkBounds);
    setBlockState(world, netherBrickFence(false, true, false, true), 12, 13, 12, chunkBounds);
    setBlockState(world, netherBrickFence(true, false, false, true), 12, 13, 0, chunkBounds);

    for (i32 k = 3; k <= 9; k += 2) {
        fillWithBlocks(world,
            chunkBounds,
            1,
            7,
            k,
            1,
            8,
            k,
            netherBrickFence(true, true, false, true),
            netherBrickFence(true, true, false, true),
            false);
        fillWithBlocks(world,
            chunkBounds,
            11,
            7,
            k,
            11,
            8,
            k,
            netherBrickFence(true, true, true, false),
            netherBrickFence(true, true, true, false),
            false);
    }

    fillWithBlocks(world, chunkBounds, 4, 2, 0, 8, 2, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 4, 12, 2, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 0, 0, 8, 1, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 0, 9, 8, 1, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 0, 4, 3, 1, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 9, 0, 4, 12, 1, 8, bricks, bricks, false);

    for (i32 l = 4; l <= 8; ++l) {
        for (i32 j = 0; j <= 2; ++j) {
            _fillColumnDown(world, bricks, l, -1, j, chunkBounds);
            _fillColumnDown(world, bricks, l, -1, 12 - j, chunkBounds);
        }
    }

    for (i32 i1 = 0; i1 <= 2; ++i1) {
        for (i32 j1 = 4; j1 <= 8; ++j1) {
            _fillColumnDown(world, bricks, i1, -1, j1, chunkBounds);
            _fillColumnDown(world, bricks, 12 - i1, -1, j1, chunkBounds);
        }
    }

    fillWithBlocks(world, chunkBounds, 5, 5, 5, 7, 5, 7, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 6, 1, 6, 6, 4, 6, air, air, false);
    setBlockState(world, bricks, 6, 0, 6, chunkBounds);
    const BlockPos lavaPos = worldPosOf(*this, 6, 5, 6);
    setBlockState(world, lavaState(), 6, 5, 6, chunkBounds);
    // TODO: 原版在该处 scheduleTick(LAVA, 0)；生成期计划刻接口尚未接入片段层，
    // 世界发布后再补（当前仅影响岩浆流动的起始时机，不影响结构形状）。
    MC_UNUSED(chunk);
    MC_UNUSED(lavaPos);
}

// ============================================================================
// CastleSmallCorridorCrossingPiece
// ============================================================================

CastleSmallCorridorCrossingPiece::CastleSmallCorridorCrossingPiece(
    i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_SMALL_CORRIDOR_CROSSING, genDepth, box, orientation)
{}

std::unique_ptr<CastleSmallCorridorCrossingPiece> CastleSmallCorridorCrossingPiece::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -1, 0, 0, 5, 7, 5, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<CastleSmallCorridorCrossingPiece>(genDepth, box, direction);
}

void CastleSmallCorridorCrossingPiece::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 1, 0, true);
    _generateChildLeft(start, accessor, rng, 0, 1, true);
    _generateChildRight(start, accessor, rng, 0, 1, true);
}

void CastleSmallCorridorCrossingPiece::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 4, 1, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 4, 5, 4, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 5, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 2, 0, 4, 5, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 4, 0, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 2, 4, 4, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 6, 0, 4, 6, 4, bricks, bricks, false);

    for (i32 i = 0; i <= 4; ++i) {
        for (i32 j = 0; j <= 4; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
        }
    }
}

// ============================================================================
// CastleSmallCorridorPiece
// ============================================================================

CastleSmallCorridorPiece::CastleSmallCorridorPiece(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_SMALL_CORRIDOR, genDepth, box, orientation)
{}

std::unique_ptr<CastleSmallCorridorPiece> CastleSmallCorridorPiece::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -1, 0, 0, 5, 7, 5, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<CastleSmallCorridorPiece>(genDepth, box, direction);
}

void CastleSmallCorridorPiece::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 1, 0, true);
}

void CastleSmallCorridorPiece::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 4, 1, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 4, 5, 4, air, air, false);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 2, 0, 4, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 1, 0, 4, 1, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 3, 0, 4, 3, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 4, 3, 1, 4, 4, 1, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 4, 3, 3, 4, 4, 3, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 0, 6, 0, 4, 6, 4, bricks, bricks, false);

    for (i32 i = 0; i <= 4; ++i) {
        for (i32 j = 0; j <= 4; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
        }
    }
}

// ============================================================================
// CastleSmallCorridorLeftTurnPiece
// ============================================================================

CastleSmallCorridorLeftTurnPiece::CastleSmallCorridorLeftTurnPiece(
    i32 genDepth, math::IRandom& rng, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_SMALL_CORRIDOR_LEFT_TURN, genDepth, box, orientation)
    , m_isNeedingChest(rng.nextInt(3) == 0)
{}

std::unique_ptr<CastleSmallCorridorLeftTurnPiece> CastleSmallCorridorLeftTurnPiece::createPiece(
    PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -1, 0, 0, 5, 7, 5, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<CastleSmallCorridorLeftTurnPiece>(genDepth, rng, box, direction);
}

void CastleSmallCorridorLeftTurnPiece::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildLeft(start, accessor, rng, 0, 1, true);
}

void CastleSmallCorridorLeftTurnPiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 4, 1, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 4, 5, 4, air, air, false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    fillWithBlocks(world, chunkBounds, 4, 2, 0, 4, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 3, 1, 4, 4, 1, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 4, 3, 3, 4, 4, 3, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 5, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 4, 3, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 4, 1, 4, 4, fenceEw, fenceEw, false);
    fillWithBlocks(world, chunkBounds, 3, 3, 4, 3, 4, 4, fenceEw, fenceEw, false);
    if (m_isNeedingChest) {
        const BlockPos chestPos = worldPosOf(*this, 3, 2, 3);
        if (chunkBounds.isInside(chestPos.x, chestPos.y, chestPos.z)) {
            m_isNeedingChest = false;
            generateChest(world, chunkBounds, rng, 3, 2, 3, ResourceLocation("minecraft:chests/nether_bridge"));
        }
    }
    fillWithBlocks(world, chunkBounds, 0, 6, 0, 4, 6, 4, bricks, bricks, false);

    for (i32 i = 0; i <= 4; ++i) {
        for (i32 j = 0; j <= 4; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
        }
    }
}

// ============================================================================
// CastleSmallCorridorRightTurnPiece
// ============================================================================

CastleSmallCorridorRightTurnPiece::CastleSmallCorridorRightTurnPiece(
    i32 genDepth, math::IRandom& rng, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_SMALL_CORRIDOR_RIGHT_TURN, genDepth, box, orientation)
    , m_isNeedingChest(rng.nextInt(3) == 0)
{}

std::unique_ptr<CastleSmallCorridorRightTurnPiece> CastleSmallCorridorRightTurnPiece::createPiece(
    PieceAccessor& accessor, math::IRandom& rng, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -1, 0, 0, 5, 7, 5, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<CastleSmallCorridorRightTurnPiece>(genDepth, rng, box, direction);
}

void CastleSmallCorridorRightTurnPiece::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildRight(start, accessor, rng, 0, 1, true);
}

void CastleSmallCorridorRightTurnPiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 4, 1, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 4, 5, 4, air, air, false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 1, 0, 4, 1, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 3, 0, 4, 3, fenceNs, fenceNs, false);
    fillWithBlocks(world, chunkBounds, 4, 2, 0, 4, 5, 0, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 4, 4, 5, 4, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 4, 1, 4, 4, fenceEw, fenceEw, false);
    fillWithBlocks(world, chunkBounds, 3, 3, 4, 3, 4, 4, fenceEw, fenceEw, false);
    if (m_isNeedingChest) {
        const BlockPos chestPos = worldPosOf(*this, 1, 2, 3);
        if (chunkBounds.isInside(chestPos.x, chestPos.y, chestPos.z)) {
            m_isNeedingChest = false;
            generateChest(world, chunkBounds, rng, 1, 2, 3, ResourceLocation("minecraft:chests/nether_bridge"));
        }
    }
    fillWithBlocks(world, chunkBounds, 0, 6, 0, 4, 6, 4, bricks, bricks, false);

    for (i32 i = 0; i <= 4; ++i) {
        for (i32 j = 0; j <= 4; ++j) {
            _fillColumnDown(world, bricks, i, -1, j, chunkBounds);
        }
    }
}

// ============================================================================
// CastleStalkRoom
// ============================================================================

CastleStalkRoom::CastleStalkRoom(i32 genDepth, const StructureBoundingBox& box, Direction orientation)
    : NetherBridgePiece(PIECE_CASTLE_STALK_ROOM, genDepth, box, orientation)
{}

std::unique_ptr<CastleStalkRoom> CastleStalkRoom::createPiece(
    PieceAccessor& accessor, i32 x, i32 y, i32 z, Direction direction, i32 genDepth)
{
    const StructureBoundingBox box = StructureBoundingBox::orientBox(x, y, z, -5, -3, 0, 13, 14, 13, direction);
    if (!isOkBox(box) || StructurePiece::findIntersecting(accessor.pieces, box) != nullptr) {
        return nullptr;
    }
    return std::make_unique<CastleStalkRoom>(genDepth, box, direction);
}

void CastleStalkRoom::addChildren(StartPiece& start, PieceAccessor& accessor, math::IRandom& rng)
{
    _generateChildForward(start, accessor, rng, 5, 3, true);
    _generateChildForward(start, accessor, rng, 5, 11, true);
}

void CastleStalkRoom::generate(IWorldWriter& world,
    math::IRandom& /*rng*/,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* bricks = netherBricks();
    const BlockState* air = airState();
    fillWithBlocks(world, chunkBounds, 0, 3, 0, 12, 4, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 0, 12, 13, 12, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 5, 0, 1, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 11, 5, 0, 12, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 11, 4, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 8, 5, 11, 10, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 9, 11, 7, 12, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 0, 4, 12, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 8, 5, 0, 10, 12, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 5, 9, 0, 7, 12, 1, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 11, 2, 10, 12, 10, bricks, bricks, false);
    const BlockState* fenceEw = netherBrickFence(false, false, true, true);
    const BlockState* fenceNs = netherBrickFence(true, true, false, false);
    const BlockState* fenceNsW = netherBrickFence(true, true, false, true);
    const BlockState* fenceNsE = netherBrickFence(true, true, true, false);

    for (i32 i = 1; i <= 11; i += 2) {
        fillWithBlocks(world, chunkBounds, i, 10, 0, i, 11, 0, fenceEw, fenceEw, false);
        fillWithBlocks(world, chunkBounds, i, 10, 12, i, 11, 12, fenceEw, fenceEw, false);
        fillWithBlocks(world, chunkBounds, 0, 10, i, 0, 11, i, fenceNs, fenceNs, false);
        fillWithBlocks(world, chunkBounds, 12, 10, i, 12, 11, i, fenceNs, fenceNs, false);
        setBlockState(world, bricks, i, 13, 0, chunkBounds);
        setBlockState(world, bricks, i, 13, 12, chunkBounds);
        setBlockState(world, bricks, 0, 13, i, chunkBounds);
        setBlockState(world, bricks, 12, 13, i, chunkBounds);
        if (i != 11) {
            setBlockState(world, fenceEw, i + 1, 13, 0, chunkBounds);
            setBlockState(world, fenceEw, i + 1, 13, 12, chunkBounds);
            setBlockState(world, fenceNs, 0, 13, i + 1, chunkBounds);
            setBlockState(world, fenceNs, 12, 13, i + 1, chunkBounds);
        }
    }

    setBlockState(world, netherBrickFence(true, false, true, false), 0, 13, 0, chunkBounds);
    setBlockState(world, netherBrickFence(false, true, true, false), 0, 13, 12, chunkBounds);
    setBlockState(world, netherBrickFence(false, true, false, true), 12, 13, 12, chunkBounds);
    setBlockState(world, netherBrickFence(true, false, false, true), 12, 13, 0, chunkBounds);

    for (i32 j1 = 3; j1 <= 9; j1 += 2) {
        fillWithBlocks(world, chunkBounds, 1, 7, j1, 1, 8, j1, fenceNsW, fenceNsW, false);
        fillWithBlocks(world, chunkBounds, 11, 7, j1, 11, 8, j1, fenceNsE, fenceNsE, false);
    }

    const BlockState* stairsNorth = netherBrickStairs(Direction::North);

    for (i32 j = 0; j <= 6; ++j) {
        const i32 k = j + 4;
        for (i32 l = 5; l <= 7; ++l) {
            setBlockState(world, stairsNorth, l, 5 + j, k, chunkBounds);
        }
        if (k >= 5 && k <= 8) {
            fillWithBlocks(world, chunkBounds, 5, 5, k, 7, j + 4, k, bricks, bricks, false);
        } else if (k >= 9 && k <= 10) {
            fillWithBlocks(world, chunkBounds, 5, 8, k, 7, j + 4, k, bricks, bricks, false);
        }
        if (j >= 1) {
            fillWithBlocks(world, chunkBounds, 5, 6 + j, k, 7, 9 + j, k, air, air, false);
        }
    }

    for (i32 k1 = 5; k1 <= 7; ++k1) {
        setBlockState(world, stairsNorth, k1, 12, 11, chunkBounds);
    }

    fillWithBlocks(world, chunkBounds, 5, 6, 7, 5, 7, 7, fenceNsE, fenceNsE, false);
    fillWithBlocks(world, chunkBounds, 7, 6, 7, 7, 7, 7, fenceNsW, fenceNsW, false);
    fillWithBlocks(world, chunkBounds, 5, 13, 12, 7, 13, 12, air, air, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 2, 3, 5, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 9, 3, 5, 10, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 2, 5, 4, 2, 5, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 9, 5, 2, 10, 5, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 9, 5, 9, 10, 5, 10, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 10, 5, 4, 10, 5, 8, bricks, bricks, false);
    const BlockState* stairsEast = netherBrickStairs(Direction::East);
    const BlockState* stairsWest = netherBrickStairs(Direction::West);
    setBlockState(world, stairsWest, 4, 5, 2, chunkBounds);
    setBlockState(world, stairsWest, 4, 5, 3, chunkBounds);
    setBlockState(world, stairsWest, 4, 5, 9, chunkBounds);
    setBlockState(world, stairsWest, 4, 5, 10, chunkBounds);
    setBlockState(world, stairsEast, 8, 5, 2, chunkBounds);
    setBlockState(world, stairsEast, 8, 5, 3, chunkBounds);
    setBlockState(world, stairsEast, 8, 5, 9, chunkBounds);
    setBlockState(world, stairsEast, 8, 5, 10, chunkBounds);
    fillWithBlocks(world, chunkBounds, 3, 4, 4, 4, 4, 8, soulSandState(), soulSandState(), false);
    fillWithBlocks(world, chunkBounds, 8, 4, 4, 9, 4, 8, soulSandState(), soulSandState(), false);
    fillWithBlocks(world, chunkBounds, 3, 5, 4, 4, 5, 8, netherWartState(), netherWartState(), false);
    fillWithBlocks(world, chunkBounds, 8, 5, 4, 9, 5, 8, netherWartState(), netherWartState(), false);
    fillWithBlocks(world, chunkBounds, 4, 2, 0, 8, 2, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 4, 12, 2, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 0, 0, 8, 1, 3, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 4, 0, 9, 8, 1, 12, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 0, 0, 4, 3, 1, 8, bricks, bricks, false);
    fillWithBlocks(world, chunkBounds, 9, 0, 4, 12, 1, 8, bricks, bricks, false);

    for (i32 l1 = 4; l1 <= 8; ++l1) {
        for (i32 i1 = 0; i1 <= 2; ++i1) {
            _fillColumnDown(world, bricks, l1, -1, i1, chunkBounds);
            _fillColumnDown(world, bricks, l1, -1, 12 - i1, chunkBounds);
        }
    }

    for (i32 i2 = 0; i2 <= 2; ++i2) {
        for (i32 j2 = 4; j2 <= 8; ++j2) {
            _fillColumnDown(world, bricks, i2, -1, j2, chunkBounds);
            _fillColumnDown(world, bricks, 12 - i2, -1, j2, chunkBounds);
        }
    }
}

// ============================================================================
// StartPiece
// ============================================================================

StartPiece::StartPiece(math::IRandom& rng, i32 x, i32 z)
    : BridgeCrossing(PIECE_START, x, z, randomHorizontalDirection(rng))
{
    m_availableBridgePieces = makeBridgeWeights();
    m_availableCastlePieces = makeCastleWeights();
}

} // namespace mc::world::gen::structure::nether_fortress
