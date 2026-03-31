#include "BeaconBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== BeaconBlock 实现 ==========

BeaconBlock::BeaconBlock(const BlockProperties& properties)
    : Block(properties) {

    // 信标没有特殊状态属性
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 信标形状是完整的方块
    m_shape = CollisionShape::fullBlock();
}

BlockState BeaconBlock::getStateForPlacement(BlockItemUseContext& context) {
    return defaultState();
}

const CollisionShape& BeaconBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_shape;
}

int BeaconBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // TODO: 从信标方块实体获取比较器信号
    // 信标的比较器输出取决于激活层数
    // 1层 = 1, 2层 = 2, 3层 = 3, 4层 = 4
    // 需要实现 BeaconEntity

    return 0;
}

} // namespace blocks
} // namespace mc
