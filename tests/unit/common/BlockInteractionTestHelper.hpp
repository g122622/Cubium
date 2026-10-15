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

#pragma once

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc::test {

/**
 * @brief 在单元测试中派发一次方块右键交互（对齐服务端 useItemOn 派发链）
 *
 * 服务端 `BlockInteractionManager::handleBlockUse` 的派发顺序为：
 * `Block::useItemOn` →（返回 `TryEmptyHandInteraction` **且为交互主手** 时）`Block::useWithoutItem`。
 * 单元测试若直接调 `useWithoutItem`，会绕过 `useItemOn` 分支而测不到真实语义。
 *
 * 本工具按同一顺序派发，并从玩家实体的权威手持槽取 stack（与生产路径一致），
 * 使测试断言与实际游戏行为对齐。
 *
 * @param block 目标方块（非 const，useItemOn/useWithoutItem 均非 const）
 * @param state 方块状态
 * @param world 世界
 * @param pos 方块位置
 * @param player 玩家实体（其物品栏提供权威手持槽）
 * @param hand 使用的手
 * @param hit 射线检测结果
 * @return 方块侧交互结果
 */
inline BlockActionResult useBlock(Block& block,
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    ItemStack& stack =
        (hand == Hand::OffHand) ? player.inventory().getOffhandItemRef() : player.inventory().getSelectedStackRef();
    BlockActionResult result = block.useItemOn(stack, state, world, pos, player, hand, hit);
    // 回退条件与生产路径一致：仅主线才转 useWithoutItem（对齐 vanilla
    // `interactionhand == InteractionHand.MAIN_HAND` 门控）。
    if (hand == Hand::MainHand && !result.consumesAction() && result.isTryEmptyHandInteraction()) {
        result = block.useWithoutItem(state, world, pos, player, hit);
    }
    return result;
}

} // namespace mc::test
