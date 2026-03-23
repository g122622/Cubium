#include "JungleTempleStructure.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../block/BlockPos.hpp"

namespace mc {
namespace world {
namespace gen {
namespace structure {

using namespace mc::Biomes;

const String JungleTempleStructure::m_name = "jungle_temple";

JungleTempleStructure::JungleTempleStructure()
    : Structure(StructureType::Temple)
{
    initializeBiomes();
}

void JungleTempleStructure::initializeBiomes() {
    m_validBiomes = {
        Jungle, JungleHills, JungleEdge, ModifiedJungle, ModifiedJungleEdge
    };
}

bool JungleTempleStructure::canGenerate(
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

std::unique_ptr<StructureStart> JungleTempleStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 丛林神庙尺寸: 12x15 地面部分
    // 计算起始位置
    i32 startX = chunkX * 16 + rng.nextInt(16);
    i32 startZ = chunkZ * 16 + rng.nextInt(16);

    // 获取地表高度
    i32 startY = 64;  // TODO: 从地形获取实际高度

    BlockPos startPos(startX, startY, startZ);

    // TODO: 生成丛林神庙
    // 结构包括：
    // 1. 主入口走廊
    // 2. 拉杆谜题房间
    // 3. 箭矢陷阱走廊
    // 4. 隐藏宝箱室
    // 5. 三层塔楼结构

    return start;
}

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
