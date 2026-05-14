#include "BubbleColumnBlock.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../VanillaBlocks.hpp"

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

BubbleColumnBlock::BubbleColumnBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DRAG())
            .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::DRAG(), false));
}

bool BubbleColumnBlock::isDrag(const BlockState& state) const
{
    return state.get(BlockStateProperties::DRAG());
}

BlockState BubbleColumnBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 根据下方方块决定是否为下拖
    bool drag = checkSource(world, pos);

    return defaultState().with(BlockStateProperties::DRAG(), drag);
}

bool BubbleColumnBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 气泡柱只能出现在水中（或已有气泡柱位置）
    const BlockState* currentState = world.getBlockState(pos);
    const bool isWater =
        currentState != nullptr && VanillaBlocks::WATER != nullptr && currentState->is(VanillaBlocks::WATER);
    const bool isBubbleColumn = currentState != nullptr && currentState->is(this);
    if (!isWater && !isBubbleColumn) {
        return false;
    }

    // 检查下方是否是气泡源或另一段气泡柱
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }

    if (VanillaBlocks::MAGMA != nullptr && belowState->is(VanillaBlocks::MAGMA)) {
        return true;
    }
    if (VanillaBlocks::SOUL_SAND != nullptr && belowState->is(VanillaBlocks::SOUL_SAND)) {
        return true;
    }
    return belowState->is(this);
}

BlockState BubbleColumnBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    // MC 1.16.5: 气泡柱更新逻辑
    // 1. 下方源变化时更新drag状态
    // 2. 上方是水时调度tick传播气泡柱

    if (facing == Direction::Down) {
        // 下方方块变化，检查源是否变化
        bool newDrag = checkSource(world, currentPos);
        if (newDrag != isDrag(state)) {
            return state.with(BlockStateProperties::DRAG(), newDrag);
        }
    }

    if (facing == Direction::Up) {
        // 上方方块变化
        // 如果上方是水，需要将其转换为气泡柱
        // 这通过tick方法处理，这里暂时不调度
        // 实际MC中会调度一个tick来处理：world.scheduleBlockTick(currentPos, *this, 5);
    }

    return state;
}

void BubbleColumnBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity)
{
    MC_UNUSED(&world);
    MC_UNUSED(&pos);

    // MC 1.16.5: 气泡柱推动实体
    // 上推速度: 0.1 (灵魂沙)
    // 下拖速度: 0.03 (岩浆块，实际是 -0.03)
    if (isDrag(state)) {
        // 下拖：向下推动实体（岩浆块产生）
        entity.addVelocity(0.0, -0.03, 0.0);
    } else {
        // 上推：向上推动实体（灵魂沙产生）
        entity.addVelocity(0.0, 0.1, 0.0);
    }

    // 重置摔落距离
    entity.setFallDistance(0.0f);
}

void BubbleColumnBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // MC 1.16.5: 气泡柱传播逻辑
    // 检查上方是否是水，如果是则放置气泡柱
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState != nullptr && aboveState->is(VanillaBlocks::WATER)) {
        // 上方是水，需要放置气泡柱
        bool currentDrag = isDrag(state);

        // 获取默认的气泡柱状态
        const BlockState& bubbleState = defaultState().with(BlockStateProperties::DRAG(), currentDrag);

        // 设置方块（替换水）
        world.setBlockState(abovePos, &bubbleState, 2);

        // 为新放置的气泡柱调度tick以继续传播
        // 注意：需要通过方块tick系统调度，这里暂时简化处理
        // 实际应该: world.scheduleBlockTick(abovePos, *this, 5);
    } else if (aboveState != nullptr && aboveState->is(this)) {
        // 上方已经是气泡柱，更新其drag状态
        bool currentDrag = isDrag(state);
        bool aboveDrag = aboveState->get(BlockStateProperties::DRAG());

        if (currentDrag != aboveDrag) {
            // 状态不一致，更新上方的drag状态
            const BlockState& newAboveState = aboveState->with(BlockStateProperties::DRAG(), currentDrag);
            world.setBlockState(abovePos, &newAboveState, 2);
        }
    }
}

const CollisionShape& BubbleColumnBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

const CollisionShape& BubbleColumnBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

bool BubbleColumnBlock::checkSource(const IWorld& world, const BlockPos& pos) const
{
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 岩浆块产生下拖气泡柱
    if (VanillaBlocks::MAGMA != nullptr && belowState->is(VanillaBlocks::MAGMA)) {
        return true;
    }

    // 灵魂沙产生上升气泡柱
    if (VanillaBlocks::SOUL_SAND != nullptr && belowState->is(VanillaBlocks::SOUL_SAND)) {
        return false;
    }

    // 继承下方气泡柱的拖拽方向
    if (belowState->is(this)) {
        return isDrag(*belowState);
    }

    return false;
}

} // namespace blocks
} // namespace mc
