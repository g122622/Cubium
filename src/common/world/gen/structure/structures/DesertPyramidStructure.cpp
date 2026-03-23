#include "DesertPyramidStructure.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/BlockPos.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const String DesertPyramidStructure::m_name = "desert_pyramid";

DesertPyramidStructure::DesertPyramidStructure()
    : Structure(StructureType::Temple)
{
    initializeBiomes();
}

void DesertPyramidStructure::initializeBiomes() {
    m_validBiomes = {
        Desert, DesertHills, DesertLakes
    };
}

bool DesertPyramidStructure::canGenerate(
    IWorld& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // 检查生物群系是否合适
    // TODO: 实现生物群系检查
    return true;
}

std::unique_ptr<StructureStart> DesertPyramidStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 沙漠神殿尺寸: 21x21 地面部分，深度 12
    // 计算起始位置（居中）
    i32 startX = chunkX * 16 + rng.nextInt(16);
    i32 startZ = chunkZ * 16 + rng.nextInt(16);

    // 获取地表高度
    i32 startY = 64;  // TODO: 从地形获取实际高度

    BlockPos startPos(startX, startY, startZ);

    // TODO: 生成沙漠神殿
    // 结构包括：
    // 1. 主体塔楼（21x21）
    // 2. 四个角塔
    // 3. 入口
    // 4. 地下宝藏室
    // 5. TNT 陷阱（可选）

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
