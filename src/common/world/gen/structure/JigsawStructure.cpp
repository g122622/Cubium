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

#include "JigsawStructure.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../block/BlockPos.hpp"
#include "../chunk/IChunkGenerator.hpp"
#include "../jigsaw/JigsawManager.hpp"
#include "../jigsaw/JigsawPattern.hpp"

namespace {

class JigsawPlacedPieceAdapter final : public mc::world::gen::structure::StructurePiece {
public:
    explicit JigsawPlacedPieceAdapter(const mc::world::gen::jigsaw::PlacedPiece& placed)
        : StructurePiece(90,
              placed.boundingBox.minX(),
              placed.boundingBox.minY(),
              placed.boundingBox.minZ(),
              placed.boundingBox.maxX(),
              placed.boundingBox.maxY(),
              placed.boundingBox.maxZ())
    {}

    void generate(mc::IWorldWriter&,
        mc::math::Random&,
        mc::i32,
        mc::i32,
        const mc::world::gen::structure::StructureBoundingBox&) override
    {
        // 实际方块放置已在 JigsawManager::assembleAndPlace 中完成
    }
};

} // anonymous namespace

namespace mc {
namespace world {
namespace gen {
namespace structure {

const std::string JigsawStructure::m_name = "jigsaw";
const std::vector<BiomeId> JigsawStructure::m_validBiomes;

JigsawStructure::JigsawStructure(const JigsawConfig& config, i32 startY, bool nearTerrain, bool adjustForTerrain)
    : Structure(StructureType::Village)
    , m_config(config)
    , m_startY(startY)
    , m_nearTerrain(nearTerrain)
    , m_adjustForTerrain(adjustForTerrain)
{}

bool JigsawStructure::canGenerate(IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ)
{
    (void)world;
    (void)rng;

    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(m_config.startPool);
    if (!startPool || startPool->isEmpty()) {
        return false;
    }

    if (m_nearTerrain) {
        const i32 centerX = chunkX * 16 + 8;
        const i32 centerZ = chunkZ * 16 + 8;
        const i32 topY = generator.getHeight(centerX, centerZ, HeightmapType::WorldSurfaceWG);
        if (topY < 20 || topY > 220) {
            return false;
        }
    }

    return true;
}

std::unique_ptr<StructureStart> JigsawStructure::generate(
    IWorldWriter& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 获取起始模板池
    auto& patternRegistry = jigsaw::JigsawPatternRegistry::instance();
    const jigsaw::JigsawPattern* startPool = patternRegistry.getPattern(m_config.startPool);

    if (!startPool || startPool->isEmpty()) {
        return start;
    }

    // 计算起始位置
    BlockPos startPos(chunkX * 16 + 8, m_startY, chunkZ * 16 + 8);

    // 组装结构
    auto placedPieces = jigsaw::JigsawManager::assemble(patternRegistry, *startPool, m_config.size, startPos, rng);

    for (const auto& placed : placedPieces) {
        start->addPiece(std::make_unique<JigsawPlacedPieceAdapter>(placed));
    }

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
