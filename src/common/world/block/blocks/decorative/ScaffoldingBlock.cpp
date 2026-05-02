#include "ScaffoldingBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../fluid/FluidRegistry.hpp"
#include "../../../fluid/FluidTags.hpp"
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

    // 检查是否含水
    const fluid::FluidState* fluidState = world.getFluidState(pos);
    bool waterlogged = fluidState != nullptr && fluidState->getFluid().isIn(fluid::FluidTags::WATER());

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
    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        MC_ASSERT(waterFluid != nullptr);
        world.tickManager().scheduleFluidTick(currentPos, *waterFluid, waterFluid->getTickDelay(world));
    }

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

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* ScaffoldingBlock::getFluidState(const BlockState& state) const {
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
