#include "RuinedPortalStructure.hpp"
#include "../StructureBoundingBox.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../IWorldWriter.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc::world::gen::structure {

// RuinedPortalPiece 实现
RuinedPortalPiece::RuinedPortalPiece(i32 x, i32 y, i32 z, i32 sizeX, i32 sizeY, i32 sizeZ)
    : StructurePiece(StructurePieceTypes::RUINED_PORTAL, x, y, z, x + sizeX, y + sizeY, z + sizeZ)
{
}

bool RuinedPortalPiece::isInBounds(i32 x, i32 y, i32 z, const StructureBoundingBox& chunkBounds) const {
    return chunkBounds.contains(x, y, z);
}

void RuinedPortalPiece::generate(IWorldWriter& world, math::Random& rng,
                                  i32 /*chunkX*/, i32 /*chunkZ*/,
                                  const StructureBoundingBox& chunkBounds)
{
    // 生成废弃传送门框架
    const BlockState* obsidianState = VanillaBlocks::getState(VanillaBlocks::OBSIDIAN);
    const BlockState* stoneBricksState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);

    // 遍历片段范围内的所有方块
    for (i32 dx = 0; dx <= maxX() - minX(); ++dx) {
        for (i32 dy = 0; dy <= maxY() - minY(); ++dy) {
            for (i32 dz = 0; dz <= maxZ() - minZ(); ++dz) {
                i32 worldX = minX() + dx;
                i32 worldY = minY() + dy;
                i32 worldZ = minZ() + dz;

                // 只在区块边界内放置
                if (!isInBounds(worldX, worldY, worldZ, chunkBounds)) {
                    continue;
                }

                // 只在边缘放置黑曜石
                bool isEdge = (dx == 0 || dx == maxX() - minX() ||
                              dy == 0 || dy == maxY() - minY() ||
                              dz == 0 || dz == maxZ() - minZ());

                if (isEdge && rng.nextFloat() < 0.8f) {
                    world.setBlockState(worldX, worldY, worldZ,
                                  obsidianState ? obsidianState : stoneBricksState);
                }
            }
        }
    }
}

const std::string RuinedPortalStructure::m_name = "ruined_portal";

const std::vector<BiomeId> RuinedPortalStructure::m_validBiomes = {
    Biomes::Plains,
    Biomes::Desert,
    Biomes::Forest,
    Biomes::Taiga,
    Biomes::Mountains,
    Biomes::SnowyPlains,
    Biomes::Swamp,
    Biomes::Badlands
};

bool RuinedPortalStructure::canGenerate(
    IWorld& /*world*/,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ)
{
    // 使用间距设置检查是否应该在此位置生成
    i32 startX, startZ;
    if (!findStructureStart(static_cast<i64>(generator.seed()), chunkX, chunkZ, m_settings, startX, startZ)) {
        return false;
    }

    // 概率检查（参考 MC）
    return rng.nextFloat() < 0.3f;
}

std::unique_ptr<StructureStart> RuinedPortalStructure::generate(
    IWorldWriter& world,
    IChunkGenerator& generator,
    math::Random& rng,
    i32 chunkX,
    i32 chunkZ) const
{
    auto start = std::make_unique<StructureStart>(chunkX, chunkZ);

    // 确定传送门位置
    i32 baseX = (chunkX << 4) + rng.nextInt(16);
    i32 baseZ = (chunkZ << 4) + rng.nextInt(16);

    // 获取地表高度
    i32 surfaceY = generator.getHeight(baseX, baseZ, HeightmapType::WorldSurfaceWG);

    // 确定传送门大小（参考 MC）
    i32 portalWidth = 4 + rng.nextInt(2);   // 4-5 格宽
    i32 portalHeight = 5 + rng.nextInt(3);  // 5-7 格高

    // 判断是否在下界（基于生物群系）
    bool isNether = false;  // 主世界废弃传送门

    // 生成传送门框架
    generatePortalFrame(world, baseX, surfaceY, baseZ, rng, isNether);

    // 创建片段并添加到起点
    auto piece = std::make_unique<RuinedPortalPiece>(
        baseX, surfaceY, baseZ, portalWidth, portalHeight, 1);
    start->addPiece(std::move(piece));

    return start;
}

void RuinedPortalStructure::generatePortalFrame(
    IWorldWriter& world,
    i32 x,
    i32 y,
    i32 z,
    math::Random& rng,
    bool /*isNether*/) const
{
    const BlockState* obsidianState = VanillaBlocks::getState(VanillaBlocks::OBSIDIAN);
    const BlockState* stoneBricksState = VanillaBlocks::getState(VanillaBlocks::STONE_BRICKS);
    const BlockState* magmaState = VanillaBlocks::getState(VanillaBlocks::MAGMA);

    // 传送门尺寸
    constexpr i32 PORTAL_WIDTH = 4;
    constexpr i32 PORTAL_HEIGHT = 5;

    // 生成黑曜石框架
    for (i32 dx = 0; dx < PORTAL_WIDTH; ++dx) {
        for (i32 dy = 0; dy < PORTAL_HEIGHT; ++dy) {
            // 只在边缘放置黑曜石
            bool isFrame = (dx == 0 || dx == PORTAL_WIDTH - 1 || dy == 0 || dy == PORTAL_HEIGHT - 1);

            if (isFrame) {
                // 部分黑曜石可能损坏
                if (rng.nextFloat() > 0.15f) {  // 85% 概率放置
                    world.setBlockState(x + dx, y + dy, z, obsidianState);
                }
            }
        }
    }

    // 随机放置一些装饰性方块
    for (i32 i = 0; i < 4 + rng.nextInt(4); ++i) {
        i32 dx = rng.nextInt(PORTAL_WIDTH + 4) - 2;
        i32 dz = rng.nextInt(3) - 1;
        i32 dy = rng.nextInt(3);

        // 放置石砖或岩浆块
        if (rng.nextBoolean()) {
            world.setBlockState(x + dx, y + dy, z + dz, stoneBricksState);
        } else if (magmaState) {
            world.setBlockState(x + dx, y + dy, z + dz, magmaState);
        }
    }
}

} // namespace mc::world::gen::structure
