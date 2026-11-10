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

#include "FortressStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/structures/NetherFortressPieces.hpp"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::structure {

// ============================================================================
// FortressStructure
// ============================================================================

const std::string FortressStructure::m_name = "fortress";

const SpawnOverrides FortressStructure::s_spawnOverrides = {
    SpawnOverrideType::Piece, {SpawnOverrideEntry{"monster", 2, 4}}};

FortressStructure::FortressStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* FortressStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_FORTRESS();
}

std::unique_ptr<StructureStart> FortressStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = makeStructureStart(chunkX, chunkZ);

    // 原版 `NetherFortressStructure.findGenerationPoint`：候选生成点固定为
    // (区块最小角, MAGIC_START_Y=64, 区块最小角)，群系校验就落在这个点上。
    const i32 chunkMinX = chunkX * world::CHUNK_WIDTH;
    const i32 chunkMinZ = chunkZ * world::CHUNK_WIDTH;
    if (!isValidBiomeAt(generator, BlockPos(chunkMinX, nether_fortress::MAGIC_START_Y, chunkMinZ))) {
        return start;
    }

    // 起始片段在区块内 (minX+2, minZ+2)，水平朝向由一次 nextInt(4) 抽出。
    auto startPiece = std::make_unique<nether_fortress::StartPiece>(rng, chunkMinX + 2, chunkMinZ + 2);
    nether_fortress::StartPiece* startPtr = startPiece.get();
    start->addPiece(std::move(startPiece));

    nether_fortress::PieceAccessor accessor{start->pieces(), startPtr->pendingChildren()};

    // 起始片段先直接展开一次；之后按「随机挑一个待展开分支 → 展开」的队列循环推进。
    // `nextInt(pending.size())` 即使 size 为 1 也会推进随机流，不能跳过。
    startPtr->addChildren(*startPtr, accessor, rng);
    auto& pending = startPtr->pendingChildren();
    while (!pending.empty()) {
        const i32 pick = rng.nextInt(static_cast<i32>(pending.size()));
        MC_ASSERT_RELEASE(pick >= 0 && static_cast<size_t>(pick) < pending.size());
        StructurePiece* current = pending[static_cast<size_t>(pick)];
        pending.erase(pending.begin() + pick);
        static_cast<nether_fortress::NetherBridgePiece*>(current)->addChildren(*startPtr, accessor, rng);
    }

    // 原版 `StructurePiecesBuilder.moveInsideHeights(random, 48, 70)`：把整座要塞平移进
    // [48, 70] 这段高度区间；只有剩余空间为正时才消耗一次 nextInt。
    i32 actualMinY = std::numeric_limits<i32>::max();
    i32 actualMaxY = std::numeric_limits<i32>::min();
    for (const auto& piece : start->pieces()) {
        const StructureBoundingBox box = piece->getBoundingBox();
        actualMinY = std::min(actualMinY, box.minY());
        actualMaxY = std::max(actualMaxY, box.maxY());
    }
    const i32 spare =
        (nether_fortress::HEIGHT_RANGE_MAX - nether_fortress::HEIGHT_RANGE_MIN + 1) - (actualMaxY - actualMinY + 1);
    const i32 targetMinY =
        spare > 0 ? nether_fortress::HEIGHT_RANGE_MIN + rng.nextInt(spare) : nether_fortress::HEIGHT_RANGE_MIN;
    const i32 offsetY = targetMinY - actualMinY;
    for (auto& piece : start->pieces()) {
        piece->offset(0, offsetY, 0);
    }

    start->recalculateStructureSize();
    return start;
}

} // namespace mc::world::gen::structure
