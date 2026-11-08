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

#include "SwampHutStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
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
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/base/StructureBoundingBox.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string SwampHutStructure::s_name = "Swamp_Hut";

const SpawnOverrides SwampHutStructure::s_spawnOverrides = {
    SpawnOverrideType::Full, {SpawnOverrideEntry{"monster", 1, 1}}};

SwampHutStructure::SwampHutStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* SwampHutStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_SWAMP_HUT();
}

std::unique_ptr<StructureStart> SwampHutStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 生物群系校验落在候选生成点（区块中心、地表高度）上；不匹配即返回空起点。
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, HeightmapType::WorldSurfaceWG))) {
        return start;
    }

    // 生成位置固定为区块最小角、Y=64（原版 `ScatteredFeaturePiece` 构造传 64），
    // 唯一一次抽样是水平朝向：`Direction.Plane.HORIZONTAL.getRandomDirection` 按
    // [NORTH, EAST, SOUTH, WEST] 取一个。
    constexpr Direction kHorizontalFaces[] = {Direction::North, Direction::East, Direction::South, Direction::West};
    const Direction direction = kHorizontalFaces[rng.nextInt(4)];

    auto piece = std::make_unique<SwampHutPiece>(
        BlockPos(chunkX * world::CHUNK_WIDTH, 64, chunkZ * world::CHUNK_WIDTH), direction);
    start->addPiece(std::move(piece));

    return start;
}

// ============================================================================
// SwampHutPiece
// ============================================================================

SwampHutPiece::SwampHutPiece(const BlockPos& pos, Direction direction)
    : StructurePiece(StructurePieceTypes::SWAMP_HUT, pos.x, pos.y, pos.z, pos.x, pos.y, pos.z)
{
    // 尺寸按 `makeBoundingBox(x, y, z, direction, width=7, height=7, depth=9)` 展开：
    // 朝向沿 Z 轴（北/南）时深度落在 Z 上，沿 X 轴（东/西）时深度落在 X 上。
    const i32 width = 7;
    const i32 height = 7;
    const i32 depth = 9;
    const bool alongZ = direction == Direction::North || direction == Direction::South;
    m_minX = pos.x;
    m_minY = pos.y;
    m_minZ = pos.z;
    m_maxX = pos.x + (alongZ ? width : depth) - 1;
    m_maxY = pos.y + height - 1;
    m_maxZ = pos.z + (alongZ ? depth : width) - 1;
    // 构件朝向同时决定旋转与镜像（原版 `setOrientation`）。
    setCoordBaseMode(direction);
}

void SwampHutPiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 chunkX,
    i32 chunkZ,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    _generateHut(world, rng, chunkBounds);
}

void SwampHutPiece::_generateHut(IWorldWriter& world, math::IRandom& rng, const StructureBoundingBox& bounds)
{
    // 生成支柱（支撑小屋）
    _generatePillars(world, bounds);

    // 生成地板
    _generateFloor(world, bounds);

    // 生成墙壁
    _generateWalls(world, bounds);

    // 生成屋顶
    _generateRoof(world, bounds);

    // 生成内部设施
    _generateInterior(world, rng, bounds);
}

void SwampHutPiece::_generatePillars(IWorldWriter& world, const StructureBoundingBox& bounds)
{
    // 四个角落的橡木栅栏支柱，从水面延伸到地板

    i32 pillarPositions[4][2] = {{m_minX, m_minZ}, {m_maxX, m_minZ}, {m_minX, m_maxZ - 2}, {m_maxX, m_maxZ - 2}};

    for (i32 i = 0; i < 4; ++i) {
        i32 px = pillarPositions[i][0];
        i32 pz = pillarPositions[i][1];

        // 从 Y-2 到地板
        for (i32 y = m_minY - 2; y <= m_minY; ++y) {
            BlockPos pos(px, y, pz);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::OAK_FENCE) {
                    world.setBlockState(pos.x,
                        pos.y,
                        pos.z,
                        &VanillaBlocks::OAK_FENCE->defaultState(),
                        world::BlockUpdateFlags::UPDATE_CLIENTS);
                }
            }
        }
    }
}

void SwampHutPiece::_generateFloor(IWorldWriter& world, const StructureBoundingBox& bounds)
{
    // 地板由橡木木板构成，外延一格

    for (i32 x = m_minX - 1; x <= m_maxX + 1; ++x) {
        for (i32 z = m_minZ; z <= m_maxZ - 1; ++z) {
            BlockPos pos(x, m_minY, z);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::OAK_PLANKS) {
                    world.setBlockState(pos.x,
                        pos.y,
                        pos.z,
                        &VanillaBlocks::OAK_PLANKS->defaultState(),
                        world::BlockUpdateFlags::UPDATE_CLIENTS);
                }
            }
        }
    }
}

void SwampHutPiece::_generateWalls(IWorldWriter& world, const StructureBoundingBox& bounds)
{
    // 墙壁由云杉木板构成

    for (i32 y = m_minY + 1; y <= m_minY + 3; ++y) {
        for (i32 x = m_minX; x <= m_maxX; ++x) {
            for (i32 z = m_minZ; z <= m_maxZ - 1; ++z) {
                // 只在边缘放置
                if (x == m_minX || x == m_maxX || z == m_minZ || z == m_maxZ - 1) {
                    // 门的位置（南面中间）
                    if (z == m_maxZ - 1 && x >= m_minX + 1 && x <= m_minX + 2 && y <= m_minY + 2) {
                        continue; // 跳过门口
                    }

                    BlockPos pos(x, y, z);
                    if (bounds.contains(pos.x, pos.y, pos.z)) {
                        if (VanillaBlocks::SPRUCE_PLANKS) {
                            world.setBlockState(pos.x,
                                pos.y,
                                pos.z,
                                &VanillaBlocks::SPRUCE_PLANKS->defaultState(),
                                world::BlockUpdateFlags::UPDATE_CLIENTS);
                        }
                    }
                }
            }
        }
    }
}

void SwampHutPiece::_generateRoof(IWorldWriter& world, const StructureBoundingBox& bounds)
{
    // 梯形屋顶，由云杉楼梯构成
    // 注: 楼梯朝向需要根据位置设置，此处使用默认状态

    // 第一层屋顶（最宽）
    for (i32 x = m_minX - 1; x <= m_maxX + 1; ++x) {
        for (i32 z = m_minZ - 1; z <= m_maxZ; ++z) {
            BlockPos pos(x, m_minY + 4, z);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::OAK_STAIRS) {
                    world.setBlockState(pos.x,
                        pos.y,
                        pos.z,
                        &VanillaBlocks::OAK_STAIRS->defaultState(),
                        world::BlockUpdateFlags::UPDATE_CLIENTS);
                }
            }
        }
    }

    // 第二层屋顶（较窄）
    for (i32 x = m_minX; x <= m_maxX; ++x) {
        for (i32 z = m_minZ; z <= m_maxZ - 1; ++z) {
            BlockPos pos(x, m_minY + 5, z);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::OAK_STAIRS) {
                    world.setBlockState(pos.x,
                        pos.y,
                        pos.z,
                        &VanillaBlocks::OAK_STAIRS->defaultState(),
                        world::BlockUpdateFlags::UPDATE_CLIENTS);
                }
            }
        }
    }
}

void SwampHutPiece::_generateInterior(IWorldWriter& world, math::IRandom& rng, const StructureBoundingBox& bounds)
{
    MC_UNUSED(rng);

    // 内部设施：炼药台（酿造台）、炼药锅、红色蘑菇盆栽、棕色蘑菇盆栽

    // 炼药台（西北角）
    BlockPos brewingPos(m_minX + 1, m_minY + 1, m_minZ + 1);
    if (bounds.contains(brewingPos.x, brewingPos.y, brewingPos.z)) {
        if (VanillaBlocks::BREWING_STAND) {
            world.setBlockState(brewingPos.x,
                brewingPos.y,
                brewingPos.z,
                &VanillaBlocks::BREWING_STAND->defaultState(),
                world::BlockUpdateFlags::UPDATE_CLIENTS);
        }
    }

    // 炼药锅（东北角）
    BlockPos cauldronPos(m_maxX - 1, m_minY + 1, m_minZ + 1);
    if (bounds.contains(cauldronPos.x, cauldronPos.y, cauldronPos.z)) {
        if (VanillaBlocks::CAULDRON) {
            world.setBlockState(cauldronPos.x,
                cauldronPos.y,
                cauldronPos.z,
                &VanillaBlocks::CAULDRON->defaultState(),
                world::BlockUpdateFlags::UPDATE_CLIENTS);
        }
    }

    // 红色蘑菇盆栽（西南角）
    BlockPos redMushroomPos(m_minX + 1, m_minY + 1, m_maxZ - 2);
    if (bounds.contains(redMushroomPos.x, redMushroomPos.y, redMushroomPos.z)) {
        if (VanillaBlocks::RED_MUSHROOM) {
            world.setBlockState(redMushroomPos.x,
                redMushroomPos.y,
                redMushroomPos.z,
                &VanillaBlocks::RED_MUSHROOM->defaultState(),
                world::BlockUpdateFlags::UPDATE_CLIENTS);
        }
    }

    // 棕色蘑菇盆栽（东南角）
    BlockPos brownMushroomPos(m_maxX - 1, m_minY + 1, m_maxZ - 2);
    if (bounds.contains(brownMushroomPos.x, brownMushroomPos.y, brownMushroomPos.z)) {
        if (VanillaBlocks::BROWN_MUSHROOM) {
            world.setBlockState(brownMushroomPos.x,
                brownMushroomPos.y,
                brownMushroomPos.z,
                &VanillaBlocks::BROWN_MUSHROOM->defaultState(),
                world::BlockUpdateFlags::UPDATE_CLIENTS);
        }
    }

    // 工作台（可选）
    BlockPos craftingPos(m_minX + 1, m_minY + 1, m_minZ + 2);
    if (bounds.contains(craftingPos.x, craftingPos.y, craftingPos.z)) {
        if (VanillaBlocks::CRAFTING_TABLE) {
            world.setBlockState(craftingPos.x,
                craftingPos.y,
                craftingPos.z,
                &VanillaBlocks::CRAFTING_TABLE->defaultState(),
                world::BlockUpdateFlags::UPDATE_CLIENTS);
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
