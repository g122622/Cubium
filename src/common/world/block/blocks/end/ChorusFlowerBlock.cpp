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
* 
*/

#include "ChorusFlowerBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../../IWorld.hpp"

namespace mc {
namespace blocks {

ChorusFlowerBlock::ChorusFlowerBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_5())
            .create([this](const Block& block, std::vector<size_t> values, const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts, const std::vector<BlockState*>* allStates, u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::AGE_0_5(), 0));

    for (int i = 0; i < 6; ++i) {
        m_shapesByAge[i] = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    }
}

i32 ChorusFlowerBlock::getAge(const BlockState& state) const
{
    return state.get(BlockStateProperties::AGE_0_5());
}

BlockState ChorusFlowerBlock::withAge(i32 age) const
{
    return defaultState().with(BlockStateProperties::AGE_0_5(), std::min(age, 5));
}

BlockState ChorusFlowerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState();
}

bool ChorusFlowerBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr || belowState->isAir()) {
        bool foundChorusPlant = false;

        for (Direction dir : {Direction::North, Direction::South, Direction::East, Direction::West}) {
            BlockPos adjPos = pos.offset(dir);
            const BlockState* adjState = world.getBlockState(adjPos);

            if (adjState == nullptr) {
                continue;
            }

            if (adjState->is(VanillaBlocks::CHORUS_PLANT)) {
                if (foundChorusPlant) {
                    return false;
                }
                foundChorusPlant = true;
            } else if (!adjState->isAir()) {
                return false;
            }
        }

        return foundChorusPlant;
    }

    const Block& belowBlock = belowState->getBlock();

    if (belowState->is(VanillaBlocks::CHORUS_PLANT)) {
        return true;
    }

    if (belowState->is(VanillaBlocks::END_STONE)) {
        return true;
    }

    if (&belowBlock == VanillaBlocks::CHORUS_FLOWER) {
        return true;
    }

    return false;
}

void ChorusFlowerBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    i32 age = getAge(state);

    if (age < getMaxAge()) {
        if (random.nextInt(5) == 0) {
            BlockState newState = withAge(age + 1);
            world.setBlockState(pos, &newState, 2);
        }
    }
}

const CollisionShape& ChorusFlowerBlock::getShape(const BlockState& state) const
{
    i32 age = getAge(state);
    return m_shapesByAge[std::min(age, 5)];
}

} // namespace blocks
} // namespace mc
