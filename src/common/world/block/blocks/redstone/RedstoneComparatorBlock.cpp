#include "RedstoneComparatorBlock.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../redstone/RedstoneHelper.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/redstone/ComparatorEntity.hpp"
#include "../../../blockentity/BlockEntity.hpp"
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
        .add(BlockStateProperties::LOCKED())
        .add(MODE_PROP())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::POWERED(), false)
        .with(BlockStateProperties::LOCKED(), false)
        .with(MODE_PROP(), ComparatorMode::Compare));
}

std::unique_ptr<BlockEntity> RedstoneComparatorBlock::createBlockEntity(const BlockPos& pos) {
    return std::make_unique<blockentity::ComparatorEntity>(pos);
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

i32 RedstoneComparatorBlock::getStoredOutputSignal(IWorld& world, const BlockPos& pos) const {
    BlockEntity* be = world.getBlockEntity(pos);
    if (auto* comparator = dynamic_cast<blockentity::ComparatorEntity*>(be)) {
        return comparator->getOutputSignal();
    }
    return 0;
}

void RedstoneComparatorBlock::storeOutputSignal(IWorld& world, const BlockPos& pos, i32 signal) const {
    BlockEntity* be = world.getBlockEntity(pos);
    if (auto* comparator = dynamic_cast<blockentity::ComparatorEntity*>(be)) {
        comparator->setOutputSignal(signal);
    }
}

bool RedstoneComparatorBlock::shouldBePowered(IWorld& world, const BlockPos& pos,
                                            const BlockState& state) const {
    // MC Java 正确逻辑：
    // 1. 获取主输入信号（背面）
    // 2. 获取侧面输入信号
    // 3. 如果输入为0，不充能
    // 4. 如果输入 > 侧面信号，充能
    // 5. 如果输入 == 侧面信号且是比较模式，充能
    // 6. 否则不充能

    i32 mainInput = getInputSignal(world, pos, state);

    // 输入为0时不激活
    if (mainInput == 0) {
        return false;
    }

    i32 sidePower = getPowerOnSides(world, pos, state);

    // 主输入 > 侧面信号时激活
    if (mainInput > sidePower) {
        return true;
    }

    // 主输入 == 侧面信号时，只有比较模式才激活
    if (mainInput == sidePower) {
        return getMode(state) == ComparatorMode::Compare;
    }

    // 其他情况不激活
    return false;
}

i32 RedstoneComparatorBlock::calculateOutputSignal(IWorld& world, const BlockPos& pos,
                                                  const BlockState& state) const {
    // MC Java: 从 BlockEntity 读取输出信号
    // 这实现了"前端信号保持"特性
    if (!isPowered(state)) {
        return 0;
    }

    // 尝试从 BlockEntity 读取存储的信号
    i32 storedSignal = getStoredOutputSignal(world, pos);
    if (storedSignal > 0) {
        return storedSignal;
    }

    // 如果 BlockEntity 中没有存储信号，重新计算
    return calculateOutput(world, pos, state);
}

void RedstoneComparatorBlock::onStateChanged(IWorld& world, const BlockPos& pos,
                                             const BlockState& oldState,
                                             const BlockState& newState) {
    // 当状态变化时，更新 BlockEntity 中的输出信号
    if (isPowered(newState)) {
        i32 outputSignal = calculateOutput(world, pos, newState);
        storeOutputSignal(world, pos, outputSignal);
    } else {
        storeOutputSignal(world, pos, 0);
    }
}

i32 RedstoneComparatorBlock::calculateOutput(IWorld& world, const BlockPos& pos,
                                            const BlockState& state) const {
    // 获取主输入信号（背面）
    // MC Java: calculateInputStrength 包含容器信号检测
    i32 mainInput = calculateInputStrength(world, pos, state);

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

i32 RedstoneComparatorBlock::calculateInputStrength(IWorld& world, const BlockPos& pos,
                                                     const BlockState& state) const {
    // 先获取基础红石信号
    i32 input = getInputSignal(world, pos, state);

    Direction facing = getFacing(state);
    BlockPos inputPos = pos.offset(Directions::opposite(facing));
    const BlockState* inputState = world.getBlockState(inputPos.x, inputPos.y, inputPos.z);

    if (!inputState || inputState->isAir()) {
        return input;
    }

    const Block& inputBlock = inputState->getBlock();

    // MC Java: 检查容器信号覆盖（hasComparatorInputOverride）
    if (inputBlock.hasComparatorInputOverride(*inputState)) {
        i32 containerSignal = inputBlock.getComparatorInputOverride(*inputState, world, inputPos);
        return containerSignal;
    }

    // 如果输入信号 < 15 且输入端是实体方块
    // 检查实体方块后面是否有容器或物品展示框
    if (input < 15 && world::redstone::RedstoneHelper::isNormalCube(*inputState)) {
        BlockPos behindPos = inputPos.offset(Directions::opposite(facing));
        const BlockState* behindState = world.getBlockState(behindPos.x, behindPos.y, behindPos.z);

        if (behindState && !behindState->isAir()) {
            const Block& behindBlock = behindState->getBlock();

            // 检查后面的容器信号
            if (behindBlock.hasComparatorInputOverride(*behindState)) {
                i32 behindSignal = behindBlock.getComparatorInputOverride(*behindState, world, behindPos);
                if (behindSignal > input) {
                    input = behindSignal;
                }
            }
        }

        // TODO: 检查物品展示框的模拟信号（需要实体系统支持）
    }

    return input;
}

} // namespace blocks
} // namespace mc
