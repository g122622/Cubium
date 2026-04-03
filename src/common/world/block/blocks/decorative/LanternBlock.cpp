#include "LanternBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

LanternBlock::LanternBlock(BlockProperties properties, u8 lightValue)
    : Block(std::move(properties))
    , m_lightValue(lightValue)
{
    // 创建状态容器（HANGING 属性）
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HANGING())
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
    setDefaultState(defaultState()
        .with(BlockStateProperties::HANGING(), false)
        .with(BlockStateProperties::WATERLOGGED(), false));

    // 创建形状
    // 站立形状：底部到中部
    m_standingShape = CollisionShape::box(5.0f, 0.0f, 5.0f, 11.0f, 7.0f, 11.0f);
    // 悬挂形状：顶部悬挂
    m_hangingShape = CollisionShape::box(5.0f, 1.0f, 5.0f, 11.0f, 8.0f, 11.0f);
}

BlockState LanternBlock::getStateForPlacement(BlockItemUseContext& context) {
    Direction clickedFace = context.getClickedFace();

    // 如果点击的是天花板，尝试悬挂
    if (clickedFace == Direction::Down) {
        return defaultState().with(BlockStateProperties::HANGING(), true);
    }

    // 默认站立
    return defaultState().with(BlockStateProperties::HANGING(), false);
}

bool LanternBlock::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // TODO: 检查是否有支撑方块
    return true;
}

BlockState LanternBlock::updatePostPlacement(
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
    // TODO: 检查支撑是否仍然存在
    return state;
}

const CollisionShape& LanternBlock::getShape(const BlockState& state) const {
    bool hanging = state.get(BlockStateProperties::HANGING());
    return hanging ? m_hangingShape : m_standingShape;
}

} // namespace blocks
} // namespace mc
