#include "RespawnAnchorBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== RespawnAnchorBlock 实现 ==========

RespawnAnchorBlock::RespawnAnchorBlock(const BlockProperties& properties)
    : Block(properties) {

    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::CHARGES_0_4())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::CHARGES_0_4(), 0));

    // 重生锚形状是完整方块
    m_shape = CollisionShape::fullBlock();
}

BlockState RespawnAnchorBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

void RespawnAnchorBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    // 重生锚的tick处理
    // 当前没有特殊的tick逻辑
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
}

void RespawnAnchorBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 在下界之外，充能的重生锚可能会爆炸
    // TODO: 检查维度，如果不是下界则爆炸
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
}

const CollisionShape& RespawnAnchorBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

int RespawnAnchorBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 充能等级
    return getCharges(state);
}

u8 RespawnAnchorBlock::getLightLevel(
    const BlockState& state,
    IWorld* world,
    const BlockPos* pos) const {

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 光照等级 = charges * 3.75，向下取整
    // 0 -> 0, 1 -> 3, 2 -> 7, 3 -> 11, 4 -> 15
    int charges = getCharges(state);
    return static_cast<u8>(std::floor(charges * 3.75f));
}

BlockState RespawnAnchorBlock::charge(IWorld& world, const BlockPos& pos, BlockState& state) {
    int charges = getCharges(state);
    if (charges < 4) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges + 1);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);
        // TODO: 播放充能音效和粒子效果
        return newState;
    }
    return state;
}

void RespawnAnchorBlock::discharge(IWorld& world, const BlockPos& pos, BlockState& state) {
    int charges = getCharges(state);
    if (charges > 0) {
        BlockState newState = state.with(BlockStateProperties::CHARGES_0_4(), charges - 1);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);
    }
}

} // namespace blocks
} // namespace mc
