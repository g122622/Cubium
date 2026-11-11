#include "OceanMonumentStructure.hpp"

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/structures/OceanMonumentPieces.hpp"
#include <memory>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const std::string OceanMonumentStructure::m_name = "ocean_monument";

const SpawnOverrides OceanMonumentStructure::m_spawnOverrides = {
    SpawnOverrideType::Full, {SpawnOverrideEntry{"monster", 4, 4}}};

OceanMonumentStructure::OceanMonumentStructure(ResourceLocation id)
    : Structure(std::move(id))
{}

const biome::BiomeTag* OceanMonumentStructure::defaultBiomeTag() const
{
    return &biome::BiomeTags::HAS_STRUCTURE_MONUMENT();
}

std::unique_ptr<StructureStart> OceanMonumentStructure::generate(
    IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const
{
    MC_UNUSED(generator);

    using namespace mc::world;

    auto start = makeStructureStart(chunkX, chunkZ);

    // 原版先按「区块内第 9 格 + 海平面」为中心取 29 半径内的群系，要求全部落在
    // `required_ocean_monument_surrounding` 标签内；再在候选生成点（区块中心、海底高度）
    // 校验本结构自身的群系标签。
    const i32 centerX = chunkX * CHUNK_WIDTH + 9;
    const i32 centerZ = chunkZ * CHUNK_WIDTH + 9;
    constexpr i32 outerRadius = 29;
    const auto* biomeSource = generator.getBiomeSource();
    MC_ASSERT_RELEASE_MSG(biomeSource != nullptr, "OceanMonumentStructure requires a biome source");
    // getBiomesWithin 内部按 4 格（一个 quart）为步长遍历方框，与这边原版
    // BiomeSource.getBiomesWithin 的采样完全一致，不再需要自己按区块步长近似。
    for (const BiomeId biome : biomeSource->getBiomesWithin(centerX, generator.seaLevel(), centerZ, outerRadius)) {
        if (!biome::BiomeTags::REQUIRED_OCEAN_MONUMENT_SURROUNDING().contains(biome)) {
            return start;
        }
    }
    if (!isValidBiomeAt(generator, onTopOfChunkCenter(generator, chunkX, chunkZ, HeightmapType::OceanFloorWG))) {
        return start;
    }

    const i32 startX = chunkX * CHUNK_WIDTH - 29;
    const i32 startZ = chunkZ * CHUNK_WIDTH - 29;

    // 随机水平方向
    static const Direction horizontalDirections[] = {
        Direction::North, Direction::South, Direction::East, Direction::West};
    Direction direction = horizontalDirections[rng.nextInt(4)];

    auto building = std::make_unique<OceanMonumentBuilding>(rng, startX, startZ, direction);

    // 只添加片段到 StructureStart；方块写入延迟到 FEATURES 阶段由 placeInChunk() 执行
    // OceanMonumentBuilding::generate() 会在 FEATURES 阶段被调用，并分发给其子片段
    start->addPiece(std::move(building));
    start->recalculateStructureSize();
    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
