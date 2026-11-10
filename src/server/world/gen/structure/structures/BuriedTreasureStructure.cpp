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

#include "BuriedTreasureStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {

// BuriedTreasurePiece 实现
BuriedTreasurePiece::BuriedTreasurePiece(i32 x, i32 y, i32 z)
    // 原版 `new BoundingBox(p_227366_)` 是**单点**盒子；生成阶段向下搜索到合适地层后
    // 才把包围盒改成落点那一格。这里若铺成 3x3x3，起点包围盒会凭空大一圈，
    // 整体尺寸、碰撞判定与差分输出都会偏离。
    : StructurePiece(StructurePieceTypes::BURIED_TREASURE, x, y, z, x, y, z)
{}

void BuriedTreasurePiece::generate(IWorldWriter& world,
    math::IRandom& rng,
    i32 /*chunkX*/,
    i32 /*chunkZ*/,
    const StructureBoundingBox& chunkBounds,
    ChunkPrimer* /*chunk*/,
    IChunkGenerator* /*generator*/)
{
    IWorld* reader = dynamic_cast<IWorld*>(&world);
    MC_ASSERT_RELEASE_MSG(reader != nullptr, "BuriedTreasurePiece::generate requires an IWorld");

    const BlockState* sandState = VanillaBlocks::getState(VanillaBlocks::SAND);
    MC_ASSERT_RELEASE(sandState != nullptr);

    const auto isAir = [](const BlockState* state) {
        return state == nullptr || state->isAir();
    };
    const auto isLiquid = [](const BlockState* state) {
        return state != nullptr && (state->is(VanillaBlocks::WATER) || state->is(VanillaBlocks::LAVA));
    };
    // 原版只认这五种「下方地层」，其余方块（含沙子）不算命中。
    const auto isTreasureFloor = [](const BlockState* state) {
        return state != nullptr &&
            (state->is(VanillaBlocks::SANDSTONE) || state->is(VanillaBlocks::STONE) ||
                state->is(VanillaBlocks::ANDESITE) || state->is(VanillaBlocks::GRANITE) ||
                state->is(VanillaBlocks::DIORITE));
    };

    // 原版从 `getHeight(OCEAN_FLOOR_WG)`（首个可放置 Y）沿列向下找第一层「下方是石质地层」
    // 的位置。本项目 `IWorld::getHeight` 返回最高方块 Y，故 +1 后才与原版光标起点一致。
    const i32 columnX = m_minX;
    const i32 columnZ = m_minZ;
    const i32 minY = reader->getMinBuildHeight();
    i32 cursorY = reader->getHeight(columnX, columnZ, HeightmapType::OceanFloorWG) + 1;

    while (cursorY > minY) {
        const BlockState* state = reader->getBlockState(columnX, cursorY, columnZ);
        const BlockState* belowState = reader->getBlockState(columnX, cursorY - 1, columnZ);
        if (isTreasureFloor(belowState)) {
            // 宝藏格保留原方块；原方块是空气/液体时铺沙子。
            const BlockState* cellState = (!isAir(state) && !isLiquid(state)) ? state : sandState;

            // 遍历顺序必须是 `Direction.values()`：Down/Up/North/South/West/East。
            // 侧面下方是空腔时铺地层方块，否则铺宝藏格方块，全部写入后才放箱子。
            for (Direction direction : Directions::all()) {
                const BlockPos neighbor(columnX + Directions::xOffset(direction),
                    cursorY + Directions::yOffset(direction),
                    columnZ + Directions::zOffset(direction));
                const BlockState* neighborState = reader->getBlockState(neighbor);
                if (!isAir(neighborState) && !isLiquid(neighborState)) {
                    continue;
                }

                const BlockState* neighborBelow = reader->getBlockState(neighbor.x, neighbor.y - 1, neighbor.z);
                if ((isAir(neighborBelow) || isLiquid(neighborBelow)) && direction != Direction::Up) {
                    world.setBlockState(
                        neighbor.x, neighbor.y, neighbor.z, belowState, world::BlockUpdateFlags::UPDATE_ALL);
                } else {
                    world.setBlockState(
                        neighbor.x, neighbor.y, neighbor.z, cellState, world::BlockUpdateFlags::UPDATE_ALL);
                }
            }

            // 原版把片段包围盒收缩为落点这一格，再以该格为锚点生成宝箱。
            m_minX = columnX;
            m_maxX = columnX;
            m_minY = cursorY;
            m_maxY = cursorY;
            m_minZ = columnZ;
            m_maxZ = columnZ;
            // 原版也忽略放置结果（越界或已有箱子时不放置，这里同样只需尝试一次）。
            (void)createChestAt(world,
                chunkBounds,
                rng,
                BlockPos(columnX, cursorY, columnZ),
                ResourceLocation("minecraft", "chests/buried_treasure"));
            return;
        }
        --cursorY;
    }
}

const std::string BuriedTreasureStructure::m_name = "buried_treasure";

const biome::BiomeTag* BuriedTreasureStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_BURIED_TREASURE();
}

std::unique_ptr<StructureStart> BuriedTreasureStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(rng);

    auto start = makeStructureStart(chunkX, chunkZ);

    // 生物群系校验落在候选生成点（区块中心、海底高度）上；不匹配即返回空起点。
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, HeightmapType::OceanFloorWG))) {
        return start;
    }

    // 生成点固定为「区块中心列 + (9, 90, 9)」，Y 的 90 只是占位值：片段的包围盒是一个
    // 单点，真正的落点在 FEATURES 阶段沿该列向下搜索时确定。这里不能抽样任何随机数以
    // 挑选 X/Z 或深度——原版 generatePieces 全程不消耗随机数。
    const BlockPos position(chunkX * world::CHUNK_WIDTH + 9, 90, chunkZ * world::CHUNK_WIDTH + 9);

    start->addPiece(std::make_unique<BuriedTreasurePiece>(position.x, position.y, position.z));
    start->recalculateStructureSize();

    MC_UNUSED(generator);
    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
