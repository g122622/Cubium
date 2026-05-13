#include "NyliumBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../../util/math/random/IRandom.hpp"

namespace mc::blocks {

// ============================================================================
// NyliumBlock 实现
// ============================================================================

NyliumBlock::NyliumBlock(BlockProperties properties)
    : Block(std::move(properties)) {
}

void NyliumBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random) {

    MC_UNUSED(random);  // 退化不需要随机数

    // 参考: MC 1.16.5 NyliumBlock.randomTick()
    // 如果位置不够暗，退化为下界岩

    if (!isDarkEnough(world, pos, state)) {
        const BlockState* netherrackState = &VanillaBlocks::NETHERRACK->defaultState();
        if (netherrackState != nullptr) {
            world.setBlockState(pos, netherrackState);
        }
    }
}

bool NyliumBlock::isDarkEnough(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state) {

    MC_UNUSED(state);  // 暂时未使用

    // 参考: MC 1.16.5 NyliumBlock.isDarkEnough()
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    // 计算光照
    u8 skyLight = world.getSkyLight(abovePos);
    u8 blockLight = world.getBlockLight(abovePos);

    // 考虑上方方块的遮挡
    u8 opacity = 0;
    if (aboveState != nullptr) {
        opacity = static_cast<u8>(aboveState->getOpacity());
    }

    // 简化计算：如果遮挡后光照仍然满亮度，则不够暗
    // 完整实现需要 LightEngine.func_215613_a
    u8 effectiveLight = 0;
    if (skyLight > opacity) {
        effectiveLight = skyLight - opacity;
    }
    effectiveLight = std::max(effectiveLight, blockLight);

    // 光照 < 15 时足够暗
    return effectiveLight < 15;
}

} // namespace mc::blocks
