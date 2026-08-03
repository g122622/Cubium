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
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc::blocks {

// ============================================================================
// 常量
// ============================================================================

namespace {

/// 雪融化的最小光照等级阈值
constexpr i32 MELT_LIGHT_LEVEL = 11;

} // namespace

// ============================================================================
// 静态形状数组
// ============================================================================

/// 按 LAYERS 索引的形状数组（索引 0-8）。
/// SHAPES[i] = box(0,0,0, 16, i*2, 16)（像素坐标），即高度 = i*2 像素。
/// - SHAPES[0] 为空形状：layers=1 的碰撞形状（无碰撞，可踩过）
/// - SHAPES[1..7]：layers 1-7 的渲染形状（2/4/.../14 像素高）
/// - SHAPES[8] 为完整方块：layers=8 的渲染形状
const std::array<CollisionShape, 9> SnowBlock::SHAPES = {
    CollisionShape::empty(),                                             // [0] 碰撞用（layers=1 时无碰撞）
    CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 2.0f, 16.0f),  // [1] layers=1 渲染 2px
    CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 4.0f, 16.0f),  // [2] layers=2 渲染 4px
    CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 6.0f, 16.0f),  // [3] layers=3 渲染 6px
    CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 8.0f, 16.0f),  // [4] layers=4 渲染 8px（半方块）
    CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 10.0f, 16.0f), // [5] layers=5 渲染 10px
    CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 12.0f, 16.0f), // [6] layers=6 渲染 12px
    CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 14.0f, 16.0f), // [7] layers=7 渲染 14px / layers=8 碰撞 14px
    CollisionShape::fullBlock()                                          // [8] layers=8 渲染完整方块
};

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
    MC_UNUSED(random);

    // MC 原版: SnowLayerBlock.randomTick 仅检查方块光照 (LightLayer.BLOCK)
    // 条件: getBrightness(LightLayer.BLOCK, pos) > 11
    // 即方块光照 > 11 时融化，不考虑天空光照
    // 参考: net.minecraft.world.level.block.SnowLayerBlock.randomTick
    u8 blockLight = world.getBlockLight(pos);

    if (blockLight > MELT_LIGHT_LEVEL) {
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

    // 5. 检查下方方块的碰撞形状上面是否完全覆盖
    //    参考: net.minecraft.block.SnowLayerBlock#canSurvive
    //    MC 使用 Block.isFaceFull(collisionShape, Direction.UP)
    return Block::isFaceFull(belowState->getCollisionShape(), Direction::Up);
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

    // 检查下方方块的碰撞形状上面是否完全覆盖
    return Block::isFaceFull(belowState->getCollisionShape(), Direction::Up);
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

    // 4. 检查下方方块的碰撞形状上面是否完全覆盖
    return Block::isFaceFull(belowState->getCollisionShape(), Direction::Up);
}

// ============================================================================
// 形状
// ============================================================================

const CollisionShape& SnowBlock::getShape(const BlockState& state) const
{
    // 渲染形状按 LAYERS 索引：layers=1→SHAPES[1](2px)，layers=8→SHAPES[8](完整方块)
    return SHAPES[static_cast<size_t>(state.get(LAYERS()))];
}

const CollisionShape& SnowBlock::getCollisionShape(const BlockState& state) const
{
    // 碰撞形状比渲染形状矮 1 层：layers=1→SHAPES[0](空，无碰撞)，layers=8→SHAPES[7](14px)
    return SHAPES[static_cast<size_t>(state.get(LAYERS())) - 1];
}

const CollisionShape& SnowBlock::getBlockSupportShape(const BlockState& state) const
{
    // 支撑形状与渲染形状一致
    return SHAPES[static_cast<size_t>(state.get(LAYERS()))];
}

} // namespace mc::blocks
