#include "OceanMonumentStructure.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/BlockPos.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const String OceanMonumentStructure::m_name = "ocean_monument";

OceanMonumentStructure::OceanMonumentStructure()
    : Structure(StructureType::Monument)
{
    initializeBiomes();
}

void OceanMonumentStructure::initializeBiomes() {
    m_validBiomes = {
        DeepOcean, DeepWarmOcean, DeepLukewarmOcean, DeepColdOcean, DeepFrozenOcean
    };
}

bool OceanMonumentStructure::canGenerate(
    IWorld& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // 检查生物群系是否合适
    // 海洋纪念碑需要足够大的水域
    // TODO: 实现详细的生物群系检查
    return true;
}

std::unique_ptr<StructureStart> OceanMonumentStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 海洋纪念碑尺寸: 58x58 地面部分，高度 23
    // 计算起始位置
    i32 startX = chunkX * 16 + rng.nextInt(16);
    i32 startZ = chunkZ * 16 + rng.nextInt(16);

    // 海洋纪念碑生成在水下，通常 Y=39 左右
    i32 startY = 39;

    BlockPos startPos(startX, startY, startZ);

    // TODO: 生成海洋纪念碑
    // 结构包括：
    // 1. 主入口（顶部）
    // 2. 中央房间（有远古守卫者）
    // 3. 翼楼（左右两边）
    // 4. 宝藏房间（海绵室）
    // 5. 守卫者生成点

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
