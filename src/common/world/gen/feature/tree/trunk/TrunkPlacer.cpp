#include "TrunkPlacer.hpp"
#include "../../../chunk/IChunkGenerator.hpp"
#include "../../../../block/BlockRegistry.hpp"
#include "../../../../block/VanillaBlocks.hpp"
#include "../../../../../core/Types.hpp"

namespace mc {

TrunkPlacer::TrunkPlacer(i32 baseHeight, i32 heightRandA, i32 heightRandB)
    : m_baseHeight(baseHeight)
    , m_heightRandA(heightRandA)
    , m_heightRandB(heightRandB)
{
}

i32 TrunkPlacer::getHeight(math::Random& random) const {
    i32 height = m_baseHeight;
    if (m_heightRandA > 0) {
        height += random.nextInt(0, m_heightRandA);
    }
    if (m_heightRandB > 0) {
        height += random.nextInt(0, m_heightRandB);
    }
    return height;
}

void TrunkPlacer::placeBlock(
    WorldGenRegion& world,
    const BlockPos& pos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock
) {
    // 检查是否在有效范围内
    if (pos.y < 0 || pos.y >= 256) {
        return;
    }

    if (trunkBlock == nullptr) {
        return;
    }

    // 设置方块
    world.setBlock(pos, trunkBlock);

    // 记录树干方块位置
    trunkBlocks.insert(pos);
}

bool TrunkPlacer::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) {
    // 检查位置是否在有效范围内
    if (pos.y < 0 || pos.y >= 256) {
        return false;
    }

    // 获取当前位置的方块
    const BlockState* state = world.getBlock(pos.x, pos.y, pos.z);
    if (state == nullptr || state->isAir()) {
        return true;  // 空气或其他可替换方块
    }

    // 检查是否是树叶
    if (state->is(VanillaBlocks::OAK_LEAVES) ||
        state->is(VanillaBlocks::SPRUCE_LEAVES) ||
        state->is(VanillaBlocks::BIRCH_LEAVES) ||
        state->is(VanillaBlocks::JUNGLE_LEAVES) ||
        state->is(VanillaBlocks::ACACIA_LEAVES) ||
        state->is(VanillaBlocks::DARK_OAK_LEAVES)) {
        return true;
    }

    return false;
}

void TrunkPlacer::placeDirtUnder(WorldGenRegion& world, const BlockPos& pos) {
    // 检查下方位置
    BlockPos belowPos = pos.down();
    if (belowPos.y < 0) {
        return;
    }

    // 获取下方方块
    const BlockState* state = world.getBlock(belowPos.x, belowPos.y, belowPos.z);
    if (state == nullptr) {
        return;
    }

    // 如果不是草方块或泥土，放置泥土
    if (!state->is(VanillaBlocks::GRASS_BLOCK) && !state->is(VanillaBlocks::DIRT)) {
        if (VanillaBlocks::DIRT) {
            world.setBlock(belowPos.x, belowPos.y, belowPos.z, &VanillaBlocks::DIRT->defaultState());
        }
    }
}

void TrunkPlacer::placeTrunkLayer2x2(
    WorldGenRegion& world,
    const BlockPos& pos,
    std::set<BlockPos>& trunkBlocks,
    const BlockState* trunkBlock
) {
    for (i32 dx = 0; dx < 2; ++dx) {
        for (i32 dz = 0; dz < 2; ++dz) {
            placeBlock(world, BlockPos(pos.x + dx, pos.y, pos.z + dz), trunkBlocks, trunkBlock);
        }
    }
}

} // namespace mc
