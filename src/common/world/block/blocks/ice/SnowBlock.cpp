#include "SnowBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include <unordered_map>

namespace mc::blocks {

SnowBlock::SnowBlock(BlockProperties properties)
    : Block(std::move(properties)) {
    // 注册 LAYERS 属性（1-8层）
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(LAYERS())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 默认状态：1层
    setDefaultState(getDefaultState().with(LAYERS(), 1));
}

void SnowBlock::randomTick(
    IWorld& world,
    const BlockPos& pos,
    BlockState& state,
    math::IRandom& random) {

    (void)random;  // 雪融化不需要随机数

    // 参考: MC 1.16.5 SnowBlock.randomTick()
    // 如果方块光照 > 11，融化

    u8 blockLight = world.getBlockLight(pos);
    u8 skyLight = world.getSkyLight(pos);

    // 计算综合光照（不考虑天气衰减的简化版本）
    u8 lightLevel = std::max(blockLight, skyLight);

    if (lightLevel > 11) {
        // 融化：生成掉落物并移除方块
        // TODO: 当添加物品系统后，这里应该掉落雪球
        // 目前直接移除方块
        const BlockState* airState = &VanillaBlocks::AIR->defaultState();
        if (airState != nullptr) {
            world.setBlock(pos, airState);
        }
    }
}

} // namespace mc::blocks
