#include "BuriedTreasureStructure.hpp"
#include "../StructureBoundingBox.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../Structure.hpp"  // for Structure::createRandom

namespace mc::world::gen::structure {

// BuriedTreasurePiece 实现
BuriedTreasurePiece::BuriedTreasurePiece(i32 x, i32 y, i32 z)
    : StructurePiece(StructurePieceTypes::BURIED_TREASURE, x, y, z, x + 2, y + 2, z + 2)  // 3x3x3 区域
{
}

bool BuriedTreasurePiece::isInBounds(i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds) const {
    return chunkBounds.contains(x, y, z);
}

void BuriedTreasurePiece::generate(IWorldWriter& world, math::Random& rng,
                                   i32 /*chunkX*/, i32 /*chunkZ*/,
                                   const StructureBoundingBox& chunkBounds)
{
    const BlockState* goldState = VanillaBlocks::getState(VanillaBlocks::GOLD_BLOCK);
    const BlockState* sandState = VanillaBlocks::getState(VanillaBlocks::SAND);
    const BlockState* stoneState = VanillaBlocks::getState(VanillaBlocks::STONE);

    // 放置宝藏箱子（中心位置）
    i32 centerX = minX() + 1;
    i32 centerY = minY() + 1;
    i32 centerZ = minZ() + 1;

    if (isInBounds(centerX, centerY, centerZ, chunkBounds)) {
        // 放置金块作为宝藏占位符（箱子方块尚未实现）
        if (goldState) {
            world.setBlockState(centerX, centerY, centerZ, goldState);
        }
    }

    // 在周围放置沙子/石头作为保护
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) continue;  // 跳过中心

            i32 x = centerX + dx;
            i32 y = centerY - 1;
            i32 z = centerZ + dz;

            if (isInBounds(x, y, z, chunkBounds)) {
                world.setBlockState(x, y, z, stoneState ? stoneState : sandState);
            }
        }
    }
}

const std::string BuriedTreasureStructure::m_name = "buried_treasure";

const std::vector<BiomeId> BuriedTreasureStructure::m_validBiomes = {
    Biomes::Beach,
    Biomes::SnowyBeach
};

bool BuriedTreasureStructure::canGenerate(
    IWorld& /*world*/,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // MC 1.16.5: BuriedTreasureStructure.func_230363_a_
    // 使用单独的 salt=10387320 计算种子，然后检查概率
    // 注意：这里的 rng 已经由 findStructureStart 使用正确的种子初始化了

    // MC 1.16.5: 埋藏宝藏的概率检查
    // nextFloat() < 0.01 (1% 概率)
    return rng.nextFloat() < 0.01f;
}

std::unique_ptr<StructureStart> BuriedTreasureStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 在区块中心附近找一个合适的位置
    i32 baseX = (chunkX << 4) + rng.nextInt(16);
    i32 baseZ = (chunkZ << 4) + rng.nextInt(16);

    // 找到地表高度（参考 MC: 在沙子下面 3-6 格）
    i32 seaLevel = generator.seaLevel();
    i32 surfaceY = generator.getHeight(baseX, baseZ, HeightmapType::OceanFloorWG);

    // 宝藏应该埋在沙子下面
    i32 treasureY = surfaceY - rng.nextInt(3, 6);
    if (treasureY < 0) {
        treasureY = 0;
    }

    // 创建并添加片段
    auto piece = std::make_unique<BuriedTreasurePiece>(baseX, treasureY, baseZ);
    start->addPiece(std::move(piece));

    // 立即在区块中生成（用于简单结构）
    StructureBoundingBox chunkBounds = StructureBoundingBox::fromChunk(chunkX, chunkZ);
    for (const auto& p : start->pieces()) {
        p->generate(world, rng, chunkX, chunkZ, chunkBounds);
    }

    return start;
}

} // namespace mc::world::gen::structure
