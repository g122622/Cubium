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
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/template/Template.hpp"
#include "common/world/gen/feature/template/TemplateManager.hpp"
#include "common/world/gen/jigsaw/JigsawAssembler.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "common/world/gen/structure/structures/WoodlandMansionStructure.hpp"

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

// 辅助函数：添加偏移到 BlockPos
BlockPos addOffset(const BlockPos& pos, i32 dx, i32 dy, i32 dz)
{
    return BlockPos(pos.x + dx, pos.y + dy, pos.z + dz);
}
} // namespace

// 辅助函数：将旋转转换为南方向
static Direction rotationToSouth(feature::template_::Rotation rotation)
{
    switch (rotation) {
        case feature::template_::Rotation::None:
            return Direction::South;
        case feature::template_::Rotation::Clockwise90:
            return Direction::West;
        case feature::template_::Rotation::Clockwise180:
            return Direction::North;
        case feature::template_::Rotation::CounterClockwise90:
            return Direction::East;
        default:
            return Direction::South;
    }
}

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

bool WoodlandMansionStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * CHUNK_WIDTH + 8, SEA_LEVEL, chunkZ * CHUNK_WIDTH + 8);
    return isValidBiome(biome);
}

std::unique_ptr<StructureStart> WoodlandMansionStructure::generate(
    IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);
    i32 z = chunkZ * CHUNK_WIDTH + rng.nextInt(CHUNK_WIDTH);

    // 获取地表高度
    i32 y = generator.getHeight(x, z, HeightmapType::WorldSurface);

    // 随机旋转
    feature::template_::Rotation rotation = static_cast<feature::template_::Rotation>(rng.nextInt(4));

    // 使用 MansionGrid 生成布局，然后用 MansionPlacer 放置模板
    MansionGrid grid(rng);
    MansionPlacer placer(rng);

    std::vector<std::unique_ptr<StructurePiece>> pieces;
    placer.createMansion(BlockPos(x, y, z), rotation, pieces, grid);

    // 将所有片段添加到 StructureStart
    for (auto& piece : pieces) {
        start->addPiece(std::move(piece));
    }

    return start;
}

// ============================================================================
// WoodlandMansionPiece 实现
// ============================================================================

WoodlandMansionPiece::WoodlandMansionPiece(const std::string& templateName,
    const BlockPos& pos,
    feature::template_::Rotation rotation,
    feature::template_::Mirror mirror)
    : StructurePiece(StructurePieceTypes::WOODLAND_MANSION, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z)
    , m_templateName(templateName)
    , m_templatePosition(pos)
    , m_rotation(rotation)
    , m_mirror(mirror)
{
    // 边界框将在生成时根据模板大小更新
}

void WoodlandMansionPiece::generate(IWorldWriter& world,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);
    MC_UNUSED(rng);

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    // 获取模板
    auto& templateManager = jigsaw::JigsawAssembler::getTemplateManager();
    const feature::template_::Template* templ =
        templateManager.getTemplate(ResourceLocation("minecraft", "woodland_mansion/" + m_templateName));

    if (!templ || templ->getBlockCount() == 0) {
        // 模板未找到，使用占位方块
        const BlockState* darkOakPlanks = VanillaBlocks::getState(VanillaBlocks::DARK_OAK_PLANKS);
        if (darkOakPlanks) {
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    for (int z = 0; z < 8; ++z) {
                        BlockPos worldPos(m_templatePosition.x + x, m_templatePosition.y + y, m_templatePosition.z + z);
                        if (chunkBounds.contains(worldPos.x, worldPos.y, worldPos.z)) {
                            world.setBlockState(worldPos.x, worldPos.y, worldPos.z, darkOakPlanks, 2);
                        }
                    }
                }
            }
        }
        return;
    }

    // 放置模板
    feature::template_::PlacementSettings settings;
    settings.setRotation(m_rotation);
    settings.setMirror(m_mirror);
    settings.setIgnoreEntities(true);

    // 创建随机数生成器用于模板放置
    math::Random localRng(0);

    // 应用模板到世界
    templ->place(world, m_templatePosition, settings, localRng, 2);

    // 更新边界框
    BlockPos size = templ->getSize();
    // 根据旋转和镜像计算实际尺寸
    // 简化处理：假设无镜像
    switch (m_rotation) {
        case feature::template_::Rotation::None:
        case feature::template_::Rotation::Clockwise180:
            m_maxX = m_minX + size.x - 1;
            m_maxZ = m_minZ + size.z - 1;
            break;
        case feature::template_::Rotation::Clockwise90:
        case feature::template_::Rotation::CounterClockwise90:
            m_maxX = m_minX + size.z - 1;
            m_maxZ = m_minZ + size.x - 1;
            break;
    }
    m_maxY = m_minY + size.y - 1;
}

// ============================================================================
// woodland_mansion::SimpleGrid 实现
// ============================================================================

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

MansionGrid::MansionGrid(math::Random& rng)
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

    // 随机分支 - 使用水平方向数组
    const Direction horizontalDirs[] = {Direction::North, Direction::East, Direction::South, Direction::West};
    for (i32 i = 0; i < 8; ++i) {
        Direction newDir = horizontalDirs[m_rng.nextInt(4)];
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

MansionPlacer::MansionPlacer(math::Random& rng)
    : m_rng(rng)
    , m_startX(0)
    , m_startY(0)
{}

void MansionPlacer::createMansion(const BlockPos& startPos,
    feature::template_::Rotation rotation,
    std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const MansionGrid& grid)
{
    m_startX = grid.entranceX() + 1;
    m_startY = grid.entranceY() + 1;

    // 放置入口
    BlockPos pos = startPos;
    feature::template_::Rotation currentRotation = rotation;
    std::string wallType = "wall_flat";

    _entrance(pieces, currentRotation, pos);

    // 一楼和二楼外墙
    BlockPos floor1Pos = pos;
    BlockPos floor2Pos = addOffset(pos, 0, FLOOR_HEIGHT, 0);

    // 遍历外墙
    _traverseOuterWalls(pieces,
        grid.baseGrid(),
        Direction::South,
        grid.entranceX() + 1,
        grid.entranceY() + 1,
        grid.entranceX() + 1,
        grid.entranceY(),
        floor1Pos,
        currentRotation,
        wallType);

    _traverseOuterWalls(pieces,
        grid.baseGrid(),
        Direction::South,
        grid.entranceX() + 1,
        grid.entranceY() + 1,
        grid.entranceX() + 1,
        grid.entranceY(),
        floor2Pos,
        currentRotation,
        "wall_window");

    // 三楼
    BlockPos floor3Pos = addOffset(startPos, 0, 19, 0);
    const SimpleGrid& thirdFloor = grid.thirdFloorGrid();

    // 找到三楼的起始位置
    bool foundThirdFloor = false;
    for (i32 y = 0; y < thirdFloor.height() && !foundThirdFloor; ++y) {
        for (i32 x = thirdFloor.width() - 1; x >= 0 && !foundThirdFloor; --x) {
            if (MansionGrid::isHouse(thirdFloor, x, y)) {
                BlockPos thirdPos = floor3Pos;
                // 调整位置
                i32 offsetX = x - m_startX;
                i32 offsetY = y - m_startY;

                // 根据旋转调整偏移
                switch (rotation) {
                    case feature::template_::Rotation::None:
                        thirdPos = addOffset(thirdPos, offsetX * ROOM_SIZE, 0, offsetY * ROOM_SIZE);
                        break;
                    case feature::template_::Rotation::Clockwise90:
                        thirdPos = addOffset(thirdPos, -offsetY * ROOM_SIZE, 0, offsetX * ROOM_SIZE);
                        break;
                    case feature::template_::Rotation::Clockwise180:
                        thirdPos = addOffset(thirdPos, -offsetX * ROOM_SIZE, 0, -offsetY * ROOM_SIZE);
                        break;
                    case feature::template_::Rotation::CounterClockwise90:
                        thirdPos = addOffset(thirdPos, offsetY * ROOM_SIZE, 0, -offsetX * ROOM_SIZE);
                        break;
                }

                _traverseWallPiece(pieces, thirdPos, rotation, "wall_window");
                _traverseOuterWalls(
                    pieces, thirdFloor, Direction::South, x, y, x, y, thirdPos, currentRotation, "wall_window");
                foundThirdFloor = true;
            }
        }
    }

    // 屋顶
    _createRoof(pieces, addOffset(startPos, 0, 16, 0), rotation, grid.baseGrid(), &thirdFloor);
    _createRoof(pieces, addOffset(startPos, 0, 27, 0), rotation, thirdFloor, nullptr);

    // 走廊地板和房间
    static FirstFloorRoomCollection firstFloorRooms;
    static SecondFloorRoomCollection secondFloorRooms;
    static ThirdFloorRoomCollection thirdFloorRooms;
    const RoomCollection* roomCollections[3] = {&firstFloorRooms, &secondFloorRooms, &thirdFloorRooms};

    for (i32 floor = 0; floor < 3; ++floor) {
        BlockPos floorPos = addOffset(startPos, 0, FLOOR_HEIGHT * floor + (floor == 2 ? 3 : 0), 0);
        const SimpleGrid& floorGrid = floor == 2 ? thirdFloor : grid.baseGrid();
        const SimpleGrid& roomGrid = grid.floorRoom(floor);

        // 放置走廊地板
        for (i32 y = 0; y < floorGrid.height(); ++y) {
            for (i32 x = 0; x < floorGrid.width(); ++x) {
                if (floorGrid.get(x, y) == 1) {
                    BlockPos corridorPos = floorPos;
                    i32 offsetX = x - m_startX;
                    i32 offsetY = y - m_startY;

                    switch (rotation) {
                        case feature::template_::Rotation::None:
                            corridorPos =
                                addOffset(corridorPos, offsetX * ROOM_SIZE, 0, offsetY * ROOM_SIZE + ROOM_SIZE);
                            break;
                        case feature::template_::Rotation::Clockwise90:
                            corridorPos =
                                addOffset(corridorPos, -offsetY * ROOM_SIZE, 0, offsetX * ROOM_SIZE + ROOM_SIZE);
                            break;
                        case feature::template_::Rotation::Clockwise180:
                            corridorPos =
                                addOffset(corridorPos, -offsetX * ROOM_SIZE, 0, -offsetY * ROOM_SIZE + ROOM_SIZE);
                            break;
                        case feature::template_::Rotation::CounterClockwise90:
                            corridorPos =
                                addOffset(corridorPos, offsetY * ROOM_SIZE, 0, -offsetX * ROOM_SIZE + ROOM_SIZE);
                            break;
                    }

                    pieces.push_back(std::make_unique<WoodlandMansionPiece>("corridor_floor", corridorPos, rotation));
                }
            }
        }

        // TODO: 放置房间逻辑，当前为简化实现
        for (i32 y = 0; y < floorGrid.height(); ++y) {
            for (i32 x = 0; x < floorGrid.width(); ++x) {
                if (floorGrid.get(x, y) == 2) {
                    i32 roomValue = roomGrid.get(x, y);
                    i32 roomType = roomValue & 0xF0000;
                    MC_UNUSED(roomType);
                    i32 roomId = roomValue & 0xFFFF;
                    MC_UNUSED(roomId);

                    BlockPos roomPos = floorPos;
                    i32 offsetX = x - m_startX - 1;
                    i32 offsetY = y - m_startY;

                    switch (rotation) {
                        case feature::template_::Rotation::None:
                            roomPos = addOffset(roomPos, offsetX * ROOM_SIZE, 0, offsetY * ROOM_SIZE + ROOM_SIZE);
                            break;
                        case feature::template_::Rotation::Clockwise90:
                            roomPos = addOffset(roomPos, -offsetY * ROOM_SIZE, 0, offsetX * ROOM_SIZE + ROOM_SIZE);
                            break;
                        case feature::template_::Rotation::Clockwise180:
                            roomPos = addOffset(roomPos, -offsetX * ROOM_SIZE, 0, -offsetY * ROOM_SIZE + ROOM_SIZE);
                            break;
                        case feature::template_::Rotation::CounterClockwise90:
                            roomPos = addOffset(roomPos, offsetY * ROOM_SIZE, 0, -offsetX * ROOM_SIZE + ROOM_SIZE);
                            break;
                    }

                    // 根据房间类型放置房间模板
                    if (roomType == 65536) {
                        // 1x1 房间
                        std::string templateName = roomCollections[floor]->get1x1(m_rng);
                        pieces.push_back(std::make_unique<WoodlandMansionPiece>(templateName, roomPos, rotation));
                    } else if (roomType == 131072) {
                        // 1x2 房间
                        std::string templateName = roomCollections[floor]->get1x2SideEntrance(m_rng, false);
                        pieces.push_back(std::make_unique<WoodlandMansionPiece>(templateName, roomPos, rotation));
                    } else if (roomType == 262144) {
                        // 2x2 房间
                        std::string templateName = roomCollections[floor]->get2x2(m_rng);
                        pieces.push_back(std::make_unique<WoodlandMansionPiece>(templateName, roomPos, rotation));
                    }
                }
            }
        }
    }
}

void MansionPlacer::_entrance(
    std::vector<std::unique_ptr<StructurePiece>>& pieces, feature::template_::Rotation& rotation, BlockPos& pos)
{
    Direction southDir = rotationToSouth(rotation);
    Direction westDir = Directions::rotateYCCW(southDir);
    i32 dx = Directions::xOffset(westDir);
    i32 dz = Directions::zOffset(westDir);

    BlockPos entrancePos = addOffset(pos, dx * 9, 0, dz * 9);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>("entrance", entrancePos, rotation));

    // 更新位置（向南移动16格）
    i32 sdx = Directions::xOffset(southDir);
    i32 sdz = Directions::zOffset(southDir);
    pos = addOffset(pos, sdx * 16, 0, sdz * 16);
}

void MansionPlacer::_traverseOuterWalls(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const SimpleGrid& grid,
    Direction startDir,
    i32 startX,
    i32 startY,
    i32 targetX,
    i32 targetY,
    BlockPos& pos,
    feature::template_::Rotation& rotation,
    const std::string& wallType)
{
    i32 x = startX;
    i32 y = startY;
    Direction dir = startDir;

    do {
        i32 dx = Directions::xOffset(dir);
        i32 dz = Directions::zOffset(dir);

        if (!MansionGrid::isHouse(grid, x + dx, y + dz)) {
            // 转角
            _traverseTurn(pieces, pos, rotation);
            dir = Directions::rotateY(dir); // CW
        } else if (MansionGrid::isHouse(grid, x + dx, y + dz) &&
            MansionGrid::isHouse(grid,
                x + dx + Directions::xOffset(Directions::rotateYCCW(dir)),
                y + dz + Directions::zOffset(Directions::rotateYCCW(dir)))) {
            // 内转角
            _traverseInnerTurn(pieces, pos, rotation);
            x += dx;
            y += dz;
            dir = Directions::rotateYCCW(dir); // CCW
        } else {
            x += dx;
            y += dz;
            if (x != targetX || y != targetY || dir != startDir) {
                _traverseWallPiece(pieces, pos, rotation, wallType);
            }
        }
    } while (x != targetX || y != targetY || dir != startDir);
}

void MansionPlacer::_traverseWallPiece(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    BlockPos& pos,
    feature::template_::Rotation rotation,
    const std::string& wallType)
{
    // 放置墙块 - rotation 代表朝向南方
    Direction southDir = rotationToSouth(rotation);
    Direction eastDir = Directions::rotateY(southDir); // 顺时针旋转得到东方
    i32 dx = Directions::xOffset(eastDir);
    i32 dz = Directions::zOffset(eastDir);

    BlockPos wallPos = addOffset(pos, dx * 7, 0, dz * 7);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(wallType, wallPos, rotation));

    // 向南移动8格
    i32 sdx = Directions::xOffset(southDir);
    i32 sdz = Directions::zOffset(southDir);
    pos = addOffset(pos, sdx * ROOM_SIZE, 0, sdz * ROOM_SIZE);
}

void MansionPlacer::_traverseTurn(
    std::vector<std::unique_ptr<StructurePiece>>& pieces, BlockPos& pos, feature::template_::Rotation& rotation)
{
    // 向南移动-1格
    Direction southDir = rotationToSouth(rotation);
    i32 dx = Directions::xOffset(southDir);
    i32 dz = Directions::zOffset(southDir);
    pos = addOffset(pos, -dx, 0, -dz);

    // 放置转角墙
    pieces.push_back(std::make_unique<WoodlandMansionPiece>("wall_corner", pos, rotation));

    // 移动并旋转
    pos = addOffset(pos, -dx * 7, 0, -dz * 7);
    Direction westDir = Directions::rotateYCCW(southDir);
    dx = Directions::xOffset(westDir);
    dz = Directions::zOffset(westDir);
    pos = addOffset(pos, -dx * 6, 0, -dz * 6);
    rotation = static_cast<feature::template_::Rotation>((static_cast<i32>(rotation) + 1) % 4);
}

void MansionPlacer::_traverseInnerTurn(
    std::vector<std::unique_ptr<StructurePiece>>& pieces, BlockPos& pos, feature::template_::Rotation& rotation)
{
    // 移动并旋转
    Direction southDir = rotationToSouth(rotation);
    i32 dx = Directions::xOffset(southDir);
    i32 dz = Directions::zOffset(southDir);
    pos = addOffset(pos, dx * 6, 0, dz * 6);

    Direction eastDir = Directions::rotateY(southDir);
    dx = Directions::xOffset(eastDir);
    dz = Directions::zOffset(eastDir);
    pos = addOffset(pos, dx * ROOM_SIZE, 0, dz * ROOM_SIZE);

    rotation = static_cast<feature::template_::Rotation>((static_cast<i32>(rotation) + 3) % 4); // CCW
}

void MansionPlacer::_createRoof(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& basePos,
    feature::template_::Rotation rotation,
    const SimpleGrid& grid,
    const SimpleGrid* upperGrid)
{
    for (i32 y = 0; y < grid.height(); ++y) {
        for (i32 x = 0; x < grid.width(); ++x) {
            bool hasUpper = upperGrid && MansionGrid::isHouse(*upperGrid, x, y);

            if (MansionGrid::isHouse(grid, x, y) && !hasUpper) {
                BlockPos roofPos = basePos;
                i32 offsetX = x - m_startX;
                i32 offsetY = y - m_startY;

                switch (rotation) {
                    case feature::template_::Rotation::None:
                        roofPos = addOffset(roofPos, offsetX * ROOM_SIZE, 3, offsetY * ROOM_SIZE + ROOM_SIZE);
                        break;
                    case feature::template_::Rotation::Clockwise90:
                        roofPos = addOffset(roofPos, -offsetY * ROOM_SIZE, 3, offsetX * ROOM_SIZE + ROOM_SIZE);
                        break;
                    case feature::template_::Rotation::Clockwise180:
                        roofPos = addOffset(roofPos, -offsetX * ROOM_SIZE, 3, -offsetY * ROOM_SIZE + ROOM_SIZE);
                        break;
                    case feature::template_::Rotation::CounterClockwise90:
                        roofPos = addOffset(roofPos, offsetY * ROOM_SIZE, 3, -offsetX * ROOM_SIZE + ROOM_SIZE);
                        break;
                }

                pieces.push_back(std::make_unique<WoodlandMansionPiece>("roof", roofPos, rotation));
            }
        }
    }
}

void MansionPlacer::_addRoom1x1(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    feature::template_::Rotation rotation,
    Direction doorDir,
    i32 floor)
{
    MC_UNUSED(doorDir);
    static FirstFloorRoomCollection firstFloor;
    static SecondFloorRoomCollection secondFloor;
    static ThirdFloorRoomCollection thirdFloor;

    const RoomCollection* collection = (floor == 0) ? static_cast<const RoomCollection*>(&firstFloor)
        : (floor == 1)                              ? static_cast<const RoomCollection*>(&secondFloor)
                                                    : static_cast<const RoomCollection*>(&thirdFloor);

    std::string templateName = collection->get1x1(m_rng);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(templateName, pos, rotation));
}

void MansionPlacer::_addRoom1x2(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    feature::template_::Rotation rotation,
    Direction roomDir,
    Direction doorDir,
    i32 floor,
    bool isStairs)
{
    MC_UNUSED(roomDir);
    MC_UNUSED(doorDir);
    static FirstFloorRoomCollection firstFloor;
    static SecondFloorRoomCollection secondFloor;
    static ThirdFloorRoomCollection thirdFloor;

    const RoomCollection* collection = (floor == 0) ? static_cast<const RoomCollection*>(&firstFloor)
        : (floor == 1)                              ? static_cast<const RoomCollection*>(&secondFloor)
                                                    : static_cast<const RoomCollection*>(&thirdFloor);

    std::string templateName = collection->get1x2SideEntrance(m_rng, isStairs);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(templateName, pos, rotation));
}

void MansionPlacer::_addRoom2x2(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    feature::template_::Rotation rotation,
    Direction roomDir,
    Direction doorDir,
    i32 floor)
{
    MC_UNUSED(roomDir);
    MC_UNUSED(doorDir);
    static FirstFloorRoomCollection firstFloor;
    static SecondFloorRoomCollection secondFloor;
    static ThirdFloorRoomCollection thirdFloor;

    const RoomCollection* collection = (floor == 0) ? static_cast<const RoomCollection*>(&firstFloor)
        : (floor == 1)                              ? static_cast<const RoomCollection*>(&secondFloor)
                                                    : static_cast<const RoomCollection*>(&thirdFloor);

    std::string templateName = collection->get2x2(m_rng);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(templateName, pos, rotation));
}

void MansionPlacer::_addRoom2x2Secret(std::vector<std::unique_ptr<StructurePiece>>& pieces,
    const BlockPos& pos,
    feature::template_::Rotation rotation,
    i32 floor)
{
    static FirstFloorRoomCollection firstFloor;
    static SecondFloorRoomCollection secondFloor;
    static ThirdFloorRoomCollection thirdFloor;

    const RoomCollection* collection = (floor == 0) ? static_cast<const RoomCollection*>(&firstFloor)
        : (floor == 1)                              ? static_cast<const RoomCollection*>(&secondFloor)
                                                    : static_cast<const RoomCollection*>(&thirdFloor);

    std::string templateName = collection->get2x2Secret(m_rng);
    pieces.push_back(std::make_unique<WoodlandMansionPiece>(templateName, pos, rotation));
}

// ============================================================================
// RoomCollection 实现
// ============================================================================

std::string FirstFloorRoomCollection::get1x1(math::Random& rng) const
{
    return "1x1_a" + std::to_string(rng.nextInt(5) + 1);
}

std::string FirstFloorRoomCollection::get1x1Secret(math::Random& rng) const
{
    return "1x1_as" + std::to_string(rng.nextInt(4) + 1);
}

std::string FirstFloorRoomCollection::get1x2SideEntrance(math::Random& rng, bool isStairs) const
{
    return "1x2_a" + std::to_string(rng.nextInt(9) + 1);
}

std::string FirstFloorRoomCollection::get1x2FrontEntrance(math::Random& rng, bool isStairs) const
{
    return "1x2_b" + std::to_string(rng.nextInt(5) + 1);
}

std::string FirstFloorRoomCollection::get1x2Secret(math::Random& rng) const
{
    return "1x2_s" + std::to_string(rng.nextInt(2) + 1);
}

std::string FirstFloorRoomCollection::get2x2(math::Random& rng) const
{
    return "2x2_a" + std::to_string(rng.nextInt(4) + 1);
}

std::string FirstFloorRoomCollection::get2x2Secret(math::Random& rng) const
{
    return "2x2_s1";
}

std::string SecondFloorRoomCollection::get1x1(math::Random& rng) const
{
    return "1x1_b" + std::to_string(rng.nextInt(4) + 1);
}

std::string SecondFloorRoomCollection::get1x1Secret(math::Random& rng) const
{
    return "1x1_as" + std::to_string(rng.nextInt(4) + 1);
}

std::string SecondFloorRoomCollection::get1x2SideEntrance(math::Random& rng, bool isStairs) const
{
    return isStairs ? "1x2_c_stairs" : "1x2_c" + std::to_string(rng.nextInt(4) + 1);
}

std::string SecondFloorRoomCollection::get1x2FrontEntrance(math::Random& rng, bool isStairs) const
{
    return isStairs ? "1x2_d_stairs" : "1x2_d" + std::to_string(rng.nextInt(5) + 1);
}

std::string SecondFloorRoomCollection::get1x2Secret(math::Random& rng) const
{
    return "1x2_se" + std::to_string(rng.nextInt(1) + 1);
}

std::string SecondFloorRoomCollection::get2x2(math::Random& rng) const
{
    return "2x2_b" + std::to_string(rng.nextInt(5) + 1);
}

std::string SecondFloorRoomCollection::get2x2Secret(math::Random& rng) const
{
    return "2x2_s1";
}

} // namespace woodland_mansion

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
