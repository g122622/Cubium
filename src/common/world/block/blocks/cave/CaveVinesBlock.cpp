/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "CaveVinesBlock.hpp"
#include "CaveVinesPlantBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/CaveBlocks.hpp"

namespace mc {
namespace blocks {

CaveVinesBlock::CaveVinesBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_25())
            .add(BlockStateProperties::BERRIES())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(
        defaultState().with(BlockStateProperties::AGE_0_25(), 0).with(BlockStateProperties::BERRIES(), false));
}

void CaveVinesBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

const CollisionShape& CaveVinesBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

void CaveVinesBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    i32 age = static_cast<i32>(state.get(BlockStateProperties::AGE_0_25()));

    // 已达最大年龄则不再生长
    if (age >= MAX_AGE) {
        return;
    }

    // 11%概率生长（MC源码：random.nextInt(9) == 0）
    if (random.nextInt(9) != 0) {
        return;
    }

    // 检查下方是否有空间生长
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr || !belowState->isAir()) {
        return;
    }

    // 在下方放置洞穴藤蔓植物体（CaveVinesPlantBlock）
    const BlockState& plantState = CaveBlocks::CAVE_VINES_PLANT->defaultState()
                                       .with(BlockStateProperties::BERRIES(), state.get(BlockStateProperties::BERRIES()));
    world.setBlockState(belowPos, &plantState, 3);

    // 当前尖端增长年龄，且重置浆果状态
    const BlockState& newState = state.with(BlockStateProperties::AGE_0_25(), age + 1)
                                     .with(BlockStateProperties::BERRIES(), false);
    world.setBlockState(pos, &newState, 3);
}

bool CaveVinesBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);
    i32 age = static_cast<i32>(state.get(BlockStateProperties::AGE_0_25()));
    return age < MAX_AGE;
}

bool CaveVinesBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    return true;
}

void CaveVinesBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);

    i32 age = static_cast<i32>(state.get(BlockStateProperties::AGE_0_25()));
    if (age >= MAX_AGE) {
        return;
    }

    // 骨粉直接生长到最大年龄
    const BlockState& newState = state.with(BlockStateProperties::AGE_0_25(), MAX_AGE);
    world.setBlockState(pos, &newState, 3);
}

ActionResultType CaveVinesBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 浆果存在时，右键收获
    if (state.get(BlockStateProperties::BERRIES())) {
        const BlockState& newState = state.with(BlockStateProperties::BERRIES(), false);
        world.setBlockState(pos, &newState, 3);
        // TODO: 掉落发光浆果物品（需要LootTable系统支持）
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

bool CaveVinesBlock::_hasBerries(const BlockState& state)
{
    return state.get(BlockStateProperties::BERRIES());
}

} // namespace blocks
} // namespace mc
