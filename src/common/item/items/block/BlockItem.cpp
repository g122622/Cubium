#include "BlockItem.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/block/Material.hpp"
#include "../../../world/IWorld.hpp"

namespace mc {

BlockItem::BlockItem(const Block& block, ItemProperties properties)
    : Item(properties)
    , m_block(&block)
{
}

ActionResultType BlockItem::onItemUse(ItemUseContext& context) {
    // 参考 MC 1.16.5: BlockItem.onItemUse
    // 创建 BlockItemUseContext 并尝试放置
    BlockItemUseContext blockContext(
        context.getWorld(),
        context.getPlayer(),
        context.getItemStack(),
        context.getHitPos(),
        context.getBlockPos(),
        context.getFace(),
        context.getPlayerYaw()
    );

    ActionResultType result = tryPlace(blockContext);

    // 如果放置失败且物品是食物，尝试食用
    if (result != ActionResultType::Success && result != ActionResultType::Consume && isFood()) {
        ItemActionResult foodResult = onItemRightClick(context.getWorld(), *context.getPlayer(), context.getHand());
        return foodResult.getType();
    }

    return result;
}

ActionResultType BlockItem::tryPlace(BlockItemUseContext& context) const {
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
        onBlockPlaced(pos, world, player, stack, *actualState);

        // 调用方块的 onBlockPlacedBy
        // 注意：需要使用 const_cast 因为 onBlockPlacedBy 是非 const 方法
        const_cast<Block&>(*m_block).onBlockPlacedBy(world, pos, *actualState);

        // TODO: 触发进度触发器
        // if (player != nullptr) {
        //     CriteriaTriggers.PLACED_BLOCK.trigger(player, pos, stack);
        // }
    }

    // 播放放置音效
    // TODO: 播放音效
    // const BlockSoundType& soundType = m_block->getSoundType();
    // world.playSound(player, pos, soundType.getPlaceSound(), SoundCategory::Blocks,
    //                 (soundType.getVolume() + 1.0f) / 2.0f, soundType.getPitch() * 0.8f);

    // 非创造模式消耗物品
    if (player == nullptr || !player->isCreative()) {
        stack.shrink(1);
    }

    // 返回成功（客户端返回 Success，服务端返回 Consume）
    // 参考 MC 1.16.5: ActionResultType.func_233537_a_(world.isClientSide)
    return ActionResultType::Success;
}

BlockItemUseContext BlockItem::getBlockItemUseContext(BlockItemUseContext& context) const {
    // 默认返回原始上下文
    return context;
}

const BlockState* BlockItem::getStateForPlacement(const BlockItemUseContext& /* context */) const {
    // 默认实现返回方块的默认状态
    // 子类可以重写以支持有方向的方块（如楼梯、门等）
    return &m_block->defaultState();
}

bool BlockItem::canPlace(const BlockItemUseContext& context, const BlockState& state) const {
    // 参考 MC 1.16.5: BlockItem.canPlace
    Player* player = context.getPlayer();

    // TODO: 实体碰撞检查
    // ISelectionContext selectionContext = player == null ? ISelectionContext.dummy() : ISelectionContext.forEntity(player);

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

    // TODO: 实体碰撞检查
    // 参考 MC 1.16.5: world.func_226663_a_(state, pos, ISelectionContext.dummy())

    // 获取当前方块
    const BlockState* currentState = context.getBlockStateAtPlacementPos();
    if (currentState != nullptr && !currentState->isAir()) {
        // 检查材质是否可替换
        const Material& material = currentState->owner().material();
        if (!material.isReplaceable() && !material.isLiquid()) {
            return false;
        }
    }

    return true;
}

bool BlockItem::checkPositionValid(const BlockItemUseContext& context) const {
    const BlockPos& pos = context.placementPos();

    // 检查世界边界
    if (pos.y < world::MIN_BUILD_HEIGHT || pos.y >= world::MAX_BUILD_HEIGHT) {
        return false;
    }

    return true;
}

bool BlockItem::placeBlock(BlockItemUseContext& context, const BlockState* state) const {
    if (state == nullptr) {
        return false;
    }

    // 参考 MC 1.16.5: BlockItem.placeBlock
    // 在世界中设置方块状态
    // 参数 11 = 1 (通知邻居) | 2 (通知观察者) | 8 (同步到客户端)
    return context.getWorld().setBlockState(context.placementPos(), state, 11);
}

bool BlockItem::onBlockPlaced(const BlockPos& pos, IWorld& world,
                               Player* player, const ItemStack& stack,
                               const BlockState& state) const {
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

const BlockState* BlockItem::applyBlockStateFromNBT(const BlockPos& pos, IWorld& world,
                                                     const ItemStack& stack,
                                                     const BlockState& state) const {
    // 参考 MC 1.16.5: BlockItem.func_219985_a
    // 从物品堆的 BlockStateTag NBT 数据应用方块状态
    (void)pos;
    (void)world;
    (void)stack;

    // TODO: 实现 NBT 方块状态应用
    // CompoundNBT tag = stack.getChildTag("BlockStateTag");
    // if (tag != null) {
    //     StateContainer<Block, BlockState> stateContainer = state.getBlock().getStateContainer();
    //     for (String key : tag.keySet()) {
    //         Property<?> property = stateContainer.getProperty(key);
    //         if (property != null) {
    //             String value = tag.get(key).getString();
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
