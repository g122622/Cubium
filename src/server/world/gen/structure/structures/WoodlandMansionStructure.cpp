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

#include "WoodlandMansionStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/Feature.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include "server/world/gen/structure/structures/WoodlandMansionStructure.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;
using namespace mc::world; // 引入 CHUNK_WIDTH、SEA_LEVEL 等常量
using namespace woodland_mansion;

namespace {
// 林地府邸网格尺寸常量
constexpr i32 MANSION_GRID_SIZE = 11;
constexpr i32 MANSION_GRID_OUTSIDE_VALUE = 5;

// 林地府邸房间尺寸常量
constexpr i32 ROOM_SIZE = 8;

// 入口偏移常量
constexpr i32 ENTRANCE_OFFSET_X = 7;
constexpr i32 ENTRANCE_OFFSET_Y = 4;

// 楼层高度
constexpr i32 FLOOR_HEIGHT = 8;

} // namespace

// ============================================================================
// WoodlandMansionStructure 实现
// ============================================================================

const std::string WoodlandMansionStructure::s_name = "Woodland_Mansion";

WoodlandMansionStructure::WoodlandMansionStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* WoodlandMansionStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_MANSION();
}

std::unique_ptr<StructureStart> WoodlandMansionStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 原版 `findGenerationPoint`：先取旋转，再在「区块内偏移 7 格」的 5x5 方框四角取最低地表，
    // 低于 Y=60 直接放弃（林地府邸必须建在高地上）。旋转会影响方框的展开方向。
    const Rotation rotation = static_cast<Rotation>(rng.nextInt(4));

    i32 sizeX = 5;
    i32 sizeZ = 5;
    if (rotation == Rotation::Clockwise90) {
        sizeX = -5;
    } else if (rotation == Rotation::Clockwise180) {
        sizeX = -5;
        sizeZ = -5;
    } else if (rotation == Rotation::CounterClockwise90) {
        sizeZ = -5;
    }

    const i32 baseX = chunkX * world::CHUNK_WIDTH + 7;
    const i32 baseZ = chunkZ * world::CHUNK_WIDTH + 7;
    // `getFirstOccupiedHeight` 返回「最高方块上方一格」，本项目的 getHeight 已经是这个口径，
    // 但世界表面高度图在生成阶段会多算一格顶面，这里减一回落到方块层。
    const i32 corners[4] = {
        generator.getHeight(baseX, baseZ, HeightmapType::WorldSurfaceWG) - 1,
        generator.getHeight(baseX, baseZ + sizeZ, HeightmapType::WorldSurfaceWG) - 1,
        generator.getHeight(baseX + sizeX, baseZ, HeightmapType::WorldSurfaceWG) - 1,
        generator.getHeight(baseX + sizeX, baseZ + sizeZ, HeightmapType::WorldSurfaceWG) - 1,
    };
    const i32 y = std::min(std::min(corners[0], corners[1]), std::min(corners[2], corners[3]));
    if (y < 60) {
        return start;
    }

    // 生物群系校验落在候选生成点（区块内偏移 7 格处的最低地表）上；不匹配即返回空起点。
    // 必须在构造网格之前完成：原版候选点校验先于 generatePieces，网格抽样在其后。
    if (!isValidBiomeAt(generator, BlockPos(baseX, y, baseZ))) {
        return start;
    }

    // 网格与放置器共用同一条随机流：网格先构造（消耗大量抽样），放置器再接着抽。
    MansionGrid grid(rng);
    MansionPlacer placer(jigsaw::JigsawAssembler::getTemplateManager(), rng);

    std::vector<std::unique_ptr<StructurePiece>> pieces;
    placer.createMansion(BlockPos(baseX, y, baseZ), rotation, pieces, grid);

    for (auto& piece : pieces) {
        start->addPiece(std::move(piece));
    }
    start->recalculateStructureSize();
    return start;
}

// ============================================================================
// WoodlandMansionPiece 实现
// ============================================================================

WoodlandMansionPiece::WoodlandMansionPiece(const std::string& templateName, const BlockPos& pos, Rotation rotation)
    : WoodlandMansionPiece(templateName, pos, rotation, Mirror::None)
{}

WoodlandMansionPiece::WoodlandMansionPiece(
    const std::string& templateName, const BlockPos& pos, Rotation rotation, Mirror mirror)
    : StructurePiece(StructurePieceTypes::WOODLAND_MANSION, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z)
    , m_templateName(templateName)
    , m_templatePosition(pos)
{
    // 旋转/镜像存进基类：界面的 getRotation()/getMirror() 读的正是基类那对字段。
    setRotation(rotation);
    setMirror(mirror);
    m_templateManager = &jigsaw::JigsawAssembler::getTemplateManager();
    _loadTemplate();
}

void WoodlandMansionPiece::_loadTemplate()
{
    if (m_templateManager == nullptr) {
        return;
    }
    m_template = m_templateManager->getTemplate(ResourceLocation("minecraft", "woodland_mansion/" + m_templateName));
    if (m_template == nullptr) {
        return;
    }
    m_size = m_template->getSize();

    // 原版 `TemplateStructurePiece` 构造期就用模板包围盒：盒子随旋转/镜像展开，
    // 起点阶段的所有几何都以它为准。
    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(getMirror());
    const StructureBoundingBox box = m_template->getBoundingBox(settings, m_templatePosition);
    m_minX = box.minX();
    m_minY = box.minY();
    m_minZ = box.minZ();
    m_maxX = box.maxX();
    m_maxY = box.maxY();
    m_maxZ = box.maxZ();
}

void WoodlandMansionPiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    if (m_template == nullptr) {
        _loadTemplate();
        if (m_template == nullptr) {
            spdlog::warn("WoodlandMansionPiece: template '{}' is missing", m_templateName);
            return;
        }
    }

    const StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    feature::template_::PlacementSettings settings;
    settings.setRotation(getRotation());
    settings.setMirror(getMirror());
    settings.setIgnoreEntities(true);
    settings.setBoundingBox(&chunkBounds);

    // 原版忽略结构方块：它们只是模板里的占位标记。
    std::vector<u32> blocksToIgnore;
    if (auto* structureBlock = VanillaBlocks::getState(VanillaBlocks::STRUCTURE_BLOCK)) {
        blocksToIgnore.push_back(structureBlock->blockId());
    }
    feature::template_::StructureProcessorList processors;
    processors.addProcessor(std::make_unique<feature::template_::BlockIgnoreStructureProcessor>(blocksToIgnore));
    settings.setProcessors(&processors);

    // TODO: 处理模板数据标记（ChestWest/ChestEast/ChestSouth/ChestNorth 宝箱、Mage/Warrior
    //       生成灾厄村民）——需要战利品表与实体生成接口，当前只保证方块几何一致。
    //       它们需要排在下方 jigsaw 覆盖之前（与原版 postProcess 的顺序一致）。
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "WoodlandMansionPiece::generate requires an IWorld");
    m_template->placeInWorld(*reader, m_templatePosition, settings, rng, world::BlockUpdateFlags::UPDATE_CLIENTS);

    // 原版 `TemplateStructurePiece.postProcess` 末尾的拼图方块 `final_state` 覆盖。
    m_template->applyJigsawReplacements(*reader, m_templatePosition, settings);
}

namespace woodland_mansion {

SimpleGrid::SimpleGrid(i32 width, i32 height, i32 valueIfOutside)
    : m_width(width)
    , m_height(height)
    , m_valueIfOutside(valueIfOutside)
{
    m_grid.resize(static_cast<size_t>(width));
    for (auto& row : m_grid) {
        row.resize(static_cast<size_t>(height), 0);
    }
}

void SimpleGrid::set(i32 x, i32 y, i32 value)
{
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_grid[static_cast<size_t>(x)][static_cast<size_t>(y)] = value;
    }
}

void SimpleGrid::set(i32 x1, i32 y1, i32 x2, i32 y2, i32 value)
{
    for (i32 y = y1; y <= y2; ++y) {
        for (i32 x = x1; x <= x2; ++x) {
            set(x, y, value);
        }
    }
}

i32 SimpleGrid::get(i32 x, i32 y) const
{
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        return m_grid[static_cast<size_t>(x)][static_cast<size_t>(y)];
    }
    return m_valueIfOutside;
}

void SimpleGrid::setIf(i32 x, i32 y, i32 oldValue, i32 newValue)
{
    if (get(x, y) == oldValue) {
        set(x, y, newValue);
    }
}

bool SimpleGrid::edgesTo(i32 x, i32 y, i32 value) const
{
    return get(x - 1, y) == value || get(x + 1, y) == value || get(x, y + 1) == value || get(x, y - 1) == value;
}

// ============================================================================
// woodland_mansion::MansionGrid 实现
// ============================================================================

MansionGrid::MansionGrid(math::IRandom& rng)
    : m_rng(rng)
    , m_entranceX(ENTRANCE_OFFSET_X)
    , m_entranceY(ENTRANCE_OFFSET_Y)
{
    // 创建 11x11 网格
    m_baseGrid = std::make_unique<SimpleGrid>(MANSION_GRID_SIZE, MANSION_GRID_SIZE, MANSION_GRID_OUTSIDE_VALUE);

    // 设置入口区域
    m_baseGrid->set(m_entranceX, m_entranceY, m_entranceX + 1, m_entranceY + 1, 3);
    m_baseGrid->set(m_entranceX - 1, m_entranceY, m_entranceX - 1, m_entranceY + 1, 2);
    m_baseGrid->set(m_entranceX + 2, m_entranceY - 2, m_entranceX + 3, m_entranceY + 3, 5);
    m_baseGrid->set(m_entranceX + 1, m_entranceY - 2, m_entranceX + 1, m_entranceY - 1, 1);
    m_baseGrid->set(m_entranceX + 1, m_entranceY + 2, m_entranceX + 1, m_entranceY + 3, 1);
    m_baseGrid->set(m_entranceX - 1, m_entranceY - 1, 1);
    m_baseGrid->set(m_entranceX - 1, m_entranceY + 2, 1);
    m_baseGrid->set(0, 0, MANSION_GRID_SIZE, 1, MANSION_GRID_OUTSIDE_VALUE);
    m_baseGrid->set(0, 9, MANSION_GRID_SIZE, MANSION_GRID_SIZE, MANSION_GRID_OUTSIDE_VALUE);

    // 递归走廊生成
    _recursiveCorridor(*m_baseGrid, m_entranceX, m_entranceY - 2, Direction::West, 6);
    _recursiveCorridor(*m_baseGrid, m_entranceX, m_entranceY + 3, Direction::West, 6);
    _recursiveCorridor(*m_baseGrid, m_entranceX - 2, m_entranceY - 1, Direction::West, 3);
    _recursiveCorridor(*m_baseGrid, m_entranceX - 2, m_entranceY + 2, Direction::West, 3);

    // 清理边缘
    while (_cleanEdges(*m_baseGrid)) {}

    // 创建楼层房间网格
    m_floorRooms[0] = std::make_unique<SimpleGrid>(MANSION_GRID_SIZE, MANSION_GRID_SIZE, MANSION_GRID_OUTSIDE_VALUE);
    m_floorRooms[1] = std::make_unique<SimpleGrid>(MANSION_GRID_SIZE, MANSION_GRID_SIZE, MANSION_GRID_OUTSIDE_VALUE);
    m_floorRooms[2] = std::make_unique<SimpleGrid>(MANSION_GRID_SIZE, MANSION_GRID_SIZE, MANSION_GRID_OUTSIDE_VALUE);

    _identifyRooms(*m_baseGrid, *m_floorRooms[0]);
    _identifyRooms(*m_baseGrid, *m_floorRooms[1]);

    // 标记入口
    m_floorRooms[0]->set(m_entranceX + 1, m_entranceY, m_entranceX + 1, m_entranceY + 1, 8388608);
    m_floorRooms[1]->set(m_entranceX + 1, m_entranceY, m_entranceX + 1, m_entranceY + 1, 8388608);

    // 设置三楼
    m_thirdFloorGrid =
        std::make_unique<SimpleGrid>(m_baseGrid->width(), m_baseGrid->height(), MANSION_GRID_OUTSIDE_VALUE);
    _setupThirdFloor();
    _identifyRooms(*m_thirdFloorGrid, *m_floorRooms[2]);
}

bool MansionGrid::isHouse(const SimpleGrid& grid, i32 x, i32 y)
{
    i32 value = grid.get(x, y);
    return value == 1 || value == 2 || value == 3 || value == 4;
}

bool MansionGrid::isRoomId(const SimpleGrid& grid, i32 x, i32 y, i32 floor, i32 roomId) const
{
    return (m_floorRooms[floor]->get(x, y) & 0xFFFF) == roomId;
}

Direction MansionGrid::get1x2RoomDirection(const SimpleGrid& grid, i32 x, i32 y, i32 floor, i32 roomId) const
{
    for (i32 i = 0; i < 4; ++i) {
        Direction dir = static_cast<Direction>(i + 2); // Start from North (2)
        i32 dx = Directions::xOffset(dir);
        i32 dz = Directions::zOffset(dir);
        if (isRoomId(grid, x + dx, y + dz, floor, roomId)) {
            return dir;
        }
    }
    return Direction::North; // 默认
}

void MansionGrid::_recursiveCorridor(SimpleGrid& grid, i32 x, i32 y, Direction dir, i32 depth)
{
    if (depth <= 0) {
        return;
    }

    grid.set(x, y, 1);

    i32 dx = Directions::xOffset(dir);
    i32 dz = Directions::zOffset(dir);

    grid.setIf(x + dx, y + dz, 0, 1);

    // 随机分支。方向表必须是 `Direction.from2DDataValue` 的顺序
    // （SOUTH=0, WEST=1, NORTH=2, EAST=3），不是 `Plane.HORIZONTAL` 的 NORTH→EAST→SOUTH→WEST：
    // 顺序不同会直接换一条走廊走向，整张布局随之后移。
    const Direction by2DData[] = {Direction::South, Direction::West, Direction::North, Direction::East};
    for (i32 i = 0; i < 8; ++i) {
        Direction newDir = by2DData[m_rng.nextInt(4)];
        // 不往回走，且有限制向东
        if (newDir != Directions::opposite(dir) && (newDir != Direction::East || !m_rng.nextBoolean())) {
            i32 nx = x + dx;
            i32 ny = y + dz;
            i32 ndx = Directions::xOffset(newDir);
            i32 ndz = Directions::zOffset(newDir);

            if (grid.get(nx + ndx, ny + ndz) == 0 && grid.get(nx + ndx * 2, ny + ndz * 2) == 0) {
                _recursiveCorridor(grid, nx + ndx, ny + ndz, newDir, depth - 1);
                break;
            }
        }
    }

    // 设置走廊两侧
    Direction leftDir = Directions::rotateYCCW(dir); // CCW
    Direction rightDir = Directions::rotateY(dir);   // CW

    i32 ldx = Directions::xOffset(leftDir);
    i32 ldz = Directions::zOffset(leftDir);

    i32 rdx = Directions::xOffset(rightDir);
    i32 rdz = Directions::zOffset(rightDir);

    grid.setIf(x + ldx, y + ldz, 0, 2);
    grid.setIf(x + rdx, y + rdz, 0, 2);
    grid.setIf(x + dx + ldx, y + dz + ldz, 0, 2);
    grid.setIf(x + dx + rdx, y + dz + rdz, 0, 2);
    grid.setIf(x + dx * 2, y + dz * 2, 0, 2);
    grid.setIf(x + ldx * 2, y + ldz * 2, 0, 2);
    grid.setIf(x + rdx * 2, y + rdz * 2, 0, 2);
}

bool MansionGrid::_cleanEdges(SimpleGrid& grid)
{
    bool changed = false;

    for (i32 y = 0; y < grid.height(); ++y) {
        for (i32 x = 0; x < grid.width(); ++x) {
            if (grid.get(x, y) == 0) {
                i32 count = 0;
                count += isHouse(grid, x + 1, y) ? 1 : 0;
                count += isHouse(grid, x - 1, y) ? 1 : 0;
                count += isHouse(grid, x, y + 1) ? 1 : 0;
                count += isHouse(grid, x, y - 1) ? 1 : 0;

                if (count >= 3) {
                    grid.set(x, y, 2);
                    changed = true;
                } else if (count == 2) {
                    i32 corners = 0;
                    corners += isHouse(grid, x + 1, y + 1) ? 1 : 0;
                    corners += isHouse(grid, x - 1, y + 1) ? 1 : 0;
                    corners += isHouse(grid, x + 1, y - 1) ? 1 : 0;
                    corners += isHouse(grid, x - 1, y - 1) ? 1 : 0;

                    if (corners <= 1) {
                        grid.set(x, y, 2);
                        changed = true;
                    }
                }
            }
        }
    }

    return changed;
}

void MansionGrid::_identifyRooms(const SimpleGrid& sourceGrid, SimpleGrid& roomGrid)
{
    // 收集所有值为 2 的位置（潜在房间）
    std::vector<std::pair<i32, i32>> rooms;
    for (i32 y = 0; y < sourceGrid.height(); ++y) {
        for (i32 x = 0; x < sourceGrid.width(); ++x) {
            if (sourceGrid.get(x, y) == 2) {
                rooms.emplace_back(x, y);
            }
        }
    }

    // 随机打乱（与MC原版使用Collections.shuffle等效）
    for (size_t i = rooms.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(m_rng.nextInt(static_cast<i32>(i)));
        std::swap(rooms[i - 1], rooms[j]);
    }

    i32 roomId = 10;

    for (const auto& room : rooms) {
        i32 x = room.first;
        i32 y = room.second;

        if (roomGrid.get(x, y) == 0) {
            i32 x1 = x;
            i32 x2 = x;
            i32 y1 = y;
            i32 y2 = y;
            i32 roomType = 65536; // 0x10000: 1x1 房间

            // 检查是否可以形成更大的房间
            if (roomGrid.get(x + 1, y) == 0 && roomGrid.get(x, y + 1) == 0 && roomGrid.get(x + 1, y + 1) == 0 &&
                sourceGrid.get(x + 1, y) == 2 && sourceGrid.get(x, y + 1) == 2 && sourceGrid.get(x + 1, y + 1) == 2) {
                // 2x2 房间
                x2 = x + 1;
                y2 = y + 1;
                roomType = 262144; // 0x40000
            } else if (roomGrid.get(x - 1, y) == 0 && roomGrid.get(x, y + 1) == 0 && roomGrid.get(x - 1, y + 1) == 0 &&
                sourceGrid.get(x - 1, y) == 2 && sourceGrid.get(x, y + 1) == 2 && sourceGrid.get(x - 1, y + 1) == 2) {
                x1 = x - 1;
                y2 = y + 1;
                roomType = 262144;
            } else if (roomGrid.get(x - 1, y) == 0 && roomGrid.get(x, y - 1) == 0 && roomGrid.get(x - 1, y - 1) == 0 &&
                sourceGrid.get(x - 1, y) == 2 && sourceGrid.get(x, y - 1) == 2 && sourceGrid.get(x - 1, y - 1) == 2) {
                x1 = x - 1;
                y1 = y - 1;
                roomType = 262144;
            } else if (roomGrid.get(x + 1, y) == 0 && sourceGrid.get(x + 1, y) == 2) {
                // 1x2 水平房间
                x2 = x + 1;
                roomType = 131072; // 0x20000
            } else if (roomGrid.get(x, y + 1) == 0 && sourceGrid.get(x, y + 1) == 2) {
                // 1x2 垂直房间
                y2 = y + 1;
                roomType = 131072;
            } else if (roomGrid.get(x - 1, y) == 0 && sourceGrid.get(x - 1, y) == 2) {
                x1 = x - 1;
                roomType = 131072;
            } else if (roomGrid.get(x, y - 1) == 0 && sourceGrid.get(x, y - 1) == 2) {
                y1 = y - 1;
                roomType = 131072;
            }

            // 随机选择门位置（从房间范围内随机选一个单元格作为门位置）
            i32 doorX = m_rng.nextBoolean() ? x1 : x2;
            i32 doorY = m_rng.nextBoolean() ? y1 : y2;

            // 检查门位置是否与走廊(value=1)相邻，0x200000 表示有走廊入口
            i32 doorFlag = 2097152; // 0x200000
            if (!sourceGrid.edgesTo(doorX, doorY, 1)) {
                // 尝试交换X
                doorX = (doorX == x1) ? x2 : x1;
                doorY = (doorY == y1) ? y2 : y1;
                if (!sourceGrid.edgesTo(doorX, doorY, 1)) {
                    // 尝试仅交换Y
                    doorY = (doorY == y1) ? y2 : y1;
                    if (!sourceGrid.edgesTo(doorX, doorY, 1)) {
                        // 尝试仅交换X
                        doorX = (doorX == x1) ? x2 : x1;
                        doorY = (doorY == y1) ? y2 : y1;
                        if (!sourceGrid.edgesTo(doorX, doorY, 1)) {
                            // 没有任何位置与走廊相邻，不设楼梯入口标志
                            doorFlag = 0;
                            doorX = x1;
                            doorY = y1;
                        }
                    }
                }
            }

            // 设置房间网格
            for (i32 ry = y1; ry <= y2; ++ry) {
                for (i32 rx = x1; rx <= x2; ++rx) {
                    if (rx == doorX && ry == doorY) {
                        // 门位置：设置 0x100000（门位置标志）| 楼梯入口标志 | 房间类型 | 房间ID
                        roomGrid.set(rx, ry, 1048576 | doorFlag | roomType | roomId);
                    } else {
                        // 非门位置：仅设置房间类型 | 房间ID
                        roomGrid.set(rx, ry, roomType | roomId);
                    }
                }
            }
            ++roomId;
        }
    }
}

void MansionGrid::_setupThirdFloor()
{
    // 找到二楼有楼梯的房间：1x2房间类型(0x20000)且有走廊入口标志(0x200000)
    std::vector<std::pair<i32, i32>> stairRooms;
    SimpleGrid& secondFloor = *m_floorRooms[1];

    for (i32 y = 0; y < m_baseGrid->height(); ++y) {
        for (i32 x = 0; x < m_baseGrid->width(); ++x) {
            i32 value = secondFloor.get(x, y);
            i32 roomType = value & 0xF0000;
            if (roomType == 0x20000 && (value & 0x200000) == 0x200000) {
                // 1x2 房间且有走廊入口（可放置楼梯）
                stairRooms.emplace_back(x, y);
            }
        }
    }

    if (stairRooms.empty()) {
        // 没有楼梯房间，三楼为空
        m_thirdFloorGrid->set(0, 0, m_thirdFloorGrid->width(), m_thirdFloorGrid->height(), MANSION_GRID_OUTSIDE_VALUE);
        return;
    }

    // 随机选择一个楼梯房间
    auto& chosen = stairRooms[static_cast<size_t>(m_rng.nextInt(static_cast<i32>(stairRooms.size())))];
    i32 sx = chosen.first;
    i32 sy = chosen.second;

    // 标记二楼该房间为通往三楼（0x400000）
    i32 oldValue = secondFloor.get(sx, sy);
    secondFloor.set(sx, sy, oldValue | 0x400000);

    // 获取1x2房间的方向，找到另一个单元格的位置
    Direction roomDir = get1x2RoomDirection(*m_baseGrid, sx, sy, 1, oldValue & 0xFFFF);
    i32 adjX = sx + Directions::xOffset(roomDir);
    i32 adjY = sy + Directions::zOffset(roomDir);

    // 设置三楼布局
    for (i32 y = 0; y < m_thirdFloorGrid->height(); ++y) {
        for (i32 x = 0; x < m_thirdFloorGrid->width(); ++x) {
            if (!isHouse(*m_baseGrid, x, y)) {
                m_thirdFloorGrid->set(x, y, MANSION_GRID_OUTSIDE_VALUE);
            } else if (x == sx && y == sy) {
                // 楼梯单元格
                m_thirdFloorGrid->set(x, y, 3);
            } else if (x == adjX && y == adjY) {
                // 1x2房间的另一个单元格，作为走廊起点
                m_thirdFloorGrid->set(x, y, 3);
                // 标记为入口（0x800000）
                m_floorRooms[2]->set(x, y, 8388608);
            }
        }
    }

    // 从走廊起点查找可用方向
    std::vector<Direction> availableDirs;
    const Direction horizontalDirs[] = {Direction::North, Direction::East, Direction::South, Direction::West};
    for (i32 i = 0; i < 4; ++i) {
        Direction dir = horizontalDirs[i];
        i32 nx = adjX + Directions::xOffset(dir);
        i32 ny = adjY + Directions::zOffset(dir);
        if (m_thirdFloorGrid->get(nx, ny) == 0) {
            availableDirs.push_back(dir);
        }
    }

    if (availableDirs.empty()) {
        // 没有可用方向，清除三楼并恢复二楼标记
        m_thirdFloorGrid->set(0, 0, m_thirdFloorGrid->width(), m_thirdFloorGrid->height(), MANSION_GRID_OUTSIDE_VALUE);
        secondFloor.set(sx, sy, oldValue); // 恢复原始值（去掉0x400000标志）
    } else {
        // 从随机可用方向开始递归走廊生成
        Direction corridorDir =
            availableDirs[static_cast<size_t>(m_rng.nextInt(static_cast<i32>(availableDirs.size())))];
        i32 startX = adjX + Directions::xOffset(corridorDir);
        i32 startY = adjY + Directions::zOffset(corridorDir);
        _recursiveCorridor(*m_thirdFloorGrid, startX, startY, corridorDir, 4);

        // 清理边缘
        while (_cleanEdges(*m_thirdFloorGrid)) {}
    }
}

// ============================================================================
// woodland_mansion::MansionPlacer 实现
// ============================================================================

namespace {

/// 原版 `BlockPos.offset(Direction, int)`：沿水平方向平移 n 格，Y 不变。
BlockPos offsetTowards(const BlockPos& pos, Direction dir, i32 n)
{
    return BlockPos(pos.x + Directions::xOffset(dir) * n, pos.y, pos.z + Directions::zOffset(dir) * n);
}

/// 原版 `rotation.rotate(Direction)`：把水平方向按旋转搬到世界朝向。
Direction rotateDir(Direction dir, Rotation rotation)
{
    return Directions::rotateDirection(dir, rotation);
}

} // namespace

MansionPlacer::MansionPlacer(feature::template_::TemplateManager& templateManager, math::IRandom& rng)
    : m_templateManager(templateManager)
    , m_rng(rng)
    , m_startX(0)
    , m_startY(0)
{}

void MansionPlacer::createMansion(const BlockPos& startPos,
    Rotation rotation,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const MansionGrid& grid)
{
    PlacementData ground;
    ground.position = startPos;
    ground.rotation = rotation;
    ground.wallType = "wall_flat";

    PlacementData first;
    _entrance(pieces, ground);
    first.position = BlockPos(ground.position.x, ground.position.y + 8, ground.position.z);
    first.rotation = ground.rotation;
    first.wallType = "wall_window";

    const SimpleGrid& baseGrid = grid.baseGrid();
    const SimpleGrid& thirdFloorGrid = grid.thirdFloorGrid();

    m_startX = grid.entranceX() + 1;
    m_startY = grid.entranceY() + 1;
    const i32 entranceCornerX = grid.entranceX() + 1;
    const i32 entranceCornerY = grid.entranceY();
    _traverseOuterWalls(
        pieces, ground, baseGrid, Direction::South, m_startX, m_startY, entranceCornerX, entranceCornerY);
    _traverseOuterWalls(
        pieces, first, baseGrid, Direction::South, m_startX, m_startY, entranceCornerX, entranceCornerY);

    PlacementData second;
    second.position = BlockPos(ground.position.x, ground.position.y + 19, ground.position.z);
    second.rotation = ground.rotation;
    second.wallType = "wall_window";

    bool found = false;
    for (i32 y = 0; y < thirdFloorGrid.height() && !found; ++y) {
        for (i32 x = thirdFloorGrid.width() - 1; x >= 0 && !found; --x) {
            if (!MansionGrid::isHouse(thirdFloorGrid, x, y)) {
                continue;
            }
            second.position =
                offsetTowards(second.position, rotateDir(Direction::South, rotation), 8 + (y - m_startY) * 8);
            second.position = offsetTowards(second.position, rotateDir(Direction::East, rotation), (x - m_startX) * 8);
            _traverseWallPiece(pieces, second);
            _traverseOuterWalls(pieces, second, thirdFloorGrid, Direction::South, x, y, x, y);
            found = true;
        }
    }

    _createRoof(pieces, BlockPos(startPos.x, startPos.y + 16, startPos.z), rotation, baseGrid, &thirdFloorGrid);
    _createRoof(pieces, BlockPos(startPos.x, startPos.y + 27, startPos.z), rotation, thirdFloorGrid, nullptr);

    const FirstFloorRoomCollection firstFloorRooms;
    const SecondFloorRoomCollection secondFloorRooms;
    const ThirdFloorRoomCollection thirdFloorRooms;
    const RoomCollection* roomCollections[3] = {&firstFloorRooms, &secondFloorRooms, &thirdFloorRooms};

    for (i32 floor = 0; floor < 3; ++floor) {
        const BlockPos floorOrigin(startPos.x, startPos.y + 8 * floor + (floor == 2 ? 3 : 0), startPos.z);
        const SimpleGrid& roomGrid = grid.floorRoom(floor);
        const SimpleGrid& floorGrid = (floor == 2) ? thirdFloorGrid : baseGrid;
        const std::string carpetSouth = floor == 0 ? "carpet_south_1" : "carpet_south_2";
        const std::string carpetWest = floor == 0 ? "carpet_west_1" : "carpet_west_2";
        const std::string indoorsWall = floor == 0 ? "indoors_wall_1" : "indoors_wall_2";
        const std::string indoorsDoor = floor == 0 ? "indoors_door_1" : "indoors_door_2";

        // ---- 走廊地板与地毯 ----
        for (i32 y = 0; y < floorGrid.height(); ++y) {
            for (i32 x = 0; x < floorGrid.width(); ++x) {
                if (floorGrid.get(x, y) != 1) {
                    continue;
                }
                BlockPos corridorPos =
                    offsetTowards(floorOrigin, rotateDir(Direction::South, rotation), 8 + (y - m_startY) * 8);
                corridorPos = offsetTowards(corridorPos, rotateDir(Direction::East, rotation), (x - m_startX) * 8);
                pieces.push_back(std::make_unique<WoodlandMansionPiece>("corridor_floor", corridorPos, rotation));

                if (floorGrid.get(x, y - 1) == 1 || (roomGrid.get(x, y - 1) & 8388608) == 8388608) {
                    BlockPos pos = offsetTowards(corridorPos, rotateDir(Direction::East, rotation), 1);
                    pos = BlockPos(pos.x, pos.y + 1, pos.z);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>("carpet_north", pos, rotation));
                }
                if (floorGrid.get(x + 1, y) == 1 || (roomGrid.get(x + 1, y) & 8388608) == 8388608) {
                    BlockPos pos = offsetTowards(corridorPos, rotateDir(Direction::South, rotation), 1);
                    pos = offsetTowards(pos, rotateDir(Direction::East, rotation), 5);
                    pos = BlockPos(pos.x, pos.y + 1, pos.z);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>("carpet_east", pos, rotation));
                }
                if (floorGrid.get(x, y + 1) == 1 || (roomGrid.get(x, y + 1) & 8388608) == 8388608) {
                    BlockPos pos = offsetTowards(corridorPos, rotateDir(Direction::South, rotation), 5);
                    pos = offsetTowards(pos, rotateDir(Direction::West, rotation), 1);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(carpetSouth, pos, rotation));
                }
                if (floorGrid.get(x - 1, y) == 1 || (roomGrid.get(x - 1, y) & 8388608) == 8388608) {
                    BlockPos pos = offsetTowards(corridorPos, rotateDir(Direction::West, rotation), 1);
                    pos = offsetTowards(pos, rotateDir(Direction::North, rotation), 1);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(carpetWest, pos, rotation));
                }
            }
        }

        // ---- 房间：内墙、门与房间模板 ----
        std::vector<Direction> openSides;
        for (i32 y = 0; y < floorGrid.height(); ++y) {
            for (i32 x = 0; x < floorGrid.width(); ++x) {
                bool isStairCell = floor == 2 && floorGrid.get(x, y) == 3;
                if (floorGrid.get(x, y) != 2 && !isStairCell) {
                    continue;
                }
                const i32 roomValue = roomGrid.get(x, y);
                const i32 roomType = roomValue & 983040;
                const i32 roomId = roomValue & 0xFFFF;
                isStairCell = isStairCell && (roomValue & 8388608) == 8388608;

                openSides.clear();
                if ((roomValue & 2097152) == 2097152) {
                    const Direction planeHorizontal[] = {
                        Direction::North, Direction::East, Direction::South, Direction::West};
                    for (const Direction dir : planeHorizontal) {
                        if (floorGrid.get(x + Directions::xOffset(dir), y + Directions::zOffset(dir)) == 1) {
                            openSides.push_back(dir);
                        }
                    }
                }

                Direction facing = Direction::None;
                if (!openSides.empty()) {
                    facing = openSides[static_cast<size_t>(m_rng.nextInt(static_cast<i32>(openSides.size())))];
                } else if ((roomValue & 1048576) == 1048576) {
                    facing = Direction::Up;
                }

                BlockPos roomPos =
                    offsetTowards(floorOrigin, rotateDir(Direction::South, rotation), 8 + (y - m_startY) * 8);
                roomPos = offsetTowards(roomPos, rotateDir(Direction::East, rotation), -1 + (x - m_startX) * 8);

                if (MansionGrid::isHouse(floorGrid, x - 1, y) && !grid.isRoomId(floorGrid, x - 1, y, floor, roomId)) {
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        facing == Direction::West ? indoorsDoor : indoorsWall, roomPos, rotation));
                }

                if (floorGrid.get(x + 1, y) == 1 && !isStairCell) {
                    BlockPos pos = offsetTowards(roomPos, rotateDir(Direction::East, rotation), 8);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        facing == Direction::East ? indoorsDoor : indoorsWall, pos, rotation));
                }

                if (MansionGrid::isHouse(floorGrid, x, y + 1) && !grid.isRoomId(floorGrid, x, y + 1, floor, roomId)) {
                    BlockPos pos = offsetTowards(roomPos, rotateDir(Direction::South, rotation), 7);
                    pos = offsetTowards(pos, rotateDir(Direction::East, rotation), 7);
                    pieces.push_back(
                        std::make_unique<WoodlandMansionPiece>(facing == Direction::South ? indoorsDoor : indoorsWall,
                            pos,
                            Rotations::add(rotation, Rotation::Clockwise90)));
                }

                if (floorGrid.get(x, y - 1) == 1 && !isStairCell) {
                    BlockPos pos = offsetTowards(roomPos, rotateDir(Direction::North, rotation), 1);
                    pos = offsetTowards(pos, rotateDir(Direction::East, rotation), 7);
                    pieces.push_back(
                        std::make_unique<WoodlandMansionPiece>(facing == Direction::North ? indoorsDoor : indoorsWall,
                            pos,
                            Rotations::add(rotation, Rotation::Clockwise90)));
                }

                if (roomType == 65536) {
                    _addRoom1x1(pieces, roomPos, rotation, facing, *roomCollections[floor]);
                } else if (roomType == 131072 && facing != Direction::None) {
                    const Direction roomDir = grid.get1x2RoomDirection(floorGrid, x, y, floor, roomId);
                    const bool isStairs = (roomValue & 4194304) == 4194304;
                    _addRoom1x2(pieces, roomPos, rotation, roomDir, facing, *roomCollections[floor], isStairs);
                } else if (roomType == 262144 && facing != Direction::None && facing != Direction::Up) {
                    Direction roomDir = Directions::rotateY(facing);
                    if (!grid.isRoomId(floorGrid,
                            x + Directions::xOffset(roomDir),
                            y + Directions::zOffset(roomDir),
                            floor,
                            roomId)) {
                        roomDir = Directions::opposite(roomDir);
                    }
                    _addRoom2x2(pieces, roomPos, rotation, roomDir, facing, *roomCollections[floor]);
                } else if (roomType == 262144 && facing == Direction::Up) {
                    _addRoom2x2Secret(pieces, roomPos, rotation, *roomCollections[floor]);
                }
            }
        }
    }
}

void MansionPlacer::_entrance(std::vector<std::unique_ptr<StructurePiece>>& pieces, PlacementData& data)
{
    const Direction west = rotateDir(Direction::West, data.rotation);
    pieces.push_back(
        std::make_unique<WoodlandMansionPiece>("entrance", offsetTowards(data.position, west, 9), data.rotation));
    data.position = offsetTowards(data.position, rotateDir(Direction::South, data.rotation), 16);
}

void MansionPlacer::_traverseWallPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces, PlacementData& data)
{
    const BlockPos pos = offsetTowards(data.position, rotateDir(Direction::East, data.rotation), 7);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(data.wallType, pos, data.rotation));
    data.position = offsetTowards(data.position, rotateDir(Direction::South, data.rotation), 8);
}

void MansionPlacer::_traverseTurn(std::vector<std::unique_ptr<StructurePiece>>& pieces, PlacementData& data)
{
    data.position = offsetTowards(data.position, rotateDir(Direction::South, data.rotation), -1);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>("wall_corner", data.position, data.rotation));
    data.position = offsetTowards(data.position, rotateDir(Direction::South, data.rotation), -7);
    data.position = offsetTowards(data.position, rotateDir(Direction::West, data.rotation), -6);
    data.rotation = Rotations::add(data.rotation, Rotation::Clockwise90);
}

void MansionPlacer::_traverseInnerTurn(std::vector<std::unique_ptr<StructurePiece>>& pieces, PlacementData& data)
{
    MC_UNUSED(pieces);
    data.position = offsetTowards(data.position, rotateDir(Direction::South, data.rotation), 6);
    data.position = offsetTowards(data.position, rotateDir(Direction::East, data.rotation), 8);
    data.rotation = Rotations::add(data.rotation, Rotation::CounterClockwise90);
}

void MansionPlacer::_traverseOuterWalls(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    PlacementData& data,
    const SimpleGrid& grid,
    Direction startDir,
    i32 startX,
    i32 startY,
    i32 targetX,
    i32 targetY)
{
    i32 x = startX;
    i32 y = startY;
    Direction dir = startDir;

    do {
        const i32 stepX = x + Directions::xOffset(dir);
        const i32 stepY = y + Directions::zOffset(dir);
        if (!MansionGrid::isHouse(grid, stepX, stepY)) {
            _traverseTurn(pieces, data);
            dir = Directions::rotateY(dir);
            if (x != targetX || y != targetY || startDir != dir) {
                _traverseWallPiece(pieces, data);
            }
        } else {
            const Direction sideDir = Directions::rotateYCCW(dir);
            if (MansionGrid::isHouse(
                    grid, stepX + Directions::xOffset(sideDir), stepY + Directions::zOffset(sideDir))) {
                _traverseInnerTurn(pieces, data);
                x += Directions::xOffset(dir);
                y += Directions::zOffset(dir);
                dir = sideDir;
            } else {
                x += Directions::xOffset(dir);
                y += Directions::zOffset(dir);
                if (x != targetX || y != targetY || startDir != dir) {
                    _traverseWallPiece(pieces, data);
                }
            }
        }
    } while (x != targetX || y != targetY || startDir != dir);
}

void MansionPlacer::_createRoof(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& basePos,
    Rotation rotation,
    const SimpleGrid& grid,
    const SimpleGrid* upperGrid)
{
    const auto cellOrigin = [&](i32 x, i32 y) {
        BlockPos pos = offsetTowards(basePos, rotateDir(Direction::South, rotation), 8 + (y - m_startY) * 8);
        return offsetTowards(pos, rotateDir(Direction::East, rotation), (x - m_startX) * 8);
    };

    // ---- 平屋顶与四面 roof_front ----
    for (i32 y = 0; y < grid.height(); ++y) {
        for (i32 x = 0; x < grid.width(); ++x) {
            const bool coveredAbove = upperGrid != nullptr && MansionGrid::isHouse(*upperGrid, x, y);
            if (!MansionGrid::isHouse(grid, x, y) || coveredAbove) {
                continue;
            }
            // 只有平屋顶本体上抬 3 格；四面的 roof_front 用未上抬的单元原点。
            const BlockPos pos = cellOrigin(x, y);
            pieces.push_back(
                std::make_unique<WoodlandMansionPiece>("roof", BlockPos(pos.x, pos.y + 3, pos.z), rotation));

            if (!MansionGrid::isHouse(grid, x + 1, y)) {
                const BlockPos front = offsetTowards(pos, rotateDir(Direction::East, rotation), 6);
                pieces.push_back(std::make_unique<WoodlandMansionPiece>("roof_front", front, rotation));
            }
            if (!MansionGrid::isHouse(grid, x - 1, y)) {
                BlockPos front = offsetTowards(pos, rotateDir(Direction::East, rotation), 0);
                front = offsetTowards(front, rotateDir(Direction::South, rotation), 7);
                pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                    "roof_front", front, Rotations::add(rotation, Rotation::Clockwise180)));
            }
            if (!MansionGrid::isHouse(grid, x, y - 1)) {
                const BlockPos front = offsetTowards(pos, rotateDir(Direction::West, rotation), 1);
                pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                    "roof_front", front, Rotations::add(rotation, Rotation::CounterClockwise90)));
            }
            if (!MansionGrid::isHouse(grid, x, y + 1)) {
                BlockPos front = offsetTowards(pos, rotateDir(Direction::East, rotation), 6);
                front = offsetTowards(front, rotateDir(Direction::South, rotation), 6);
                pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                    "roof_front", front, Rotations::add(rotation, Rotation::Clockwise90)));
            }
        }
    }

    // ---- 上下层交界处的小墙与转角 ----
    if (upperGrid != nullptr) {
        for (i32 y = 0; y < grid.height(); ++y) {
            for (i32 x = 0; x < grid.width(); ++x) {
                if (!MansionGrid::isHouse(grid, x, y) || !MansionGrid::isHouse(*upperGrid, x, y)) {
                    continue;
                }
                const BlockPos pos = cellOrigin(x, y);

                if (!MansionGrid::isHouse(grid, x + 1, y)) {
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "small_wall", offsetTowards(pos, rotateDir(Direction::East, rotation), 7), rotation));
                }
                if (!MansionGrid::isHouse(grid, x - 1, y)) {
                    BlockPos wall = offsetTowards(pos, rotateDir(Direction::West, rotation), 1);
                    wall = offsetTowards(wall, rotateDir(Direction::South, rotation), 6);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "small_wall", wall, Rotations::add(rotation, Rotation::Clockwise180)));
                }
                if (!MansionGrid::isHouse(grid, x, y - 1)) {
                    BlockPos wall = offsetTowards(pos, rotateDir(Direction::West, rotation), 0);
                    wall = offsetTowards(wall, rotateDir(Direction::North, rotation), 1);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "small_wall", wall, Rotations::add(rotation, Rotation::CounterClockwise90)));
                }
                if (!MansionGrid::isHouse(grid, x, y + 1)) {
                    BlockPos wall = offsetTowards(pos, rotateDir(Direction::East, rotation), 6);
                    wall = offsetTowards(wall, rotateDir(Direction::South, rotation), 7);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "small_wall", wall, Rotations::add(rotation, Rotation::Clockwise90)));
                }

                if (!MansionGrid::isHouse(grid, x + 1, y)) {
                    if (!MansionGrid::isHouse(grid, x, y - 1)) {
                        BlockPos corner = offsetTowards(pos, rotateDir(Direction::East, rotation), 7);
                        corner = offsetTowards(corner, rotateDir(Direction::North, rotation), 2);
                        pieces.push_back(std::make_unique<WoodlandMansionPiece>("small_wall_corner", corner, rotation));
                    }
                    if (!MansionGrid::isHouse(grid, x, y + 1)) {
                        BlockPos corner = offsetTowards(pos, rotateDir(Direction::East, rotation), 8);
                        corner = offsetTowards(corner, rotateDir(Direction::South, rotation), 7);
                        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                            "small_wall_corner", corner, Rotations::add(rotation, Rotation::Clockwise90)));
                    }
                }
                if (!MansionGrid::isHouse(grid, x - 1, y)) {
                    if (!MansionGrid::isHouse(grid, x, y - 1)) {
                        BlockPos corner = offsetTowards(pos, rotateDir(Direction::West, rotation), 2);
                        corner = offsetTowards(corner, rotateDir(Direction::North, rotation), 1);
                        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                            "small_wall_corner", corner, Rotations::add(rotation, Rotation::CounterClockwise90)));
                    }
                    if (!MansionGrid::isHouse(grid, x, y + 1)) {
                        BlockPos corner = offsetTowards(pos, rotateDir(Direction::West, rotation), 1);
                        corner = offsetTowards(corner, rotateDir(Direction::South, rotation), 8);
                        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                            "small_wall_corner", corner, Rotations::add(rotation, Rotation::Clockwise180)));
                    }
                }
            }
        }
    }

    // ---- 屋顶转角与内转角 ----
    for (i32 y = 0; y < grid.height(); ++y) {
        for (i32 x = 0; x < grid.width(); ++x) {
            const bool coveredAbove = upperGrid != nullptr && MansionGrid::isHouse(*upperGrid, x, y);
            if (!MansionGrid::isHouse(grid, x, y) || coveredAbove) {
                continue;
            }
            const BlockPos pos = cellOrigin(x, y);

            if (!MansionGrid::isHouse(grid, x + 1, y)) {
                const BlockPos east = offsetTowards(pos, rotateDir(Direction::East, rotation), 6);
                if (!MansionGrid::isHouse(grid, x, y + 1)) {
                    const BlockPos corner = offsetTowards(east, rotateDir(Direction::South, rotation), 6);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>("roof_corner", corner, rotation));
                } else if (MansionGrid::isHouse(grid, x + 1, y + 1)) {
                    const BlockPos corner = offsetTowards(east, rotateDir(Direction::South, rotation), 5);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>("roof_inner_corner", corner, rotation));
                }
                if (!MansionGrid::isHouse(grid, x, y - 1)) {
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "roof_corner", east, Rotations::add(rotation, Rotation::CounterClockwise90)));
                } else if (MansionGrid::isHouse(grid, x + 1, y - 1)) {
                    BlockPos corner = offsetTowards(pos, rotateDir(Direction::East, rotation), 9);
                    corner = offsetTowards(corner, rotateDir(Direction::North, rotation), 2);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "roof_inner_corner", corner, Rotations::add(rotation, Rotation::Clockwise90)));
                }
            }

            if (!MansionGrid::isHouse(grid, x - 1, y)) {
                const BlockPos west = pos;
                if (!MansionGrid::isHouse(grid, x, y + 1)) {
                    const BlockPos corner = offsetTowards(west, rotateDir(Direction::South, rotation), 6);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "roof_corner", corner, Rotations::add(rotation, Rotation::Clockwise90)));
                } else if (MansionGrid::isHouse(grid, x - 1, y + 1)) {
                    BlockPos corner = offsetTowards(west, rotateDir(Direction::South, rotation), 8);
                    corner = offsetTowards(corner, rotateDir(Direction::West, rotation), 3);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "roof_inner_corner", corner, Rotations::add(rotation, Rotation::CounterClockwise90)));
                }
                if (!MansionGrid::isHouse(grid, x, y - 1)) {
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "roof_corner", west, Rotations::add(rotation, Rotation::Clockwise180)));
                } else if (MansionGrid::isHouse(grid, x - 1, y - 1)) {
                    const BlockPos corner = offsetTowards(west, rotateDir(Direction::South, rotation), 1);
                    pieces.push_back(std::make_unique<WoodlandMansionPiece>(
                        "roof_inner_corner", corner, Rotations::add(rotation, Rotation::Clockwise180)));
                }
            }
        }
    }
}

void MansionPlacer::_addRoom1x1(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    Rotation rotation,
    Direction doorDir,
    const RoomCollection& rooms)
{
    Rotation roomRotation = Rotation::None;
    std::string name = rooms.get1x1(m_rng);
    if (doorDir != Direction::East) {
        if (doorDir == Direction::North) {
            roomRotation = Rotations::add(roomRotation, Rotation::CounterClockwise90);
        } else if (doorDir == Direction::West) {
            roomRotation = Rotations::add(roomRotation, Rotation::Clockwise180);
        } else if (doorDir == Direction::South) {
            roomRotation = Rotations::add(roomRotation, Rotation::Clockwise90);
        } else {
            name = rooms.get1x1Secret(m_rng);
        }
    }

    BlockPos offset =
        feature::template_::Template::getZeroPositionWithTransform(BlockPos(1, 0, 0), Mirror::None, roomRotation, 7, 7);
    roomRotation = Rotations::add(roomRotation, rotation);
    offset = ::mc::rotate(offset, rotation);
    const BlockPos roomPos(pos.x + offset.x, pos.y, pos.z + offset.z);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(name, roomPos, roomRotation));
}

void MansionPlacer::_addRoom1x2(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    Rotation rotation,
    Direction roomDir,
    Direction doorDir,
    const RoomCollection& rooms,
    bool isStairs)
{
    const Direction east = rotateDir(Direction::East, rotation);
    const Direction south = rotateDir(Direction::South, rotation);
    const Direction north = rotateDir(Direction::North, rotation);
    const Direction west = rotateDir(Direction::West, rotation);

    if (doorDir == Direction::East && roomDir == Direction::South) {
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2SideEntrance(m_rng, isStairs), offsetTowards(pos, east, 1), rotation));
    } else if (doorDir == Direction::East && roomDir == Direction::North) {
        BlockPos target = offsetTowards(pos, east, 1);
        target = offsetTowards(target, south, 6);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2SideEntrance(m_rng, isStairs), target, rotation, Mirror::LeftRight));
    } else if (doorDir == Direction::West && roomDir == Direction::North) {
        BlockPos target = offsetTowards(pos, east, 7);
        target = offsetTowards(target, south, 6);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2SideEntrance(m_rng, isStairs), target, Rotations::add(rotation, Rotation::Clockwise180)));
    } else if (doorDir == Direction::West && roomDir == Direction::South) {
        const BlockPos target = offsetTowards(pos, east, 7);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2SideEntrance(m_rng, isStairs), target, rotation, Mirror::FrontBack));
    } else if (doorDir == Direction::South && roomDir == Direction::East) {
        const BlockPos target = offsetTowards(pos, east, 1);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(rooms.get1x2SideEntrance(m_rng, isStairs),
            target,
            Rotations::add(rotation, Rotation::Clockwise90),
            Mirror::LeftRight));
    } else if (doorDir == Direction::South && roomDir == Direction::West) {
        const BlockPos target = offsetTowards(pos, east, 7);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2SideEntrance(m_rng, isStairs), target, Rotations::add(rotation, Rotation::Clockwise90)));
    } else if (doorDir == Direction::North && roomDir == Direction::West) {
        BlockPos target = offsetTowards(pos, east, 7);
        target = offsetTowards(target, south, 6);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(rooms.get1x2SideEntrance(m_rng, isStairs),
            target,
            Rotations::add(rotation, Rotation::Clockwise90),
            Mirror::FrontBack));
    } else if (doorDir == Direction::North && roomDir == Direction::East) {
        BlockPos target = offsetTowards(pos, east, 1);
        target = offsetTowards(target, south, 6);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2SideEntrance(m_rng, isStairs), target, Rotations::add(rotation, Rotation::CounterClockwise90)));
    } else if (doorDir == Direction::South && roomDir == Direction::North) {
        BlockPos target = offsetTowards(pos, east, 1);
        target = offsetTowards(target, north, 8);
        pieces.push_back(
            std::make_unique<WoodlandMansionPiece>(rooms.get1x2FrontEntrance(m_rng, isStairs), target, rotation));
    } else if (doorDir == Direction::North && roomDir == Direction::South) {
        BlockPos target = offsetTowards(pos, east, 7);
        target = offsetTowards(target, south, 14);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2FrontEntrance(m_rng, isStairs), target, Rotations::add(rotation, Rotation::Clockwise180)));
    } else if (doorDir == Direction::West && roomDir == Direction::East) {
        const BlockPos target = offsetTowards(pos, east, 15);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2FrontEntrance(m_rng, isStairs), target, Rotations::add(rotation, Rotation::Clockwise90)));
    } else if (doorDir == Direction::East && roomDir == Direction::West) {
        BlockPos target = offsetTowards(pos, west, 7);
        target = offsetTowards(target, south, 6);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(rooms.get1x2FrontEntrance(m_rng, isStairs),
            target,
            Rotations::add(rotation, Rotation::CounterClockwise90)));
    } else if (doorDir == Direction::Up && roomDir == Direction::East) {
        const BlockPos target = offsetTowards(pos, east, 15);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(
            rooms.get1x2Secret(m_rng), target, Rotations::add(rotation, Rotation::Clockwise90)));
    } else if (doorDir == Direction::Up && roomDir == Direction::South) {
        const BlockPos target = offsetTowards(pos, east, 1);
        pieces.push_back(std::make_unique<WoodlandMansionPiece>(rooms.get1x2Secret(m_rng), target, rotation));
    }
}

void MansionPlacer::_addRoom2x2(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    Rotation rotation,
    Direction roomDir,
    Direction doorDir,
    const RoomCollection& rooms)
{
    i32 offsetX = 0;
    i32 offsetY = 0;
    Rotation roomRotation = rotation;
    Mirror mirror = Mirror::None;

    if (doorDir == Direction::East && roomDir == Direction::South) {
        offsetX = -7;
    } else if (doorDir == Direction::East && roomDir == Direction::North) {
        offsetX = -7;
        offsetY = 6;
        mirror = Mirror::LeftRight;
    } else if (doorDir == Direction::North && roomDir == Direction::East) {
        offsetX = 1;
        offsetY = 14;
        roomRotation = Rotations::add(rotation, Rotation::CounterClockwise90);
    } else if (doorDir == Direction::North && roomDir == Direction::West) {
        offsetX = 7;
        offsetY = 14;
        roomRotation = Rotations::add(rotation, Rotation::CounterClockwise90);
        mirror = Mirror::LeftRight;
    } else if (doorDir == Direction::South && roomDir == Direction::West) {
        offsetX = 7;
        offsetY = -8;
        roomRotation = Rotations::add(rotation, Rotation::Clockwise90);
    } else if (doorDir == Direction::South && roomDir == Direction::East) {
        offsetX = 1;
        offsetY = -8;
        roomRotation = Rotations::add(rotation, Rotation::Clockwise90);
        mirror = Mirror::LeftRight;
    } else if (doorDir == Direction::West && roomDir == Direction::North) {
        offsetX = 15;
        offsetY = 6;
        roomRotation = Rotations::add(rotation, Rotation::Clockwise180);
    } else if (doorDir == Direction::West && roomDir == Direction::South) {
        offsetX = 15;
        mirror = Mirror::FrontBack;
    }

    BlockPos target = offsetTowards(pos, rotateDir(Direction::East, rotation), offsetX);
    target = offsetTowards(target, rotateDir(Direction::South, rotation), offsetY);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(rooms.get2x2(m_rng), target, roomRotation, mirror));
}

void MansionPlacer::_addRoom2x2Secret(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    Rotation rotation,
    const RoomCollection& rooms)
{
    const BlockPos target = offsetTowards(pos, rotateDir(Direction::East, rotation), 1);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(rooms.get2x2Secret(m_rng), target, rotation, Mirror::None));
}

// ============================================================================
// RoomCollection 实现
// ============================================================================

std::string FirstFloorRoomCollection::get1x1(math::IRandom& rng) const
{
    return "1x1_a" + std::to_string(rng.nextInt(5) + 1);
}

std::string FirstFloorRoomCollection::get1x1Secret(math::IRandom& rng) const
{
    return "1x1_as" + std::to_string(rng.nextInt(4) + 1);
}

std::string FirstFloorRoomCollection::get1x2SideEntrance(math::IRandom& rng, bool isStairs) const
{
    return "1x2_a" + std::to_string(rng.nextInt(9) + 1);
}

std::string FirstFloorRoomCollection::get1x2FrontEntrance(math::IRandom& rng, bool isStairs) const
{
    return "1x2_b" + std::to_string(rng.nextInt(5) + 1);
}

std::string FirstFloorRoomCollection::get1x2Secret(math::IRandom& rng) const
{
    return "1x2_s" + std::to_string(rng.nextInt(2) + 1);
}

std::string FirstFloorRoomCollection::get2x2(math::IRandom& rng) const
{
    return "2x2_a" + std::to_string(rng.nextInt(4) + 1);
}

std::string FirstFloorRoomCollection::get2x2Secret(math::IRandom& rng) const
{
    return "2x2_s1";
}

std::string SecondFloorRoomCollection::get1x1(math::IRandom& rng) const
{
    return "1x1_b" + std::to_string(rng.nextInt(4) + 1);
}

std::string SecondFloorRoomCollection::get1x1Secret(math::IRandom& rng) const
{
    return "1x1_as" + std::to_string(rng.nextInt(4) + 1);
}

std::string SecondFloorRoomCollection::get1x2SideEntrance(math::IRandom& rng, bool isStairs) const
{
    return isStairs ? "1x2_c_stairs" : "1x2_c" + std::to_string(rng.nextInt(4) + 1);
}

std::string SecondFloorRoomCollection::get1x2FrontEntrance(math::IRandom& rng, bool isStairs) const
{
    return isStairs ? "1x2_d_stairs" : "1x2_d" + std::to_string(rng.nextInt(5) + 1);
}

std::string SecondFloorRoomCollection::get1x2Secret(math::IRandom& rng) const
{
    return "1x2_se" + std::to_string(rng.nextInt(1) + 1);
}

std::string SecondFloorRoomCollection::get2x2(math::IRandom& rng) const
{
    return "2x2_b" + std::to_string(rng.nextInt(5) + 1);
}

std::string SecondFloorRoomCollection::get2x2Secret(math::IRandom& rng) const
{
    return "2x2_s1";
}

} // namespace woodland_mansion
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
