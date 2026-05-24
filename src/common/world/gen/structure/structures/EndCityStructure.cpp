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

#include "EndCityStructure.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../block/BlockPos.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../StructureBoundingBox.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string EndCityStructure::s_name = "End_City";
const std::vector<BiomeId> EndCityStructure::s_validBiomes = {EndMidlands, EndHighlands};

EndCityStructure::EndCityStructure()
    : Structure(StructureType::Temple)
{}

bool EndCityStructure::canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * 16 + 8, 64, chunkZ * 16 + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            // 末地城只在高度 >= 60 的位置生成
            return getYPosition(chunkX, chunkZ, generator) >= 60;
        }
    }
    return false;
}

std::unique_ptr<StructureStart> EndCityStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(world);

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * 16 + 8;
    i32 z = chunkZ * 16 + 8;
    i32 y = getYPosition(chunkX, chunkZ, generator);

    if (y < 60) {
        return start;
    }

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

    // MC 1.16.5: 末地城从基础层开始，随机添加塔楼和房屋
    auto piece = std::make_unique<EndCityPiece>(BlockPos(x, y, z), rotation, "base_floor");
    start->addPiece(std::move(piece));

    return start;
}

i32 EndCityStructure::getYPosition(i32 chunkX, i32 chunkZ, IChunkGenerator& generator)
{
    math::Random random(static_cast<i64>(chunkX + chunkZ * 10387313));
    feature::template_::Rotation rotation = static_cast<feature::template_::Rotation>(random.nextInt(4));

    i32 i = 5;
    i32 j = 5;
    switch (rotation) {
        case feature::template_::Rotation::Clockwise90:
            i = -5;
            break;
        case feature::template_::Rotation::Clockwise180:
            i = -5;
            j = -5;
            break;
        case feature::template_::Rotation::CounterClockwise90:
            j = -5;
            break;
        default:
            break;
    }

    i32 k = (chunkX << 4) + 7;
    i32 l = (chunkZ << 4) + 7;
    i32 i1 = generator.getHeight(k, l, HeightmapType::WorldSurface);
    i32 j1 = generator.getHeight(k, l + j, HeightmapType::WorldSurface);
    i32 k1 = generator.getHeight(k + i, l, HeightmapType::WorldSurface);
    i32 l1 = generator.getHeight(k + i, l + j, HeightmapType::WorldSurface);

    return std::min({i1, j1, k1, l1});
}

// ============================================================================
// EndCityPiece
// ============================================================================

EndCityPiece::EndCityPiece(const BlockPos& pos, feature::template_::Rotation rotation, const std::string& templateName)
    : StructurePiece(StructurePieceTypes::END_CITY, pos.x, pos.y, pos.z, pos.x + 15, pos.y + 20, pos.z + 15)
    , m_rotation(rotation)
    , m_templateName(templateName)
{
    // 边界框根据模板大小调整
    m_minX = pos.x;
    m_minY = pos.y;
    m_minZ = pos.z;
    m_maxX = pos.x + 14;
    m_maxY = pos.y + 19;
    m_maxZ = pos.z + 14;
}

void EndCityPiece::generate(
    IWorldWriter& world, math::Random& rng, i32 chunkX, i32 chunkZ, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    // 简化实现：生成基础塔楼
    generateBase(world, chunkBounds);

    // 随机生成塔楼部分
    if (rng.nextInt(3) > 0) {
        generateTower(world, rng, chunkBounds);
    }
}

void EndCityPiece::generateBase(IWorldWriter& world, const StructureBoundingBox& bounds)
{
    // MC 1.16.5: 末地城基础层由末地石砖和紫珀块构成
    // 简化实现：直接放置方块

    // 基础平台 5x5
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            BlockPos pos(m_minX + x, m_minY, m_minZ + z);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::END_STONE_BRICKS) {
                    world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::END_STONE_BRICKS->defaultState(), 2);
                }
            }
        }
    }

    // 墙壁
    for (int y = 1; y <= 3; ++y) {
        for (int x = 0; x < 5; ++x) {
            for (int z = 0; z < 5; ++z) {
                if (x == 0 || x == 4 || z == 0 || z == 4) {
                    BlockPos pos(m_minX + x, m_minY + y, m_minZ + z);
                    if (bounds.contains(pos.x, pos.y, pos.z)) {
                        // 使用紫珀块作为墙壁
                        if (VanillaBlocks::PURPUR_BLOCK) {
                            world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::PURPUR_BLOCK->defaultState(), 2);
                        }
                    }
                }
            }
        }
    }
}

void EndCityPiece::generateTower(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds)
{
    MC_UNUSED(rng);

    // 塔楼部分
    for (int y = 4; y <= 10; ++y) {
        for (int x = 1; x < 4; ++x) {
            for (int z = 1; z < 4; ++z) {
                if (x == 1 || x == 3 || z == 1 || z == 3) {
                    BlockPos pos(m_minX + x, m_minY + y, m_minZ + z);
                    if (bounds.contains(pos.x, pos.y, pos.z)) {
                        if (VanillaBlocks::PURPUR_PILLAR) {
                            world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::PURPUR_PILLAR->defaultState(), 2);
                        }
                    }
                }
            }
        }
    }

    // 塔顶
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            BlockPos pos(m_minX + x, m_minY + 11, m_minZ + z);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::END_STONE_BRICKS) {
                    world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::END_STONE_BRICKS->defaultState(), 2);
                }
            }
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
