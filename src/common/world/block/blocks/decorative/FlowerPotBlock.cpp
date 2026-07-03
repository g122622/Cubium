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

#include "FlowerPotBlock.hpp"

#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItem.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gameevent/GameEvents.hpp"

namespace mc {
namespace blocks {

// 静态映射表初始化
std::unordered_map<const Block*, const FlowerPotBlock*> FlowerPotBlock::s_pottedByContent;

FlowerPotBlock::FlowerPotBlock(const BlockProperties& properties, const Block* potted)
    : Block(properties)
    , m_potted(potted)
{
    // 花盆形状：底部圆形 + 顶部边缘
    // 使用单个盒子近似 MC 的 Block.column(6.0, 0.0, 6.0)
    // 坐标系为 0-16 像素，等价于 box(5, 0, 5, 11, 6, 11)
    m_shape = CollisionShape::box(5.0f / 16.0f, 0.0f, 5.0f / 16.0f, 11.0f / 16.0f, 6.0f / 16.0f, 11.0f / 16.0f);
    m_collisionShape =
        CollisionShape::box(5.0f / 16.0f, 0.0f, 5.0f / 16.0f, 11.0f / 16.0f, 6.0f / 16.0f, 11.0f / 16.0f);

    // 注册到反查映射表（空花盆不参与）
    if (m_potted != nullptr) {
        s_pottedByContent[m_potted] = this;
    }
}

const CollisionShape& FlowerPotBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& FlowerPotBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_collisionShape;
}

bool FlowerPotBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 花盆可以放置在任何完整方块上
    return true;
}

BlockState FlowerPotBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    // 如果下方方块被移除，则移除花盆
    if (facing == Direction::Down) {
        const BlockPos belowPos = currentPos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || belowState->isAir()) {
            return VanillaBlocks::AIR->defaultState();
        }
    }
    return state;
}

ActionResultType FlowerPotBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    const ItemStack& heldStack = player.getHeldItem(hand);

    // 分支1：玩家手持物品右键花盆
    if (!heldStack.isEmpty()) {
        const Item* heldItem = heldStack.getItem();

        // 仅当手持的是 BlockItem 时，才查询其关联方块是否可入盆
        const auto* blockItem = dynamic_cast<const BlockItem*>(heldItem);
        const Block* contentBlock = (blockItem != nullptr) ? &blockItem->block() : nullptr;
        const FlowerPotBlock* targetPot = (contentBlock != nullptr) ? getByContent(*contentBlock) : nullptr;

        if (targetPot == nullptr) {
            // 手持物品不是可盆栽植物，交给其他处理器（如空手交互）
            return ActionResultType::Pass;
        }

        // 已有内容物的花盆：消费物品但不执行动作（与 MC Java 一致）
        if (!isEmpty()) {
            return ActionResultType::Consume;
        }

        // 空花盆：放入植物，花盆变为对应 potted_* 方块
        if (world.isClientSide()) {
            return ActionResultType::Success;
        }

        // 服务端：替换方块为对应 potted_*，并消耗物品
        const BlockState& pottedState = targetPot->defaultState();
        world.setBlockState(pos, &pottedState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);

        // 消耗物品（玩家手持物品数量 -1）
        ItemStack& mutableHeld = player.getHeldItem(hand);
        mutableHeld.shrink(1);
        return ActionResultType::Success;
    }

    // 分支2：玩家空手右键花盆
    if (isEmpty()) {
        // 空花盆：无操作
        return ActionResultType::Consume;
    }

    // 已有内容物的花盆：取出内容物，花盆变回空花盆
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 服务端：生成内容物物品，优先放入玩家背包，放不下则丢弃
    {
        // 通过 BlockItemRegistry 反查内容物方块的对应物品
        const BlockItem* contentBlockItem = BlockItemRegistry::instance().getBlockItem(*m_potted);
        if (contentBlockItem != nullptr) {
            // BlockItem 继承自 Item，可直接构造 ItemStack
            const Item* contentItem = contentBlockItem;
            ItemStack dropStack(*contentItem, 1);
            // 优先放入背包，放不下则丢弃
            player.inventory().placeItemBackInInventory(
                dropStack, [&player](const ItemStack& remaining, bool /*keepOwnership*/) {
                    ItemStack copy = remaining;
                    player.dropItem(copy, false, false);
                });
        }
    }

    // 花盆变回空花盆
    if (VanillaBlocks::AIR != nullptr) {
        // 查找空花盆方块（minecraft:flower_pot）
        Block* emptyPot = BlockRegistry::instance().getBlock(ResourceLocation("minecraft:flower_pot"));
        if (emptyPot != nullptr) {
            const BlockState& emptyState = emptyPot->defaultState();
            world.setBlockState(pos, &emptyState, 3);
            world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &state);
        }
    }
    return ActionResultType::Success;
}

const FlowerPotBlock* FlowerPotBlock::getByContent(const Block& content)
{
    auto it = s_pottedByContent.find(&content);
    return (it != s_pottedByContent.end()) ? it->second : nullptr;
}

} // namespace blocks
} // namespace mc
