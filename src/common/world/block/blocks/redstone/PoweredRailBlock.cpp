#include "PoweredRailBlock.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/redstone/RedstonePower.hpp"

namespace mc {
namespace blocks {

PoweredRailBlock::PoweredRailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, true)
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(SHAPE())
        .add(POWERED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(SHAPE(), RailShape::NorthSouth)
        .with(POWERED(), false));
}

void PoweredRailBlock::fillStateContainer(StateContainer<Block, BlockState>& container) {
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
}

i32 PoweredRailBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 动力铁轨不输出红石信号
    return 0;
}

void PoweredRailBlock::neighborChanged(
    IWorld& world,
    const BlockPos& pos,
    Block& neighborBlock,
    const BlockPos& neighborPos,
    bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 检查是否应该更新激活状态
    const BlockState* currentState = world.getBlockState(pos.x, pos.y, pos.z);
    if (!currentState) return;

    // TODO: 检查红石信号
    // bool shouldPower = RedstonePower::isPowered(world, pos);
    // if (shouldPower != isPowered(*currentState)) {
    //     world.setBlockState(pos.x, pos.y, pos.z, currentState->with(POWERED(), shouldPower), 3);
    // }
}

RailShape PoweredRailBlock::getRailShape(const BlockState& state) const {
    return state.get(SHAPE());
}

BlockState PoweredRailBlock::withRailShape(const BlockState& state, RailShape shape) const {
    return state.with(SHAPE(), shape);
}

bool PoweredRailBlock::isPowered(const BlockState& state) {
    return state.get(POWERED());
}

} // namespace blocks
} // namespace mc
