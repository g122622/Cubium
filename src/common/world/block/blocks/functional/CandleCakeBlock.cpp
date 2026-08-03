/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
 * LIABILITY, ARISING FROM, AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CandleCakeBlock.hpp"

#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../Block.hpp"
#include "../../BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/blocks/decorative/AbstractCandleBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

CandleCakeBlock::CandleCakeBlock(BlockProperties properties, Block* candleBlock)
    : AbstractCandleBlock(std::move(properties))
    , m_candleBlock(candleBlock)
{
    // 创建状态容器：仅 LIT（蜡烛蛋糕没有 CANDLES 和 WATERLOGGED）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LIT())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：未点燃
    setDefaultState(defaultState().with(BlockStateProperties::LIT(), false));

    // 蜡烛蛋糕形状：蛋糕主体 + 蜡烛柱体
    // 参考 MC Java: Shapes.or(Block.column(2.0, 8.0, 14.0), Block.column(14.0, 0.0, 8.0))
    // 即：蜡烛部分 y=8-14, 半径1px(2/16)；蛋糕部分 y=0-8, 半径7px(14/16)
    // 使用 fromPixelBox，坐标系为像素 (0-16)
    m_shape = CollisionShape::fromPixelBox(1.0f, 0.0f, 1.0f, 15.0f, 8.0f, 15.0f);
}

BlockState CandleCakeBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    // 蜡烛蛋糕默认未点燃
    return defaultState();
}

bool CandleCakeBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 下方方块必须是固体
    BlockPos belowPos = pos.down();
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }
    return belowState->isSolid();
}

BlockState CandleCakeBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

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

const CollisionShape& CandleCakeBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

u8 CandleCakeBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 点燃时亮度为3
    if (isLit(state)) {
        return 3;
    }
    return 0;
}

std::vector<Vector3f> CandleCakeBlock::getParticleOffsets(const BlockState& state) const
{
    MC_UNUSED(state);
    // 蜡烛蛋糕只有一根蜡烛，偏移位置固定在中心偏上
    return {{0.5f, 0.5f, 0.5f}};
}

i32 CandleCakeBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 蜡烛蛋糕始终输出满信号（类似完整蛋糕）
    return 14;
}

// ========== 交互 ==========

BlockActionResult CandleCakeBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    const ItemStack& heldItem = player.getHeldItem(hand);

    // 如果手持打火石或火焰弹，返回 Pass 让物品自身处理点燃逻辑
    // FlintAndSteelItem 和 FireChargeItem 均支持含 LIT 属性方块的点燃

    // 空手点击蜡烛部分（上半部 y > 0.5）且已点燃 → 熄灭
    if (heldItem.isEmpty() && isLit(state)) {
        f32 hitY = hit.hitPosition().y - static_cast<f32>(pos.y);
        if (hitY > 0.5f) {
            if (world.isClientSide()) {
                return ActionResultType::Success;
            }
            BlockState mutableState = state;
            extinguish(world, pos, mutableState, &player);
            return ActionResultType::Success;
        }
    }

    // 其他情况：吃蛋糕
    // 参考 MC Java: CandleCakeBlock.useWithoutItem 调用 CakeBlock.eat 后掉落蜡烛
    // 如果玩家无法进食则返回 Pass
    if (!player.canEat(false)) {
        return ActionResultType::Pass;
    }

    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 玩家进食蛋糕（+2 饥饿值, 0.1 饱和度）
    player.foodStats().addStats(2, 0.1f);

    // 将蜡烛蛋糕替换为一片已被咬的普通蛋糕
    // 参考 MC Java: CakeBlock.eat 使用 BITES=0 的蛋糕状态，第一口后变为 BITES=1
    Block* cakeBlock = VanillaBlocks::CAKE;
    if (cakeBlock != nullptr) {
        const BlockState& cakeState = cakeBlock->getDefaultState();
        if (cakeState.hasProperty(BlockStateProperties::BITES_0_6())) {
            BlockState bittenState = cakeState.with(BlockStateProperties::BITES_0_6(), 1);
            world.setBlockState(pos, &bittenState, 3);
        } else {
            world.setBlockState(pos, &cakeState, 3);
        }
    } else {
        // 如果蛋糕方块不可用，直接移除方块
        if (auto* airState = BlockRegistry::instance().airState()) {
            world.setBlockState(pos, airState, 3);
        }
    }

    // 掉落蜡烛物品
    Block::dropResources(world, pos, state);

    return ActionResultType::Success;
}

// ========== 静态工具方法 ==========

bool CandleCakeBlock::canLight(const BlockState& state)
{
    // 蜡烛蛋糕没有 WATERLOGGED 属性，只需检查 LIT
    return state.hasProperty(BlockStateProperties::LIT()) && !state.get(BlockStateProperties::LIT());
}

} // namespace blocks
} // namespace mc
