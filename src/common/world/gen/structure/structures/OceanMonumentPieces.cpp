#include "common/world/gen/structure/structures/OceanMonumentPieces.hpp"
#include "common/entity/combat/DifficultyInstance.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/ocean/ElderGuardianEntity.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <array>

namespace mc {
namespace world {
namespace gen {
namespace structure {

namespace {

constexpr std::array<Direction, 6> ROOM_DIRECTIONS = {
    Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

[[nodiscard]] bool isSpecialRoomIndex(i32 index)
{
    return index >= 75;
}

[[nodiscard]] StructureBoundingBox createRoomBox(
    Direction direction, OceanMonumentRoomDefinition* room, i32 sizeX, i32 sizeY, i32 sizeZ)
{
    MC_ASSERT_RELEASE(room != nullptr);
    const i32 index = room->getIndex();
    const i32 gridY = index / 25;
    const i32 gridRem = index % 25;
    const i32 gridZ = gridRem / 5;
    const i32 gridX = gridRem % 5;

    const i32 minX = gridX * 8;
    const i32 minY = gridY * 4;
    const i32 minZ = gridZ * 8;
    const i32 maxX = minX + sizeX * 8 - 1;
    const i32 maxY = minY + sizeY * 4 - 1;
    const i32 maxZ = minZ + sizeZ * 8 - 1;

    StructureBoundingBox box(minX, minY, minZ, maxX, maxY, maxZ);
    (void)direction;
    return box;
}

} // namespace

const BlockState* OceanMonumentPiece::s_roughPrismarine = nullptr;
const BlockState* OceanMonumentPiece::s_bricksPrismarine = nullptr;
const BlockState* OceanMonumentPiece::s_darkPrismarine = nullptr;
const BlockState* OceanMonumentPiece::s_seaLantern = nullptr;
const BlockState* OceanMonumentPiece::s_water = nullptr;

OceanMonumentRoomDefinition::OceanMonumentRoomDefinition(i32 index)
    : m_index(index)
{}

void OceanMonumentRoomDefinition::setConnection(i32 direction, OceanMonumentRoomDefinition* room)
{
    MC_ASSERT_RELEASE(direction >= 0 && direction < 6);
    m_connections[direction] = room;
}

OceanMonumentRoomDefinition* OceanMonumentRoomDefinition::getConnection(i32 direction) const
{
    MC_ASSERT_RELEASE(direction >= 0 && direction < 6);
    return m_connections[direction];
}

const OceanMonumentRoomDefinition* OceanMonumentRoomDefinition::getConnectionConst(i32 direction) const
{
    MC_ASSERT_RELEASE(direction >= 0 && direction < 6);
    return m_connections[direction];
}

void OceanMonumentRoomDefinition::updateOpenings()
{
    for (i32 direction = 0; direction < 6; ++direction) {
        m_hasOpening[direction] = m_connections[direction] != nullptr;
    }
}

bool OceanMonumentRoomDefinition::findSource(i32 scanIndex)
{
    if (m_isSource) {
        return true;
    }
    m_scanIndex = scanIndex;
    for (i32 direction = 0; direction < 6; ++direction) {
        if (!m_hasOpening[direction]) {
            continue;
        }
        OceanMonumentRoomDefinition* connection = m_connections[direction];
        if (connection == nullptr || connection->m_scanIndex == scanIndex) {
            continue;
        }
        if (connection->findSource(scanIndex)) {
            return true;
        }
    }
    return false;
}

bool OceanMonumentRoomDefinition::isSpecial() const
{
    return isSpecialRoomIndex(m_index);
}

i32 OceanMonumentRoomDefinition::countOpenings() const
{
    i32 count = 0;
    for (bool opening : m_hasOpening) {
        count += opening ? 1 : 0;
    }
    return count;
}

bool OceanMonumentRoomDefinition::hasOpening(i32 direction) const
{
    MC_ASSERT_RELEASE(direction >= 0 && direction < 6);
    return m_hasOpening[direction];
}

void OceanMonumentRoomDefinition::setHasOpening(i32 direction, bool opening)
{
    MC_ASSERT_RELEASE(direction >= 0 && direction < 6);
    m_hasOpening[direction] = opening;
}

OceanMonumentPiece::OceanMonumentPiece(i32 type, Direction direction)
    : StructurePiece(type, 0, 39, 0, 0, 39, 0)
{
    setCoordBaseMode(direction);
    if (s_roughPrismarine == nullptr) {
        s_roughPrismarine = &VanillaBlocks::PRISMARINE->defaultState();
        s_bricksPrismarine = &VanillaBlocks::PRISMARINE_BRICKS->defaultState();
        s_darkPrismarine = &VanillaBlocks::DARK_PRISMARINE->defaultState();
        s_seaLantern = &VanillaBlocks::SEA_LANTERN->defaultState();
        s_water = &VanillaBlocks::WATER->defaultState();
    }
}

OceanMonumentPiece::OceanMonumentPiece(i32 type, Direction direction, const StructureBoundingBox& bounds)
    : StructurePiece(type, bounds.minX(), bounds.minY(), bounds.minZ(), bounds.maxX(), bounds.maxY(), bounds.maxZ())
{
    setCoordBaseMode(direction);
    if (s_roughPrismarine == nullptr) {
        s_roughPrismarine = &VanillaBlocks::PRISMARINE->defaultState();
        s_bricksPrismarine = &VanillaBlocks::PRISMARINE_BRICKS->defaultState();
        s_darkPrismarine = &VanillaBlocks::DARK_PRISMARINE->defaultState();
        s_seaLantern = &VanillaBlocks::SEA_LANTERN->defaultState();
        s_water = &VanillaBlocks::WATER->defaultState();
    }
}

void OceanMonumentPiece::generateDefaultFloor(
    IWorldWriter& world, const StructureBoundingBox& bounds, i32 x, i32 z, bool hasOpeningDownwards)
{
    if (hasOpeningDownwards) {
        fillWithBlocks(world, bounds, x + 0, 0, z + 0, x + 2, 0, z + 7, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, bounds, x + 5, 0, z + 0, x + 7, 0, z + 7, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, bounds, x + 3, 0, z + 0, x + 4, 0, z + 2, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, bounds, x + 3, 0, z + 5, x + 4, 0, z + 7, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, bounds, x + 3, 0, z + 2, x + 4, 0, z + 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, bounds, x + 3, 0, z + 5, x + 4, 0, z + 5, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, bounds, x + 2, 0, z + 3, x + 2, 0, z + 4, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, bounds, x + 5, 0, z + 3, x + 5, 0, z + 4, s_bricksPrismarine, s_bricksPrismarine, false);
        return;
    }

    fillWithBlocks(world, bounds, x, 0, z, x + 7, 0, z + 7, s_roughPrismarine, s_roughPrismarine, false);
}

void OceanMonumentPiece::generateBoxOnFillOnly(IWorldWriter& world,
    const StructureBoundingBox& bounds,
    i32 minX,
    i32 minY,
    i32 minZ,
    i32 maxX,
    i32 maxY,
    i32 maxZ,
    const BlockState* state)
{
    IWorld* readableWorld = dynamic_cast<IWorld*>(&world);
    if (readableWorld == nullptr) {
        fillWithBlocks(world, bounds, minX, minY, minZ, maxX, maxY, maxZ, state, state, false);
        return;
    }

    for (i32 y = minY; y <= maxY; ++y) {
        for (i32 x = minX; x <= maxX; ++x) {
            for (i32 z = minZ; z <= maxZ; ++z) {
                if (getBlockStateFromPos(*readableWorld, x, y, z, bounds) == s_water) {
                    setBlockState(world, state, x, y, z, bounds);
                }
            }
        }
    }
}

void OceanMonumentPiece::makeOpening(
    IWorldWriter& world, const StructureBoundingBox& bounds, i32 x1, i32 y1, i32 z1, i32 x2, i32 y2, i32 z2)
{
    IWorld* readableWorld = dynamic_cast<IWorld*>(&world);
    if (readableWorld == nullptr) {
        fillWithBlocks(world, bounds, x1, y1, z1, x2, y2, z2, s_water, s_water, false);
        return;
    }

    for (i32 y = y1; y <= y2; ++y) {
        for (i32 x = x1; x <= x2; ++x) {
            for (i32 z = z1; z <= z2; ++z) {
                const BlockState* current = getBlockStateFromPos(*readableWorld, x, y, z, bounds);
                if (current != s_roughPrismarine && current != s_bricksPrismarine && current != s_darkPrismarine &&
                    current != s_seaLantern) {
                    setBlockState(world, s_water, x, y, z, bounds);
                }
            }
        }
    }
}

bool OceanMonumentPiece::doesChunkIntersect(
    const StructureBoundingBox& chunkBounds, i32 x1, i32 z1, i32 x2, i32 z2) const
{
    StructureBoundingBox box(getXWithOffset(x1, z1),
        getYWithOffset(0),
        getZWithOffset(x1, z1),
        getXWithOffset(x2, z2),
        getYWithOffset(8),
        getZWithOffset(x2, z2));
    return box.intersects(chunkBounds);
}

bool OceanMonumentPiece::spawnElderGuardian(
    IWorldWriter& world, const StructureBoundingBox& bounds, i32 x, i32 y, i32 z)
{
    IWorld* fullWorld = dynamic_cast<IWorld*>(&world);
    if (fullWorld == nullptr) {
        return false;
    }
    auto elder = ElderGuardianEntity::create(fullWorld);
    auto* entity = dynamic_cast<ElderGuardianEntity*>(elder.get());
    if (entity == nullptr) {
        return false;
    }
    const f32 worldX = static_cast<f32>(getXWithOffset(x, z)) + 0.5f;
    const f32 worldY = static_cast<f32>(getYWithOffset(y));
    const f32 worldZ = static_cast<f32>(getZWithOffset(x, z)) + 0.5f;
    if (!bounds.contains(static_cast<i32>(worldX), static_cast<i32>(worldY), static_cast<i32>(worldZ))) {
        return false;
    }
    entity->setPosition(worldX, worldY, worldZ);
    entity->setRotation(0.0f, 0.0f);

    // 对 MobEntity 调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
    auto* mobEntity = dynamic_cast<MobEntity*>(elder.get());
    if (mobEntity != nullptr) {
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*fullWorld,
            BlockPos(
                static_cast<i32>(std::floor(worldX)), static_cast<i32>(worldY), static_cast<i32>(std::floor(worldZ))));
        mobEntity->finalizeSpawn(*fullWorld, difficultyInstance, world::spawn::SpawnReason::Structure);
    }

    return fullWorld->spawnEntity(std::move(elder)) != EntityInstanceId(0);
}

OceanMonumentDoubleXRoom::OceanMonumentDoubleXRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::DOUBLE_X_ROOM, direction, createRoomBox(direction, room, 2, 1, 1))
{
    setRoomDefinition(room);
}

void OceanMonumentDoubleXRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    OceanMonumentRoomDefinition* eastRoom = room->getConnection(5);
    MC_ASSERT_RELEASE(room != nullptr);
    MC_ASSERT_RELEASE(eastRoom != nullptr);

    if (room->getIndex() / 25 > 0) {
        generateDefaultFloor(world, chunkBounds, 8, 0, eastRoom->hasOpening(0));
        generateDefaultFloor(world, chunkBounds, 0, 0, room->hasOpening(0));
    }

    if (room->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 4, 1, 7, 4, 6, s_roughPrismarine);
    }
    if (eastRoom->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 8, 4, 1, 14, 4, 6, s_roughPrismarine);
    }

    fillWithBlocks(world, chunkBounds, 0, 3, 0, 0, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 15, 3, 0, 15, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 0, 15, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 7, 14, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 15, 2, 0, 15, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 0, 15, 2, 0, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 7, 14, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 1, 0, 0, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 15, 1, 0, 15, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 0, 15, 1, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 7, 14, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 1, 0, 10, 1, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 2, 0, 9, 2, 3, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 3, 0, 10, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    setBlockState(world, s_seaLantern, 6, 2, 3, chunkBounds);
    setBlockState(world, s_seaLantern, 9, 2, 3, chunkBounds);

    if (room->hasOpening(3)) makeOpening(world, chunkBounds, 3, 1, 0, 4, 2, 0);
    if (room->hasOpening(2)) makeOpening(world, chunkBounds, 3, 1, 7, 4, 2, 7);
    if (room->hasOpening(4)) makeOpening(world, chunkBounds, 0, 1, 3, 0, 2, 4);
    if (eastRoom->hasOpening(3)) makeOpening(world, chunkBounds, 11, 1, 0, 12, 2, 0);
    if (eastRoom->hasOpening(2)) makeOpening(world, chunkBounds, 11, 1, 7, 12, 2, 7);
    if (eastRoom->hasOpening(5)) makeOpening(world, chunkBounds, 15, 1, 3, 15, 2, 4);
}

OceanMonumentDoubleXYRoom::OceanMonumentDoubleXYRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::DOUBLE_XY_ROOM, direction, createRoomBox(direction, room, 2, 2, 1))
{
    setRoomDefinition(room);
}

void OceanMonumentDoubleXYRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    OceanMonumentRoomDefinition* eastRoom = room->getConnection(5);
    MC_ASSERT_RELEASE(room != nullptr);
    MC_ASSERT_RELEASE(eastRoom != nullptr);
    OceanMonumentRoomDefinition* upRoom = room->getConnection(1);
    OceanMonumentRoomDefinition* eastUpRoom = eastRoom->getConnection(1);
    MC_ASSERT_RELEASE(upRoom != nullptr);
    MC_ASSERT_RELEASE(eastUpRoom != nullptr);

    if (room->getIndex() / 25 > 0) {
        generateDefaultFloor(world, chunkBounds, 8, 0, eastRoom->hasOpening(0));
        generateDefaultFloor(world, chunkBounds, 0, 0, room->hasOpening(0));
    }
    if (upRoom->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 8, 1, 7, 8, 6, s_roughPrismarine);
    }
    if (eastUpRoom->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 8, 8, 1, 14, 8, 6, s_roughPrismarine);
    }

    for (i32 i = 1; i <= 7; ++i) {
        const BlockState* block = (i == 2 || i == 6) ? s_roughPrismarine : s_bricksPrismarine;
        fillWithBlocks(world, chunkBounds, 0, i, 0, 0, i, 7, block, block, false);
        fillWithBlocks(world, chunkBounds, 15, i, 0, 15, i, 7, block, block, false);
        fillWithBlocks(world, chunkBounds, 1, i, 0, 15, i, 0, block, block, false);
        fillWithBlocks(world, chunkBounds, 1, i, 7, 14, i, 7, block, block, false);
    }

    fillWithBlocks(world, chunkBounds, 2, 1, 3, 2, 7, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 1, 2, 4, 7, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 1, 5, 4, 7, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 13, 1, 3, 13, 7, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 11, 1, 2, 12, 7, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 11, 1, 5, 12, 7, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 1, 3, 5, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 1, 3, 10, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 7, 2, 10, 7, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 5, 2, 5, 7, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 5, 2, 10, 7, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 5, 5, 5, 7, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 5, 5, 10, 7, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    setBlockState(world, s_bricksPrismarine, 6, 6, 2, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 9, 6, 2, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 6, 6, 5, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 9, 6, 5, chunkBounds);
    fillWithBlocks(world, chunkBounds, 5, 4, 3, 6, 4, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 9, 4, 3, 10, 4, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    setBlockState(world, s_seaLantern, 5, 4, 2, chunkBounds);
    setBlockState(world, s_seaLantern, 5, 4, 5, chunkBounds);
    setBlockState(world, s_seaLantern, 10, 4, 2, chunkBounds);
    setBlockState(world, s_seaLantern, 10, 4, 5, chunkBounds);

    if (room->hasOpening(3)) makeOpening(world, chunkBounds, 3, 1, 0, 4, 2, 0);
    if (room->hasOpening(2)) makeOpening(world, chunkBounds, 3, 1, 7, 4, 2, 7);
    if (room->hasOpening(4)) makeOpening(world, chunkBounds, 0, 1, 3, 0, 2, 4);
    if (eastRoom->hasOpening(3)) makeOpening(world, chunkBounds, 11, 1, 0, 12, 2, 0);
    if (eastRoom->hasOpening(2)) makeOpening(world, chunkBounds, 11, 1, 7, 12, 2, 7);
    if (eastRoom->hasOpening(5)) makeOpening(world, chunkBounds, 15, 1, 3, 15, 2, 4);
    if (upRoom->hasOpening(3)) makeOpening(world, chunkBounds, 3, 5, 0, 4, 6, 0);
    if (upRoom->hasOpening(2)) makeOpening(world, chunkBounds, 3, 5, 7, 4, 6, 7);
    if (upRoom->hasOpening(4)) makeOpening(world, chunkBounds, 0, 5, 3, 0, 6, 4);
    if (eastUpRoom->hasOpening(3)) makeOpening(world, chunkBounds, 11, 5, 0, 12, 6, 0);
    if (eastUpRoom->hasOpening(2)) makeOpening(world, chunkBounds, 11, 5, 7, 12, 6, 7);
    if (eastUpRoom->hasOpening(5)) makeOpening(world, chunkBounds, 15, 5, 3, 15, 6, 4);
}

OceanMonumentDoubleYRoom::OceanMonumentDoubleYRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::DOUBLE_Y_ROOM, direction, createRoomBox(direction, room, 1, 2, 1))
{
    setRoomDefinition(room);
}

void OceanMonumentDoubleYRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    MC_ASSERT_RELEASE(room != nullptr);
    if (room->getIndex() / 25 > 0) {
        generateDefaultFloor(world, chunkBounds, 0, 0, room->hasOpening(0));
    }

    OceanMonumentRoomDefinition* upRoom = room->getConnection(1);
    MC_ASSERT_RELEASE(upRoom != nullptr);
    if (upRoom->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 8, 1, 6, 8, 6, s_roughPrismarine);
    }

    fillWithBlocks(world, chunkBounds, 0, 4, 0, 0, 4, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 4, 0, 7, 4, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 4, 0, 6, 4, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 4, 7, 6, 4, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, 4, 1, 2, 4, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 4, 2, 1, 4, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 4, 1, 5, 4, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 4, 2, 6, 4, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, 4, 5, 2, 4, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 4, 5, 1, 4, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 4, 5, 5, 4, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 4, 5, 6, 4, 5, s_bricksPrismarine, s_bricksPrismarine, false);

    OceanMonumentRoomDefinition* currentRoom = room;
    for (i32 i = 1; i <= 5; i += 4) {
        i32 z = 0;
        if (currentRoom->hasOpening(3)) {
            fillWithBlocks(world, chunkBounds, 2, i, z, 2, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 5, i, z, 5, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 3, i + 2, z, 4, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, 0, i, z, 7, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 0, i + 1, z, 7, i + 1, z, s_roughPrismarine, s_roughPrismarine, false);
        }

        z = 7;
        if (currentRoom->hasOpening(2)) {
            fillWithBlocks(world, chunkBounds, 2, i, z, 2, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 5, i, z, 5, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 3, i + 2, z, 4, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, 0, i, z, 7, i + 2, z, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 0, i + 1, z, 7, i + 1, z, s_roughPrismarine, s_roughPrismarine, false);
        }

        i32 x = 0;
        if (currentRoom->hasOpening(4)) {
            fillWithBlocks(world, chunkBounds, x, i, 2, x, i + 2, 2, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, i, 5, x, i + 2, 5, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, i + 2, 3, x, i + 2, 4, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, x, i, 0, x, i + 2, 7, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, i + 1, 0, x, i + 1, 7, s_roughPrismarine, s_roughPrismarine, false);
        }

        x = 7;
        if (currentRoom->hasOpening(5)) {
            fillWithBlocks(world, chunkBounds, x, i, 2, x, i + 2, 2, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, i, 5, x, i + 2, 5, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, i + 2, 3, x, i + 2, 4, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, x, i, 0, x, i + 2, 7, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, i + 1, 0, x, i + 1, 7, s_roughPrismarine, s_roughPrismarine, false);
        }

        currentRoom = upRoom;
    }
}

OceanMonumentDoubleYZRoom::OceanMonumentDoubleYZRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::DOUBLE_YZ_ROOM, direction, createRoomBox(direction, room, 1, 2, 2))
{
    setRoomDefinition(room);
}

void OceanMonumentDoubleYZRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    OceanMonumentRoomDefinition* northRoom = room->getConnection(2);
    MC_ASSERT_RELEASE(room != nullptr);
    MC_ASSERT_RELEASE(northRoom != nullptr);
    OceanMonumentRoomDefinition* northUpRoom = northRoom->getConnection(1);
    OceanMonumentRoomDefinition* upRoom = room->getConnection(1);
    MC_ASSERT_RELEASE(northUpRoom != nullptr);
    MC_ASSERT_RELEASE(upRoom != nullptr);

    if (room->getIndex() / 25 > 0) {
        generateDefaultFloor(world, chunkBounds, 0, 8, northRoom->hasOpening(0));
        generateDefaultFloor(world, chunkBounds, 0, 0, room->hasOpening(0));
    }
    if (upRoom->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 8, 1, 6, 8, 7, s_roughPrismarine);
    }
    if (northUpRoom->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 8, 8, 6, 8, 14, s_roughPrismarine);
    }

    for (i32 i = 1; i <= 7; ++i) {
        const BlockState* block = (i == 2 || i == 6) ? s_roughPrismarine : s_bricksPrismarine;
        fillWithBlocks(world, chunkBounds, 0, i, 0, 0, i, 15, block, block, false);
        fillWithBlocks(world, chunkBounds, 7, i, 0, 7, i, 15, block, block, false);
        fillWithBlocks(world, chunkBounds, 1, i, 0, 6, i, 0, block, block, false);
        fillWithBlocks(world, chunkBounds, 1, i, 15, 6, i, 15, block, block, false);
    }

    for (i32 i = 1; i <= 7; ++i) {
        const BlockState* block = (i == 2 || i == 6) ? s_seaLantern : s_darkPrismarine;
        fillWithBlocks(world, chunkBounds, 3, i, 7, 4, i, 8, block, block, false);
    }

    if (room->hasOpening(3)) makeOpening(world, chunkBounds, 3, 1, 0, 4, 2, 0);
    if (room->hasOpening(5)) makeOpening(world, chunkBounds, 7, 1, 3, 7, 2, 4);
    if (room->hasOpening(4)) makeOpening(world, chunkBounds, 0, 1, 3, 0, 2, 4);
    if (northRoom->hasOpening(2)) makeOpening(world, chunkBounds, 3, 1, 15, 4, 2, 15);
    if (northRoom->hasOpening(4)) makeOpening(world, chunkBounds, 0, 1, 11, 0, 2, 12);
    if (northRoom->hasOpening(5)) makeOpening(world, chunkBounds, 7, 1, 11, 7, 2, 12);
    if (upRoom->hasOpening(3)) makeOpening(world, chunkBounds, 3, 5, 0, 4, 6, 0);
    if (upRoom->hasOpening(5)) {
        makeOpening(world, chunkBounds, 7, 5, 3, 7, 6, 4);
        fillWithBlocks(world, chunkBounds, 5, 4, 2, 6, 4, 5, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 6, 1, 2, 6, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 6, 1, 5, 6, 3, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    }
    if (upRoom->hasOpening(4)) {
        makeOpening(world, chunkBounds, 0, 5, 3, 0, 6, 4);
        fillWithBlocks(world, chunkBounds, 1, 4, 2, 2, 4, 5, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 1, 2, 1, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 1, 5, 1, 3, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    }
    if (northUpRoom->hasOpening(2)) makeOpening(world, chunkBounds, 3, 5, 15, 4, 6, 15);
    if (northUpRoom->hasOpening(4)) {
        makeOpening(world, chunkBounds, 0, 5, 11, 0, 6, 12);
        fillWithBlocks(world, chunkBounds, 1, 4, 10, 2, 4, 13, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 1, 10, 1, 3, 10, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 1, 13, 1, 3, 13, s_bricksPrismarine, s_bricksPrismarine, false);
    }
    if (northUpRoom->hasOpening(5)) {
        makeOpening(world, chunkBounds, 7, 5, 11, 7, 6, 12);
        fillWithBlocks(world, chunkBounds, 5, 4, 10, 6, 4, 13, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 6, 1, 10, 6, 3, 10, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 6, 1, 13, 6, 3, 13, s_bricksPrismarine, s_bricksPrismarine, false);
    }
}

OceanMonumentDoubleZRoom::OceanMonumentDoubleZRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::DOUBLE_Z_ROOM, direction, createRoomBox(direction, room, 1, 1, 2))
{
    setRoomDefinition(room);
}

void OceanMonumentDoubleZRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    OceanMonumentRoomDefinition* northRoom = room->getConnection(2);
    MC_ASSERT_RELEASE(room != nullptr);
    MC_ASSERT_RELEASE(northRoom != nullptr);

    if (room->getIndex() / 25 > 0) {
        generateDefaultFloor(world, chunkBounds, 0, 8, northRoom->hasOpening(0));
        generateDefaultFloor(world, chunkBounds, 0, 0, room->hasOpening(0));
    }
    if (room->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 4, 1, 6, 4, 7, s_roughPrismarine);
    }
    if (northRoom->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 4, 8, 6, 4, 14, s_roughPrismarine);
    }

    fillWithBlocks(world, chunkBounds, 0, 3, 0, 0, 3, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 3, 0, 7, 3, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 0, 7, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 15, 6, 3, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 2, 15, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 2, 0, 7, 2, 15, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 0, 7, 2, 0, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 15, 6, 2, 15, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 1, 0, 0, 1, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 1, 0, 7, 1, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 0, 7, 1, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 15, 6, 1, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 1, 1, 1, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 1, 1, 6, 1, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 1, 1, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 3, 1, 6, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 13, 1, 1, 14, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 1, 13, 6, 1, 14, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 13, 1, 3, 14, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 3, 13, 6, 3, 14, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, 1, 6, 2, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 1, 6, 5, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, 1, 9, 2, 3, 9, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 1, 9, 5, 3, 9, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 2, 6, 4, 2, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 2, 9, 4, 2, 9, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, 2, 7, 2, 2, 8, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 2, 7, 5, 2, 8, s_bricksPrismarine, s_bricksPrismarine, false);
    setBlockState(world, s_seaLantern, 2, 2, 5, chunkBounds);
    setBlockState(world, s_seaLantern, 5, 2, 5, chunkBounds);
    setBlockState(world, s_seaLantern, 2, 2, 10, chunkBounds);
    setBlockState(world, s_seaLantern, 5, 2, 10, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 2, 3, 5, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 5, 3, 5, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 2, 3, 10, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 5, 3, 10, chunkBounds);

    if (room->hasOpening(3)) makeOpening(world, chunkBounds, 3, 1, 0, 4, 2, 0);
    if (room->hasOpening(5)) makeOpening(world, chunkBounds, 7, 1, 3, 7, 2, 4);
    if (room->hasOpening(4)) makeOpening(world, chunkBounds, 0, 1, 3, 0, 2, 4);
    if (northRoom->hasOpening(2)) makeOpening(world, chunkBounds, 3, 1, 15, 4, 2, 15);
    if (northRoom->hasOpening(4)) makeOpening(world, chunkBounds, 0, 1, 11, 0, 2, 12);
    if (northRoom->hasOpening(5)) makeOpening(world, chunkBounds, 7, 1, 11, 7, 2, 12);
}

OceanMonumentEntryRoom::OceanMonumentEntryRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::ENTRY_ROOM, direction, createRoomBox(direction, room, 1, 1, 1))
{
    setRoomDefinition(room);
}

void OceanMonumentEntryRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    MC_ASSERT_RELEASE(room != nullptr);

    fillWithBlocks(world, chunkBounds, 0, 3, 0, 2, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 3, 0, 7, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 1, 2, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 2, 0, 7, 2, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 1, 0, 0, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 1, 0, 7, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 1, 7, 7, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 0, 2, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 1, 0, 6, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);

    if (room->hasOpening(2)) {
        makeOpening(world, chunkBounds, 3, 1, 7, 4, 2, 7);
    }
    if (room->hasOpening(4)) {
        makeOpening(world, chunkBounds, 0, 1, 3, 1, 2, 4);
    }
    if (room->hasOpening(5)) {
        makeOpening(world, chunkBounds, 6, 1, 3, 7, 2, 4);
    }
}

OceanMonumentSimpleRoom::OceanMonumentSimpleRoom(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
    : OceanMonumentPiece(OceanMonumentPieceTypes::SIMPLE_ROOM, direction, createRoomBox(direction, room, 1, 1, 1))
    , m_mainDesign(rng.nextInt(3))
{
    setRoomDefinition(room);
}

void OceanMonumentSimpleRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    MC_ASSERT_RELEASE(room != nullptr);

    if (room->getIndex() / 25 > 0) {
        generateDefaultFloor(world, chunkBounds, 0, 0, room->hasOpening(0));
    }
    if (room->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 4, 1, 6, 4, 6, s_roughPrismarine);
    }

    const bool hasCenterColumn = m_mainDesign != 0 && rng.nextBoolean() && !room->hasOpening(0) &&
        !room->hasOpening(1) && room->countOpenings() > 1;

    if (m_mainDesign == 0) {
        fillWithBlocks(world, chunkBounds, 0, 1, 0, 2, 1, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 3, 0, 2, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 2, 2, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 2, 0, 2, 2, 0, s_roughPrismarine, s_roughPrismarine, false);
        setBlockState(world, s_seaLantern, 1, 2, 1, chunkBounds);

        fillWithBlocks(world, chunkBounds, 5, 1, 0, 7, 1, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 5, 3, 0, 7, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 2, 0, 7, 2, 2, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 5, 2, 0, 6, 2, 0, s_roughPrismarine, s_roughPrismarine, false);
        setBlockState(world, s_seaLantern, 6, 2, 1, chunkBounds);

        fillWithBlocks(world, chunkBounds, 0, 1, 5, 2, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 3, 5, 2, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 2, 5, 0, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 2, 7, 2, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
        setBlockState(world, s_seaLantern, 1, 2, 6, chunkBounds);

        fillWithBlocks(world, chunkBounds, 5, 1, 5, 7, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 5, 3, 5, 7, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 2, 5, 7, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 5, 2, 7, 6, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
        setBlockState(world, s_seaLantern, 6, 2, 6, chunkBounds);

        if (room->hasOpening(3)) {
            fillWithBlocks(world, chunkBounds, 3, 3, 0, 4, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, 3, 3, 0, 4, 3, 1, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 3, 2, 0, 4, 2, 0, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 3, 1, 0, 4, 1, 1, s_bricksPrismarine, s_bricksPrismarine, false);
        }

        if (room->hasOpening(2)) {
            fillWithBlocks(world, chunkBounds, 3, 3, 7, 4, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, 3, 3, 6, 4, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 3, 2, 7, 4, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 3, 1, 6, 4, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        }

        if (room->hasOpening(4)) {
            fillWithBlocks(world, chunkBounds, 0, 3, 3, 0, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, 0, 3, 3, 1, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 0, 2, 3, 0, 2, 4, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 0, 1, 3, 1, 1, 4, s_bricksPrismarine, s_bricksPrismarine, false);
        }

        if (room->hasOpening(5)) {
            fillWithBlocks(world, chunkBounds, 7, 3, 3, 7, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
        } else {
            fillWithBlocks(world, chunkBounds, 6, 3, 3, 7, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 7, 2, 3, 7, 2, 4, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 6, 1, 3, 7, 1, 4, s_bricksPrismarine, s_bricksPrismarine, false);
        }
    } else if (m_mainDesign == 1) {
        fillWithBlocks(world, chunkBounds, 2, 1, 2, 2, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 2, 1, 5, 2, 3, 5, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 5, 1, 5, 5, 3, 5, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 5, 1, 2, 5, 3, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        setBlockState(world, s_seaLantern, 2, 2, 2, chunkBounds);
        setBlockState(world, s_seaLantern, 2, 2, 5, chunkBounds);
        setBlockState(world, s_seaLantern, 5, 2, 5, chunkBounds);
        setBlockState(world, s_seaLantern, 5, 2, 2, chunkBounds);

        fillWithBlocks(world, chunkBounds, 0, 1, 0, 1, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 1, 1, 0, 3, 1, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 1, 7, 1, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 1, 6, 0, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 6, 1, 7, 7, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 1, 6, 7, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 6, 1, 0, 7, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 1, 1, 7, 3, 1, s_bricksPrismarine, s_bricksPrismarine, false);

        setBlockState(world, s_roughPrismarine, 1, 2, 0, chunkBounds);
        setBlockState(world, s_roughPrismarine, 0, 2, 1, chunkBounds);
        setBlockState(world, s_roughPrismarine, 1, 2, 7, chunkBounds);
        setBlockState(world, s_roughPrismarine, 0, 2, 6, chunkBounds);
        setBlockState(world, s_roughPrismarine, 6, 2, 7, chunkBounds);
        setBlockState(world, s_roughPrismarine, 7, 2, 6, chunkBounds);
        setBlockState(world, s_roughPrismarine, 6, 2, 0, chunkBounds);
        setBlockState(world, s_roughPrismarine, 7, 2, 1, chunkBounds);

        if (!room->hasOpening(3)) {
            fillWithBlocks(world, chunkBounds, 1, 3, 0, 6, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 1, 2, 0, 6, 2, 0, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 1, 1, 0, 6, 1, 0, s_bricksPrismarine, s_bricksPrismarine, false);
        }
        if (!room->hasOpening(2)) {
            fillWithBlocks(world, chunkBounds, 1, 3, 7, 6, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 1, 2, 7, 6, 2, 7, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 1, 1, 7, 6, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        }
        if (!room->hasOpening(4)) {
            fillWithBlocks(world, chunkBounds, 0, 3, 1, 0, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 0, 2, 1, 0, 2, 6, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 0, 1, 1, 0, 1, 6, s_bricksPrismarine, s_bricksPrismarine, false);
        }
        if (!room->hasOpening(5)) {
            fillWithBlocks(world, chunkBounds, 7, 3, 1, 7, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, 7, 2, 1, 7, 2, 6, s_roughPrismarine, s_roughPrismarine, false);
            fillWithBlocks(world, chunkBounds, 7, 1, 1, 7, 1, 6, s_bricksPrismarine, s_bricksPrismarine, false);
        }
    } else if (m_mainDesign == 2) {
        fillWithBlocks(world, chunkBounds, 0, 1, 0, 0, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 1, 0, 7, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 1, 0, 6, 1, 0, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 1, 7, 6, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 2, 7, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 2, 0, 7, 2, 7, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 2, 0, 6, 2, 0, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 2, 7, 6, 2, 7, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 3, 0, 0, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 3, 0, 7, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 3, 0, 6, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 1, 3, 7, 6, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 0, 1, 3, 0, 2, 4, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 1, 3, 7, 2, 4, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 3, 1, 0, 4, 2, 0, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 3, 1, 7, 4, 2, 7, s_darkPrismarine, s_darkPrismarine, false);

        if (room->hasOpening(3)) {
            makeOpening(world, chunkBounds, 3, 1, 0, 4, 2, 0);
        }
        if (room->hasOpening(2)) {
            makeOpening(world, chunkBounds, 3, 1, 7, 4, 2, 7);
        }
        if (room->hasOpening(4)) {
            makeOpening(world, chunkBounds, 0, 1, 3, 0, 2, 4);
        }
        if (room->hasOpening(5)) {
            makeOpening(world, chunkBounds, 7, 1, 3, 7, 2, 4);
        }
    }

    if (hasCenterColumn) {
        fillWithBlocks(world, chunkBounds, 3, 1, 3, 4, 1, 4, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 3, 2, 3, 4, 2, 4, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 3, 3, 3, 4, 3, 4, s_bricksPrismarine, s_bricksPrismarine, false);
    }
}

OceanMonumentSimpleTopRoom::OceanMonumentSimpleTopRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::SIMPLE_TOP_ROOM, direction, createRoomBox(direction, room, 1, 1, 1))
{
    setRoomDefinition(room);
}

void OceanMonumentSimpleTopRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    OceanMonumentRoomDefinition* room = getRoomDefinition();
    MC_ASSERT_RELEASE(room != nullptr);
    if (room->getIndex() / 25 > 0) {
        generateDefaultFloor(world, chunkBounds, 0, 0, room->hasOpening(0));
    }
    if (room->getConnection(1) == nullptr) {
        generateBoxOnFillOnly(world, chunkBounds, 1, 4, 1, 6, 4, 6, s_roughPrismarine);
    }

    for (i32 x = 1; x <= 6; ++x) {
        for (i32 z = 1; z <= 6; ++z) {
            if (rng.nextInt(3) != 0) {
                const i32 y = 2 + (rng.nextInt(4) == 0 ? 0 : 1);
                const BlockState* sponge = &VanillaBlocks::WET_SPONGE->defaultState();
                fillWithBlocks(world, chunkBounds, x, y, z, x, 3, z, sponge, sponge, false);
            }
        }
    }

    fillWithBlocks(world, chunkBounds, 0, 1, 0, 0, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 1, 0, 7, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 0, 6, 1, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 1, 7, 6, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 2, 0, 0, 2, 7, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 2, 0, 7, 2, 7, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 0, 6, 2, 0, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 2, 7, 6, 2, 7, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 3, 0, 0, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 3, 0, 7, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 0, 6, 3, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 3, 7, 6, 3, 7, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 1, 3, 0, 2, 4, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 7, 1, 3, 7, 2, 4, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 1, 0, 4, 2, 0, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 1, 7, 4, 2, 7, s_darkPrismarine, s_darkPrismarine, false);
    if (room->hasOpening(3)) {
        makeOpening(world, chunkBounds, 3, 1, 0, 4, 2, 0);
    }
}

OceanMonumentCoreRoom::OceanMonumentCoreRoom(Direction direction, OceanMonumentRoomDefinition* room)
    : OceanMonumentPiece(OceanMonumentPieceTypes::CORE_ROOM, direction, createRoomBox(direction, room, 2, 2, 2))
{
    setRoomDefinition(room);
}

void OceanMonumentCoreRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    generateBoxOnFillOnly(world, chunkBounds, 1, 8, 0, 14, 8, 14, s_roughPrismarine);
    fillWithBlocks(world, chunkBounds, 0, 7, 0, 0, 7, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 15, 7, 0, 15, 7, 15, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 7, 0, 15, 7, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 7, 15, 14, 7, 15, s_bricksPrismarine, s_bricksPrismarine, false);

    for (i32 y = 1; y <= 6; ++y) {
        const BlockState* wallState = (y == 2 || y == 6) ? s_roughPrismarine : s_bricksPrismarine;
        fillWithBlocks(world, chunkBounds, 0, y, 0, 0, y, 1, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 0, y, 6, 0, y, 9, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 0, y, 14, 0, y, 15, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 15, y, 0, 15, y, 1, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 15, y, 6, 15, y, 9, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 15, y, 14, 15, y, 15, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 1, y, 0, 1, y, 0, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 6, y, 0, 9, y, 0, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 14, y, 0, 14, y, 0, wallState, wallState, false);
        fillWithBlocks(world, chunkBounds, 1, y, 15, 14, y, 15, wallState, wallState, false);
    }

    fillWithBlocks(world, chunkBounds, 6, 3, 6, 9, 6, 9, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world,
        chunkBounds,
        7,
        4,
        7,
        8,
        5,
        8,
        &VanillaBlocks::GOLD_BLOCK->defaultState(),
        &VanillaBlocks::GOLD_BLOCK->defaultState(),
        false);

    for (i32 y : {3, 6}) {
        for (i32 x : {6, 9}) {
            for (i32 z : {6, 9}) {
                setBlockState(world, s_seaLantern, x, y, z, chunkBounds);
            }
        }
    }

    fillWithBlocks(world, chunkBounds, 5, 1, 6, 5, 2, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 1, 9, 5, 2, 9, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 1, 6, 10, 2, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 1, 9, 10, 2, 9, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 1, 5, 6, 2, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 9, 1, 5, 9, 2, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 1, 10, 6, 2, 10, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 9, 1, 10, 9, 2, 10, s_bricksPrismarine, s_bricksPrismarine, false);

    fillWithBlocks(world, chunkBounds, 5, 2, 5, 5, 6, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 2, 10, 5, 6, 10, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 2, 5, 10, 6, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 2, 10, 10, 6, 10, s_bricksPrismarine, s_bricksPrismarine, false);

    fillWithBlocks(world, chunkBounds, 5, 7, 1, 5, 7, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 5, 7, 9, 5, 7, 14, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 7, 1, 10, 7, 6, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 7, 9, 10, 7, 14, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 7, 5, 6, 7, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 9, 7, 5, 14, 7, 5, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 7, 10, 6, 7, 10, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 9, 7, 10, 14, 7, 10, s_bricksPrismarine, s_bricksPrismarine, false);

    fillWithBlocks(world, chunkBounds, 2, 1, 2, 2, 1, 3, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 1, 2, 3, 1, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 13, 1, 2, 13, 1, 3, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 12, 1, 2, 12, 1, 2, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, 1, 12, 2, 1, 13, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 3, 1, 13, 3, 1, 13, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 13, 1, 12, 13, 1, 13, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 12, 1, 13, 12, 1, 13, s_bricksPrismarine, s_bricksPrismarine, false);
}

OceanMonumentPenthouse::OceanMonumentPenthouse(Direction direction, const StructureBoundingBox& bounds)
    : OceanMonumentPiece(OceanMonumentPieceTypes::PENTHOUSE, direction, bounds)
{}

void OceanMonumentPenthouse::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    fillWithBlocks(world, chunkBounds, 2, -1, 2, 11, -1, 11, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, -1, 0, 1, -1, 11, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 12, -1, 0, 13, -1, 11, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, -1, 0, 11, -1, 1, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 2, -1, 12, 11, -1, 13, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 0, 0, 13, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 13, 0, 0, 13, 0, 13, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 0, 0, 12, 0, 0, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 1, 0, 13, 12, 0, 13, s_bricksPrismarine, s_bricksPrismarine, false);

    for (i32 i = 2; i <= 11; i += 3) {
        setBlockState(world, s_seaLantern, 0, 0, i, chunkBounds);
        setBlockState(world, s_seaLantern, 13, 0, i, chunkBounds);
        setBlockState(world, s_seaLantern, i, 0, 0, chunkBounds);
    }

    fillWithBlocks(world, chunkBounds, 2, 0, 3, 4, 0, 9, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 9, 0, 3, 11, 0, 9, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 4, 0, 9, 9, 0, 11, s_bricksPrismarine, s_bricksPrismarine, false);
    setBlockState(world, s_bricksPrismarine, 5, 0, 8, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 8, 0, 8, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 10, 0, 10, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 3, 0, 10, chunkBounds);
    fillWithBlocks(world, chunkBounds, 3, 0, 3, 3, 0, 7, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 10, 0, 3, 10, 0, 7, s_darkPrismarine, s_darkPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, 0, 10, 7, 0, 10, s_darkPrismarine, s_darkPrismarine, false);

    for (i32 x : {3, 10}) {
        fillWithBlocks(world, chunkBounds, x, 0, 2, x, 2, 2, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, x, 0, 5, x, 2, 5, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, x, 0, 8, x, 2, 8, s_bricksPrismarine, s_bricksPrismarine, false);
    }
    fillWithBlocks(world, chunkBounds, 5, 0, 10, 5, 2, 10, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 8, 0, 10, 8, 2, 10, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 6, -1, 7, 7, -1, 8, s_darkPrismarine, s_darkPrismarine, false);
    makeOpening(world, chunkBounds, 6, -1, 3, 7, -1, 4);
    spawnElderGuardian(world, chunkBounds, 6, 1, 6);
}

OceanMonumentWingRoom::OceanMonumentWingRoom(Direction direction, const StructureBoundingBox& bounds, i32 design)
    : OceanMonumentPiece(OceanMonumentPieceTypes::WING_ROOM, direction, bounds)
    , m_mainDesign(design)
{}

void OceanMonumentWingRoom::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    if (m_mainDesign == 0) {
        for (i32 i = 0; i < 4; ++i) {
            fillWithBlocks(world,
                chunkBounds,
                10 - i,
                3 - i,
                20 - i,
                12 + i,
                3 - i,
                20,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }

        fillWithBlocks(world, chunkBounds, 7, 0, 6, 15, 0, 16, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 6, 0, 6, 6, 3, 20, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 16, 0, 6, 16, 3, 20, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 1, 7, 7, 1, 20, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 15, 1, 7, 15, 1, 20, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 7, 1, 6, 9, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 13, 1, 6, 15, 3, 6, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 8, 1, 7, 9, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 13, 1, 7, 14, 1, 7, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 9, 0, 5, 13, 0, 5, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 10, 0, 7, 12, 0, 7, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 8, 0, 10, 8, 0, 12, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 14, 0, 10, 14, 0, 12, s_darkPrismarine, s_darkPrismarine, false);

        for (i32 z = 18; z >= 7; z -= 3) {
            setBlockState(world, s_seaLantern, 6, 3, z, chunkBounds);
            setBlockState(world, s_seaLantern, 16, 3, z, chunkBounds);
        }

        setBlockState(world, s_seaLantern, 10, 0, 10, chunkBounds);
        setBlockState(world, s_seaLantern, 12, 0, 10, chunkBounds);
        setBlockState(world, s_seaLantern, 10, 0, 12, chunkBounds);
        setBlockState(world, s_seaLantern, 12, 0, 12, chunkBounds);
        setBlockState(world, s_seaLantern, 8, 3, 6, chunkBounds);
        setBlockState(world, s_seaLantern, 14, 3, 6, chunkBounds);

        setBlockState(world, s_bricksPrismarine, 4, 2, 4, chunkBounds);
        setBlockState(world, s_seaLantern, 4, 1, 4, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 4, 0, 4, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 18, 2, 4, chunkBounds);
        setBlockState(world, s_seaLantern, 18, 1, 4, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 18, 0, 4, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 4, 2, 18, chunkBounds);
        setBlockState(world, s_seaLantern, 4, 1, 18, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 4, 0, 18, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 18, 2, 18, chunkBounds);
        setBlockState(world, s_seaLantern, 18, 1, 18, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 18, 0, 18, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 9, 7, 20, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 13, 7, 20, chunkBounds);
        fillWithBlocks(world, chunkBounds, 6, 0, 21, 7, 4, 21, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 15, 0, 21, 16, 4, 21, s_bricksPrismarine, s_bricksPrismarine, false);
        spawnElderGuardian(world, chunkBounds, 11, 2, 16);
    } else {
        fillWithBlocks(world, chunkBounds, 9, 3, 18, 13, 3, 20, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 9, 0, 18, 9, 2, 18, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 13, 0, 18, 13, 2, 18, s_bricksPrismarine, s_bricksPrismarine, false);

        i32 x = 9;
        for (i32 i = 0; i < 2; ++i) {
            setBlockState(world, s_bricksPrismarine, x, 6, 20, chunkBounds);
            setBlockState(world, s_seaLantern, x, 5, 20, chunkBounds);
            setBlockState(world, s_bricksPrismarine, x, 4, 20, chunkBounds);
            x = 13;
        }

        fillWithBlocks(world, chunkBounds, 7, 3, 7, 15, 3, 14, s_bricksPrismarine, s_bricksPrismarine, false);
        x = 10;
        for (i32 i = 0; i < 2; ++i) {
            fillWithBlocks(world, chunkBounds, x, 0, 10, x, 6, 10, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, 0, 12, x, 6, 12, s_bricksPrismarine, s_bricksPrismarine, false);
            setBlockState(world, s_seaLantern, x, 0, 10, chunkBounds);
            setBlockState(world, s_seaLantern, x, 0, 12, chunkBounds);
            setBlockState(world, s_seaLantern, x, 4, 10, chunkBounds);
            setBlockState(world, s_seaLantern, x, 4, 12, chunkBounds);
            x = 12;
        }

        x = 8;
        for (i32 i = 0; i < 2; ++i) {
            fillWithBlocks(world, chunkBounds, x, 0, 7, x, 2, 7, s_bricksPrismarine, s_bricksPrismarine, false);
            fillWithBlocks(world, chunkBounds, x, 0, 14, x, 2, 14, s_bricksPrismarine, s_bricksPrismarine, false);
            x = 14;
        }

        fillWithBlocks(world, chunkBounds, 8, 3, 8, 8, 3, 13, s_darkPrismarine, s_darkPrismarine, false);
        fillWithBlocks(world, chunkBounds, 14, 3, 8, 14, 3, 13, s_darkPrismarine, s_darkPrismarine, false);
        spawnElderGuardian(world, chunkBounds, 11, 5, 13);
    }
}

OceanMonumentBuilding::OceanMonumentBuilding(math::Random& rng, i32 x, i32 z, Direction direction)
    : OceanMonumentPiece(
          OceanMonumentPieceTypes::MONUMENT_BUILDING, direction, StructureBoundingBox(x, 39, z, x + 57, 61, z + 57))
{
    auto roomList = _generateRoomGraph(rng);
    MC_ASSERT_RELEASE(!roomList.empty());
    m_sourceRoom = roomList.front();

    m_childPieces.emplace_back(std::make_unique<OceanMonumentEntryRoom>(direction, m_sourceRoom));

    for (OceanMonumentRoomDefinition* room : roomList) {
        if (room == nullptr || room->isClaimed() || room->isSpecial()) {
            continue;
        }

        std::array<std::unique_ptr<IMonumentRoomFitHelper>, 7> fitHelpers = {std::make_unique<XYDoubleRoomFitHelper>(),
            std::make_unique<YZDoubleRoomFitHelper>(),
            std::make_unique<XDoubleRoomFitHelper>(),
            std::make_unique<YDoubleRoomFitHelper>(),
            std::make_unique<ZDoubleRoomFitHelper>(),
            std::make_unique<FitSimpleRoomTopHelper>(),
            std::make_unique<FitSimpleRoomHelper>()};

        for (const auto& helper : fitHelpers) {
            if (!room->isClaimed() && helper->fits(room)) {
                m_childPieces.emplace_back(helper->create(direction, room, rng));
                break;
            }
        }
    }

    OceanMonumentRoomDefinition* coreRoom = m_roomDefinitions[static_cast<size_t>(GRIDROOM_TOP_CONNECT_INDEX)].get();
    coreRoom->setClaimed(true);
    m_coreRoom = coreRoom;
    m_childPieces.emplace_back(std::make_unique<OceanMonumentCoreRoom>(direction, coreRoom));

    // MC 1.21.11: 将房间子片段的边界框从网格相对坐标偏移到世界坐标
    // 原版中 Room/EntryRoom/CoreRoom 使用 createRoomBox() 生成网格相对坐标，
    // 需要通过 getWorldPos(9, 0, 22) 偏移到世界坐标，使 intersectsChunk() 正确工作。
    // WingRoom 和 Penthouse 使用 minX()+offset 已经是世界坐标，不需要偏移。
    const i32 offsetX = getXWithOffset(9, 22);
    const i32 offsetY = getYWithOffset(0);
    const i32 offsetZ = getZWithOffset(9, 22);
    for (auto& childPiece : m_childPieces) {
        childPiece->offset(offsetX, offsetY, offsetZ);
    }

    StructureBoundingBox leftWingBox(minX() + 8, minY() + 4, minZ() + 29, minX() + 24, minY() + 11, minZ() + 49);
    StructureBoundingBox rightWingBox(minX() + 33, minY() + 4, minZ() + 29, minX() + 49, minY() + 11, minZ() + 49);
    StructureBoundingBox penthouseBox(minX() + 21, minY() + 13, minZ() + 21, minX() + 36, minY() + 16, minZ() + 36);
    m_childPieces.emplace_back(std::make_unique<OceanMonumentWingRoom>(direction, leftWingBox, 0));
    m_childPieces.emplace_back(std::make_unique<OceanMonumentWingRoom>(direction, rightWingBox, 1));
    m_childPieces.emplace_back(std::make_unique<OceanMonumentPenthouse>(direction, penthouseBox));
}

std::vector<OceanMonumentRoomDefinition*> OceanMonumentBuilding::_generateRoomGraph(math::Random& rng)
{
    m_roomDefinitions.clear();
    m_roomDefinitions.reserve(75);
    for (i32 index = 0; index < 75; ++index) {
        m_roomDefinitions.emplace_back(std::make_unique<OceanMonumentRoomDefinition>(index));
    }

    std::array<OceanMonumentRoomDefinition*, 75> roomGrid = {};
    for (i32 x = 0; x < 5; ++x) {
        for (i32 z = 0; z < 4; ++z) {
            roomGrid[static_cast<size_t>(OceanMonumentPiece::getRoomIndex(x, 0, z))] =
                m_roomDefinitions[static_cast<size_t>(OceanMonumentPiece::getRoomIndex(x, 0, z))].get();
        }
    }
    for (i32 x = 0; x < 5; ++x) {
        for (i32 z = 0; z < 4; ++z) {
            roomGrid[static_cast<size_t>(OceanMonumentPiece::getRoomIndex(x, 1, z))] =
                m_roomDefinitions[static_cast<size_t>(OceanMonumentPiece::getRoomIndex(x, 1, z))].get();
        }
    }
    for (i32 x = 1; x < 4; ++x) {
        for (i32 z = 0; z < 2; ++z) {
            roomGrid[static_cast<size_t>(OceanMonumentPiece::getRoomIndex(x, 2, z))] =
                m_roomDefinitions[static_cast<size_t>(OceanMonumentPiece::getRoomIndex(x, 2, z))].get();
        }
    }

    m_sourceRoom = roomGrid[static_cast<size_t>(GRIDROOM_SOURCE_INDEX)];
    MC_ASSERT_RELEASE(m_sourceRoom != nullptr);

    for (i32 x = 0; x < 5; ++x) {
        for (i32 z = 0; z < 5; ++z) {
            for (i32 y = 0; y < 3; ++y) {
                const i32 roomIndex = OceanMonumentPiece::getRoomIndex(x, y, z);
                OceanMonumentRoomDefinition* room = roomGrid[static_cast<size_t>(roomIndex)];
                if (room == nullptr) {
                    continue;
                }
                for (Direction direction : ROOM_DIRECTIONS) {
                    const i32 nextX = x + Directions::xOffset(direction);
                    const i32 nextY = y + Directions::yOffset(direction);
                    const i32 nextZ = z + Directions::zOffset(direction);
                    if (nextX < 0 || nextX >= 5 || nextY < 0 || nextY >= 3 || nextZ < 0 || nextZ >= 5) {
                        continue;
                    }
                    const i32 neighborIndex = OceanMonumentPiece::getRoomIndex(nextX, nextY, nextZ);
                    OceanMonumentRoomDefinition* neighbor = roomGrid[static_cast<size_t>(neighborIndex)];
                    if (neighbor == nullptr) {
                        continue;
                    }
                    const i32 dirIndex = static_cast<i32>(direction);
                    if (nextZ == z) {
                        room->setConnection(dirIndex, neighbor);
                    } else {
                        room->setConnection(static_cast<i32>(Directions::opposite(direction)), neighbor);
                    }
                }
            }
        }
    }

    auto topRoom = std::make_unique<OceanMonumentRoomDefinition>(1003);
    auto leftWingRoom = std::make_unique<OceanMonumentRoomDefinition>(1001);
    auto rightWingRoom = std::make_unique<OceanMonumentRoomDefinition>(1002);
    OceanMonumentRoomDefinition* topRoomPtr = topRoom.get();
    OceanMonumentRoomDefinition* leftWingRoomPtr = leftWingRoom.get();
    OceanMonumentRoomDefinition* rightWingRoomPtr = rightWingRoom.get();
    m_roomDefinitions.emplace_back(std::move(topRoom));
    m_roomDefinitions.emplace_back(std::move(leftWingRoom));
    m_roomDefinitions.emplace_back(std::move(rightWingRoom));

    roomGrid[static_cast<size_t>(GRIDROOM_TOP_CONNECT_INDEX)]->setConnection(1, topRoomPtr);
    roomGrid[static_cast<size_t>(GRIDROOM_LEFTWING_CONNECT_INDEX)]->setConnection(3, leftWingRoomPtr);
    roomGrid[static_cast<size_t>(GRIDROOM_RIGHTWING_CONNECT_INDEX)]->setConnection(3, rightWingRoomPtr);
    topRoomPtr->setClaimed(true);
    leftWingRoomPtr->setClaimed(true);
    rightWingRoomPtr->setClaimed(true);

    m_sourceRoom->setSource(true);
    const i32 coreIndex = OceanMonumentPiece::getRoomIndex(rng.nextInt(4), 0, 2);
    m_coreRoom = roomGrid[static_cast<size_t>(coreIndex)];
    MC_ASSERT_RELEASE(m_coreRoom != nullptr);
    m_coreRoom->setClaimed(true);
    m_coreRoom->getConnection(5)->setClaimed(true);
    m_coreRoom->getConnection(2)->setClaimed(true);
    m_coreRoom->getConnection(5)->getConnection(2)->setClaimed(true);
    m_coreRoom->getConnection(1)->setClaimed(true);
    m_coreRoom->getConnection(5)->getConnection(1)->setClaimed(true);
    m_coreRoom->getConnection(2)->getConnection(1)->setClaimed(true);
    m_coreRoom->getConnection(5)->getConnection(2)->getConnection(1)->setClaimed(true);

    std::vector<OceanMonumentRoomDefinition*> result;
    result.reserve(78);
    for (OceanMonumentRoomDefinition* room : roomGrid) {
        if (room != nullptr) {
            room->updateOpenings();
            result.push_back(room);
        }
    }
    topRoomPtr->updateOpenings();
    leftWingRoomPtr->updateOpenings();
    rightWingRoomPtr->updateOpenings();

    rng.shuffle(result);
    i32 scanIndex = 1;
    for (OceanMonumentRoomDefinition* room : result) {
        i32 removedConnections = 0;
        i32 attempts = 0;
        while (removedConnections < 2 && attempts < 5) {
            ++attempts;
            const i32 openingIndex = rng.nextInt(6);
            if (!room->hasOpening(openingIndex)) {
                continue;
            }
            OceanMonumentRoomDefinition* neighbor = room->getConnection(openingIndex);
            MC_ASSERT_RELEASE(neighbor != nullptr);
            const i32 oppositeIndex =
                static_cast<i32>(Directions::opposite(ROOM_DIRECTIONS[static_cast<size_t>(openingIndex)]));
            room->setHasOpening(openingIndex, false);
            neighbor->setHasOpening(oppositeIndex, false);
            if (room->findSource(scanIndex++) && neighbor->findSource(scanIndex++)) {
                ++removedConnections;
            } else {
                room->setHasOpening(openingIndex, true);
                neighbor->setHasOpening(oppositeIndex, true);
            }
        }
    }
    result.push_back(topRoomPtr);
    result.push_back(leftWingRoomPtr);
    result.push_back(rightWingRoomPtr);
    return result;
}

void OceanMonumentBuilding::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(rng);
    fillWithBlocks(world, chunkBounds, 0, 0, 0, 57, 22, 57, s_bricksPrismarine, s_water, false);
    _generateEntranceArchs(world, rng, chunkBounds);
    _generateEntranceWall(world, rng, chunkBounds);
    _generateLowerWall(world, rng, chunkBounds);
    _generateMiddleWall(world, rng, chunkBounds);
    _generateUpperWall(world, rng, chunkBounds);
    _generateRoofPiece(world, rng, chunkBounds);

    for (const auto& childPiece : m_childPieces) {
        if (childPiece->intersectsChunk(chunkX, chunkZ)) {
            childPiece->generate(world, rng, chunkX, chunkZ, chunkBounds);
        }
    }
}

void OceanMonumentBuilding::_generateWing(
    bool isLeft, i32 startX, IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);

    if (!doesChunkIntersect(chunkBounds, startX, 0, startX + 23, 20)) {
        return;
    }

    fillWithBlocks(world, chunkBounds, startX, 0, 0, startX + 24, 0, 20, s_roughPrismarine, s_roughPrismarine, false);
    makeOpening(world, chunkBounds, startX, 1, 0, startX + 24, 10, 20);

    for (i32 layer = 0; layer < 4; ++layer) {
        fillWithBlocks(world,
            chunkBounds,
            startX + layer,
            layer + 1,
            layer,
            startX + layer,
            layer + 1,
            20,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            startX + layer + 7,
            layer + 5,
            layer + 7,
            startX + layer + 7,
            layer + 5,
            20,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            startX + 17 - layer,
            layer + 5,
            layer + 7,
            startX + 17 - layer,
            layer + 5,
            20,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            startX + 24 - layer,
            layer + 1,
            layer,
            startX + 24 - layer,
            layer + 1,
            20,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            startX + layer + 1,
            layer + 1,
            layer,
            startX + 23 - layer,
            layer + 1,
            layer,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            startX + layer + 8,
            layer + 5,
            layer + 7,
            startX + 16 - layer,
            layer + 5,
            layer + 7,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
    }

    fillWithBlocks(
        world, chunkBounds, startX + 4, 4, 4, startX + 6, 4, 20, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(
        world, chunkBounds, startX + 7, 4, 4, startX + 17, 4, 6, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(
        world, chunkBounds, startX + 18, 4, 4, startX + 20, 4, 20, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(
        world, chunkBounds, startX + 11, 8, 11, startX + 13, 8, 20, s_roughPrismarine, s_roughPrismarine, false);
    setBlockState(world, s_seaLantern, startX + 12, 9, 12, chunkBounds);
    setBlockState(world, s_seaLantern, startX + 12, 9, 15, chunkBounds);
    setBlockState(world, s_seaLantern, startX + 12, 9, 18, chunkBounds);

    const i32 innerColumnX = startX + (isLeft ? 19 : 5);
    const i32 outerColumnX = startX + (isLeft ? 5 : 19);

    for (i32 z = 20; z >= 5; z -= 3) {
        setBlockState(world, s_seaLantern, innerColumnX, 5, z, chunkBounds);
    }
    for (i32 z = 19; z >= 7; z -= 3) {
        setBlockState(world, s_seaLantern, outerColumnX, 5, z, chunkBounds);
    }
    for (i32 index = 0; index < 4; ++index) {
        const i32 x = isLeft ? startX + 24 - (17 - index * 3) : startX + 17 - index * 3;
        setBlockState(world, s_seaLantern, x, 5, 5, chunkBounds);
    }
    setBlockState(world, s_seaLantern, outerColumnX, 5, 5, chunkBounds);

    fillWithBlocks(
        world, chunkBounds, startX + 11, 1, 12, startX + 13, 7, 12, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(
        world, chunkBounds, startX + 12, 1, 11, startX + 12, 7, 13, s_roughPrismarine, s_roughPrismarine, false);
}

void OceanMonumentBuilding::_generateEntranceArchs(
    IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);
    if (!doesChunkIntersect(chunkBounds, 22, 5, 35, 17)) {
        return;
    }

    makeOpening(world, chunkBounds, 25, 0, 0, 32, 8, 20);

    for (i32 index = 0; index < 4; ++index) {
        const i32 z = 5 + index * 4;
        fillWithBlocks(world, chunkBounds, 24, 2, z, 24, 4, z, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 22, 4, z, 23, 4, z, s_bricksPrismarine, s_bricksPrismarine, false);
        setBlockState(world, s_bricksPrismarine, 25, 5, z, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 26, 6, z, chunkBounds);
        setBlockState(world, s_seaLantern, 26, 5, z, chunkBounds);
        fillWithBlocks(world, chunkBounds, 33, 2, z, 33, 4, z, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 34, 4, z, 35, 4, z, s_bricksPrismarine, s_bricksPrismarine, false);
        setBlockState(world, s_bricksPrismarine, 32, 5, z, chunkBounds);
        setBlockState(world, s_bricksPrismarine, 31, 6, z, chunkBounds);
        setBlockState(world, s_seaLantern, 31, 5, z, chunkBounds);
        fillWithBlocks(world, chunkBounds, 27, 6, z, 30, 6, z, s_roughPrismarine, s_roughPrismarine, false);
    }
}

void OceanMonumentBuilding::_generateEntranceWall(
    IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);
    if (!doesChunkIntersect(chunkBounds, 15, 20, 42, 21)) {
        return;
    }

    fillWithBlocks(world, chunkBounds, 15, 0, 21, 42, 0, 21, s_roughPrismarine, s_roughPrismarine, false);
    makeOpening(world, chunkBounds, 26, 1, 21, 31, 3, 21);
    fillWithBlocks(world, chunkBounds, 21, 12, 21, 36, 12, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 17, 11, 21, 40, 11, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 16, 10, 21, 41, 10, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 15, 7, 21, 42, 9, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 16, 6, 21, 41, 6, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 17, 5, 21, 40, 5, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 21, 4, 21, 36, 4, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 22, 3, 21, 26, 3, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 31, 3, 21, 35, 3, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 23, 2, 21, 25, 2, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 32, 2, 21, 34, 2, 21, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 28, 4, 20, 29, 4, 21, s_bricksPrismarine, s_bricksPrismarine, false);
    setBlockState(world, s_bricksPrismarine, 27, 3, 21, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 30, 3, 21, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 26, 2, 21, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 31, 2, 21, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 25, 1, 21, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 32, 1, 21, chunkBounds);

    for (i32 index = 0; index < 7; ++index) {
        setBlockState(world, s_darkPrismarine, 28 - index, 6 + index, 21, chunkBounds);
        setBlockState(world, s_darkPrismarine, 29 + index, 6 + index, 21, chunkBounds);
    }
    for (i32 index = 0; index < 4; ++index) {
        setBlockState(world, s_darkPrismarine, 28 - index, 9 + index, 21, chunkBounds);
        setBlockState(world, s_darkPrismarine, 29 + index, 9 + index, 21, chunkBounds);
    }
    setBlockState(world, s_darkPrismarine, 28, 12, 21, chunkBounds);
    setBlockState(world, s_darkPrismarine, 29, 12, 21, chunkBounds);

    for (i32 index = 0; index < 3; ++index) {
        setBlockState(world, s_darkPrismarine, 22 - index * 2, 8, 21, chunkBounds);
        setBlockState(world, s_darkPrismarine, 22 - index * 2, 9, 21, chunkBounds);
        setBlockState(world, s_darkPrismarine, 35 + index * 2, 8, 21, chunkBounds);
        setBlockState(world, s_darkPrismarine, 35 + index * 2, 9, 21, chunkBounds);
    }

    makeOpening(world, chunkBounds, 15, 13, 21, 42, 15, 21);
    makeOpening(world, chunkBounds, 15, 1, 21, 15, 6, 21);
    makeOpening(world, chunkBounds, 16, 1, 21, 16, 5, 21);
    makeOpening(world, chunkBounds, 17, 1, 21, 20, 4, 21);
    makeOpening(world, chunkBounds, 21, 1, 21, 21, 3, 21);
    makeOpening(world, chunkBounds, 22, 1, 21, 22, 2, 21);
    makeOpening(world, chunkBounds, 23, 1, 21, 24, 1, 21);
    makeOpening(world, chunkBounds, 42, 1, 21, 42, 6, 21);
    makeOpening(world, chunkBounds, 41, 1, 21, 41, 5, 21);
    makeOpening(world, chunkBounds, 37, 1, 21, 40, 4, 21);
    makeOpening(world, chunkBounds, 36, 1, 21, 36, 3, 21);
    makeOpening(world, chunkBounds, 33, 1, 21, 34, 1, 21);
    makeOpening(world, chunkBounds, 35, 1, 21, 35, 2, 21);
}

void OceanMonumentBuilding::_generateRoofPiece(
    IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);
    if (!doesChunkIntersect(chunkBounds, 21, 21, 36, 36)) {
        return;
    }

    fillWithBlocks(world, chunkBounds, 21, 0, 22, 36, 0, 36, s_roughPrismarine, s_roughPrismarine, false);
    makeOpening(world, chunkBounds, 21, 1, 22, 36, 23, 36);

    for (i32 index = 0; index < 4; ++index) {
        fillWithBlocks(world,
            chunkBounds,
            21 + index,
            13 + index,
            21 + index,
            36 - index,
            13 + index,
            21 + index,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            21 + index,
            13 + index,
            36 - index,
            36 - index,
            13 + index,
            36 - index,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            21 + index,
            13 + index,
            22 + index,
            21 + index,
            13 + index,
            35 - index,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
        fillWithBlocks(world,
            chunkBounds,
            36 - index,
            13 + index,
            22 + index,
            36 - index,
            13 + index,
            35 - index,
            s_bricksPrismarine,
            s_bricksPrismarine,
            false);
    }

    fillWithBlocks(world, chunkBounds, 25, 16, 25, 32, 16, 32, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 25, 17, 25, 25, 19, 25, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 32, 17, 25, 32, 19, 25, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 25, 17, 32, 25, 19, 32, s_bricksPrismarine, s_bricksPrismarine, false);
    fillWithBlocks(world, chunkBounds, 32, 17, 32, 32, 19, 32, s_bricksPrismarine, s_bricksPrismarine, false);

    setBlockState(world, s_bricksPrismarine, 26, 20, 26, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 27, 21, 27, chunkBounds);
    setBlockState(world, s_seaLantern, 27, 20, 27, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 26, 20, 31, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 27, 21, 30, chunkBounds);
    setBlockState(world, s_seaLantern, 27, 20, 30, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 31, 20, 31, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 30, 21, 30, chunkBounds);
    setBlockState(world, s_seaLantern, 30, 20, 30, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 31, 20, 26, chunkBounds);
    setBlockState(world, s_bricksPrismarine, 30, 21, 27, chunkBounds);
    setBlockState(world, s_seaLantern, 30, 20, 27, chunkBounds);
    fillWithBlocks(world, chunkBounds, 28, 21, 27, 29, 21, 27, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 27, 21, 28, 27, 21, 29, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 28, 21, 30, 29, 21, 30, s_roughPrismarine, s_roughPrismarine, false);
    fillWithBlocks(world, chunkBounds, 30, 21, 28, 30, 21, 29, s_roughPrismarine, s_roughPrismarine, false);
}

void OceanMonumentBuilding::_generateLowerWall(
    IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);
    if (doesChunkIntersect(chunkBounds, 0, 21, 6, 58)) {
        fillWithBlocks(world, chunkBounds, 0, 0, 21, 6, 0, 57, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 0, 1, 21, 6, 7, 57);
        fillWithBlocks(world, chunkBounds, 4, 4, 21, 6, 4, 53, s_roughPrismarine, s_roughPrismarine, false);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                index,
                index + 1,
                21,
                index,
                index + 1,
                57 - index,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
        for (i32 z = 23; z < 53; z += 3) {
            setBlockState(world, s_seaLantern, 5, 5, z, chunkBounds);
        }
        setBlockState(world, s_seaLantern, 5, 5, 52, chunkBounds);
        fillWithBlocks(world, chunkBounds, 4, 1, 52, 6, 3, 52, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 5, 1, 51, 5, 3, 53, s_roughPrismarine, s_roughPrismarine, false);
    }

    if (doesChunkIntersect(chunkBounds, 51, 21, 58, 58)) {
        fillWithBlocks(world, chunkBounds, 51, 0, 21, 57, 0, 57, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 51, 1, 21, 57, 7, 57);
        fillWithBlocks(world, chunkBounds, 51, 4, 21, 53, 4, 53, s_roughPrismarine, s_roughPrismarine, false);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                57 - index,
                index + 1,
                21,
                57 - index,
                index + 1,
                57 - index,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
        for (i32 z = 23; z < 53; z += 3) {
            setBlockState(world, s_seaLantern, 52, 5, z, chunkBounds);
        }
        setBlockState(world, s_seaLantern, 52, 5, 52, chunkBounds);
        fillWithBlocks(world, chunkBounds, 51, 1, 52, 53, 3, 52, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 52, 1, 51, 52, 3, 53, s_roughPrismarine, s_roughPrismarine, false);
    }

    if (doesChunkIntersect(chunkBounds, 0, 51, 57, 57)) {
        fillWithBlocks(world, chunkBounds, 7, 0, 51, 50, 0, 57, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 7, 1, 51, 50, 10, 57);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                index + 1,
                index + 1,
                57 - index,
                56 - index,
                index + 1,
                57 - index,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
    }
}

void OceanMonumentBuilding::_generateMiddleWall(
    IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);
    if (doesChunkIntersect(chunkBounds, 7, 21, 13, 50)) {
        fillWithBlocks(world, chunkBounds, 7, 0, 21, 13, 0, 50, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 7, 1, 21, 13, 10, 50);
        fillWithBlocks(world, chunkBounds, 11, 8, 21, 13, 8, 53, s_roughPrismarine, s_roughPrismarine, false);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                index + 7,
                index + 5,
                21,
                index + 7,
                index + 5,
                54,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
        for (i32 z = 21; z <= 45; z += 3) {
            setBlockState(world, s_seaLantern, 12, 9, z, chunkBounds);
        }
    }

    if (doesChunkIntersect(chunkBounds, 44, 21, 50, 54)) {
        fillWithBlocks(world, chunkBounds, 44, 0, 21, 50, 0, 50, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 44, 1, 21, 50, 10, 50);
        fillWithBlocks(world, chunkBounds, 44, 8, 21, 46, 8, 53, s_roughPrismarine, s_roughPrismarine, false);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                50 - index,
                index + 5,
                21,
                50 - index,
                index + 5,
                54,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
        for (i32 z = 21; z <= 45; z += 3) {
            setBlockState(world, s_seaLantern, 45, 9, z, chunkBounds);
        }
    }

    if (doesChunkIntersect(chunkBounds, 8, 44, 49, 54)) {
        fillWithBlocks(world, chunkBounds, 14, 0, 44, 43, 0, 50, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 14, 1, 44, 43, 10, 50);

        for (i32 x = 12; x <= 45; x += 3) {
            setBlockState(world, s_seaLantern, x, 9, 45, chunkBounds);
            setBlockState(world, s_seaLantern, x, 9, 52, chunkBounds);
            if (x == 12 || x == 18 || x == 24 || x == 33 || x == 39 || x == 45) {
                setBlockState(world, s_seaLantern, x, 9, 47, chunkBounds);
                setBlockState(world, s_seaLantern, x, 9, 50, chunkBounds);
                setBlockState(world, s_seaLantern, x, 10, 45, chunkBounds);
                setBlockState(world, s_seaLantern, x, 10, 46, chunkBounds);
                setBlockState(world, s_seaLantern, x, 10, 51, chunkBounds);
                setBlockState(world, s_seaLantern, x, 10, 52, chunkBounds);
                setBlockState(world, s_seaLantern, x, 11, 47, chunkBounds);
                setBlockState(world, s_seaLantern, x, 11, 50, chunkBounds);
                setBlockState(world, s_seaLantern, x, 12, 48, chunkBounds);
                setBlockState(world, s_seaLantern, x, 12, 49, chunkBounds);
            }
        }

        for (i32 index = 0; index < 3; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                8 + index,
                5 + index,
                54,
                49 - index,
                5 + index,
                54,
                s_roughPrismarine,
                s_roughPrismarine,
                false);
        }

        fillWithBlocks(world, chunkBounds, 11, 8, 54, 46, 8, 54, s_bricksPrismarine, s_bricksPrismarine, false);
        fillWithBlocks(world, chunkBounds, 14, 8, 44, 43, 8, 53, s_roughPrismarine, s_roughPrismarine, false);
    }
}

void OceanMonumentBuilding::_generateUpperWall(
    IWorldWriter& world, math::Random& rng, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);
    if (doesChunkIntersect(chunkBounds, 14, 21, 20, 43)) {
        fillWithBlocks(world, chunkBounds, 14, 0, 21, 20, 0, 43, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 14, 1, 22, 20, 14, 43);
        fillWithBlocks(world, chunkBounds, 18, 12, 22, 20, 12, 39, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 18, 12, 21, 20, 12, 21, s_bricksPrismarine, s_bricksPrismarine, false);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                index + 14,
                index + 9,
                21,
                index + 14,
                index + 9,
                43 - index,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
        for (i32 z = 23; z <= 39; z += 3) {
            setBlockState(world, s_seaLantern, 19, 13, z, chunkBounds);
        }
    }

    if (doesChunkIntersect(chunkBounds, 37, 21, 43, 43)) {
        fillWithBlocks(world, chunkBounds, 37, 0, 21, 43, 0, 43, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 37, 1, 22, 43, 14, 43);
        fillWithBlocks(world, chunkBounds, 37, 12, 22, 39, 12, 39, s_roughPrismarine, s_roughPrismarine, false);
        fillWithBlocks(world, chunkBounds, 37, 12, 21, 39, 12, 21, s_bricksPrismarine, s_bricksPrismarine, false);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                43 - index,
                index + 9,
                21,
                43 - index,
                index + 9,
                43 - index,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
        for (i32 z = 23; z <= 39; z += 3) {
            setBlockState(world, s_seaLantern, 38, 13, z, chunkBounds);
        }
    }

    if (doesChunkIntersect(chunkBounds, 15, 37, 42, 43)) {
        fillWithBlocks(world, chunkBounds, 21, 0, 37, 36, 0, 43, s_roughPrismarine, s_roughPrismarine, false);
        makeOpening(world, chunkBounds, 21, 1, 37, 36, 14, 43);
        fillWithBlocks(world, chunkBounds, 21, 12, 37, 36, 12, 39, s_roughPrismarine, s_roughPrismarine, false);

        for (i32 index = 0; index < 4; ++index) {
            fillWithBlocks(world,
                chunkBounds,
                15 + index,
                index + 9,
                43 - index,
                42 - index,
                index + 9,
                43 - index,
                s_bricksPrismarine,
                s_bricksPrismarine,
                false);
        }
        for (i32 x = 21; x <= 36; x += 3) {
            setBlockState(world, s_seaLantern, x, 13, 38, chunkBounds);
        }
    }
}

bool FitSimpleRoomHelper::fits(OceanMonumentRoomDefinition* definition)
{
    return definition != nullptr && !definition->isClaimed();
}

OceanMonumentPiece* FitSimpleRoomHelper::create(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
{
    room->setClaimed(true);
    return new OceanMonumentSimpleRoom(direction, room, rng);
}

bool FitSimpleRoomTopHelper::fits(OceanMonumentRoomDefinition* definition)
{
    return definition != nullptr && !definition->isClaimed() && definition->getIndex() / 25 == 2;
}

OceanMonumentPiece* FitSimpleRoomTopHelper::create(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
{
    MC_UNUSED(rng);
    room->setClaimed(true);
    return new OceanMonumentSimpleTopRoom(direction, room);
}

bool XDoubleRoomFitHelper::fits(OceanMonumentRoomDefinition* definition)
{
    return definition != nullptr && !definition->isClaimed() && definition->hasOpening(5) &&
        definition->getConnection(5) != nullptr && !definition->getConnection(5)->isClaimed();
}

OceanMonumentPiece* XDoubleRoomFitHelper::create(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
{
    MC_UNUSED(rng);
    room->setClaimed(true);
    room->getConnection(5)->setClaimed(true);
    return new OceanMonumentDoubleXRoom(direction, room);
}

bool YDoubleRoomFitHelper::fits(OceanMonumentRoomDefinition* definition)
{
    return definition != nullptr && !definition->isClaimed() && definition->hasOpening(1) &&
        definition->getConnection(1) != nullptr && !definition->getConnection(1)->isClaimed();
}

OceanMonumentPiece* YDoubleRoomFitHelper::create(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
{
    MC_UNUSED(rng);
    room->setClaimed(true);
    room->getConnection(1)->setClaimed(true);
    return new OceanMonumentDoubleYRoom(direction, room);
}

bool ZDoubleRoomFitHelper::fits(OceanMonumentRoomDefinition* definition)
{
    return definition != nullptr && !definition->isClaimed() && definition->hasOpening(2) &&
        definition->getConnection(2) != nullptr && !definition->getConnection(2)->isClaimed();
}

OceanMonumentPiece* ZDoubleRoomFitHelper::create(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
{
    MC_UNUSED(rng);
    OceanMonumentRoomDefinition* roomToUse = room;
    if (!room->hasOpening(2) || room->getConnection(2)->isClaimed()) {
        roomToUse = room->getConnection(3);
    }
    roomToUse->setClaimed(true);
    roomToUse->getConnection(2)->setClaimed(true);
    return new OceanMonumentDoubleZRoom(direction, roomToUse);
}

bool XYDoubleRoomFitHelper::fits(OceanMonumentRoomDefinition* definition)
{
    if (definition == nullptr || definition->isClaimed() || !definition->hasOpening(5) || !definition->hasOpening(1)) {
        return false;
    }
    OceanMonumentRoomDefinition* east = definition->getConnection(5);
    OceanMonumentRoomDefinition* up = definition->getConnection(1);
    if (east == nullptr || up == nullptr || east->isClaimed() || up->isClaimed() || !east->hasOpening(1)) {
        return false;
    }
    OceanMonumentRoomDefinition* eastUp = east->getConnection(1);
    return eastUp != nullptr && !eastUp->isClaimed();
}

OceanMonumentPiece* XYDoubleRoomFitHelper::create(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
{
    MC_UNUSED(rng);
    room->setClaimed(true);
    room->getConnection(5)->setClaimed(true);
    room->getConnection(1)->setClaimed(true);
    room->getConnection(5)->getConnection(1)->setClaimed(true);
    return new OceanMonumentDoubleXYRoom(direction, room);
}

bool YZDoubleRoomFitHelper::fits(OceanMonumentRoomDefinition* definition)
{
    if (definition == nullptr || definition->isClaimed() || !definition->hasOpening(2) || !definition->hasOpening(1)) {
        return false;
    }
    OceanMonumentRoomDefinition* north = definition->getConnection(2);
    OceanMonumentRoomDefinition* up = definition->getConnection(1);
    if (north == nullptr || up == nullptr || north->isClaimed() || up->isClaimed() || !north->hasOpening(1)) {
        return false;
    }
    OceanMonumentRoomDefinition* northUp = north->getConnection(1);
    return northUp != nullptr && !northUp->isClaimed();
}

OceanMonumentPiece* YZDoubleRoomFitHelper::create(
    Direction direction, OceanMonumentRoomDefinition* room, math::Random& rng)
{
    MC_UNUSED(rng);
    room->setClaimed(true);
    room->getConnection(2)->setClaimed(true);
    room->getConnection(1)->setClaimed(true);
    room->getConnection(2)->getConnection(1)->setClaimed(true);
    return new OceanMonumentDoubleYZRoom(direction, room);
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
