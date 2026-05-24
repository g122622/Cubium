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

#include "IglooStructure.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/IChunk.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../StructureBoundingBox.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string IglooStructure::s_name = "Igloo";
const std::vector<BiomeId> IglooStructure::s_validBiomes = {
    SnowyPlains, SnowyTaiga, SnowyTaigaHills, SnowyTaigaMountains};

IglooStructure::IglooStructure()
    : Structure(StructureType::Temple)
{}

bool IglooStructure::canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * 16 + 8, 64, chunkZ * 16 + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<StructureStart> IglooStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * 16 + rng.nextInt(16);
    i32 z = chunkZ * 16 + rng.nextInt(16);

    // 获取地表高度
    i32 y = generator.getHeight(x, z, HeightmapType::WorldSurface);

    // 随机旋转
    feature::template_::Rotation rotation;
    i32 rotValue = rng.nextInt(4);
    switch (rotValue) {
        case 0:
            rotation = feature::template_::Rotation::None;
            break;
        case 1:
            rotation = feature::template_::Rotation::Clockwise90;
            break;
        case 2:
            rotation = feature::template_::Rotation::Clockwise180;
            break;
        case 3:
        default:
            rotation = feature::template_::Rotation::CounterClockwise90;
            break;
    }

    // MC 1.16.5: 雪屋有50%概率有地下室
    bool hasBasement = rng.nextFloat() < 0.5f;

    auto piece = std::make_unique<IglooPiece>(BlockPos(x, y, z), rotation, hasBasement);
    start->addPiece(std::move(piece));

    return start;
}

// ============================================================================
// IglooPiece
// ============================================================================

IglooPiece::IglooPiece(const BlockPos& pos, feature::template_::Rotation rotation, bool hasBasement)
    : StructurePiece(StructurePieceTypes::IGLOO, pos.x, pos.y, pos.z, pos.x + 9, pos.y + 8, pos.z + 9)
    , m_rotation(rotation)
    , m_hasBasement(hasBasement)
{
    // 边界框根据模板大小调整
    // MC 1.16.5: 雪屋模板为 7x5x8
    m_minX = pos.x;
    m_minY = pos.y;
    m_minZ = pos.z;
    m_maxX = pos.x + 6;
    m_maxY = pos.y + 4;
    m_maxZ = pos.z + 7;

    if (hasBasement) {
        // 地下室向下延伸到 Y=-3
        m_minY = pos.y - 6;
        m_maxY = pos.y + 4;
    }
}

void IglooPiece::generate(
    IWorldWriter& world, math::Random& rng, i32 chunkX, i32 chunkZ, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(rng);
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    // 生成地上部分
    generateTop(world, chunkBounds);

    // 生成地下室（如果有）
    if (m_hasBasement) {
        generateBasement(world, rng, chunkBounds);
    }
}

void IglooPiece::generateTop(IWorldWriter& world, const StructureBoundingBox& bounds)
{
    // MC 1.16.5: 雪屋由雪块构成，圆顶结构
    // 简化实现：直接放置方块

    BlockPos basePos(m_minX, m_minY, m_minZ);

    // 底层 7x7 雪块平台
    for (int x = 0; x < 7; ++x) {
        for (int z = 0; z < 7; ++z) {
            BlockPos placePos(m_minX + x, m_minY, m_minZ + z);
            if (bounds.contains(placePos.x, placePos.y, placePos.z)) {
                if (VanillaBlocks::SNOW_BLOCK) {
                    world.setBlockState(
                        placePos.x, placePos.y, placePos.z, &VanillaBlocks::SNOW_BLOCK->defaultState(), 2);
                }
            }
        }
    }

    // 第一层墙壁（带入口）
    for (int x = 0; x < 7; ++x) {
        for (int z = 0; z < 7; ++z) {
            // 只在边缘放置
            if (x == 0 || x == 6 || z == 0 || z == 6) {
                // 入口位置（南面中间）
                if (z == 6 && x >= 2 && x <= 4) {
                    continue; // 跳过入口
                }

                BlockPos placePos(basePos.x + x, basePos.y + 1, basePos.z + z);
                if (bounds.contains(placePos.x, placePos.y, placePos.z)) {
                    if (VanillaBlocks::SNOW_BLOCK) {
                        world.setBlockState(
                            placePos.x, placePos.y, placePos.z, &VanillaBlocks::SNOW_BLOCK->defaultState(), 2);
                    }
                }
            }
        }
    }

    // 第二层墙壁（收缩）
    for (int x = 1; x < 6; ++x) {
        for (int z = 1; z < 6; ++z) {
            if (x == 1 || x == 5 || z == 1 || z == 5) {
                BlockPos placePos(basePos.x + x, basePos.y + 2, basePos.z + z);
                if (bounds.contains(placePos.x, placePos.y, placePos.z)) {
                    if (VanillaBlocks::SNOW_BLOCK) {
                        world.setBlockState(
                            placePos.x, placePos.y, placePos.z, &VanillaBlocks::SNOW_BLOCK->defaultState(), 2);
                    }
                }
            }
        }
    }

    // 第三层墙壁（再收缩）
    for (int x = 2; x < 5; ++x) {
        for (int z = 2; z < 5; ++z) {
            if (x == 2 || x == 4 || z == 2 || z == 4) {
                BlockPos placePos(basePos.x + x, basePos.y + 3, basePos.z + z);
                if (bounds.contains(placePos.x, placePos.y, placePos.z)) {
                    if (VanillaBlocks::SNOW_BLOCK) {
                        world.setBlockState(
                            placePos.x, placePos.y, placePos.z, &VanillaBlocks::SNOW_BLOCK->defaultState(), 2);
                    }
                }
            }
        }
    }

    // 顶部
    for (int x = 3; x <= 3; ++x) {
        for (int z = 3; z <= 3; ++z) {
            BlockPos placePos(basePos.x + x, basePos.y + 4, basePos.z + z);
            if (bounds.contains(placePos.x, placePos.y, placePos.z)) {
                if (VanillaBlocks::SNOW_BLOCK) {
                    world.setBlockState(
                        placePos.x, placePos.y, placePos.z, &VanillaBlocks::SNOW_BLOCK->defaultState(), 2);
                }
            }
        }
    }

    // 地毯地板
    for (int x = 1; x < 6; ++x) {
        for (int z = 1; z < 6; ++z) {
            if (z == 6 && x >= 2 && x <= 4) {
                continue; // 入口
            }

            BlockPos placePos(basePos.x + x, basePos.y + 1, basePos.z + z);
            if (bounds.contains(placePos.x, placePos.y, placePos.z)) {
                if (VanillaBlocks::WHITE_CARPET) {
                    world.setBlockState(
                        placePos.x, placePos.y, placePos.z, &VanillaBlocks::WHITE_CARPET->defaultState(), 2);
                }
            }
        }
    }
}

void IglooPiece::generateBasement(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds)
{
    MC_UNUSED(rng);

    // MC 1.16.5: 地下室位于雪屋下方，通过活板门进入
    // 地下室为 9x6x7 的空间，包含熔炉、工作台、红石火把和酿造台

    BlockPos basePos(m_minX - 1, m_minY - 6, m_minZ - 1);

    // 挖空地下室空间
    for (int x = 0; x < 9; ++x) {
        for (int z = 0; z < 9; ++z) {
            for (int y = 0; y < 6; ++y) {
                BlockPos placePos(basePos.x + x, basePos.y + y, basePos.z + z);
                if (bounds.contains(placePos.x, placePos.y, placePos.z)) {
                    // 内部为空气
                    if (x > 0 && x < 8 && z > 0 && z < 8 && y > 0 && y < 5) {
                        world.setBlockState(placePos.x, placePos.y, placePos.z, nullptr, 2);
                    }
                }
            }
        }
    }

    // 石砖地板和天花板
    for (int x = 0; x < 9; ++x) {
        for (int z = 0; z < 9; ++z) {
            // 地板
            BlockPos floorPos(basePos.x + x, basePos.y, basePos.z + z);
            if (bounds.contains(floorPos.x, floorPos.y, floorPos.z)) {
                if (VanillaBlocks::STONE_BRICKS) {
                    world.setBlockState(
                        floorPos.x, floorPos.y, floorPos.z, &VanillaBlocks::STONE_BRICKS->defaultState(), 2);
                }
            }

            // 天花板（上方是雪块）
            BlockPos ceilingPos(basePos.x + x, basePos.y + 5, basePos.z + z);
            if (bounds.contains(ceilingPos.x, ceilingPos.y, ceilingPos.z)) {
                if (VanillaBlocks::STONE_BRICKS) {
                    world.setBlockState(
                        ceilingPos.x, ceilingPos.y, ceilingPos.z, &VanillaBlocks::STONE_BRICKS->defaultState(), 2);
                }
            }
        }
    }

    // 石砖墙壁
    for (int y = 1; y < 5; ++y) {
        for (int x = 0; x < 9; ++x) {
            for (int z = 0; z < 9; ++z) {
                if (x == 0 || x == 8 || z == 0 || z == 8) {
                    BlockPos wallPos(basePos.x + x, basePos.y + y, basePos.z + z);
                    if (bounds.contains(wallPos.x, wallPos.y, wallPos.z)) {
                        // 50% 概率苔藓石砖
                        bool useMossy = (rng.nextInt(100) < 50);
                        const BlockState* state = (useMossy && VanillaBlocks::MOSSY_STONE_BRICKS)
                            ? &VanillaBlocks::MOSSY_STONE_BRICKS->defaultState()
                            : (VanillaBlocks::STONE_BRICKS ? &VanillaBlocks::STONE_BRICKS->defaultState() : nullptr);
                        if (state) {
                            world.setBlockState(wallPos.x, wallPos.y, wallPos.z, state, 2);
                        }
                    }
                }
            }
        }
    }

    // 放置熔炉（东墙）- FURNACE 尚未注册，使用 COBBLESTONE 占位
    BlockPos furnacePos(basePos.x + 8, basePos.y + 2, basePos.z + 4);
    if (bounds.contains(furnacePos.x, furnacePos.y, furnacePos.z)) {
        if (VanillaBlocks::COBBLESTONE) {
            world.setBlockState(
                furnacePos.x, furnacePos.y, furnacePos.z, &VanillaBlocks::COBBLESTONE->defaultState(), 2);
        }
    }

    // 放置工作台（西墙）
    BlockPos craftingPos(basePos.x, basePos.y + 2, basePos.z + 4);
    if (bounds.contains(craftingPos.x, craftingPos.y, craftingPos.z)) {
        if (VanillaBlocks::CRAFTING_TABLE) {
            world.setBlockState(
                craftingPos.x, craftingPos.y, craftingPos.z, &VanillaBlocks::CRAFTING_TABLE->defaultState(), 2);
        }
    }

    // 红石火把（照明）
    BlockPos torchPos(basePos.x + 4, basePos.y + 2, basePos.z);
    if (bounds.contains(torchPos.x, torchPos.y, torchPos.z)) {
        if (VanillaBlocks::REDSTONE_WALL_TORCH) {
            world.setBlockState(
                torchPos.x, torchPos.y, torchPos.z, &VanillaBlocks::REDSTONE_WALL_TORCH->defaultState(), 2);
        }
    }

    // 入口活板门
    BlockPos trapdoorPos(basePos.x + 4, basePos.y + 5, basePos.z + 4);
    if (bounds.contains(trapdoorPos.x, trapdoorPos.y, trapdoorPos.z)) {
        if (VanillaBlocks::OAK_TRAPDOOR) {
            // 注: 活板门默认为关闭状态，实际应设置 half 和 facing 属性
            auto state = &VanillaBlocks::OAK_TRAPDOOR->defaultState();
            world.setBlockState(trapdoorPos.x, trapdoorPos.y, trapdoorPos.z, state, 2);
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
