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

#include "StrongholdPieces.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/spawner/MobSpawnerBlockEntity.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <algorithm>
#include <cmath>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// ============================================================================
// StrongholdPiece 基类实现
// ============================================================================

StrongholdPiece::StrongholdPiece(i32 type, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ)
    : StructurePiece(type, minX, minY, minZ, maxX, maxY, maxZ)
{}

StrongholdPiece::Door StrongholdPiece::getRandomDoor(math::Random& rng)
{
    i32 value = rng.nextInt(5);
    switch (value) {
        case 0:
        case 1:
        default:
            return Door::Opening;
        case 2:
            return Door::WoodDoor;
        case 3:
            return Door::Grates;
        case 4:
            return Door::IronDoor;
    }
}

void StrongholdPiece::generateDoor(
    IWorldWriter& world, const StructureBoundingBox& bounds, math::Random& rng, Door door, i32 x, i32 y, i32 z)
{
    (void)rng; // 不需要随机数，保持接口一致

    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    const BlockState* ironBars = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);
    const BlockState* oakDoor = VanillaBlocks::getState(VanillaBlocks::OAK_DOOR);
    const BlockState* ironDoor = VanillaBlocks::getState(VanillaBlocks::IRON_DOOR);
    const BlockState* stoneButton = VanillaBlocks::getState(VanillaBlocks::STONE_BUTTON);

    switch (door) {
        case Door::Opening:
            // 简单的 3x3 开口
            for (i32 dx = 0; dx < 3; ++dx) {
                for (i32 dy = 0; dy < 3; ++dy) {
                    setBlockState(world, caveAir, x + dx, y + dy, z, bounds);
                }
            }
            break;

        case Door::WoodDoor: {
            // 石砖框架
            setBlockState(world, stoneBricks, x, y, z, bounds);
            setBlockState(world, stoneBricks, x, y + 1, z, bounds);
            setBlockState(world, stoneBricks, x, y + 2, z, bounds);
            setBlockState(world, stoneBricks, x + 1, y + 2, z, bounds);
            setBlockState(world, stoneBricks, x + 2, y + 2, z, bounds);
            setBlockState(world, stoneBricks, x + 2, y + 1, z, bounds);
            setBlockState(world, stoneBricks, x + 2, y, z, bounds);
            // 木门
            setBlockState(world, oakDoor, x + 1, y, z, bounds);
            break;
        }

        case Door::Grates:
            // 铁栏杆门
            setBlockState(world, caveAir, x + 1, y, z, bounds);
            setBlockState(world, caveAir, x + 1, y + 1, z, bounds);
            setBlockState(world, ironBars, x, y, z, bounds);
            setBlockState(world, ironBars, x, y + 1, z, bounds);
            setBlockState(world, ironBars, x, y + 2, z, bounds);
            setBlockState(world, ironBars, x + 1, y + 2, z, bounds);
            setBlockState(world, ironBars, x + 2, y + 2, z, bounds);
            setBlockState(world, ironBars, x + 2, y + 1, z, bounds);
            setBlockState(world, ironBars, x + 2, y, z, bounds);
            break;

        case Door::IronDoor: {
            // 石砖框架 + 铁门
            setBlockState(world, stoneBricks, x, y, z, bounds);
            setBlockState(world, stoneBricks, x, y + 1, z, bounds);
            setBlockState(world, stoneBricks, x, y + 2, z, bounds);
            setBlockState(world, stoneBricks, x + 1, y + 2, z, bounds);
            setBlockState(world, stoneBricks, x + 2, y + 2, z, bounds);
            setBlockState(world, stoneBricks, x + 2, y + 1, z, bounds);
            setBlockState(world, stoneBricks, x + 2, y, z, bounds);
            // 铁门
            setBlockState(world, ironDoor, x + 1, y, z, bounds);
            // 石按钮
            setBlockState(world, stoneButton, x + 2, y + 1, z + 1, bounds);
            setBlockState(world, stoneButton, x + 2, y + 1, z - 1, bounds);
            break;
        }
    }
}

bool StrongholdPiece::canStrongholdGoDeeper(const StructureBoundingBox& box)
{
    return box.minY() > 10;
}

StructurePiece* StrongholdPiece::getNextComponentNormal(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 offsetX,
    i32 offsetY)
{
    Direction direction = getCoordBaseMode();
    if (direction == Direction::None) {
        return nullptr;
    }

    i32 x = 0, y = 0, z = 0;
    Direction newDirection = direction;

    switch (direction) {
        case Direction::North:
            x = boundingBox().minX() + offsetX;
            y = boundingBox().minY() + offsetY;
            z = boundingBox().minZ() - 1;
            break;
        case Direction::South:
            x = boundingBox().minX() + offsetX;
            y = boundingBox().minY() + offsetY;
            z = boundingBox().maxZ() + 1;
            break;
        case Direction::West:
            x = boundingBox().minX() - 1;
            y = boundingBox().minY() + offsetY;
            z = boundingBox().minZ() + offsetX;
            break;
        case Direction::East:
            x = boundingBox().maxX() + 1;
            y = boundingBox().minY() + offsetY;
            z = boundingBox().minZ() + offsetX;
            break;
        default:
            return nullptr;
    }

    return generateAndAddPiece(start, pieces, rng, x, y, z, newDirection, getComponentType() + 1);
}

StructurePiece* StrongholdPiece::getNextComponentX(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 offsetX,
    i32 offsetY)
{
    Direction direction = getCoordBaseMode();
    if (direction == Direction::None) {
        return nullptr;
    }

    i32 x = 0, y = 0, z = 0;
    Direction newDirection = Direction::North;

    switch (direction) {
        case Direction::North:
            x = boundingBox().minX() - 1;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().minZ() + offsetY;
            newDirection = Direction::West;
            break;
        case Direction::South:
            x = boundingBox().minX() - 1;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().minZ() + offsetY;
            newDirection = Direction::West;
            break;
        case Direction::West:
            x = boundingBox().minX() + offsetY;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().minZ() - 1;
            newDirection = Direction::North;
            break;
        case Direction::East:
            x = boundingBox().minX() + offsetY;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().minZ() - 1;
            newDirection = Direction::North;
            break;
        default:
            return nullptr;
    }

    return generateAndAddPiece(start, pieces, rng, x, y, z, newDirection, getComponentType() + 1);
}

StructurePiece* StrongholdPiece::getNextComponentZ(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 offsetX,
    i32 offsetY)
{
    Direction direction = getCoordBaseMode();
    if (direction == Direction::None) {
        return nullptr;
    }

    i32 x = 0, y = 0, z = 0;
    Direction newDirection = Direction::East;

    switch (direction) {
        case Direction::North:
            x = boundingBox().maxX() + 1;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().minZ() + offsetY;
            newDirection = Direction::East;
            break;
        case Direction::South:
            x = boundingBox().maxX() + 1;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().minZ() + offsetY;
            newDirection = Direction::East;
            break;
        case Direction::West:
            x = boundingBox().minX() + offsetY;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().maxZ() + 1;
            newDirection = Direction::South;
            break;
        case Direction::East:
            x = boundingBox().minX() + offsetY;
            y = boundingBox().minY() + offsetX;
            z = boundingBox().maxZ() + 1;
            newDirection = Direction::South;
            break;
        default:
            return nullptr;
    }

    return generateAndAddPiece(start, pieces, rng, x, y, z, newDirection, getComponentType() + 1);
}

// ============================================================================
// StrongholdStonesSelector 实现
// ============================================================================

void StrongholdStonesSelector::selectBlocks(math::Random& rng, i32 x, i32 y, i32 z, bool isWall)
{
    (void)x;
    (void)y;
    (void)z;

    if (isWall) {
        f32 f = rng.nextFloat();
        if (f < 0.2f) {
            m_blockState = VanillaBlocks::getState(VanillaBlocks::CRACKED_STONE_BRICKS);
        } else if (f < 0.5f) {
            m_blockState = VanillaBlocks::getState(VanillaBlocks::MOSSY_STONE_BRICKS);
        } else if (f < 0.55f) {
            // 5% 概率生成被虫蚀的石砖
            m_blockState = VanillaBlocks::getState(VanillaBlocks::INFESTED_STONE_BRICKS);
        } else {
            m_blockState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
        }
    } else {
        // 要塞内部使用洞穴空气
        m_blockState = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
    }
}

// ============================================================================
// StrongholdStraight 实现
// ============================================================================

StrongholdStraight::StrongholdStraight(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_expandsLeft(rng.nextInt(2) == 0)
    , m_expandsRight(rng.nextInt(2) == 0)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdStraight::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;

    // 填充墙壁
    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 4, 4, 6, true, rng, selector);

    // 生成入口门
    generateDoor(world, chunkBounds, rng, entryDoor(), 1, 1, 0);

    // 生成出口门
    generateDoor(world, chunkBounds, rng, Door::Opening, 1, 1, 6);

    // 生成侧面开口
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    if (m_expandsLeft) {
        fillWithBlocks(world, chunkBounds, 0, 1, 2, 0, 3, 4, air, air, false);
    }
    if (m_expandsRight) {
        fillWithBlocks(world, chunkBounds, 4, 1, 2, 4, 3, 4, air, air, false);
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdStraight::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    // 正向连接
    getNextComponentNormal(start, pieces, rng, 1, 1);

    // X方向扩展
    if (m_expandsLeft) {
        getNextComponentX(start, pieces, rng, 1, 2);
    }

    // Z方向扩展
    if (m_expandsRight) {
        getNextComponentZ(start, pieces, rng, 1, 2);
    }
}

StrongholdStraight* StrongholdStraight::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 7, direction);

    if (!canStrongholdGoDeeper(box)) {
        return nullptr;
    }

    if (StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }

    return new StrongholdStraight(StrongholdPieceTypes::STRAIGHT,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// ============================================================================
// 其他片段的简化实现
// ============================================================================

// 简化实现：Prison, LeftTurn, RightTurn, RoomCrossing, StairsStraight, Stairs, Crossing, ChestCorridor, Library,
// PortalRoom, Corridor

StrongholdPrison::StrongholdPrison(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdPrison::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* ironBars = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 8, 4, 10, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 1, 1, 0);
    fillWithBlocks(world, chunkBounds, 1, 1, 10, 3, 3, 10, air, air, false);

    // 简化监狱栏杆
    for (i32 i = 1; i <= 3; ++i) {
        setBlockState(world, ironBars, 4, i, 4, chunkBounds);
        setBlockState(world, ironBars, 4, i, 5, chunkBounds);
        setBlockState(world, ironBars, 4, i, 6, chunkBounds);
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdPrison::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    getNextComponentNormal(start, pieces, rng, 1, 1);
}

StrongholdPrison* StrongholdPrison::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 9, 5, 11, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdPrison(StrongholdPieceTypes::PRISON,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// LeftTurn
StrongholdLeftTurn::StrongholdLeftTurn(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdLeftTurn::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 4, 4, 4, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 1, 1, 0);

    Direction dir = getCoordBaseMode();
    if (dir != Direction::North && dir != Direction::East) {
        fillWithBlocks(world, chunkBounds, 4, 1, 1, 4, 3, 3, air, air, false);
    } else {
        fillWithBlocks(world, chunkBounds, 0, 1, 1, 0, 3, 3, air, air, false);
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdLeftTurn::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    Direction direction = getCoordBaseMode();
    if (direction != Direction::North && direction != Direction::East) {
        getNextComponentZ(start, pieces, rng, 1, 1);
    } else {
        getNextComponentX(start, pieces, rng, 1, 1);
    }
}

StrongholdLeftTurn* StrongholdLeftTurn::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 5, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdLeftTurn(StrongholdPieceTypes::LEFT_TURN,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// RightTurn
StrongholdRightTurn::StrongholdRightTurn(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdRightTurn::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 4, 4, 4, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 1, 1, 0);

    Direction dir = getCoordBaseMode();
    if (dir != Direction::North && dir != Direction::East) {
        fillWithBlocks(world, chunkBounds, 0, 1, 1, 0, 3, 3, air, air, false);
    } else {
        fillWithBlocks(world, chunkBounds, 4, 1, 1, 4, 3, 3, air, air, false);
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdRightTurn::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    Direction direction = getCoordBaseMode();
    if (direction != Direction::North && direction != Direction::East) {
        getNextComponentX(start, pieces, rng, 1, 1);
    } else {
        getNextComponentZ(start, pieces, rng, 1, 1);
    }
}

StrongholdRightTurn* StrongholdRightTurn::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 5, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdRightTurn(StrongholdPieceTypes::RIGHT_TURN,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// RoomCrossing
StrongholdRoomCrossing::StrongholdRoomCrossing(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_roomType(rng.nextInt(5))
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdRoomCrossing::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* stoneSlab = VanillaBlocks::getState(VanillaBlocks::STONE_SLAB);
    const BlockState* cobblestone = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE);
    const BlockState* oakPlanks = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 10, 6, 10, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 4, 1, 0);
    fillWithBlocks(world, chunkBounds, 4, 1, 10, 6, 3, 10, air, air, false);
    fillWithBlocks(world, chunkBounds, 0, 1, 4, 0, 3, 6, air, air, false);
    fillWithBlocks(world, chunkBounds, 10, 1, 4, 10, 3, 6, air, air, false);

    // 根据房间类型生成不同内容
    switch (m_roomType) {
        case 0: // 喷泉房间
            for (i32 i = 0; i < 5; ++i) {
                setBlockState(world, stoneBricks, 3, 1, 3 + i, chunkBounds);
                setBlockState(world, stoneBricks, 7, 1, 3 + i, chunkBounds);
                setBlockState(world, stoneBricks, 3 + i, 1, 3, chunkBounds);
                setBlockState(world, stoneBricks, 3 + i, 1, 7, chunkBounds);
            }
            setBlockState(world, stoneBricks, 5, 1, 5, chunkBounds);
            setBlockState(world, stoneBricks, 5, 2, 5, chunkBounds);
            setBlockState(world, stoneBricks, 5, 3, 5, chunkBounds);
            break;

        case 2: // 宝箱房间
            for (i32 i = 1; i <= 9; ++i) {
                setBlockState(world, cobblestone, 1, 3, i, chunkBounds);
                setBlockState(world, cobblestone, 9, 3, i, chunkBounds);
                setBlockState(world, cobblestone, i, 3, 1, chunkBounds);
                setBlockState(world, cobblestone, i, 3, 9, chunkBounds);
            }
            for (i32 l = 2; l <= 8; ++l) {
                setBlockState(world, oakPlanks, 2, 3, l, chunkBounds);
                setBlockState(world, oakPlanks, 3, 3, l, chunkBounds);
                setBlockState(world, oakPlanks, 7, 3, l, chunkBounds);
                setBlockState(world, oakPlanks, 8, 3, l, chunkBounds);
            }
            generateChest(
                world, chunkBounds, rng, 3, 4, 8, ResourceLocation("minecraft", "chests/stronghold_crossing"));
            break;

        default:
            break;
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdRoomCrossing::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    getNextComponentNormal(start, pieces, rng, 4, 1);
    getNextComponentX(start, pieces, rng, 1, 4);
    getNextComponentZ(start, pieces, rng, 1, 4);
}

StrongholdRoomCrossing* StrongholdRoomCrossing::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 11, 7, 11, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdRoomCrossing(StrongholdPieceTypes::ROOM_CROSSING,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// StairsStraight
StrongholdStairsStraight::StrongholdStairsStraight(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdStairsStraight::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* cobblestoneStairs = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE_STAIRS);
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 4, 10, 7, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 1, 7, 0);
    generateDoor(world, chunkBounds, rng, Door::Opening, 1, 1, 7);

    for (i32 i = 0; i < 6; ++i) {
        setBlockState(world, cobblestoneStairs, 1, 6 - i, 1 + i, chunkBounds);
        setBlockState(world, cobblestoneStairs, 2, 6 - i, 1 + i, chunkBounds);
        setBlockState(world, cobblestoneStairs, 3, 6 - i, 1 + i, chunkBounds);
        if (i < 5) {
            setBlockState(world, stoneBricks, 1, 5 - i, 1 + i, chunkBounds);
            setBlockState(world, stoneBricks, 2, 5 - i, 1 + i, chunkBounds);
            setBlockState(world, stoneBricks, 3, 5 - i, 1 + i, chunkBounds);
        }
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdStairsStraight::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    getNextComponentNormal(start, pieces, rng, 1, 1);
}

StrongholdStairsStraight* StrongholdStairsStraight::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -7, 0, 5, 11, 8, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdStairsStraight(StrongholdPieceTypes::STAIRS_STRAIGHT,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// Stairs
StrongholdStairs::StrongholdStairs(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdStairs::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* stoneSlab = VanillaBlocks::getState(VanillaBlocks::STONE_SLAB);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 4, 10, 4, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 1, 7, 0);
    generateDoor(world, chunkBounds, rng, Door::Opening, 1, 1, 4);

    setBlockState(world, stoneBricks, 2, 6, 1, chunkBounds);
    setBlockState(world, stoneBricks, 1, 5, 1, chunkBounds);
    setBlockState(world, stoneSlab, 1, 6, 1, chunkBounds);
    setBlockState(world, stoneBricks, 1, 5, 2, chunkBounds);
    setBlockState(world, stoneBricks, 1, 4, 3, chunkBounds);
    setBlockState(world, stoneSlab, 1, 5, 3, chunkBounds);
    setBlockState(world, stoneBricks, 2, 4, 3, chunkBounds);
    setBlockState(world, stoneBricks, 3, 3, 3, chunkBounds);
    setBlockState(world, stoneSlab, 3, 4, 3, chunkBounds);
    setBlockState(world, stoneBricks, 3, 3, 2, chunkBounds);
    setBlockState(world, stoneBricks, 3, 2, 1, chunkBounds);
    setBlockState(world, stoneSlab, 3, 3, 1, chunkBounds);
    setBlockState(world, stoneBricks, 2, 2, 1, chunkBounds);
    setBlockState(world, stoneBricks, 1, 1, 1, chunkBounds);
    setBlockState(world, stoneSlab, 1, 2, 1, chunkBounds);
    setBlockState(world, stoneBricks, 1, 1, 2, chunkBounds);
    setBlockState(world, stoneSlab, 1, 1, 3, chunkBounds);

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdStairs::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    // 如果是起始楼梯，设置下一个组件类型为交叉点
    if (m_isSource) {
        start->setImposedPieceType(StrongholdPieceTypes::CROSSING);
    }

    getNextComponentNormal(start, pieces, rng, 1, 1);
}

StrongholdStairs* StrongholdStairs::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -7, 0, 5, 11, 5, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdStairs(StrongholdPieceTypes::STAIRS,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// StartStairs
StrongholdStartStairs::StrongholdStartStairs(math::Random& rng, i32 x, i32 z)
    : StrongholdStairs(StrongholdPieceTypes::START_STAIRS,
          rng,
          x,
          64,
          z,
          x + 4,
          74,
          z + 4,
          static_cast<Direction>(rng.nextInt(4) + 2))
{
    m_isSource = true;
    setEntryDoor(Door::Opening);
    // 初始化片段权重列表
    initializeStrongholdPieceWeights(m_weights);
}

void StrongholdStartStairs::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    StrongholdStairs::buildComponent(component, pieces, rng);
}

// Crossing
StrongholdCrossing::StrongholdCrossing(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_leftLow(rng.nextBoolean())
    , m_leftHigh(rng.nextBoolean())
    , m_rightLow(rng.nextBoolean())
    , m_rightHigh(rng.nextInt(3) > 0)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdCrossing::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 9, 8, 10, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 4, 3, 0);

    if (m_leftLow) {
        fillWithBlocks(world, chunkBounds, 0, 3, 1, 0, 5, 3, air, air, false);
    }
    if (m_rightLow) {
        fillWithBlocks(world, chunkBounds, 9, 3, 1, 9, 5, 3, air, air, false);
    }
    if (m_leftHigh) {
        fillWithBlocks(world, chunkBounds, 0, 5, 7, 0, 7, 9, air, air, false);
    }
    if (m_rightHigh) {
        fillWithBlocks(world, chunkBounds, 9, 5, 7, 9, 7, 9, air, air, false);
    }

    fillWithBlocks(world, chunkBounds, 5, 1, 10, 7, 3, 10, air, air, false);

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdCrossing::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    // 正向连接
    getNextComponentNormal(start, pieces, rng, 5, 1);

    // 根据方向计算偏移
    i32 leftLowOffset = 3;
    i32 leftHighOffset = 5;
    i32 rightLowOffset = 3;
    i32 rightHighOffset = 5;

    Direction direction = getCoordBaseMode();
    if (direction == Direction::West || direction == Direction::North) {
        leftLowOffset = 8 - leftLowOffset;
        leftHighOffset = 8 - leftHighOffset;
        rightLowOffset = 8 - rightLowOffset;
        rightHighOffset = 8 - rightHighOffset;
    }

    // 左下出口
    if (m_leftLow) {
        getNextComponentX(start, pieces, rng, leftLowOffset, 1);
    }

    // 左上出口
    if (m_leftHigh) {
        getNextComponentX(start, pieces, rng, leftHighOffset, 7);
    }

    // 右下出口
    if (m_rightLow) {
        getNextComponentZ(start, pieces, rng, rightLowOffset, 1);
    }

    // 右上出口
    if (m_rightHigh) {
        getNextComponentZ(start, pieces, rng, rightHighOffset, 7);
    }
}

StrongholdCrossing* StrongholdCrossing::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -3, 0, 10, 9, 11, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdCrossing(StrongholdPieceTypes::CROSSING,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// ChestCorridor
StrongholdChestCorridor::StrongholdChestCorridor(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdChestCorridor::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* stoneSlab = VanillaBlocks::getState(VanillaBlocks::STONE_SLAB);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 4, 4, 6, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 1, 1, 0);
    generateDoor(world, chunkBounds, rng, Door::Opening, 1, 1, 6);

    fillWithBlocks(world, chunkBounds, 3, 1, 2, 3, 1, 4, stoneBricks, stoneBricks, false);
    setBlockState(world, stoneSlab, 3, 1, 1, chunkBounds);
    setBlockState(world, stoneSlab, 3, 1, 5, chunkBounds);

    if (!m_hasChest) {
        i32 chestX = getXWithOffset(3, 3);
        i32 chestY = getYWithOffset(2);
        i32 chestZ = getZWithOffset(3, 3);
        if (chunkBounds.isInside(chestX, chestY, chestZ)) {
            m_hasChest = true;
            generateChest(
                world, chunkBounds, rng, 3, 2, 3, ResourceLocation("minecraft", "chests/stronghold_corridor"));
        }
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdChestCorridor::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start == nullptr) {
        return;
    }

    getNextComponentNormal(start, pieces, rng, 1, 1);
}

StrongholdChestCorridor* StrongholdChestCorridor::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 7, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdChestCorridor(StrongholdPieceTypes::CHEST_CORRIDOR,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// Library
StrongholdLibrary::StrongholdLibrary(i32 componentType,
    math::Random& rng,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_isLargeRoom((maxY - minY) > 6)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdLibrary::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* oakPlanks = VanillaBlocks::getState(VanillaBlocks::OAK_PLANKS);
    const BlockState* bookshelf = VanillaBlocks::getState(VanillaBlocks::BOOKSHELF);
    const BlockState* cobweb = VanillaBlocks::getState(VanillaBlocks::COBWEB);

    i32 height = m_isLargeRoom ? 11 : 6;

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 13, height - 1, 14, true, rng, selector);
    generateDoor(world, chunkBounds, rng, entryDoor(), 4, 1, 0);

    randomlyRareFillWithBlocks(world, chunkBounds, 2, 1, 1, 11, 4, 13, cobweb);

    for (i32 l = 1; l <= 13; ++l) {
        if ((l - 1) % 4 == 0) {
            fillWithBlocks(world, chunkBounds, 1, 1, l, 1, 4, l, oakPlanks, oakPlanks, false);
            fillWithBlocks(world, chunkBounds, 12, 1, l, 12, 4, l, oakPlanks, oakPlanks, false);
            if (m_isLargeRoom) {
                fillWithBlocks(world, chunkBounds, 1, 6, l, 1, 9, l, oakPlanks, oakPlanks, false);
                fillWithBlocks(world, chunkBounds, 12, 6, l, 12, 9, l, oakPlanks, oakPlanks, false);
            }
        } else {
            fillWithBlocks(world, chunkBounds, 1, 1, l, 1, 4, l, bookshelf, bookshelf, false);
            fillWithBlocks(world, chunkBounds, 12, 1, l, 12, 4, l, bookshelf, bookshelf, false);
            if (m_isLargeRoom) {
                fillWithBlocks(world, chunkBounds, 1, 6, l, 1, 9, l, bookshelf, bookshelf, false);
                fillWithBlocks(world, chunkBounds, 12, 6, l, 12, 9, l, bookshelf, bookshelf, false);
            }
        }
    }

    for (i32 l1 = 3; l1 < 12; l1 += 2) {
        fillWithBlocks(world, chunkBounds, 3, 1, l1, 4, 3, l1, bookshelf, bookshelf, false);
        fillWithBlocks(world, chunkBounds, 6, 1, l1, 7, 3, l1, bookshelf, bookshelf, false);
        fillWithBlocks(world, chunkBounds, 9, 1, l1, 10, 3, l1, bookshelf, bookshelf, false);
    }

    generateChest(world, chunkBounds, rng, 3, 3, 5, ResourceLocation("minecraft", "chests/stronghold_library"));

    // 大型图书馆在高层增加第二个宝箱
    if (m_isLargeRoom) {
        // 在 (12, 9, 1) 处放置洞穴空气以清理上方空间
        const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);
        setBlockState(world, caveAir, 12, 9, 1, chunkBounds);
        generateChest(world, chunkBounds, rng, 12, 8, 1, ResourceLocation("minecraft", "chests/stronghold_library"));
    }

    (void)chunkX;
    (void)chunkZ;
}

StrongholdLibrary* StrongholdLibrary::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 14, 11, 15, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 14, 6, 15, direction);
        if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
            return nullptr;
        }
    }
    return new StrongholdLibrary(StrongholdPieceTypes::LIBRARY,
        rng,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// PortalRoom
StrongholdPortalRoom::StrongholdPortalRoom(
    i32 componentType, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
}

void StrongholdPortalRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    StrongholdStonesSelector selector;
    const BlockState* lava = VanillaBlocks::getState(VanillaBlocks::LAVA);
    const BlockState* ironBars = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);
    const BlockState* cobblestoneStairs = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE_STAIRS);
    const BlockState* spawner = VanillaBlocks::getState(VanillaBlocks::SPAWNER);
    const BlockState* caveAir = VanillaBlocks::getState(VanillaBlocks::CAVE_AIR);

    fillWithRandomizedBlocks(world, chunkBounds, 0, 0, 0, 10, 7, 15, false, rng, selector);
    generateDoor(world, chunkBounds, rng, Door::Grates, 4, 1, 0);

    i32 i = 6;
    fillWithRandomizedBlocks(world, chunkBounds, 1, i, 1, 1, i, 14, false, rng, selector);
    fillWithRandomizedBlocks(world, chunkBounds, 9, i, 1, 9, i, 14, false, rng, selector);
    fillWithRandomizedBlocks(world, chunkBounds, 2, i, 1, 8, i, 2, false, rng, selector);
    fillWithRandomizedBlocks(world, chunkBounds, 2, i, 14, 8, i, 14, false, rng, selector);

    fillWithRandomizedBlocks(world, chunkBounds, 1, 1, 1, 2, 1, 4, false, rng, selector);
    fillWithRandomizedBlocks(world, chunkBounds, 8, 1, 1, 9, 1, 4, false, rng, selector);
    fillWithBlocks(world, chunkBounds, 1, 1, 1, 1, 1, 3, lava, lava, false);
    fillWithBlocks(world, chunkBounds, 9, 1, 1, 9, 1, 3, lava, lava, false);

    fillWithRandomizedBlocks(world, chunkBounds, 3, 1, 8, 7, 1, 12, false, rng, selector);
    fillWithBlocks(world, chunkBounds, 4, 1, 9, 6, 1, 11, lava, lava, false);

    // 铁栏杆
    for (i32 j = 3; j < 14; j += 2) {
        setBlockState(world, ironBars, 0, 3, j, chunkBounds);
        setBlockState(world, ironBars, 0, 4, j, chunkBounds);
        setBlockState(world, ironBars, 10, 3, j, chunkBounds);
        setBlockState(world, ironBars, 10, 4, j, chunkBounds);
    }

    for (i32 i1 = 2; i1 < 9; i1 += 2) {
        setBlockState(world, ironBars, i1, 3, 15, chunkBounds);
        setBlockState(world, ironBars, i1, 4, 15, chunkBounds);
    }

    fillWithRandomizedBlocks(world, chunkBounds, 4, 1, 5, 6, 1, 7, false, rng, selector);
    fillWithRandomizedBlocks(world, chunkBounds, 4, 2, 6, 6, 2, 7, false, rng, selector);
    fillWithRandomizedBlocks(world, chunkBounds, 4, 3, 7, 6, 3, 7, false, rng, selector);

    for (i32 k = 4; k <= 6; ++k) {
        setBlockState(world, cobblestoneStairs, k, 1, 4, chunkBounds);
        setBlockState(world, cobblestoneStairs, k, 2, 5, chunkBounds);
        setBlockState(world, cobblestoneStairs, k, 3, 6, chunkBounds);
    }

    // 末地传送门框架 - 带随机眼睛状态
    // MC: 每个框架有 10% 概率已有眼睛（nextFloat() > 0.9F）
    bool eyeStates[12];
    bool allEyesFilled = true;
    for (i32 eyeIdx = 0; eyeIdx < 12; ++eyeIdx) {
        eyeStates[eyeIdx] = rng.nextFloat() > 0.9f; // 10% 概率有眼睛
        if (!eyeStates[eyeIdx]) {
            allEyesFilled = false;
        }
    }

    // 放置末地传送门框架方块环
    // 传送门中心在局部坐标 (5, 3, 10)，框架围绕中心 ±2 格
    placeEndPortalFrames(world, chunkBounds, 5, 3, 10, eyeStates, allEyesFilled);

    // 蠹虫刷怪笼
    if (!m_hasSpawner) {
        i32 spawnerX = getXWithOffset(5, 6);
        i32 spawnerY = getYWithOffset(3);
        i32 spawnerZ = getZWithOffset(5, 6);
        if (chunkBounds.isInside(spawnerX, spawnerY, spawnerZ)) {
            m_hasSpawner = true;
            setBlockState(world, spawner, 5, 3, 6, chunkBounds);

            // 配置刷怪笼实体类型为蠹虫
            IWorld* iworld = dynamic_cast<IWorld*>(&world);
            if (iworld != nullptr) {
                BlockPos spawnerPos(spawnerX, spawnerY, spawnerZ);
                BlockEntity* blockEntity = iworld->getBlockEntity(spawnerPos);
                if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::MobSpawner) {
                    auto* mobSpawner = static_cast<blockentity::MobSpawnerBlockEntity*>(blockEntity);
                    mobSpawner->setEntityId(ResourceLocation(entity::EntityTypeKeys::SILVERFISH), rng);
                }
            }
        }
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdPortalRoom::buildComponent(
    StructurePiece* component, std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng)
{
    // PortalRoom 是终点，不再生成后续组件
    // 但需要将自身注册到 start 的 portalRoom 引用
    auto* start = dynamic_cast<StrongholdStartStairs*>(component);
    if (start != nullptr) {
        start->setPortalRoom(this);
    }

    (void)pieces;
    (void)rng;
}

StrongholdPortalRoom* StrongholdPortalRoom::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces, i32 x, i32 y, i32 z, Direction direction, i32 depth)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 11, 8, 16, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdPortalRoom(StrongholdPieceTypes::PORTAL_ROOM,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

// Corridor
StrongholdCorridor::StrongholdCorridor(
    i32 componentType, i32 steps, i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ, Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_steps(steps)
{
    setCoordBaseMode(direction);
}

void StrongholdCorridor::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);

    for (i32 i = 0; i < m_steps; ++i) {
        for (i32 x = 0; x < 5; ++x) {
            setBlockState(world, stoneBricks, x, 0, i, chunkBounds);
        }
        for (i32 y = 1; y <= 3; ++y) {
            setBlockState(world, stoneBricks, 0, y, i, chunkBounds);
            setBlockState(world, air, 1, y, i, chunkBounds);
            setBlockState(world, air, 2, y, i, chunkBounds);
            setBlockState(world, air, 3, y, i, chunkBounds);
            setBlockState(world, stoneBricks, 4, y, i, chunkBounds);
        }
        for (i32 x = 0; x < 5; ++x) {
            setBlockState(world, stoneBricks, x, 4, i, chunkBounds);
        }
    }

    (void)rng;
    (void)chunkX;
    (void)chunkZ;
}

StrongholdCorridor* StrongholdCorridor::createPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    StructureBoundingBox box = findPieceBox(pieces, rng, x, y, z, direction);
    if (!box.isValid() || box.minY() <= 1) {
        return nullptr;
    }
    i32 steps = (direction == Direction::North || direction == Direction::South) ? (box.maxX() - box.minX() + 1)
                                                                                 : (box.maxZ() - box.minZ() + 1);
    return new StrongholdCorridor(StrongholdPieceTypes::CORRIDOR,
        steps,
        box.minX(),
        box.minY(),
        box.minZ(),
        box.maxX(),
        box.maxY(),
        box.maxZ(),
        direction);
}

StructureBoundingBox StrongholdCorridor::findPieceBox(
    std::vector<std::unique_ptr<StructurePiece>>& pieces, math::Random& rng, i32 x, i32 y, i32 z, Direction direction)
{

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 4, direction);
    StructurePiece* intersecting = StructurePiece::findIntersecting(pieces, box);
    if (intersecting == nullptr) {
        return box;
    }
    if (intersecting->minY() == box.minY()) {
        for (i32 j = 3; j >= 1; --j) {
            box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, j - 1, direction);
            if (!intersecting->intersects(box)) {
                return StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, j, direction);
            }
        }
    }
    return StructureBoundingBox();
}

// ============================================================================
// 辅助函数实现
// ============================================================================

void initializeStrongholdPieceWeights(std::vector<StrongholdPieceWeight>& weights)
{
    weights.clear();
    // Library 需要 depth > 4
    // PortalRoom 需要 depth > 5
    weights.emplace_back(StrongholdPieceTypes::STRAIGHT, 40, 0, 0);
    weights.emplace_back(StrongholdPieceTypes::PRISON, 5, 5, 0);
    weights.emplace_back(StrongholdPieceTypes::LEFT_TURN, 20, 0, 0);
    weights.emplace_back(StrongholdPieceTypes::RIGHT_TURN, 20, 0, 0);
    weights.emplace_back(StrongholdPieceTypes::ROOM_CROSSING, 10, 6, 0);
    weights.emplace_back(StrongholdPieceTypes::STAIRS_STRAIGHT, 5, 5, 0);
    weights.emplace_back(StrongholdPieceTypes::STAIRS, 5, 5, 0);
    weights.emplace_back(StrongholdPieceTypes::CROSSING, 5, 4, 0);
    weights.emplace_back(StrongholdPieceTypes::CHEST_CORRIDOR, 5, 4, 0);
    weights.emplace_back(StrongholdPieceTypes::LIBRARY, 10, 2, 4);     // depth > 4
    weights.emplace_back(StrongholdPieceTypes::PORTAL_ROOM, 20, 1, 5); // depth > 5
}

bool canAddStructurePieces(std::vector<StrongholdPieceWeight>& weights, i32& outTotalWeight)
{
    bool canAdd = false;
    outTotalWeight = 0;

    for (auto& weight : weights) {
        // 跳过权重为0的项（已达到生成上限）
        if (weight.weight == 0) {
            continue;
        }
        if (weight.instancesLimit > 0 && weight.instancesSpawned < weight.instancesLimit) {
            canAdd = true;
        }
        outTotalWeight += weight.weight;
    }

    return canAdd;
}

StrongholdPiece* createStrongholdPiece(i32 pieceType,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{

    switch (pieceType) {
        case StrongholdPieceTypes::STRAIGHT:
            return StrongholdStraight::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::PRISON:
            return StrongholdPrison::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::LEFT_TURN:
            return StrongholdLeftTurn::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::RIGHT_TURN:
            return StrongholdRightTurn::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::ROOM_CROSSING:
            return StrongholdRoomCrossing::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::STAIRS_STRAIGHT:
            return StrongholdStairsStraight::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::STAIRS:
            return StrongholdStairs::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::CROSSING:
            return StrongholdCrossing::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::CHEST_CORRIDOR:
            return StrongholdChestCorridor::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::LIBRARY:
            return StrongholdLibrary::createPiece(pieces, rng, x, y, z, direction, depth);
        case StrongholdPieceTypes::PORTAL_ROOM:
            return StrongholdPortalRoom::createPiece(pieces, x, y, z, direction, depth);
        case StrongholdPieceTypes::CORRIDOR:
            return StrongholdCorridor::createPiece(pieces, rng, x, y, z, direction, depth);
        default:
            return nullptr;
    }
}

StrongholdPiece* generatePieceFromSmallDoor(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth,
    std::vector<StrongholdPieceWeight>& weights,
    StrongholdPieceWeight*& lastPlaced)
{
    // 强制片段类型机制：如果有强制片段类型，优先创建
    if (start != nullptr && start->imposedPieceType() >= 0) {
        i32 imposedType = start->imposedPieceType();
        start->setImposedPieceType(-1); // 消费强制类型

        StrongholdPiece* piece = createStrongholdPiece(imposedType, pieces, rng, x, y, z, direction, depth);
        if (piece != nullptr) {
            // 找到对应的权重并更新计数
            for (auto& weight : weights) {
                if (weight.pieceType == imposedType) {
                    weight.instancesSpawned++;
                    lastPlaced = &weight;
                    // 如果达到限制，从列表中移除
                    if (!weight.canSpawnMoreStructures()) {
                        // 使用 swap-and-pop 惯用法移除，避免迭代器失效
                        // 注意：lastPlaced 此时指向被移除元素的引用，需要置空
                        if (lastPlaced == &weight) {
                            lastPlaced = nullptr;
                        }
                        weight.weight = 0; // 将权重设为0，使其不再被选中
                    }
                    break;
                }
            }
            return piece;
        }
    }

    i32 totalWeight = 0;
    if (!canAddStructurePieces(weights, totalWeight)) {
        return nullptr;
    }

    // 尝试最多 5 次
    for (i32 attempt = 0; attempt < 5; ++attempt) {
        i32 randomValue = rng.nextInt(totalWeight);

        for (auto& weight : weights) {
            randomValue -= weight.weight;
            if (randomValue < 0) {
                // 跳过权重为0的片段（已达到生成上限）
                if (weight.weight == 0) {
                    continue;
                }
                // 检查是否可以生成该类型的片段
                if (!weight.canSpawnMoreStructuresOfType(depth)) {
                    break;
                }
                // 跳过上一个放置的片段类型，避免连续生成相同类型
                if (lastPlaced != nullptr && &weight == lastPlaced) {
                    break;
                }

                StrongholdPiece* piece =
                    createStrongholdPiece(weight.pieceType, pieces, rng, x, y, z, direction, depth);

                if (piece != nullptr) {
                    weight.instancesSpawned++;
                    lastPlaced = &weight;

                    // 如果达到限制，将权重设为0使其不再被选中
                    // 将权重置0使其不再被选中（等效于从列表中移除）
                    // 使用权重置0而非移除，避免迭代器失效和 lastPlaced 悬垂指针问题
                    if (!weight.canSpawnMoreStructures()) {
                        weight.weight = 0;
                        if (lastPlaced == &weight) {
                            lastPlaced = nullptr;
                        }
                    }

                    return piece;
                }
            }
        }
    }

    // 如果所有尝试都失败，创建填充走廊
    StructureBoundingBox box = StrongholdCorridor::findPieceBox(pieces, rng, x, y, z, direction);
    if (box.isValid() && box.minY() > 1) {
        i32 steps = (direction == Direction::North || direction == Direction::South) ? (box.maxX() - box.minX() + 1)
                                                                                     : (box.maxZ() - box.minZ() + 1);
        return new StrongholdCorridor(StrongholdPieceTypes::CORRIDOR,
            steps,
            box.minX(),
            box.minY(),
            box.minZ(),
            box.maxX(),
            box.maxY(),
            box.maxZ(),
            direction);
    }

    return nullptr;
}

StructurePiece* generateAndAddPiece(StrongholdStartStairs* start,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x,
    i32 y,
    i32 z,
    Direction direction,
    i32 depth)
{
    // 深度限制：最多 50 层
    if (depth > 50) {
        return nullptr;
    }

    // 距离限制：距离起点不超过 112 格
    if (std::abs(x - start->boundingBox().minX()) > 112 || std::abs(z - start->boundingBox().minZ()) > 112) {
        return nullptr;
    }

    // 使用 StrongholdStartStairs 成员变量存储状态（替代 thread_local 静态变量）
    std::vector<StrongholdPieceWeight>& weights = start->weights();
    StrongholdPieceWeight*& lastPlaced = start->lastPlacedRef();

    StrongholdPiece* piece =
        generatePieceFromSmallDoor(start, pieces, rng, x, y, z, direction, depth, weights, lastPlaced);

    if (piece != nullptr) {
        pieces.emplace_back(piece);
        start->addPendingChild(piece);
    }

    return piece;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
