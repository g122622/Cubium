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

const std::string WoodlandMansionStructure::s_name = "Woodland_Mansion";
const std::vector<BiomeId> WoodlandMansionStructure::s_validBiomes = {DarkForest, DarkForestHills};

WoodlandMansionStructure::WoodlandMansionStructure()
    : Structure(StructureType::Temple)
{}

bool WoodlandMansionStructure::canGenerate(
    IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    MC_UNUSED(world);
    MC_UNUSED(rng);

    // 检查生物群系
    BiomeId biome = generator.getBiome(chunkX * 16 + 8, 64, chunkZ * 16 + 8);
    for (BiomeId valid : s_validBiomes) {
        if (biome == valid) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<StructureStart> WoodlandMansionStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(world);

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

    auto piece = std::make_unique<WoodlandMansionPiece>(BlockPos(x, y, z), rotation);
    start->addPiece(std::move(piece));

    return start;
}

// ============================================================================
// WoodlandMansionPiece
// ============================================================================

WoodlandMansionPiece::WoodlandMansionPiece(const BlockPos& pos, feature::template_::Rotation rotation)
    : StructurePiece(StructurePieceTypes::WOODLAND_MANSION, pos.x, pos.y, pos.z, pos.x + 58, pos.y + 20, pos.z + 58)
    , m_rotation(rotation)
{
    // MC 1.16.5: 林地府邸尺寸约 58x20x58
    m_minX = pos.x;
    m_minY = pos.y;
    m_minZ = pos.z;
    m_maxX = pos.x + 57;
    m_maxY = pos.y + 19;
    m_maxZ = pos.z + 57;
}

void WoodlandMansionPiece::generate(
    IWorldWriter& world, math::Random& rng, i32 chunkX, i32 chunkZ, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    generateMansion(world, rng, chunkBounds);
}

void WoodlandMansionPiece::generateMansion(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds)
{
    // MC 1.16.5: 林地府邸是一座大型三层建筑
    // 简化实现：生成基本框架

    // 地基（深色橡木木板）
    for (int x = 0; x < 58; ++x) {
        for (int z = 0; z < 58; ++z) {
            BlockPos pos(m_minX + x, m_minY, m_minZ + z);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::DARK_OAK_PLANKS) {
                    world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::DARK_OAK_PLANKS->defaultState(), 2);
                }
            }
        }
    }

    // 生成各层
    for (int floor = 0; floor < 3; ++floor) {
        generateFloor(world, m_minY + 1 + floor * 6, bounds);
    }

    // 屋顶
    generateRoof(world, bounds);
}

void WoodlandMansionPiece::generateFloor(IWorldWriter& world, i32 floorY, const StructureBoundingBox& bounds)
{
    // 外墙（深色橡木木板）
    for (int x = 0; x < 58; ++x) {
        for (int z = 0; z < 58; ++z) {
            if (x == 0 || x == 57 || z == 0 || z == 57) {
                for (int y = 0; y < 5; ++y) {
                    BlockPos pos(m_minX + x, floorY + y, m_minZ + z);
                    if (bounds.contains(pos.x, pos.y, pos.z)) {
                        if (VanillaBlocks::DARK_OAK_PLANKS) {
                            world.setBlockState(
                                pos.x, pos.y, pos.z, &VanillaBlocks::DARK_OAK_PLANKS->defaultState(), 2);
                        }
                    }
                }
            }
        }
    }

    // 内部走廊框架（深色橡木原木）
    for (int x = 5; x < 53; x += 10) {
        for (int z = 5; z < 53; ++z) {
            for (int y = 0; y < 5; ++y) {
                BlockPos pos(m_minX + x, floorY + y, m_minZ + z);
                if (bounds.contains(pos.x, pos.y, pos.z)) {
                    if (VanillaBlocks::DARK_OAK_LOG) {
                        world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::DARK_OAK_LOG->defaultState(), 2);
                    }
                }
            }
        }
    }
}

void WoodlandMansionPiece::generateRoof(IWorldWriter& world, const StructureBoundingBox& bounds)
{
    // 屋顶（深色橡木楼梯）
    for (int x = -1; x <= 58; ++x) {
        for (int z = -1; z <= 58; ++z) {
            BlockPos pos(m_minX + x, m_maxY, m_minZ + z);
            if (bounds.contains(pos.x, pos.y, pos.z)) {
                if (VanillaBlocks::DARK_OAK_PLANKS) {
                    world.setBlockState(pos.x, pos.y, pos.z, &VanillaBlocks::DARK_OAK_PLANKS->defaultState(), 2);
                }
            }
        }
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
