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

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/BlockComponentRegistry.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockSoundType.hpp"
#include "common/world/block/blocks/ShulkerBoxBlock.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include <optional>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

BlockItem::BlockItem(const Block& block, ItemProperties properties)
    : Item(properties)
    , m_block(&block)
{}

ActionResultType BlockItem::onItemUse(ItemUseContext& context)
{
    // 创建 BlockItemUseContext 并尝试放置
    // 透传 yaw 与 pitch，pitch 用于 getNearestLookingDirections 的方向排序
    BlockItemUseContext blockContext(context.getWorld(),
        context.getPlayer(),
        context.getItemStack(),
        context.getHitPos(),
        context.getBlockPos(),
        context.getFace(),
        context.getPlayerYaw(),
        context.getPlayerPitch());

    ActionResultType result = tryPlace(blockContext);

    // 如果放置失败且物品是食物，尝试食用
    if (result != ActionResultType::Success && result != ActionResultType::Consume && isFood()) {
        ItemActionResult foodResult = onItemRightClick(context.getWorld(), *context.getPlayer(), context.getHand());
        return foodResult.getType();
    }

    return result;
}

bool BlockItem::canFitInsideContainerItems() const
{
    // 潜影盒不能放入收纳袋（防止递归存储）
    // 对应 MC 1.21.11 的 BlockItem#canFitInsideContainerItems
    return !blocks::ShulkerBoxBlock::isShulkerBox(*m_block);
}

ActionResultType BlockItem::tryPlace(BlockItemUseContext& context) const
{
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

    // 派发自定义方块组件回调 - beforeOnPlayerPlace（可取消）
    auto& blockCompReg = mc::mod::bedrock::addon::BlockComponentRegistry::instance();
    std::string blockTypeId = m_block->blockLocation().toString();
    if (blockCompReg.hasPlayerPlaceBeforeCallback(blockTypeId)) {
        mc::mod::bedrock::addon::BlockComponentPlayerPlaceBeforeEvent event;
        event.blockTypeId = blockTypeId;
        event.blockX = blockContext.placementPos().x;
        event.blockY = blockContext.placementPos().y;
        event.blockZ = blockContext.placementPos().z;
        event.dimensionId = blockContext.getWorld().dimension();
        event.playerId = blockContext.getPlayer() ? static_cast<PlayerId>(blockContext.getPlayer()->id())
                                                  : std::optional<PlayerId>();
        event.permutationToPlaceTypeId = state->getBlock().blockLocation().toString();
        event.face = static_cast<i32>(blockContext.getFace());
        if (blockCompReg.dispatchPlayerPlaceBefore(blockTypeId, event)) {
            // 脚本取消了放置
            return ActionResultType::Fail;
        }
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
        // 注意：m_block 是 const Block* 成员指针（非 BlockState 来源），无法使用 getBlockMutable()，
        // 因此需要 const_cast 将其转为可变引用以调用非 const 方法 onBlockPlacedBy
        const_cast<Block&>(*m_block).onBlockPlacedBy(world, pos, *actualState, stack);

        // 派发自定义方块组件回调 - onPlace
        if (blockCompReg.hasPlaceCallback(blockTypeId)) {
            mc::mod::bedrock::addon::BlockComponentOnPlaceEvent placeEvent;
            placeEvent.blockTypeId = blockTypeId;
            placeEvent.blockX = pos.x;
            placeEvent.blockY = pos.y;
            placeEvent.blockZ = pos.z;
            placeEvent.dimensionId = world.dimension();
            // 获取被替换的方块类型ID（放置前的方块，通常是空气）
            placeEvent.previousBlockTypeId = "minecraft:air"; // 放置前通常是空气
            blockCompReg.dispatchPlace(blockTypeId, placeEvent);
        }

        // 触发进度触发器
        if (player != nullptr) {
            static_cast<void>(world.onBlockPlaced(static_cast<PlayerId>(player->id()), pos, actualState, &stack));
        }
    }

    // 播放放置音效
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
    if (currentState != nullptr && !currentState->getBlock().isReplaceable(*currentState, context)) {
        return false;
    }

    // 实体碰撞检查
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

    // 在世界中设置方块状态
    // 参数 11 = 1 (通知邻居) | 2 (通知观察者) | 8 (同步到客户端)
    return context.getWorld().setBlockState(context.placementPos(), state, 11);
}

bool BlockItem::onBlockPlaced(
    const BlockPos& pos, IWorld& world, Player* player, const ItemStack& stack, const BlockState& state) const
{
    // 将物品堆中的 BlockEntityTag 数据应用到方块实体
    return setTileEntityNBT(world, player, pos, stack);
}

bool BlockItem::setTileEntityNBT(IWorld& world, Player* player, const BlockPos& pos, const ItemStack& stack) const
{
    // 检查物品堆是否有 BlockEntityTag
    const nlohmann::json* blockEntityTag = stack.getChildTag("BlockEntityTag");
    if (blockEntityTag == nullptr || !blockEntityTag->is_object()) {
        return false;
    }

    // 获取该位置的方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return false;
    }

    // 权限检查：如果方块实体仅允许 OP 修改 NBT，则需要验证玩家权限
    // 需要OP权限的方块实体类型包括：CommandBlock, Sign, HangingSign, StructureBlock, JigsawBlock, TrialSpawner, Lectern
    if (blockEntity->onlyOpsCanSetNbt()) {
        // 玩家必须非空且拥有游戏管理员方块使用权限（创造模式 + OP等级>=2）
        if (player == nullptr || !player->canUseGameMasterBlocks()) {
            return false;
        }
    }

    // 验证 BlockEntityTag 中的类型ID与实际方块实体类型匹配
    auto idIt = blockEntityTag->find("id");
    if (idIt != blockEntityTag->end() && idIt->is_string()) {
        std::string tagTypeId = idIt->get<std::string>();
        ResourceLocation tagType(tagTypeId);
        BlockEntityType expectedType = blockEntityTypeFromId(tagType);
        if (expectedType != BlockEntityType::Unknown && expectedType != blockEntity->getType()) {
            // 类型不匹配，拒绝加载
            return false;
        }
    }

    // 将 BlockEntityTag 中的数据合并到方块实体
    // 流程：保存当前数据 -> 合并物品NBT -> 加载合并后数据 -> 失败则回滚
    // 本项目使用 JSON 存储自定义数据，直接合并即可

    // 先保存当前方块实体的数据（用于失败时回滚）
    nlohmann::json currentData;
    blockEntity->save(currentData);

    // 合并 BlockEntityTag 到当前数据
    // 注意：移除 "id" 字段，因为它是类型标识符而非数据
    nlohmann::json mergedData = currentData;
    ItemStack::mergeJsonObjects(mergedData, *blockEntityTag);
    mergedData.erase("id");

    // 尝试加载合并后的数据
    bool success = blockEntity->load(mergedData);
    if (!success) {
        // 加载失败，回滚到原始数据
        blockEntity->load(currentData);
        return false;
    }

    // 标记方块实体已修改，触发保存和客户端同步
    blockEntity->setChanged();

    return true;
}

const BlockState* BlockItem::applyBlockStateFromNBT(
    const BlockPos& pos, IWorld& world, const ItemStack& stack, const BlockState& state) const
{
    // 从物品堆的 BlockStateTag 子标签中读取方块状态属性并应用

    const nlohmann::json* blockStateTag = stack.getChildTag("BlockStateTag");
    if (blockStateTag == nullptr || !blockStateTag->is_object()) {
        return &state;
    }

    // 如果 BlockStateTag 为空对象，直接返回
    if (blockStateTag->empty()) {
        return &state;
    }

    const Block& block = state.getBlock();
    const auto& container = block.stateContainer();
    const BlockState* currentState = &state;

    // 遍历 BlockStateTag 中的每个属性名-值对
    for (const auto& [propName, propValue] : blockStateTag->items()) {
        // 跳过非字符串值
        if (!propValue.is_string()) {
            continue;
        }

        std::string valueStr = propValue.get<std::string>();

        // 通过属性名查找属性定义
        const IProperty* prop = container.getProperty(propName);
        if (prop == nullptr) {
            // 属性在此方块上不存在，跳过
            continue;
        }

        // 检查当前方块状态是否拥有此属性
        auto currentValueIndex = currentState->getValueIndex(*prop);
        if (!currentValueIndex.has_value()) {
            // 此属性不属于当前方块，跳过
            continue;
        }

        // 将字符串值解析为属性值索引
        auto parsedIndex = prop->parseValue(valueStr);
        if (!parsedIndex.has_value()) {
            // 值字符串无法解析为此属性的有效值，跳过
            continue;
        }

        // 使用类型擦除的 withValueIndex 方法设置属性
        currentState = &currentState->withValueIndex(*prop, *parsedIndex);
    }

    // 如果状态发生了变化，更新世界中的方块状态
    if (currentState != &state) {
        world.setBlockState(pos, currentState, 2);
    }

    return currentState;
}

} // namespace mc
