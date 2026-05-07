#include "SnowBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../util/math/random/Random.hpp"
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
        // 融化：掉落雪球并移除方块
        // 参考 MC 1.16.5: spawnDrops(state, world, pos)
        // 雪层掉落雪球数量等于层数
        i32 layers = state.get(LAYERS());
        if (layers > 0 && Items::SNOWBALL != nullptr) {
            ItemStack dropStack(*Items::SNOWBALL, layers);
            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world, dropStack,
                static_cast<f64>(pos.x) + 0.5,
                static_cast<f64>(pos.y) + 0.5,
                static_cast<f64>(pos.z) + 0.5,
                rng);
        }
        const BlockState* airState = &VanillaBlocks::AIR->defaultState();
        if (airState != nullptr) {
            world.setBlockState(pos, airState);
        }
    }
}

} // namespace mc::blocks
