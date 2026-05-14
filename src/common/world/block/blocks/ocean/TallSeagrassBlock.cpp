#include "TallSeagrassBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "../../WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

using DoubleBlockHalf = BlockStateProperties::DoubleBlockHalf;

TallSeagrassBlock::TallSeagrassBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower)
            .with(BlockStateProperties::WATERLOGGED(), true));

    // 形状
    m_lowerShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
    m_upperShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

BlockStateProperties::DoubleBlockHalf TallSeagrassBlock::getHalf(const BlockState& state) const
{
    return state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
}

BlockState TallSeagrassBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr || aboveState->isAir()) {
        return defaultState().with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower);
    }

    return defaultState();
}

bool TallSeagrassBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    auto half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 上半部分需要在下半部分之上
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        return belowState != nullptr && belowState->is(this) &&
            belowState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == DoubleBlockHalf::Lower;
    } else {
        // 下半部分需要支撑
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        return belowState != nullptr && belowState->isSolid();
    }
}

BlockState TallSeagrassBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    auto half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 上半部分检查下方
        if (facing == Direction::Down) {
            if (!facingState.is(this) ||
                facingState.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) != DoubleBlockHalf::Lower) {
                if (auto* airState = BlockRegistry::instance().airState()) {
                    return *airState;
                }
            }
        }
    } else {
        // 下半部分检查上方
        if (facing == Direction::Up) {
            if (!facingState.is(this) ||
                facingState.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) != DoubleBlockHalf::Upper) {
                // 上方没有上半部分 - 可能需要清理，但这里暂不处理
            }
        }
    }

    return state;
}

const CollisionShape& TallSeagrassBlock::getShape(const BlockState& state) const
{
    auto half = getHalf(state);
    return (half == DoubleBlockHalf::Upper) ? m_upperShape : m_lowerShape;
}

const CollisionShape& TallSeagrassBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
