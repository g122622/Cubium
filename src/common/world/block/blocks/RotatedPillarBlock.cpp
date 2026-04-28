#include "RotatedPillarBlock.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"

namespace mc {

namespace {
    // 静态属性实例
    std::unique_ptr<EnumProperty<Axis>> g_axisProperty;
}

const EnumProperty<Axis>& RotatedPillarBlock::AXIS() {
    if (!g_axisProperty) {
        g_axisProperty = AxisProperty::create("axis");
    }
    return *g_axisProperty;
}

RotatedPillarBlock::RotatedPillarBlock(BlockProperties properties)
    : Block(properties) {
    // 创建带有axis属性的状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(AXIS())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
    // 设置默认轴向为 Y（原版行为）
    setDefaultState(withAxis(defaultState(), Axis::Y));
}

Axis RotatedPillarBlock::getAxis(const BlockState& state) const {
    return state.get(AXIS());
}

const BlockState& RotatedPillarBlock::withAxis(const BlockState& state, Axis axis) const {
    return state.with(AXIS(), axis);
}

const BlockState& RotatedPillarBlock::rotate(const BlockState& state, Rotation rotation) const {
    // 参考: net.minecraft.block.RotatedPillarBlock#rotate
    // 90度旋转时，X轴和Z轴互换
    if (rotation == Rotation::Clockwise90 || rotation == Rotation::CounterClockwise90) {
        Axis currentAxis = state.get(AXIS());
        if (currentAxis == Axis::X) {
            return state.with(AXIS(), Axis::Z);
        } else if (currentAxis == Axis::Z) {
            return state.with(AXIS(), Axis::X);
        }
    }
    // Y轴旋转或无旋转时保持不变
    return state;
}

BlockState RotatedPillarBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 参考: net.minecraft.block.RotatedPillarBlock#getStateForPlacement
    // 根据放置面的轴向设置初始状态
    Direction clickedFace = context.getClickedFace();
    Axis axis = Directions::getAxis(clickedFace);
    return withAxis(defaultState(), axis);
}

} // namespace mc
