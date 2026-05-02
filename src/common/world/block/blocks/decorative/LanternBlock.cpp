#include "LanternBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/FluidTags.hpp"
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
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否含水
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = fluidState != nullptr && fluidState->getFluid().isIn(fluid::FluidTags::WATER());

    // 如果点击的是天花板，尝试悬挂
    if (clickedFace == Direction::Down) {
        return defaultState()
            .with(BlockStateProperties::HANGING(), true)
            .with(BlockStateProperties::WATERLOGGED(), waterlogged);
    }

    // 默认站立
    return defaultState()
        .with(BlockStateProperties::HANGING(), false)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
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
    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        MC_ASSERT(waterFluid != nullptr);
        world.tickManager().scheduleFluidTick(currentPos, *waterFluid, waterFluid->getTickDelay(world));
    }

    // TODO: 检查支撑是否仍然存在
    return state;
}

const CollisionShape& LanternBlock::getShape(const BlockState& state) const {
    bool hanging = state.get(BlockStateProperties::HANGING());
    return hanging ? m_hangingShape : m_standingShape;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* LanternBlock::getFluidState(const BlockState& state) const {
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(
            fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            return &waterFluid->defaultState();
        }
    }
    return Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
