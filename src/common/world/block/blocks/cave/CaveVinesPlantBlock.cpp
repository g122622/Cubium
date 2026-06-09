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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "CaveVinesPlantBlock.hpp"
#include "CaveVinesBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

CaveVinesPlantBlock::CaveVinesPlantBlock(const BlockProperties& properties)
    : GrowingPlantBodyBlock(properties, Direction::Down, CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15))
    , IGrowable()
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::BERRIES())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::BERRIES(), false));
}

void CaveVinesPlantBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

const Block* CaveVinesPlantBlock::getHeadBlock() const
{
    return VanillaBlocks::CAVE_VINES;
}

const Block* CaveVinesPlantBlock::getBodyBlock() const
{
    return VanillaBlocks::CAVE_VINES_PLANT;
}

BlockState CaveVinesPlantBlock::updateHeadAfterConvertedFromBody(const BlockState& bodyState) const
{
    // 身体变成头部时，传递 BERRIES 状态
    const Block* headBlock = getHeadBlock();
    if (headBlock) {
        return headBlock->defaultState()
            .with(BlockStateProperties::AGE_0_25(), 0)
            .with(BlockStateProperties::BERRIES(), bodyState.get(BlockStateProperties::BERRIES()));
    }
    return bodyState;
}

bool CaveVinesPlantBlock::canGrow(
    IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);
    // MC 1.21.11: 身体方块的骨粉检查也是 !hasBerries
    return !state.get(BlockStateProperties::BERRIES());
}

bool CaveVinesPlantBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    return true;
}

void CaveVinesPlantBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(random);
    // MC 1.21.11: 身体方块骨粉效果也是设置 BERRIES=true
    if (!state.get(BlockStateProperties::BERRIES())) {
        const BlockState& newState = state.with(BlockStateProperties::BERRIES(), true);
        world.setBlockState(pos, &newState, 3);
    }
}

ActionResultType CaveVinesPlantBlock::onBlockActivated(const BlockState& state,
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

} // namespace blocks
} // namespace mc
