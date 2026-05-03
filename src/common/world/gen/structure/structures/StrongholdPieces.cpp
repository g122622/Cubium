#include "StrongholdPieces.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../IWorld.hpp"
#include "../StructureBoundingBox.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/storage/ChestEntity.hpp"
#include "../../../../resource/ResourceLocation.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/Direction.hpp"
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
{
}

StrongholdPiece::Door StrongholdPiece::getRandomDoor(math::Random& rng) {
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

void StrongholdPiece::generateDoor(IWorldWriter& world, const StructureBoundingBox& bounds,
                                    math::Random& rng, Door door, i32 x, i32 y, i32 z) {
    (void)rng;  // 不需要随机数，保持接口一致

    const BlockState* stoneBricks = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* air = VanillaBlocks::getState(VanillaBlocks::AIR);
    const BlockState* ironBars = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);

    switch (door) {
        case Door::Opening:
            // 简单的 3x3 开口
            for (i32 dx = 0; dx < 3; ++dx) {
                for (i32 dy = 0; dy < 3; ++dy) {
                    setBlockState(world, air, x + dx, y + dy, z, bounds);
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
            // 木门（使用空气作为占位符）
            setBlockState(world, air, x + 1, y, z, bounds);
            break;
        }

        case Door::Grates:
            // 铁栏杆门
            setBlockState(world, air, x + 1, y, z, bounds);
            setBlockState(world, air, x + 1, y + 1, z, bounds);
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
            // 铁门（使用空气作为占位符）
            setBlockState(world, air, x + 1, y, z, bounds);
            break;
        }
    }
}

bool StrongholdPiece::canStrongholdGoDeeper(const StructureBoundingBox& box) {
    return box.minY() > 10;
}

void StrongholdPiece::generateChest(IWorldWriter& world, const StructureBoundingBox& bounds,
                                      math::Random& rng, i32 x, i32 y, i32 z,
                                      const String& lootTable) {
    // 检查边界
    i32 worldX = getXWithOffset(x, z);
    i32 worldY = getYWithOffset(y);
    i32 worldZ = getZWithOffset(x, z);

    if (bounds.isInside(worldX, worldY, worldZ)) {
        const BlockState* chest = VanillaBlocks::getState(VanillaBlocks::CHEST);
        setBlockState(world, chest, x, y, z, bounds);

        // MC 1.16.5: 设置战利品表到箱子实体
        // 参考: LockableLootTileEntity.setLootTable
        IWorld* iworld = dynamic_cast<IWorld*>(&world);
        if (iworld != nullptr && !lootTable.empty()) {
            BlockPos chestPos(worldX, worldY, worldZ);
            BlockEntity* blockEntity = iworld->getBlockEntity(chestPos);
            if (blockEntity != nullptr) {
                BlockEntityType entityType = blockEntity->getType();
                if (entityType == BlockEntityType::Chest || entityType == BlockEntityType::TrappedChest) {
                    auto* chestEntity = static_cast<blockentity::ChestEntity*>(blockEntity);
                    chestEntity->setLootTable(ResourceLocation(lootTable), rng.nextLong());
                }
            }
        }
    }
}

// ============================================================================
// StrongholdStonesSelector 实现
// ============================================================================

void StrongholdStonesSelector::selectBlocks(math::Random& rng, i32 x, i32 y, i32 z, bool isWall) {
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
            // INFESTED_STONE_BRICKS 未实现，使用普通石砖
            m_blockState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
        } else {
            m_blockState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
        }
    } else {
        m_blockState = VanillaBlocks::getState(VanillaBlocks::AIR);
    }
}

// ============================================================================
// StrongholdStraight 实现
// ============================================================================

StrongholdStraight::StrongholdStraight(i32 componentType, math::Random& rng,
                                       i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                       Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_expandsLeft(rng.nextInt(2) == 0)
    , m_expandsRight(rng.nextInt(2) == 0)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdStraight::generate(IWorldWriter& world, math::Random& rng,
                                   i32 chunkX, i32 chunkZ,
                                   const StructureBoundingBox& chunkBounds) {
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

void StrongholdStraight::buildComponent(StructurePiece* component,
                                         std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                         math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdStraight* StrongholdStraight::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 7, direction);

    if (!canStrongholdGoDeeper(box)) {
        return nullptr;
    }

    if (StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }

    return new StrongholdStraight(StrongholdPieceTypes::STRAIGHT, rng,
                                   box.minX(), box.minY(), box.minZ(),
                                   box.maxX(), box.maxY(), box.maxZ(), direction);
}

// ============================================================================
// 其他片段的简化实现
// ============================================================================

// 简化实现：Prison, LeftTurn, RightTurn, RoomCrossing, StairsStraight, Stairs, Crossing, ChestCorridor, Library, PortalRoom, Corridor

StrongholdPrison::StrongholdPrison(i32 componentType, math::Random& rng,
                                   i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                   Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdPrison::generate(IWorldWriter& world, math::Random& rng,
                                 i32 chunkX, i32 chunkZ,
                                 const StructureBoundingBox& chunkBounds) {
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

void StrongholdPrison::buildComponent(StructurePiece* component,
                                       std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                       math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdPrison* StrongholdPrison::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 9, 5, 11, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdPrison(StrongholdPieceTypes::PRISON, rng,
                                 box.minX(), box.minY(), box.minZ(),
                                 box.maxX(), box.maxY(), box.maxZ(), direction);
}

// LeftTurn
StrongholdLeftTurn::StrongholdLeftTurn(i32 componentType, math::Random& rng,
                                       i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                       Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdLeftTurn::generate(IWorldWriter& world, math::Random& rng,
                                   i32 chunkX, i32 chunkZ,
                                   const StructureBoundingBox& chunkBounds) {
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

void StrongholdLeftTurn::buildComponent(StructurePiece* component,
                                         std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                         math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdLeftTurn* StrongholdLeftTurn::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 5, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdLeftTurn(StrongholdPieceTypes::LEFT_TURN, rng,
                                   box.minX(), box.minY(), box.minZ(),
                                   box.maxX(), box.maxY(), box.maxZ(), direction);
}

// RightTurn
StrongholdRightTurn::StrongholdRightTurn(i32 componentType, math::Random& rng,
                                         i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                         Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdRightTurn::generate(IWorldWriter& world, math::Random& rng,
                                    i32 chunkX, i32 chunkZ,
                                    const StructureBoundingBox& chunkBounds) {
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

void StrongholdRightTurn::buildComponent(StructurePiece* component,
                                          std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                          math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdRightTurn* StrongholdRightTurn::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 5, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdRightTurn(StrongholdPieceTypes::RIGHT_TURN, rng,
                                    box.minX(), box.minY(), box.minZ(),
                                    box.maxX(), box.maxY(), box.maxZ(), direction);
}

// RoomCrossing
StrongholdRoomCrossing::StrongholdRoomCrossing(i32 componentType, math::Random& rng,
                                               i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                               Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_roomType(rng.nextInt(5))
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdRoomCrossing::generate(IWorldWriter& world, math::Random& rng,
                                       i32 chunkX, i32 chunkZ,
                                       const StructureBoundingBox& chunkBounds) {
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
        case 0:  // 喷泉房间
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

        case 2:  // 宝箱房间
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
            generateChest(world, chunkBounds, rng, 3, 4, 8, "minecraft:chests/stronghold_crossing");
            break;

        default:
            break;
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdRoomCrossing::buildComponent(StructurePiece* component,
                                             std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                             math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdRoomCrossing* StrongholdRoomCrossing::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 11, 7, 11, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdRoomCrossing(StrongholdPieceTypes::ROOM_CROSSING, rng,
                                       box.minX(), box.minY(), box.minZ(),
                                       box.maxX(), box.maxY(), box.maxZ(), direction);
}

// StairsStraight
StrongholdStairsStraight::StrongholdStairsStraight(i32 componentType, math::Random& rng,
                                                   i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                                   Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdStairsStraight::generate(IWorldWriter& world, math::Random& rng,
                                         i32 chunkX, i32 chunkZ,
                                         const StructureBoundingBox& chunkBounds) {
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

void StrongholdStairsStraight::buildComponent(StructurePiece* component,
                                               std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                               math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdStairsStraight* StrongholdStairsStraight::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -7, 0, 5, 11, 8, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdStairsStraight(StrongholdPieceTypes::STAIRS_STRAIGHT, rng,
                                         box.minX(), box.minY(), box.minZ(),
                                         box.maxX(), box.maxY(), box.maxZ(), direction);
}

// Stairs
StrongholdStairs::StrongholdStairs(i32 componentType, math::Random& rng,
                                   i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                   Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdStairs::generate(IWorldWriter& world, math::Random& rng,
                                 i32 chunkX, i32 chunkZ,
                                 const StructureBoundingBox& chunkBounds) {
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

void StrongholdStairs::buildComponent(StructurePiece* component,
                                       std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                       math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdStairs* StrongholdStairs::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -7, 0, 5, 11, 5, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdStairs(StrongholdPieceTypes::STAIRS, rng,
                                 box.minX(), box.minY(), box.minZ(),
                                 box.maxX(), box.maxY(), box.maxZ(), direction);
}

// StartStairs
StrongholdStartStairs::StrongholdStartStairs(math::Random& rng, i32 x, i32 z)
    : StrongholdStairs(StrongholdPieceTypes::START_STAIRS, rng,
                        x, 64, z, x + 4, 74, z + 4,
                        static_cast<Direction>(rng.nextInt(4) + 2))
{
    m_isSource = true;
    setEntryDoor(Door::Opening);
}

void StrongholdStartStairs::buildComponent(StructurePiece* component,
                                            std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                            math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

// Crossing
StrongholdCrossing::StrongholdCrossing(i32 componentType, math::Random& rng,
                                       i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
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

void StrongholdCrossing::generate(IWorldWriter& world, math::Random& rng,
                                   i32 chunkX, i32 chunkZ,
                                   const StructureBoundingBox& chunkBounds) {
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

void StrongholdCrossing::buildComponent(StructurePiece* component,
                                         std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                         math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdCrossing* StrongholdCrossing::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -3, 0, 10, 9, 11, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdCrossing(StrongholdPieceTypes::CROSSING, rng,
                                   box.minX(), box.minY(), box.minZ(),
                                   box.maxX(), box.maxY(), box.maxZ(), direction);
}

// ChestCorridor
StrongholdChestCorridor::StrongholdChestCorridor(i32 componentType, math::Random& rng,
                                                  i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                                  Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdChestCorridor::generate(IWorldWriter& world, math::Random& rng,
                                        i32 chunkX, i32 chunkZ,
                                        const StructureBoundingBox& chunkBounds) {
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
            generateChest(world, chunkBounds, rng, 3, 2, 3, "minecraft:chests/stronghold_corridor");
        }
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdChestCorridor::buildComponent(StructurePiece* component,
                                              std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                              math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdChestCorridor* StrongholdChestCorridor::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -1, -1, 0, 5, 5, 7, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdChestCorridor(StrongholdPieceTypes::CHEST_CORRIDOR, rng,
                                         box.minX(), box.minY(), box.minZ(),
                                         box.maxX(), box.maxY(), box.maxZ(), direction);
}

// Library
StrongholdLibrary::StrongholdLibrary(i32 componentType, math::Random& rng,
                                     i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                     Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_isLargeRoom((maxY - minY) > 6)
{
    setCoordBaseMode(direction);
    setEntryDoor(getRandomDoor(rng));
}

void StrongholdLibrary::generate(IWorldWriter& world, math::Random& rng,
                                  i32 chunkX, i32 chunkZ,
                                  const StructureBoundingBox& chunkBounds) {
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

    generateChest(world, chunkBounds, rng, 3, 3, 5, "minecraft:chests/stronghold_library");

    (void)chunkX;
    (void)chunkZ;
}

StrongholdLibrary* StrongholdLibrary::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 14, 11, 15, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 14, 6, 15, direction);
        if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
            return nullptr;
        }
    }
    return new StrongholdLibrary(StrongholdPieceTypes::LIBRARY, rng,
                                  box.minX(), box.minY(), box.minZ(),
                                  box.maxX(), box.maxY(), box.maxZ(), direction);
}

// PortalRoom
StrongholdPortalRoom::StrongholdPortalRoom(i32 componentType,
                                           i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                           Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
{
    setCoordBaseMode(direction);
}

void StrongholdPortalRoom::generate(IWorldWriter& world, math::Random& rng,
                                     i32 chunkX, i32 chunkZ,
                                     const StructureBoundingBox& chunkBounds) {
    StrongholdStonesSelector selector;
    const BlockState* lava = VanillaBlocks::getState(VanillaBlocks::LAVA);
    const BlockState* ironBars = VanillaBlocks::getState(VanillaBlocks::IRON_BARS);
    const BlockState* cobblestoneStairs = VanillaBlocks::getState(VanillaBlocks::COBBLESTONE_STAIRS);
    const BlockState* endPortalFrame = VanillaBlocks::getState(VanillaBlocks::END_PORTAL_FRAME);
    const BlockState* endPortal = VanillaBlocks::getState(VanillaBlocks::END_PORTAL);

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

    // 末地传送门框架
    setBlockState(world, endPortalFrame, 4, 3, 8, chunkBounds);
    setBlockState(world, endPortalFrame, 5, 3, 8, chunkBounds);
    setBlockState(world, endPortalFrame, 6, 3, 8, chunkBounds);
    setBlockState(world, endPortalFrame, 4, 3, 12, chunkBounds);
    setBlockState(world, endPortalFrame, 5, 3, 12, chunkBounds);
    setBlockState(world, endPortalFrame, 6, 3, 12, chunkBounds);
    setBlockState(world, endPortalFrame, 3, 3, 9, chunkBounds);
    setBlockState(world, endPortalFrame, 3, 3, 10, chunkBounds);
    setBlockState(world, endPortalFrame, 3, 3, 11, chunkBounds);
    setBlockState(world, endPortalFrame, 7, 3, 9, chunkBounds);
    setBlockState(world, endPortalFrame, 7, 3, 10, chunkBounds);
    setBlockState(world, endPortalFrame, 7, 3, 11, chunkBounds);

    // 末地传送门
    for (i32 px = 4; px <= 6; ++px) {
        for (i32 pz = 9; pz <= 11; ++pz) {
            setBlockState(world, endPortal, px, 3, pz, chunkBounds);
        }
    }

    (void)chunkX;
    (void)chunkZ;
}

void StrongholdPortalRoom::buildComponent(StructurePiece* component,
                                           std::vector<std::unique_ptr<StructurePiece>>& pieces,
                                           math::Random& rng) {
    (void)component;
    (void)pieces;
    (void)rng;
}

StrongholdPortalRoom* StrongholdPortalRoom::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = StructureBoundingBox::createBox(x, y, z, -4, -1, 0, 11, 8, 16, direction);
    if (!canStrongholdGoDeeper(box) || StructurePiece::findIntersecting(pieces, box) != nullptr) {
        return nullptr;
    }
    return new StrongholdPortalRoom(StrongholdPieceTypes::PORTAL_ROOM,
                                     box.minX(), box.minY(), box.minZ(),
                                     box.maxX(), box.maxY(), box.maxZ(), direction);
}

// Corridor
StrongholdCorridor::StrongholdCorridor(i32 componentType, i32 steps,
                                       i32 minX, i32 minY, i32 minZ, i32 maxX, i32 maxY, i32 maxZ,
                                       Direction direction)
    : StrongholdPiece(componentType, minX, minY, minZ, maxX, maxY, maxZ)
    , m_steps(steps)
{
    setCoordBaseMode(direction);
}

void StrongholdCorridor::generate(IWorldWriter& world, math::Random& rng,
                                   i32 chunkX, i32 chunkZ,
                                   const StructureBoundingBox& chunkBounds) {
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

StrongholdCorridor* StrongholdCorridor::createPiece(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction, i32 depth) {

    StructureBoundingBox box = findPieceBox(pieces, rng, x, y, z, direction);
    if (!box.isValid() || box.minY() <= 1) {
        return nullptr;
    }
    i32 steps = (direction == Direction::North || direction == Direction::South) ?
                (box.maxX() - box.minX() + 1) : (box.maxZ() - box.minZ() + 1);
    return new StrongholdCorridor(StrongholdPieceTypes::CORRIDOR, steps,
                                   box.minX(), box.minY(), box.minZ(),
                                   box.maxX(), box.maxY(), box.maxZ(), direction);
}

StructureBoundingBox StrongholdCorridor::findPieceBox(
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng, i32 x, i32 y, i32 z, Direction direction) {

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

void initializeStrongholdPieceWeights(std::vector<StrongholdPieceWeight>& weights) {
    weights.clear();
    weights.emplace_back(StrongholdPieceTypes::STRAIGHT, 40, 0);
    weights.emplace_back(StrongholdPieceTypes::PRISON, 5, 5);
    weights.emplace_back(StrongholdPieceTypes::LEFT_TURN, 20, 0);
    weights.emplace_back(StrongholdPieceTypes::RIGHT_TURN, 20, 0);
    weights.emplace_back(StrongholdPieceTypes::ROOM_CROSSING, 10, 6);
    weights.emplace_back(StrongholdPieceTypes::STAIRS_STRAIGHT, 5, 5);
    weights.emplace_back(StrongholdPieceTypes::STAIRS, 5, 5);
    weights.emplace_back(StrongholdPieceTypes::CROSSING, 5, 4);
    weights.emplace_back(StrongholdPieceTypes::CHEST_CORRIDOR, 5, 4);
    weights.emplace_back(StrongholdPieceTypes::LIBRARY, 10, 2);
    weights.emplace_back(StrongholdPieceTypes::PORTAL_ROOM, 20, 1);
}

StrongholdPiece* createStrongholdPiece(
    i32 pieceType,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    math::Random& rng,
    i32 x, i32 y, i32 z,
    Direction direction,
    i32 depth) {

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

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
