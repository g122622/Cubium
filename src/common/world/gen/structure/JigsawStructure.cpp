#include "JigsawStructure.hpp"
#include "../jigsaw/JigsawManager.hpp"
#include "../jigsaw/JigsawPattern.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../block/BlockPos.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

const String JigsawStructure::m_name = "jigsaw";
const std::vector<BiomeId> JigsawStructure::m_validBiomes;

JigsawStructure::JigsawStructure(
    const JigsawConfig& config,
    i32 startY,
    bool nearTerrain,
    bool adjustForTerrain)
    : Structure(StructureType::Village)
    , m_config(config)
    , m_startY(startY)
    , m_nearTerrain(nearTerrain)
    , m_adjustForTerrain(adjustForTerrain)
{
}

bool JigsawStructure::canGenerate(
    IWorld& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    return true;
}

std::unique_ptr<StructureStart> JigsawStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
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
    auto placedPieces = jigsaw::JigsawManager::assemble(
        patternRegistry,
        *startPool,
        m_config.size,
        startPos,
        rng
    );

    // TODO: 将 placedPieces 转换为 StructurePieces 并添加到 StructureStart

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
