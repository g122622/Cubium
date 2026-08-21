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

#include "common/item/items/special/FlintAndSteelItem.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/decorative/CampfireBlock.hpp"
#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <utility>

namespace mc {
namespace item {
namespace tool {

FlintAndSteelItem::FlintAndSteelItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType FlintAndSteelItem::onItemUse(ItemUseContext& context)
{
    Player* player = context.getPlayer();
    IWorld& world = context.getWorld();
    const BlockPos& blockPos = context.getBlockPos();
    Direction face = context.getFace();
    const BlockState* blockStatePtr = world.getBlockState(blockPos);

    if (blockStatePtr == nullptr) {
        return ActionResultType::Fail;
    }

    // 检查是否可以点燃方块（营火、蜡烛等）
    // 参考: CampfireBlock.canBeLit / CandleBlock.canLight / CandleCakeBlock.canLight
    if (blockStatePtr->hasProperty(BlockStateProperties::LIT())) {
        if (!blockStatePtr->get(BlockStateProperties::LIT())) {
            // 含水方块不可点燃（如含水蜡烛）
            if (blockStatePtr->hasProperty(BlockStateProperties::WATERLOGGED()) &&
                blockStatePtr->get(BlockStateProperties::WATERLOGGED())) {
                return ActionResultType::Fail;
            }

            // 点燃方块
            BlockState newState = blockStatePtr->with(BlockStateProperties::LIT(), true);
            world.setBlockState(blockPos, &newState, 11);

            // 消耗耐久：直接对玩家权威手持物（player->getHeldItem(hand)）做 hurtAndBreak，而非
            // context.getItemStackMut()（调用方局部拷贝，耐久损耗不回写权威物品栏——同桶类对齐缺陷）。
            // 外层 useItemOnBlock/handleItemUseOn 的 damage 对比会检测到权威槽 damage 变化跳过通用
            // shrink，避免把耐久损耗误当数量消耗（vanilla 打火石点火损耗 1 耐久，数量不变）。
            if (player != nullptr) {
                ItemStack& heldItem = player->getHeldItem(context.getHand());
                LivingEntity::hurtAndBreak(heldItem, 1, player, EquipmentSlot::MainHand);
            }
            return ActionResultType::Success;
        }
    }

    // 否则尝试在点击面的相邻位置放置火焰
    BlockPos firePos = blockPos.offset(face);

    // 检查是否可以放置火焰
    if (canLightBlock(world, firePos)) {
        // 获取应该放置的火焰方块（普通火或灵魂火）
        Block* fireBlock = getFireForPlacement(world, firePos);
        if (fireBlock != nullptr) {
            // 放置火焰
            const BlockState& fireState = fireBlock->getDefaultState();
            world.setBlockState(firePos, &fireState, 11);

            // 消耗耐久：同上方点燃分支，操作权威手持（player->getHeldItem(hand)）做 hurtAndBreak。
            if (player != nullptr) {
                ItemStack& heldItem = player->getHeldItem(context.getHand());
                LivingEntity::hurtAndBreak(heldItem, 1, player, EquipmentSlot::MainHand);
            }

            return ActionResultType::Success;
        }
    }

    return ActionResultType::Fail;
}

bool FlintAndSteelItem::canLightBlock(IWorld& world, const BlockPos& pos)
{
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr) {
        return false;
    }

    const BlockState& state = *statePtr;

    // 如果位置不是空气，不能点燃
    if (!state.isAir()) {
        return false;
    }

    // 获取应该放置的火焰方块，并检查其是否能在该位置有效存在
    Block* fireBlock = getFireForPlacement(world, pos);
    if (fireBlock == nullptr) {
        return false;
    }

    // 检查火焰方块是否能放置在该位置
    const BlockState& fireState = fireBlock->getDefaultState();
    IBlockReader& blockReader = static_cast<IBlockReader&>(world);
    return fireBlock->isValidPosition(fireState, blockReader, pos);
}

Block* FlintAndSteelItem::getFireForPlacement(IWorld& world, const BlockPos& pos)
{
    // 检查下方是否是灵魂沙/灵魂土，如果是则返回灵魂火
    const BlockState* belowStatePtr = world.getBlockState(pos.down());

    if (belowStatePtr != nullptr && BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*belowStatePtr)) {
        // 灵魂火基座方块上放置灵魂火
        return VanillaBlocks::SOUL_FIRE;
    }

    // 其他情况放置普通火
    return VanillaBlocks::FIRE;
}

} // namespace tool
} // namespace item
} // namespace mc
