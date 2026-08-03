/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "JigsawPlacer.hpp"

#include "JigsawAssembler.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorldWriter.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/jigsaw/AssemblyTypes.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

void JigsawPlacer::placePieces(IWorldWriter& world,
    std::vector<PlacedPiece>& placedPieces,
    math::Random& rng,
    const structure::StructureBoundingBox* bounds,
    world::chunk::ChunkPrimer* chunk,
    IChunkGenerator* generator)
{
    // 对应 MC 1.21 JigsawManager.placePieces()：遍历所有已组装的 PlacedPiece，
    // 通过 virtual place() 多态分发到各子类（SingleJigsawPiece/ListJigsawPiece/FeatureJigsawPiece/EmptyJigsawPiece）。
    for (const auto& placed : placedPieces) {
        if (!placed.piece) {
            continue;
        }

        placed.piece->place(world, placed, JigsawAssembler::getTemplateManager(), rng, bounds, chunk, generator);
    }
}

void JigsawPlacer::placePiece(IWorldWriter& world,
    const PlacedPiece& placed,
    math::Random& rng,
    const structure::StructureBoundingBox* bounds,
    world::chunk::ChunkPrimer* chunk,
    IChunkGenerator* generator)
{
    // 对应原 JigsawManager::placePieceRecursive 的单块放置入口。
    // 各结构文件的 JigsawPlacedPieceAdapter::generate 调用此方法放置单个 PlacedPiece。
    if (placed.piece) {
        placed.piece->place(world, placed, JigsawAssembler::getTemplateManager(), rng, bounds, chunk, generator);
    }
}

void JigsawPlacer::placeFallbackBlocks(
    IWorldWriter& world, const PlacedPiece& placed, math::Random& rng, const structure::StructureBoundingBox* bounds)
{
    // 当模板未找到时，放置简单的方块来标记结构位置（从 JigsawManager::_placeFallbackBlocks 迁移）
    const BlockState* markerBlock = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);

    if (!markerBlock) {
        markerBlock = VanillaBlocks::getState(VanillaBlocks::STONE);
    }

    if (!markerBlock) {
        return; // 无法获取任何方块
    }

    // 获取边界框并在其中放置方块
    const auto& box = placed.boundingBox;
    for (i32 y = box.minY(); y <= box.maxY(); ++y) {
        for (i32 x = box.minX(); x <= box.maxX(); ++x) {
            for (i32 z = box.minZ(); z <= box.maxZ(); ++z) {
                if (bounds != nullptr && !bounds->contains(x, y, z)) {
                    continue;
                }

                // 只在边缘放置方块（创建框架）
                if (y == box.minY() || y == box.maxY() || x == box.minX() || x == box.maxX() || z == box.minZ() ||
                    z == box.maxZ()) {
                    // 添加一些随机性，避免过于规则
                    if (rng.nextInt(100) < 80) {
                        world.setBlockState(x, y, z, markerBlock, 18);
                    }
                }
            }
        }
    }
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
