#include "NetherCaveCarver.hpp"
#include "../../block/BlockRegistry.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../../util/math/random/Random.hpp"

namespace mc {

// ============================================================================
// 下界可雕刻方块集合 - 使用延迟初始化避免静态初始化顺序问题
// ============================================================================

static const std::unordered_set<BlockId>& getNetherCarvableBlocks()
{
    static std::unordered_set<BlockId> blocks = {
        VanillaBlocks::STONE->blockId(),
        VanillaBlocks::GRANITE->blockId(),
        VanillaBlocks::DIORITE->blockId(),
        VanillaBlocks::ANDESITE->blockId(),
        VanillaBlocks::DIRT->blockId(),
        VanillaBlocks::COARSE_DIRT->blockId(),
        VanillaBlocks::PODZOL->blockId(),
        VanillaBlocks::GRASS_BLOCK->blockId(),
        VanillaBlocks::NETHERRACK->blockId(),
        VanillaBlocks::SOUL_SAND->blockId(),
        VanillaBlocks::SOUL_SOIL->blockId(),
        VanillaBlocks::CRIMSON_NYLIUM->blockId(),
        VanillaBlocks::WARPED_NYLIUM->blockId(),
        VanillaBlocks::NETHER_WART_BLOCK->blockId(),
        VanillaBlocks::WARPED_WART_BLOCK->blockId(),
        VanillaBlocks::BASALT->blockId(),
        VanillaBlocks::BLACKSTONE->blockId()
    };
    return blocks;
}

// ============================================================================
// NetherCaveCarver 实现
// ============================================================================

NetherCaveCarver::NetherCaveCarver()
    : CaveCarver(128)  // 下界最大高度 128
{
}

f32 NetherCaveCarver::getCaveRadius(math::IRandom& rng) const
{
    // 参考 MC NetherCaveCarver.func_230359_a_
    // return (rand.nextFloat() * 2.0F + rand.nextFloat()) * 2.0F;
    return (rng.nextFloat() * 2.0f + rng.nextFloat()) * 2.0f;
}

i32 NetherCaveCarver::getCaveStartY(math::IRandom& rng) const
{
    // 参考 MC NetherCaveCarver.func_230361_b_
    // return rand.nextInt(this.maxHeight);
    return rng.nextInt(m_maxHeight);
}

bool NetherCaveCarver::isNetherCarvable(const BlockState& state)
{
    const auto& blocks = getNetherCarvableBlocks();
    return blocks.find(state.blockId()) != blocks.end();
}

bool NetherCaveCarver::canCarveBlock(const BlockState* state, const BlockState* aboveState) const
{
    if (!state) {
        return false;
    }

    // 检查是否在下界可雕刻方块列表中
    if (isNetherCarvable(*state)) {
        return true;
    }

    // 沙子和沙砾可以在特定条件下雕刻
    // 参考 MC: (state.isIn(Blocks.SAND) || state.isIn(Blocks.GRAVEL)) && !aboveState.getFluidState().isTagged(FluidTags.WATER)
    bool isSandOrGravel = state->is(VanillaBlocks::SAND) || state->is(VanillaBlocks::GRAVEL);
    if (isSandOrGravel && aboveState) {
        // 下界中检查熔岩而不是水
        // 但由于下界很少有水和沙子组合，这里简化处理
        return !aboveState->isLiquid();
    }

    return false;
}

} // namespace mc
