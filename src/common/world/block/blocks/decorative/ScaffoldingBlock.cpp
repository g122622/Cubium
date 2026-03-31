#include "ScaffoldingBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"

namespace mc {
namespace blocks {

ScaffoldingBlock::ScaffoldingBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::DISTANCE_1_7())
        .add(BlockStateProperties::WATERLOGGED())
        .add(BlockStateProperties::BOTTOM())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
        .with(BlockStateProperties::DISTANCE_1_7(), 7)
        .with(BlockStateProperties::WATERLOGGED(), false)
        .with(BlockStateProperties::BOTTOM(), false));

    // 创建形状
    // 脚手架是薄框架结构
    m_baseShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f / 16.0f, 1.0f);
    m_fullShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

BlockState ScaffoldingBlock::getStateForPlacement(BlockItemUseContext& context) {
    BlockPos pos = context.placementPos();
    const IWorld& world = context.getWorld();

    // 计算距离支撑点的距离
    i32 distance = 7; // TODO: 计算实际距离
    bool bottom = !hasSupport(const_cast<IBlockReader&>(static_cast<const IBlockReader&>(world)), pos);
    bool waterlogged = false; // TODO: 检查流体状态

    return defaultState()
        .with(BlockStateProperties::DISTANCE_1_7(), distance)
        .with(BlockStateProperties::BOTTOM(), bottom)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState ScaffoldingBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);
    // TODO: 更新距离和底部状态
    return state;
}

bool ScaffoldingBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 检查距离支撑点是否过远
    return true;
}

const CollisionShape& ScaffoldingBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    // 返回完整形状（用于渲染）
    return m_fullShape;
}

const CollisionShape& ScaffoldingBlock::getCollisionShape(const BlockState& state) const {
    bool bottom = state.get(BlockStateProperties::BOTTOM());
    // 只有底部有碰撞（玩家可以穿过脚手架）
    if (bottom) {
        return m_baseShape;
    }
    return m_fullShape;
}

bool ScaffoldingBlock::hasSupport(IBlockReader& world, const BlockPos& pos) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 检查下方是否有支撑
    return true;
}

} // namespace blocks
} // namespace mc
