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

#include "CaveVinesBlock.hpp"
#include "CaveVinesPlantBlock.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

CaveVinesBlock::CaveVinesBlock(const BlockProperties& properties)
    : GrowingPlantHeadBlock(properties,
          Direction::Down,
          CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15),
          0.1f // MC 1.21.11: growPerTickProbability = 0.1 (10%)
          )
    , IGrowable()
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

const Block* CaveVinesBlock::getHeadBlock() const
{
    return VanillaBlocks::CAVE_VINES;
}

const Block* CaveVinesBlock::getBodyBlock() const
{
    return VanillaBlocks::CAVE_VINES_PLANT;
}

BlockState CaveVinesBlock::getGrowIntoState(
    IWorld& world, const BlockPos& pos, BlockState& currentState, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 新生长的头部：年龄递增 + 11%概率有浆果
    const i32 age = getAge(currentState);
    const bool hasBerries = random.nextFloat() < CHANCE_OF_BERRIES_ON_GROWTH;
    return withAge(age + 1).with(BlockStateProperties::BERRIES(), hasBerries);
}

BlockState CaveVinesBlock::updateBodyAfterConvertedFromHead(const BlockState& headState) const
{
    // 当头部变成身体时，传递 BERRIES 状态
    const Block* bodyBlock = getBodyBlock();
    if (bodyBlock) {
        return bodyBlock->defaultState().with(
            BlockStateProperties::BERRIES(), headState.get(BlockStateProperties::BERRIES()));
    }
    return headState;
}

bool CaveVinesBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(isClientSide);
    // MC 1.21.11: CaveVines canGrow 检查 !hasBerries，而不是年龄
    // 骨粉的效果是让藤蔓长出浆果
    return !state.get(BlockStateProperties::BERRIES());
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
    // MC 1.21.11: 骨粉对洞穴藤蔓的效果是设置 BERRIES=true
    if (!state.get(BlockStateProperties::BERRIES())) {
        const BlockState& newState = state.with(BlockStateProperties::BERRIES(), true);
        world.setBlockState(pos, &newState, 3);
    }
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

} // namespace blocks
} // namespace mc
