#include "LilyPadBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../fluid/Fluid.hpp"

namespace mc {
namespace blocks {

LilyPadBlock::LilyPadBlock(const BlockProperties& properties)
    : BushBlock(properties)
{

    // 睡莲形状：扁平的圆形，略微高于水面
    m_shape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 0.015625f, 0.9375f);
}

BlockState LilyPadBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

bool LilyPadBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方是否为水
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 参考 MC 1.16.5: LilyPadBlock.isValidPosition
    // 检查是否为水方块（包括静止水和流动水）
    if (!belowState->is(VanillaBlocks::WATER)) {
        // 也可以检查材料是否为水
        const Material& material = belowState->getMaterial();
        if (!material.isLiquid()) {
            return false;
        }
    }

    // 检查当前方块是否为空气或水
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || currentState->isAir()) {
        return true;
    }

    // 如果当前是水，检查流体级别是否为满（级别 0 表示满）
    // 参考 MC 1.16.5: 水位必须为 0（满水源）才能放置睡莲
    const fluid::FluidState* fluidState = currentState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        // 满水源的级别为 0-7，只有级别 0-7 的静止水可以放置睡莲
        return fluidState->getLevel() <= 7;
    }

    return false;
}

const CollisionShape& LilyPadBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& LilyPadBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 睡莲有很小的碰撞箱，可以踩上去
    return m_shape;
}

void LilyPadBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);

    // 踩在睡莲上不会造成伤害
    // 但如果实体太大可能会破坏睡莲
}

bool LilyPadBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{

    MC_UNUSED(world);
    MC_UNUSED(groundPos);

    // 睡莲需要水作为支撑
    const Material& material = groundState.getMaterial();
    return material.isLiquid();
}

} // namespace blocks
} // namespace mc
