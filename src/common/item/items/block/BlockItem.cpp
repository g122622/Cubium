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

#include "BlockItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include "../../../sound/SoundCategory.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockSoundType.hpp"
#include "../../../world/block/Material.hpp"

namespace mc {

BlockItem::BlockItem(const Block& block, ItemProperties properties)
    : Item(properties)
    , m_block(&block)
{}

ActionResultType BlockItem::onItemUse(ItemUseContext& context)
{
    // 参考 MC 1.16.5: BlockItem.onItemUse
    // 创建 BlockItemUseContext 并尝试放置
    BlockItemUseContext blockContext(context.getWorld(),
        context.getPlayer(),
        context.getItemStack(),
        context.getHitPos(),
        context.getBlockPos(),
        context.getFace(),
        context.getPlayerYaw());

    ActionResultType result = tryPlace(blockContext);

    // 如果放置失败且物品是食物，尝试食用
    if (result != ActionResultType::Success && result != ActionResultType::Consume && isFood()) {
        ItemActionResult foodResult = onItemRightClick(context.getWorld(), *context.getPlayer(), context.getHand());
        return foodResult.getType();
    }

    return result;
}

ActionResultType BlockItem::tryPlace(BlockItemUseContext& context) const
{
    // 参考 MC 1.16.5: BlockItem.tryPlace
    if (!context.canPlace()) {
        return ActionResultType::Fail;
    }

    // 获取放置上下文（子类可重写）
    BlockItemUseContext blockContext = getBlockItemUseContext(context);

    // 获取放置状态
    const BlockState* state = getStateForPlacement(blockContext);
    if (state == nullptr) {
        return ActionResultType::Fail;
    }

    // 检查状态是否可以放置
    if (!canPlace(blockContext, *state)) {
        return ActionResultType::Fail;
    }

    // 执行放置
    if (!placeBlock(blockContext, state)) {
        return ActionResultType::Fail;
    }

    // 放置成功后的处理
    const BlockPos& pos = blockContext.placementPos();
    IWorld& world = blockContext.getWorld();
    Player* player = blockContext.getPlayer();
    ItemStack& stack = blockContext.getItemStack();

    // 获取实际放置的方块状态
    const BlockState* actualState = world.getBlockState(pos);
    if (actualState == nullptr) {
        return ActionResultType::Fail;
    }

    // 检查放置的方块是否正确
    if (&actualState->owner() == m_block) {
        // 从 NBT 应用方块状态
        actualState = applyBlockStateFromNBT(pos, world, stack, *actualState);

        // 处理方块实体 NBT
        static_cast<void>(onBlockPlaced(pos, world, player, stack, *actualState));

        // 调用方块的 onBlockPlacedBy
        // 注意：需要使用 const_cast 因为 onBlockPlacedBy 是非 const 方法
        const_cast<Block&>(*m_block).onBlockPlacedBy(world, pos, *actualState);

        // 触发进度触发器
        // 参考 MC 1.16.5: BlockItem.onItemUse() 中的 CriteriaTriggers.PLACED_BLOCK.trigger()
        if (player != nullptr) {
            static_cast<void>(world.onBlockPlaced(static_cast<PlayerId>(player->id()), pos, actualState, &stack));
        }
    }

    // 播放放置音效
    // 参考 MC 1.16.5: world.playSound(player, pos, soundType.getSound(SoundType.PLACE), SoundCategory.BLOCKS, (volume
    // + 1.0F) / 2.0F, pitch * 0.8F)
    const BlockSoundType& soundType = m_block->getSoundType();
    world.playSound(soundType.getPlaceSound(),
        sound::SoundCategory::Blocks,
        pos.center(),
        (soundType.getVolume() + 1.0f) / 2.0f,
        soundType.getPitch() * 0.8f);

    // 非创造模式消耗物品
    if (player == nullptr || !player->isCreative()) {
        stack.shrink(1);
    }

    // 返回成功（客户端返回 Success，服务端返回 Consume）
    // 参考 MC 1.16.5: ActionResultType.func_233537_a_(world.isClientSide)
    return ActionResultType::Success;
}

BlockItemUseContext BlockItem::getBlockItemUseContext(BlockItemUseContext& context) const
{
    // 默认返回原始上下文
    return context;
}

const BlockState* BlockItem::getStateForPlacement(const BlockItemUseContext& /* context */) const
{
    // 默认实现返回方块的默认状态
    // 子类可以重写以支持有方向的方块（如楼梯、门等）
    return &m_block->defaultState();
}

bool BlockItem::canPlace(const BlockItemUseContext& context, const BlockState& state) const
{
    // 参考 MC 1.16.5: BlockItem.canPlace
    Player* player = context.getPlayer();

    // 检查方块位置有效性
    if (checkPosition()) {
        // 检查位置是否有效
        if (!checkPositionValid(context)) {
            return false;
        }

        const BlockPos& pos = context.placementPos();

        // 检查放置位置是否在世界边界内
        if (!context.getWorld().isWithinWorldBounds(pos)) {
            return false;
        }

        // 调用方块的 isValidPosition 检查放置条件
        IBlockReader& blockReader = const_cast<IBlockReader&>(static_cast<const IBlockReader&>(context.getWorld()));
        if (!m_block->isValidPosition(state, blockReader, pos)) {
            return false;
        }
    }

    // 获取当前方块
    const BlockState* currentState = context.getBlockStateAtPlacementPos();
    if (currentState != nullptr && !currentState->isAir()) {
        // 检查材质是否可替换
        const Material& material = currentState->owner().material();
        if (!material.isReplaceable() && !material.isLiquid()) {
            return false;
        }
    }

    // 实体碰撞检查
    // 参考 MC 1.16.5: world.func_226663_a_(state, pos, ISelectionContext.dummy())
    // 检查要放置的方块是否会与实体发生碰撞
    const BlockPos& pos = context.placementPos();
    const CollisionShape& collisionShape = state.getCollisionShape();

    // 只有当方块有碰撞箱时才检查实体碰撞
    if (!collisionShape.isEmpty()) {
        // 获取方块在世界坐标下的碰撞箱
        auto worldBoxes = collisionShape.getWorldBoxes(pos.x, pos.y, pos.z);

        // 检查每个碰撞箱是否与实体碰撞
        IWorld& world = const_cast<IWorld&>(context.getWorld());
        for (const auto& box : worldBoxes) {
            // 检查是否与实体碰撞（排除放置者）
            if (world.hasEntityCollision(box, player)) {
                return false;
            }
        }
    }

    return true;
}

bool BlockItem::checkPositionValid(const BlockItemUseContext& context) const
{
    const BlockPos& pos = context.placementPos();

    // 检查世界边界
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    return true;
}

bool BlockItem::placeBlock(BlockItemUseContext& context, const BlockState* state) const
{
    if (state == nullptr) {
        return false;
    }

    // 参考 MC 1.16.5: BlockItem.placeBlock
    // 在世界中设置方块状态
    // 参数 11 = 1 (通知邻居) | 2 (通知观察者) | 8 (同步到客户端)
    return context.getWorld().setBlockState(context.placementPos(), state, 11);
}

bool BlockItem::onBlockPlaced(
    const BlockPos& pos, IWorld& world, Player* player, const ItemStack& stack, const BlockState& state) const
{
    // 参考 MC 1.16.5: BlockItem.onBlockPlaced
    // 默认处理方块实体 NBT 数据
    (void)pos;
    (void)world;
    (void)player;
    (void)stack;
    (void)state;

    // TODO: 实现方块实体 NBT 数据设置
    // return setTileEntityNBT(world, player, pos, stack);
    return false;
}

const BlockState* BlockItem::applyBlockStateFromNBT(
    const BlockPos& pos, IWorld& world, const ItemStack& stack, const BlockState& state) const
{
    // 参考 MC 1.16.5: BlockItem.func_219985_a
    // 从物品堆的 BlockStateTag NBT 数据应用方块状态
    (void)pos;
    (void)world;
    (void)stack;

    // TODO: 实现 NBT 方块状态应用
    // CompoundNBT tag = stack.getChildTag("BlockStateTag");
    // if (tag != null) {
    //     StateContainer<Block, BlockState> stateContainer = state.getBlock().getStateContainer();
    //     for (std::string key : tag.keySet()) {
    //         Property<?> property = stateContainer.getProperty(key);
    //         if (property != null) {
    //             std::string value = tag.get(key).getString();
    //             state = applyProperty(state, property, value);
    //         }
    //     }
    //     if (state != originalState) {
    //         world.setBlockState(pos, state, 2);
    //     }
    // }

    return &state;
}

} // namespace mc
