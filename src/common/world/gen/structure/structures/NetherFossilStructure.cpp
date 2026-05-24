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

#include "NetherFossilStructure.hpp"
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

const std::string NetherFossilStructure::s_name = "Nether_Fossil";
const std::vector<BiomeId> NetherFossilStructure::s_validBiomes = {SoulSandValley};

NetherFossilStructure::NetherFossilStructure()
    : Structure(StructureType::Temple)
{}

bool NetherFossilStructure::canGenerate(
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

std::unique_ptr<StructureStart> NetherFossilStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(world);

    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 计算生成位置
    i32 x = chunkX * 16 + rng.nextInt(16);
    i32 z = chunkZ * 16 + rng.nextInt(16);

    // 下界化石通常在 Y=30-60 之间生成
    i32 y = 30 + rng.nextInt(30);

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

    // MC 1.16.5: 有多种化石类型
    i32 fossilType = rng.nextInt(5);

    auto piece = std::make_unique<NetherFossilPiece>(BlockPos(x, y, z), fossilType, rotation);
    start->addPiece(std::move(piece));

    return start;
}

// ============================================================================
// NetherFossilPiece
// ============================================================================

NetherFossilPiece::NetherFossilPiece(const BlockPos& pos, i32 fossilType, feature::template_::Rotation rotation)
    : StructurePiece(StructurePieceTypes::NETHER_FOSSIL, pos.x, pos.y, pos.z, pos.x + 15, pos.y + 10, pos.z + 15)
    , m_fossilType(fossilType)
    , m_rotation(rotation)
{
    // MC 1.16.5: 下界化石尺寸约 15x10x15
    m_minX = pos.x;
    m_minY = pos.y;
    m_minZ = pos.z;
    m_maxX = pos.x + 14;
    m_maxY = pos.y + 9;
    m_maxZ = pos.z + 14;
}

void NetherFossilPiece::generate(
    IWorldWriter& world, math::Random& rng, i32 chunkX, i32 chunkZ, const StructureBoundingBox& chunkBounds)
{
    MC_UNUSED(chunkX);
    MC_UNUSED(chunkZ);

    // 检查边界框是否与区块相交
    StructureBoundingBox pieceBounds(m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ);
    if (!pieceBounds.intersects(chunkBounds)) {
        return;
    }

    generateFossil(world, rng, chunkBounds);
}

void NetherFossilPiece::generateFossil(IWorldWriter& world, math::Random& rng, const StructureBoundingBox& bounds)
{
    // MC 1.16.5: 下界化石是由骨块构成的随机形状
    // 有 5 种预设形状，这里简化实现为程序化生成

    // 骨块状态
    const BlockState* boneBlockState = VanillaBlocks::BONE_BLOCK ? &VanillaBlocks::BONE_BLOCK->defaultState() : nullptr;
    if (!boneBlockState) {
        return;
    }

    // 根据化石类型生成不同的骨块结构
    // 这里简化实现，生成一个大致的"脊椎"或"肋骨"形状

    switch (m_fossilType % 5) {
        case 0: // 脊椎形状
            for (int i = 0; i < 10; ++i) {
                // 主干
                BlockPos spinePos(m_minX + 7, m_minY + 5, m_minZ + i);
                if (bounds.contains(spinePos.x, spinePos.y, spinePos.z)) {
                    world.setBlockState(spinePos.x, spinePos.y, spinePos.z, boneBlockState, 2);
                }
                // 周围随机骨块
                if (rng.nextFloat() < 0.3f) {
                    BlockPos extraPos(m_minX + 6 + rng.nextInt(3), m_minY + 4 + rng.nextInt(3), m_minZ + i);
                    if (bounds.contains(extraPos.x, extraPos.y, extraPos.z)) {
                        world.setBlockState(extraPos.x, extraPos.y, extraPos.z, boneBlockState, 2);
                    }
                }
            }
            break;

        case 1: // 头骨形状
            // 头骨底部
            for (int x = 4; x <= 10; ++x) {
                for (int z = 4; z <= 10; ++z) {
                    if ((x - 7) * (x - 7) + (z - 7) * (z - 7) <= 9) {
                        BlockPos pos(m_minX + x, m_minY + 3, m_minZ + z);
                        if (bounds.contains(pos.x, pos.y, pos.z)) {
                            world.setBlockState(pos.x, pos.y, pos.z, boneBlockState, 2);
                        }
                    }
                }
            }
            // 头骨顶部
            for (int x = 5; x <= 9; ++x) {
                for (int z = 5; z <= 9; ++z) {
                    if ((x - 7) * (x - 7) + (z - 7) * (z - 7) <= 4) {
                        BlockPos pos(m_minX + x, m_minY + 5, m_minZ + z);
                        if (bounds.contains(pos.x, pos.y, pos.z)) {
                            world.setBlockState(pos.x, pos.y, pos.z, boneBlockState, 2);
                        }
                    }
                }
            }
            break;

        case 2: // 肋骨形状
            for (int side = 0; side < 2; ++side) {
                int xBase = (side == 0) ? m_minX + 5 : m_minX + 9;
                for (int rib = 0; rib < 4; ++rib) {
                    // 肋骨弧线
                    for (int h = 0; h <= 3; ++h) {
                        BlockPos pos(xBase, m_minY + h, m_minZ + rib * 2 + 3);
                        if (bounds.contains(pos.x, pos.y, pos.z)) {
                            world.setBlockState(pos.x, pos.y, pos.z, boneBlockState, 2);
                        }
                    }
                }
            }
            // 脊柱
            for (int z = 3; z <= 11; ++z) {
                BlockPos pos(m_minX + 7, m_minY + 1, m_minZ + z);
                if (bounds.contains(pos.x, pos.y, pos.z)) {
                    world.setBlockState(pos.x, pos.y, pos.z, boneBlockState, 2);
                }
            }
            break;

        case 3: // 散落的骨头
            for (int i = 0; i < 20; ++i) {
                int x = m_minX + rng.nextInt(15);
                int y = m_minY + rng.nextInt(8);
                int z = m_minZ + rng.nextInt(15);
                BlockPos pos(x, y, z);
                if (bounds.contains(pos.x, pos.y, pos.z)) {
                    world.setBlockState(pos.x, pos.y, pos.z, boneBlockState, 2);
                }
            }
            break;

        case 4: // 大型骨骼结构
        default:
            // 水平主干
            for (int x = 2; x <= 12; ++x) {
                BlockPos pos(m_minX + x, m_minY + 4, m_minZ + 7);
                if (bounds.contains(pos.x, pos.y, pos.z)) {
                    world.setBlockState(pos.x, pos.y, pos.z, boneBlockState, 2);
                }
            }
            // 垂直支撑
            for (int y = 0; y <= 6; ++y) {
                BlockPos pos1(m_minX + 4, m_minY + y, m_minZ + 7);
                BlockPos pos2(m_minX + 10, m_minY + y, m_minZ + 7);
                if (bounds.contains(pos1.x, pos1.y, pos1.z)) {
                    world.setBlockState(pos1.x, pos1.y, pos1.z, boneBlockState, 2);
                }
                if (bounds.contains(pos2.x, pos2.y, pos2.z)) {
                    world.setBlockState(pos2.x, pos2.y, pos2.z, boneBlockState, 2);
                }
            }
            break;
    }
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
