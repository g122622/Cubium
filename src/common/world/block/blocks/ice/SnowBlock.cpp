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

#include "SnowBlock.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../Block.hpp"
#include "../../BlockTags.hpp"
#include "../../registry/VanillaBlocks.hpp"

namespace mc::blocks {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 雪融化的最小光照等级阈值
constexpr i32 MELT_LIGHT_LEVEL = 11;

} // namespace

// ============================================================================
// SnowBlock 实现
// ============================================================================

SnowBlock::SnowBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 注册 LAYERS 属性（1-8层）
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(LAYERS()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 默认状态：1层
    setDefaultState(getDefaultState().with(LAYERS(), 1));
}

void SnowBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    (void)random; // 雪融化不需要随机数

    // 如果方块光照 > 11，融化
    u8 blockLight = world.getBlockLight(pos);
    u8 skyLight = world.getSkyLight(pos);

    // 计算综合光照（不考虑天气衰减的简化版本）
    i32 lightLevel = static_cast<i32>(std::max(blockLight, skyLight));

    if (lightLevel > MELT_LIGHT_LEVEL) {
        // 融化：掉落雪球并移除方块
        // 雪层掉落雪球数量等于层数
        i32 layers = state.get(LAYERS());
        if (layers > 0) {
            ItemStack dropStack(*Items::SNOWBALL, layers);
            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world,
                dropStack,
                static_cast<f64>(pos.x) + 0.5,
                static_cast<f64>(pos.y) + 0.5,
                static_cast<f64>(pos.z) + 0.5,
                rng);
        }
        const BlockState* airState = &VanillaBlocks::AIR->defaultState();
        world.setBlockState(pos, airState);
    }
}

bool SnowBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 1. 检查下方方块
    const BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (!belowState) {
        return false;
    }

    // 2. 下方方块不能在 SNOW_LAYER_CANNOT_SURVIVE_ON 标签中（冰、浮冰、屏障）
    if (BlockTags::SNOW_LAYER_CANNOT_SURVIVE_ON().contains(*belowState)) {
        return false;
    }

    // 3. 下方方块在 SNOW_LAYER_CAN_SURVIVE_ON 标签中时允许放置（蜂蜜块、灵魂沙、泥巴）
    if (BlockTags::SNOW_LAYER_CAN_SURVIVE_ON().contains(*belowState)) {
        return true;
    }

    // 4. 下方为满层(8层)雪层时允许放置
    if (belowState->is(VanillaBlocks::SNOW) && belowState->get(LAYERS()) == 8) {
        return true;
    }

    // 5. 否则，下方方块的碰撞形状上表面必须完整（即下方方块有坚固的上表面）
    //    TODO: 当 doesSideFillSquare 实现完整的面投影检查后，
    //          应替换为碰撞形状上表面检查，以支持蜂蜜块/灵魂沙等非完整面方块的精确判断
    return belowState->isSolidSide(world, belowPos, Direction::Up);
}

BlockState SnowBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 当下方方块变化时，如果不再满足放置条件则变为空气
    if (facing == Direction::Down && !_canSurvive(world, currentPos)) {
        if (auto* airBlock = VanillaBlocks::AIR) {
            return airBlock->defaultState();
        }
        return Block::defaultState();
    }

    return Block::updatePostPlacement(state, facing, facingState, world, currentPos, facingPos);
}

bool SnowBlock::_canSurvive(IWorld& world, const BlockPos& pos) const
{
    // 与 isValidPosition 相同的逻辑，但使用 IWorld 接口
    // 供 updatePostPlacement 使用，避免 IWorld 到 IBlockReader 的向下转型
    const BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (!belowState) {
        return false;
    }

    if (BlockTags::SNOW_LAYER_CANNOT_SURVIVE_ON().contains(*belowState)) {
        return false;
    }

    if (BlockTags::SNOW_LAYER_CAN_SURVIVE_ON().contains(*belowState)) {
        return true;
    }

    if (belowState->is(VanillaBlocks::SNOW) && belowState->get(LAYERS()) == 8) {
        return true;
    }

    return belowState->isSolidSide(world, belowPos, Direction::Up);
}

bool SnowBlock::canSurviveAt(const IWorld& world, const BlockPos& pos)
{
    // 静态工具方法，供 Biome::shouldSnow 等场景使用
    // 逻辑与 isValidPosition 一致，但接受 const IWorld& 参数
    const BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (!belowState) {
        return false;
    }

    // 1. 下方方块不能在 SNOW_LAYER_CANNOT_SURVIVE_ON 标签中（冰、浮冰、屏障）
    if (BlockTags::SNOW_LAYER_CANNOT_SURVIVE_ON().contains(*belowState)) {
        return false;
    }

    // 2. 下方方块在 SNOW_LAYER_CAN_SURVIVE_ON 标签中时允许放置（蜂蜜块、灵魂沙、泥巴）
    if (BlockTags::SNOW_LAYER_CAN_SURVIVE_ON().contains(*belowState)) {
        return true;
    }

    // 3. 下方为满层(8层)雪层时允许放置
    if (belowState->is(VanillaBlocks::SNOW) && belowState->get(LAYERS()) == 8) {
        return true;
    }

    // 4. 否则，检查下方方块是否有坚固的上表面
    //    isSolidSide 语义上是只读操作，const_cast 安全
    return belowState->isSolidSide(const_cast<IWorld&>(world), belowPos, Direction::Up);
}

} // namespace mc::blocks
