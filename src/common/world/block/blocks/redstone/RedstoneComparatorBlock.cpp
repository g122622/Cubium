#include "RedstoneComparatorBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include <unordered_map>

namespace mc {

// EnumProperty Traits 实现 - 必须在 mc 命名空间
String EnumProperty<blocks::ComparatorMode>::Traits::toString(const blocks::ComparatorMode& value) {
    switch (value) {
        case blocks::ComparatorMode::Compare: return "compare";
        case blocks::ComparatorMode::Subtract: return "subtract";
        default: return "compare";
    }
}

Optional<blocks::ComparatorMode> EnumProperty<blocks::ComparatorMode>::Traits::fromName(StringView name) {
    if (name == "compare") return blocks::ComparatorMode::Compare;
    if (name == "subtract") return blocks::ComparatorMode::Subtract;
    return std::nullopt;
}

namespace blocks {

// 比较器模式属性
namespace {
    const EnumProperty<ComparatorMode>& MODE_PROP() {
        static auto prop = EnumProperty<ComparatorMode>::create("mode", {
            ComparatorMode::Compare,
            ComparatorMode::Subtract
        });
        return *prop;
    }
}

RedstoneComparatorBlock::RedstoneComparatorBlock(const BlockProperties& properties)
    : RedstoneDiodeBlock("redstone_comparator", properties) {

    // 创建状态容器 - 包含基类的 FACING 和 POWERED，以及比较器特有的 MODE
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::POWERED())
        .add(MODE_PROP())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::POWERED(), false)
        .with(MODE_PROP(), ComparatorMode::Compare));
}

i32 RedstoneComparatorBlock::getDelay(const BlockState& state) const {
    MC_UNUSED(state);
    return COMPARATOR_DELAY;
}

ComparatorMode RedstoneComparatorBlock::getMode(const BlockState& state) {
    return state.get(MODE_PROP());
}

BlockState RedstoneComparatorBlock::withMode(BlockState state, ComparatorMode mode) {
    return state.with(MODE_PROP(), mode);
}

bool RedstoneComparatorBlock::isSubtractMode(const BlockState& state) {
    return getMode(state) == ComparatorMode::Subtract;
}

bool RedstoneComparatorBlock::shouldBePowered(IWorld& world, const BlockPos& pos,
                                            const BlockState& state) const {
    // 计算输出信号，如果大于0则应该激活
    return calculateOutput(world, pos, state) > 0;
}

i32 RedstoneComparatorBlock::calculateOutputSignal(IWorld& world, const BlockPos& pos,
                                                  const BlockState& state) const {
    if (!isPowered(state)) {
        return 0;
    }
    return calculateOutput(world, pos, state);
}

i32 RedstoneComparatorBlock::calculateOutput(IWorld& world, const BlockPos& pos,
                                            const BlockState& state) const {
    // 获取主输入信号（背面）
    i32 mainInput = getInputSignal(world, pos, state);

    // 获取侧面输入信号
    i32 sidePower = getPowerOnSides(world, pos, state);

    if (isSubtractMode(state)) {
        // 减法模式：输出 = 主输入 - 侧面输入
        return std::max(0, mainInput - sidePower);
    } else {
        // 比较模式：如果主输入 >= 侧面输入，输出主输入；否则输出0
        if (mainInput >= sidePower) {
            return mainInput;
        }
        return 0;
    }
}

} // namespace blocks
} // namespace mc
