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

#include "CandleBlock.hpp"

#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/decorative/AbstractCandleBlock.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

CandleBlock::CandleBlock(BlockProperties properties)
    : AbstractCandleBlock(std::move(properties))
{
    // 创建状态容器：CANDLES + LIT + WATERLOGGED
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::CANDLES())
            .add(BlockStateProperties::LIT())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：1根蜡烛、未点燃、未含水
    setDefaultState(defaultState()
            .with(BlockStateProperties::CANDLES(), 1)
            .with(BlockStateProperties::LIT(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 初始化碰撞形状（像素单位，使用 fromPixelBox 转换）
    // 1根蜡烛：2px宽柱体，6px高
    m_shapesByCount[0] = CollisionShape::fromPixelBox(7.0f, 0.0f, 7.0f, 9.0f, 6.0f, 9.0f);
    // 2根蜡烛
    m_shapesByCount[1] = CollisionShape::fromPixelBox(5.0f, 0.0f, 6.0f, 11.0f, 6.0f, 9.0f);
    // 3根蜡烛
    m_shapesByCount[2] = CollisionShape::fromPixelBox(5.0f, 0.0f, 6.0f, 10.0f, 6.0f, 11.0f);
    // 4根蜡烛
    m_shapesByCount[3] = CollisionShape::fromPixelBox(5.0f, 0.0f, 5.0f, 11.0f, 6.0f, 10.0f);
}

// ========== 放置逻辑 ==========

BlockState CandleBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查目标位置是否已有同类型蜡烛（堆叠逻辑）
    const BlockState* existingState = world.getBlockState(pos);
    if (existingState != nullptr && existingState->is(this)) {
        i32 currentCount = existingState->get(BlockStateProperties::CANDLES());
        if (currentCount < 4) {
            // 增加蜡烛数量
            return existingState->with(BlockStateProperties::CANDLES(), currentCount + 1);
        }
        // 已满4根，无法继续堆叠
        return *existingState;
    }

    // 新放置：1根蜡烛，检测含水状态
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);
    return defaultState()
        .with(BlockStateProperties::CANDLES(), 1)
        .with(BlockStateProperties::LIT(), false)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool CandleBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 与 MC 1.21.11 CandleBlock.canSurvive 一致：
    //   Block.canSupportCenter(world, pos.below(), Direction.UP)
    return Block::canSupportCenter(world, pos.down(), Direction::Up);
}

BlockState CandleBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 含水时调度水流tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 下方支撑变化时检查有效性
    if (facing == Direction::Down) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

bool CandleBlock::isReplaceable(const BlockState& state, const BlockItemUseContext& context) const
{
    // 潜行时不堆叠，回退到基类行为
    Player* player = context.getPlayer();
    if (player != nullptr && player->isSneaking()) {
        return AbstractCandleBlock::isReplaceable(state, context);
    }

    // 手持物品必须是此方块对应的物品
    const ItemStack& heldItem = context.getItemStack();
    if (heldItem.isEmpty()) {
        return AbstractCandleBlock::isReplaceable(state, context);
    }

    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*this);
    if (blockItem == nullptr || heldItem.getItem() != blockItem) {
        return AbstractCandleBlock::isReplaceable(state, context);
    }

    // 数量 < 4 时允许堆叠替换
    return state.get(BlockStateProperties::CANDLES()) < 4;
}

// ========== 形状 ==========

const CollisionShape& CandleBlock::getShape(const BlockState& state) const
{
    i32 count = state.get(BlockStateProperties::CANDLES());
    MC_ASSERT_DEBUG(count >= 1 && count <= 4);
    return m_shapesByCount[count - 1];
}

// ========== 光照 ==========

u8 CandleBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    if (isLit(state)) {
        i32 count = state.get(BlockStateProperties::CANDLES());
        return static_cast<u8>(3 * count);
    }
    return 0;
}

// ========== 点燃 ==========

bool CandleBlock::canBeLit(const BlockState& state) const
{
    return !state.get(BlockStateProperties::LIT()) && !state.get(BlockStateProperties::WATERLOGGED());
}

// ========== 粒子 ==========

std::vector<Vector3f> CandleBlock::getParticleOffsets(const BlockState& state) const
{
    i32 count = state.get(BlockStateProperties::CANDLES());

    switch (count) {
        case 1:
            return {{0.5f, 0.5f, 0.5f}};
        case 2:
            return {{0.375f, 0.4375f, 0.5f}, {0.625f, 0.5f, 0.4375f}};
        case 3:
            return {{0.5f, 0.3125f, 0.625f}, {0.375f, 0.4375f, 0.5f}, {0.5625f, 0.5f, 0.4375f}};
        case 4:
            return {
                {0.4375f, 0.3125f, 0.5625f},
                {0.625f, 0.4375f, 0.5625f},
                {0.375f, 0.4375f, 0.375f},
                {0.5625f, 0.5f, 0.375f},
            };
        default:
            // 防御性默认值
            return {{0.5f, 0.5f, 0.5f}};
    }
}

// ========== 交互 ==========

BlockActionResult CandleBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    const ItemStack& heldItem = player.getHeldItem(hand);

    // 空手 + 可以建造 + 蜡烛已点燃 → 熄灭
    if (heldItem.isEmpty() && player.mayBuild() && isLit(state)) {
        if (world.isClientSide()) {
            return ActionResultType::Success;
        }
        BlockState mutableState = state;
        extinguish(world, pos, mutableState, &player);
        return ActionResultType::Success;
    }

    // 其他情况（如打火石/火焰弹点燃）由物品自身的 onItemUse 处理
    // FlintAndSteelItem 和 FireChargeItem 均支持含 LIT 属性方块的点燃，并会检查 WATERLOGGED 属性
    return ActionResultType::Pass;
}

// ========== Tick ==========

void CandleBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 含水时自动熄灭（通过 ticksRandomly() 注册随机刻，由随机刻系统触发此 tick）
    if (state.get(BlockStateProperties::WATERLOGGED()) && isLit(state)) {
        extinguish(world, pos, state, nullptr);
    }
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CandleBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ========== 静态工具方法 ==========

bool CandleBlock::canLight(const BlockState& state)
{
    return state.hasProperty(BlockStateProperties::LIT()) && state.hasProperty(BlockStateProperties::WATERLOGGED()) &&
        !state.get(BlockStateProperties::LIT()) && !state.get(BlockStateProperties::WATERLOGGED());
}

} // namespace blocks
} // namespace mc
