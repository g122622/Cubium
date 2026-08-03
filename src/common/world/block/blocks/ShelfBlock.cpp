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

#include "ShelfBlock.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/HorizontalBlock.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/ShelfBlockEntity.hpp"
#include "common/world/gameevent/GameEvents.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// ShelfBlock 构造函数
// ============================================================================

ShelfBlock::ShelfBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    // 书架碰撞箱（朝北时的形状）：
    // 顶部横条：(0, 12, 11) - (16, 16, 13)
    // 背板：(0, 0, 13) - (16, 16, 16)
    // 底部横条：(0, 0, 11) - (16, 4, 13)
    CollisionShape northTop = CollisionShape::box(0.0f, 12.0f, 11.0f, 16.0f, 16.0f, 13.0f);
    CollisionShape northBack = CollisionShape::box(0.0f, 0.0f, 13.0f, 16.0f, 16.0f, 16.0f);
    CollisionShape northBottom = CollisionShape::box(0.0f, 0.0f, 11.0f, 16.0f, 4.0f, 13.0f);
    CollisionShape northShape = CollisionShape::combine(northTop, northBack, CollisionShape::CombineOp::OR);
    northShape = CollisionShape::combine(northShape, northBottom, CollisionShape::CombineOp::OR);

    // 旋转到四个方向
    m_shapesByDirection[Direction::North] = northShape;

    // South: 顶部(0,12,3,16,16,5) 背板(0,0,0,16,16,3) 底部(0,0,3,16,4,5)
    CollisionShape southTop = CollisionShape::box(0.0f, 12.0f, 3.0f, 16.0f, 16.0f, 5.0f);
    CollisionShape southBack = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f, 16.0f, 3.0f);
    CollisionShape southBottom = CollisionShape::box(0.0f, 0.0f, 3.0f, 16.0f, 4.0f, 5.0f);
    CollisionShape southShape = CollisionShape::combine(southTop, southBack, CollisionShape::CombineOp::OR);
    southShape = CollisionShape::combine(southShape, southBottom, CollisionShape::CombineOp::OR);
    m_shapesByDirection[Direction::South] = southShape;

    // East: 顶部(3,12,0,5,16,16) 背板(0,0,0,3,16,16) 底部(3,0,0,5,4,16)
    CollisionShape eastTop = CollisionShape::box(3.0f, 12.0f, 0.0f, 5.0f, 16.0f, 16.0f);
    CollisionShape eastBack = CollisionShape::box(0.0f, 0.0f, 0.0f, 3.0f, 16.0f, 16.0f);
    CollisionShape eastBottom = CollisionShape::box(3.0f, 0.0f, 0.0f, 5.0f, 4.0f, 16.0f);
    CollisionShape eastShape = CollisionShape::combine(eastTop, eastBack, CollisionShape::CombineOp::OR);
    eastShape = CollisionShape::combine(eastShape, eastBottom, CollisionShape::CombineOp::OR);
    m_shapesByDirection[Direction::East] = eastShape;

    // West: 顶部(11,12,0,13,16,16) 背板(13,0,0,16,16,16) 底部(11,0,0,13,4,16)
    CollisionShape westTop = CollisionShape::box(11.0f, 12.0f, 0.0f, 13.0f, 16.0f, 16.0f);
    CollisionShape westBack = CollisionShape::box(13.0f, 0.0f, 0.0f, 16.0f, 16.0f, 16.0f);
    CollisionShape westBottom = CollisionShape::box(11.0f, 0.0f, 0.0f, 13.0f, 4.0f, 16.0f);
    CollisionShape westShape = CollisionShape::combine(westTop, westBack, CollisionShape::CombineOp::OR);
    westShape = CollisionShape::combine(westShape, westBottom, CollisionShape::CombineOp::OR);
    m_shapesByDirection[Direction::West] = westShape;

    // HorizontalBlock 已添加 HORIZONTAL_FACING，需额外添加 POWERED、SIDE_CHAIN_PART、WATERLOGGED
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::SIDE_CHAIN_PART())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::SIDE_CHAIN_PART(), BlockStateProperties::SideChainPart::Unconnected)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

// ============================================================================
// 放置和更新
// ============================================================================

BlockState ShelfBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state =
        defaultState()
            .with(FACING(), Directions::opposite(context.horizontalDirection()))
            .with(BlockStateProperties::POWERED(),
                world::redstone::RedstonePower::isPowered(context.getWorld(), context.placementPos()))
            .with(BlockStateProperties::SIDE_CHAIN_PART(), BlockStateProperties::SideChainPart::Unconnected)
            .with(BlockStateProperties::WATERLOGGED(), false);

    // 检测放置位置是否含水
    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState ShelfBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态的流体 tick 调度
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

void ShelfBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    if (state.get(BlockStateProperties::POWERED())) {
        // 充能时：尝试连接邻居书架
        // 注意：onBlockAdded 无法获取之前的方块状态（previousState），
        // 因此使用当前 SIDE_CHAIN_PART 状态作为递归保护：
        // 如果已经处于连接状态，说明是由邻居的侧链更新触发的，跳过。
        auto currentPart = state.get(BlockStateProperties::SIDE_CHAIN_PART());
        if (!BlockStateProperties::isConnected(currentPart)) {
            updateSelfAndNeighborsOnPoweringUp(world, pos, state);
        }
    } else {
        // 未充能时：断开邻居的侧链连接
        updateNeighborsAfterPoweringDown(world, pos, state);
    }
}

void ShelfBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    if (!world.isClientSide()) {
        const BlockState* statePtr = world.getBlockState(pos);
        if (statePtr == nullptr) {
            return;
        }
        BlockState state = *statePtr;

        bool hasSignal = world::redstone::RedstonePower::isPowered(world, pos);
        bool isPowered = state.get(BlockStateProperties::POWERED());

        if (isPowered != hasSignal) {
            // 红石信号发生变化
            state = state.with(BlockStateProperties::POWERED(), hasSignal);

            if (!hasSignal) {
                // 断电时重置侧链状态
                state = state.with(
                    BlockStateProperties::SIDE_CHAIN_PART(), BlockStateProperties::SideChainPart::Unconnected);
            }

            world.setBlockState(pos, &state, 3);

            // 播放充能/断电音效
            playSound(world, pos, hasSignal ? SoundEvents::BLOCK_SHELF_ACTIVATE : SoundEvents::BLOCK_SHELF_DEACTIVATE);

            // 触发游戏事件
            world.gameEvent(hasSignal ? gameevent::GameEvents::BLOCK_ACTIVATE : gameevent::GameEvents::BLOCK_DEACTIVATE,
                pos,
                &state);
        }
    }
}

void ShelfBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 掉落书架内物品
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Shelf) {
        auto* shelf = static_cast<blockentity::ShelfBlockEntity*>(blockEntity);
        IInventory* inventory = shelf->getInventory();
        if (inventory != nullptr && !world.isClientSide()) {
            math::Random rng;
            for (i32 i = 0; i < inventory->getContainerSize(); ++i) {
                ItemStack stack = inventory->removeItemNoUpdate(i);
                if (!stack.isEmpty()) {
                    ItemDropHelper::spawnItemEntity(&world,
                        stack,
                        static_cast<f32>(pos.x) + 0.5f,
                        static_cast<f32>(pos.y) + 0.5f,
                        static_cast<f32>(pos.z) + 0.5f,
                        rng);
                }
            }
        }
    }

    // 断开侧链连接
    updateNeighborsAfterPoweringDown(world, pos, state);

    Block::onBlockRemoved(world, pos, state);
}

// ============================================================================
// 碰撞箱
// ============================================================================

const CollisionShape& ShelfBlock::getShape(const BlockState& state) const
{
    Direction facing = state.get(FACING());
    auto it = m_shapesByDirection.find(facing);
    if (it != m_shapesByDirection.end()) {
        return it->second;
    }
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ============================================================================
// 红石
// ============================================================================

i32 ShelfBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);

    // 从 ShelfBlockEntity 读取3位二进制编码的比较器信号
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Shelf) {
        auto* shelf = static_cast<blockentity::ShelfBlockEntity*>(blockEntity);
        return shelf->getAnalogOutputSignal();
    }

    return 0;
}

// ============================================================================
// 方块实体
// ============================================================================

std::unique_ptr<BlockEntity> ShelfBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::ShelfBlockEntity>(pos);
}

// ============================================================================
// IWaterLoggable 接口实现
// ============================================================================

const fluid::FluidState* ShelfBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

// ============================================================================
// 交互
// ============================================================================

BlockActionResult ShelfBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    // 副手不处理
    if (hand == Hand::OffHand) {
        return ActionResultType::Pass;
    }

    // 获取书架方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Shelf) {
        return ActionResultType::Pass;
    }
    auto* shelfEntity = static_cast<blockentity::ShelfBlockEntity*>(blockEntity);

    // 计算点击的槽位
    Direction facing = state.get(FACING());
    i32 hitSlot = getHitSlot(hit, facing);
    if (hitSlot < 0) {
        // 未命中书架正面
        return ActionResultType::Pass;
    }

    // 客户端仅返回是否成功
    if (world.isClientSide()) {
        ItemStack heldItem = player.getHeldItem(hand);
        return heldItem.isEmpty() ? ActionResultType::Pass : ActionResultType::Success;
    }

    ItemStack& heldItem = player.getHeldItem(hand);
    PlayerInventory& inventory = player.inventory();

    if (!state.get(BlockStateProperties::POWERED())) {
        // 未充能模式：单物品交换
        // 注意：MC 1.21.11 中 p_433583_ 是引用，但 Java 的引用语义与 C++ 不同——
        // inventory.setItem() 会替换数组槽位的引用，而 p_433583_ 仍指向原对象。
        // C++ 中 heldItem 是对 m_items[selectedSlot] 的引用，setItem 后 heldItem
        // 反映新值。因此这里先记录原手持物品是否为空，用于后续音效与 Pass 判断。
        const bool heldItemWasEmpty = heldItem.isEmpty();

        bool wasSwapOrTake = swapSingleItem(heldItem, player, *shelfEntity, hitSlot, inventory);

        if (wasSwapOrTake) {
            // 取出或交换了物品
            // 取出（原手持为空）→ TAKE_ITEM；交换（原手持非空）→ SINGLE_SWAP
            playSound(world,
                pos,
                heldItemWasEmpty ? SoundEvents::BLOCK_SHELF_TAKE_ITEM : SoundEvents::BLOCK_SHELF_SINGLE_SWAP);
        } else {
            // 放入了物品（书架原为空）
            // 若原手持为空 → 无操作 → Pass；否则 → PLACE_ITEM
            if (heldItemWasEmpty) {
                return ActionResultType::Pass;
            }
            playSound(world, pos, SoundEvents::BLOCK_SHELF_PLACE_ITEM);
        }

        // 参考 MC 1.21.11 ShelfBlock.useItemOn 第 179 行：
        // return InteractionResult.SUCCESS.heldItemTransformedTo(p_433583_);
        // swapSingleItem 通过 inventory.setItem 已更新手持物品，heldItem 引用反映最新值。
        // 携带转换后的手持物品，供 BlockInteractionManager 同步到 InventoryManager 与客户端。
        return BlockActionResult::success(ItemStack(heldItem));
    } else {
        // 充能模式：热栏整体交换
        // 参考 MC 1.21.11 ShelfBlock.useItemOn 第 181-189 行：
        //   ItemStack itemstack = inventory.getSelectedItem();
        //   boolean flag = this.swapHotbar(...);
        //   if (!flag) return InteractionResult.CONSUME;
        //   this.playSound(...);
        //   return itemstack == inventory.getSelectedItem()
        //       ? InteractionResult.SUCCESS
        //       : InteractionResult.SUCCESS.heldItemTransformedTo(inventory.getSelectedItem());
        // MC 使用引用比较（==）判断选中物品是否变化，我们用值比较语义一致：
        // 若交换前后选中槽位物品相同则不携带 heldItemTransformedTo，否则携带新物品。
        ItemStack oldSelectedItem = inventory.getSelectedStack();
        bool swapped = swapHotbar(world, pos, inventory);

        if (!swapped) {
            return ActionResultType::Consume;
        }

        playSound(world, pos, SoundEvents::BLOCK_SHELF_MULTI_SWAP);

        ItemStack newSelectedItem = inventory.getSelectedStack();
        if (oldSelectedItem == newSelectedItem) {
            // 交换前后选中物品相同（值比较），无需携带 heldItemTransformedTo
            return BlockActionResult::success();
        }
        // 选中槽位物品已变化，携带转换后的新物品
        return BlockActionResult::success(ItemStack(newSelectedItem));
    }
}

// ============================================================================
// 侧链连接
// ============================================================================

bool ShelfBlock::isConnectable(const BlockState& state)
{
    // 方块必须在 WOODEN_SHELVES 标签中，且 POWERED=true
    return BlockTags::WOODEN_SHELVES().contains(state) && state.hasProperty(BlockStateProperties::POWERED()) &&
        state.get(BlockStateProperties::POWERED());
}

std::vector<BlockPos> ShelfBlock::getAllBlocksConnectedTo(IWorld& world, const BlockPos& pos)
{
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr || !isConnectable(*statePtr)) {
        return {};
    }

    Direction facing = statePtr->get(FACING());
    auto sidePart = statePtr->get(BlockStateProperties::SIDE_CHAIN_PART());

    std::vector<BlockPos> result;

    // 首先沿左侧搜索
    Direction leftDir = Directions::rotateY(facing); // 面朝方向的左侧 = 顺时针旋转
    for (i32 i = 1; i < MAX_CHAIN_LENGTH; ++i) {
        BlockPos neighborPos(pos.x + i * Directions::xOffset(leftDir), pos.y, pos.z + i * Directions::zOffset(leftDir));
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState == nullptr || !isConnectable(*neighborState)) {
            break;
        }
        auto neighborPart = neighborState->get(BlockStateProperties::SIDE_CHAIN_PART());
        if (!BlockStateProperties::isConnected(neighborPart)) {
            break;
        }
        // 邻居必须朝相同方向
        if (neighborState->get(FACING()) != facing) {
            break;
        }
        // 邻居必须朝右侧连接（向当前方向）
        if (!BlockStateProperties::isConnectionTowards(neighborPart, BlockStateProperties::SideChainPart::Right)) {
            break;
        }
        result.insert(result.begin(), neighborPos); // 前插，保持从左到右顺序
        if (BlockStateProperties::isChainEnd(neighborPart)) {
            break;
        }
    }

    // 加入自身
    result.push_back(pos);

    // 然后沿右侧搜索
    Direction rightDir = Directions::rotateYCCW(facing); // 面朝方向的右侧 = 逆时针旋转
    for (i32 i = 1; i < MAX_CHAIN_LENGTH; ++i) {
        BlockPos neighborPos(
            pos.x + i * Directions::xOffset(rightDir), pos.y, pos.z + i * Directions::zOffset(rightDir));
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState == nullptr || !isConnectable(*neighborState)) {
            break;
        }
        auto neighborPart = neighborState->get(BlockStateProperties::SIDE_CHAIN_PART());
        if (!BlockStateProperties::isConnected(neighborPart)) {
            break;
        }
        if (neighborState->get(FACING()) != facing) {
            break;
        }
        if (!BlockStateProperties::isConnectionTowards(neighborPart, BlockStateProperties::SideChainPart::Left)) {
            break;
        }
        result.push_back(neighborPos); // 后插
        if (BlockStateProperties::isChainEnd(neighborPart)) {
            break;
        }
    }

    return result;
}

void ShelfBlock::updateSelfAndNeighborsOnPoweringUp(IWorld& world, const BlockPos& pos, const BlockState& currentState)
{
    if (!isConnectable(currentState)) {
        return;
    }

    // 递归保护：如果当前状态已经是连接状态，说明是邻居更新触发的，跳过
    auto currentPart = currentState.get(BlockStateProperties::SIDE_CHAIN_PART());
    if (BlockStateProperties::isConnected(currentPart)) {
        return;
    }

    Direction facing = currentState.get(FACING());
    Direction leftDir = Directions::rotateY(facing);
    Direction rightDir = Directions::rotateYCCW(facing);

    // 检查左侧邻居
    BlockPos leftPos(pos.x + Directions::xOffset(leftDir), pos.y, pos.z + Directions::zOffset(leftDir));
    const BlockState* leftState = world.getBlockState(leftPos);
    i32 leftChainSize = 0;
    if (leftState != nullptr && isConnectable(*leftState) && leftState->get(FACING()) == facing) {
        leftChainSize = static_cast<i32>(getAllBlocksConnectedTo(world, leftPos).size());
    }

    // 检查右侧邻居
    BlockPos rightPos(pos.x + Directions::xOffset(rightDir), pos.y, pos.z + Directions::zOffset(rightDir));
    const BlockState* rightState = world.getBlockState(rightPos);
    i32 rightChainSize = 0;
    if (rightState != nullptr && isConnectable(*rightState) && rightState->get(FACING()) == facing) {
        rightChainSize = static_cast<i32>(getAllBlocksConnectedTo(world, rightPos).size());
    }

    auto sidePart = BlockStateProperties::SideChainPart::Unconnected;
    i32 currentChainSize = 1; // 自身

    // 尝试连接左侧
    if (canConnect(leftChainSize, currentChainSize)) {
        sidePart = BlockStateProperties::whenConnectedToTheLeft(sidePart);
        // 更新左侧邻居的侧链状态
        if (leftState != nullptr) {
            auto leftPart = leftState->get(BlockStateProperties::SIDE_CHAIN_PART());
            setSideChainPart(world, leftPos, BlockStateProperties::whenConnectedToTheRight(leftPart));
        }
        currentChainSize += leftChainSize;
    }

    // 尝试连接右侧
    if (canConnect(rightChainSize, currentChainSize)) {
        sidePart = BlockStateProperties::whenConnectedToTheRight(sidePart);
        // 更新右侧邻居的侧链状态
        if (rightState != nullptr) {
            auto rightPart = rightState->get(BlockStateProperties::SIDE_CHAIN_PART());
            setSideChainPart(world, rightPos, BlockStateProperties::whenConnectedToTheLeft(rightPart));
        }
    }

    // 更新自身的侧链状态
    setSideChainPart(world, pos, sidePart);
}

void ShelfBlock::updateNeighborsAfterPoweringDown(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    Direction facing = state.get(FACING());
    Direction leftDir = Directions::rotateY(facing);
    Direction rightDir = Directions::rotateYCCW(facing);

    // 通知左侧邻居断开
    BlockPos leftPos(pos.x + Directions::xOffset(leftDir), pos.y, pos.z + Directions::zOffset(leftDir));
    const BlockState* leftState = world.getBlockState(leftPos);
    if (leftState != nullptr && isConnectable(*leftState) && leftState->get(FACING()) == facing) {
        auto leftPart = leftState->get(BlockStateProperties::SIDE_CHAIN_PART());
        setSideChainPart(world, leftPos, BlockStateProperties::whenDisconnectedFromTheRight(leftPart));
    }

    // 通知右侧邻居断开
    BlockPos rightPos(pos.x + Directions::xOffset(rightDir), pos.y, pos.z + Directions::zOffset(rightDir));
    const BlockState* rightState = world.getBlockState(rightPos);
    if (rightState != nullptr && isConnectable(*rightState) && rightState->get(FACING()) == facing) {
        auto rightPart = rightState->get(BlockStateProperties::SIDE_CHAIN_PART());
        setSideChainPart(world, rightPos, BlockStateProperties::whenDisconnectedFromTheLeft(rightPart));
    }
}

void ShelfBlock::setSideChainPart(IWorld& world, const BlockPos& pos, BlockStateProperties::SideChainPart part)
{
    const BlockState* statePtr = world.getBlockState(pos);
    if (statePtr == nullptr) {
        return;
    }
    auto currentPart = statePtr->get(BlockStateProperties::SIDE_CHAIN_PART());
    if (currentPart != part) {
        BlockState newState = statePtr->with(BlockStateProperties::SIDE_CHAIN_PART(), part);
        world.setBlockState(pos, &newState, 3);
    }
}

bool ShelfBlock::canConnect(i32 neighborChainSize, i32 currentChainSize)
{
    return neighborChainSize > 0 && currentChainSize + neighborChainSize <= MAX_CHAIN_LENGTH;
}

// ============================================================================
// 交互内部方法
// ============================================================================

i32 ShelfBlock::getHitSlot(const BlockRaycastResult& hit, Direction facing)
{
    // 只有点击书架正面（与 FACING 相同的面）才有效
    if (hit.face() != facing) {
        return -1;
    }

    // 计算点击位置相对于方块面的坐标
    // hitPosition() 返回世界坐标，blockPos() 返回方块位置
    Vector3 hitPos = hit.hitPosition();
    const BlockPos& blockPos = hit.blockPos();

    // 转换为方块内的相对坐标 [0, 1)
    f32 relX = static_cast<f32>(hitPos.x - static_cast<double>(blockPos.x));
    f32 relY = static_cast<f32>(hitPos.y - static_cast<double>(blockPos.y));
    f32 relZ = static_cast<f32>(hitPos.z - static_cast<double>(blockPos.z));

    // 将坐标限制在 [0, 1) 范围
    if (relX < 0.0f) relX = 0.0f;
    if (relX >= 1.0f) relX = 0.999f;
    if (relY < 0.0f) relY = 0.0f;
    if (relY >= 1.0f) relY = 0.999f;
    if (relZ < 0.0f) relZ = 0.0f;
    if (relZ >= 1.0f) relZ = 0.999f;

    // 根据面朝方向转换为面局部坐标
    // 参考: net.minecraft.world.level.block.SelectableSlotContainer
    f32 x = 0.0f;
    f32 y = 0.0f;

    switch (facing) {
        case Direction::North:
            x = 1.0f - relX;
            y = relY;
            break;
        case Direction::South:
            x = relX;
            y = relY;
            break;
        case Direction::West:
            x = relZ;
            y = relY;
            break;
        case Direction::East:
            x = 1.0f - relZ;
            y = relY;
            break;
        default:
            return -1; // 上下方向不处理
    }

    // 行索引（书架只有1行，所以总是0）
    f32 ySection = (1.0f - y) * 16.0f;
    f32 ySectionSize = 16.0f / static_cast<f32>(ROWS);
    i32 row = static_cast<i32>(ySection / ySectionSize);
    row = std::clamp(row, 0, ROWS - 1);

    // 列索引：将水平坐标分为3等份
    f32 xSection = x * 16.0f;
    f32 xSectionSize = 16.0f / static_cast<f32>(COLUMNS);
    i32 column = static_cast<i32>(xSection / xSectionSize);
    column = std::clamp(column, 0, COLUMNS - 1);

    return column + row * COLUMNS;
}

bool ShelfBlock::swapSingleItem(ItemStack& heldItem,
    Player& player,
    blockentity::ShelfBlockEntity& shelfEntity,
    i32 slotIndex,
    PlayerInventory& inventory)
{
    // 交换书架槽位和手持物品
    ItemStack oldItem = shelfEntity.swapItemNoUpdate(slotIndex, heldItem);

    // 创造模式处理
    ItemStack resultItem = oldItem;
    if (player.isCreative() && oldItem.isEmpty()) {
        resultItem = heldItem.copy();
    }

    // 更新玩家手持物品
    i32 selectedSlot = inventory.getSelectedSlot();
    inventory.setItem(selectedSlot, resultItem);
    inventory.setChanged();

    // 标记书架实体已更改
    shelfEntity.markChanged();

    return !oldItem.isEmpty();
}

bool ShelfBlock::swapHotbar(IWorld& world, const BlockPos& pos, PlayerInventory& inventory) const
{
    std::vector<BlockPos> chainBlocks = getAllBlocksConnectedTo(world, pos);
    if (chainBlocks.empty()) {
        return false;
    }

    bool anySwapped = false;
    i32 chainSize = static_cast<i32>(chainBlocks.size());

    for (i32 i = 0; i < chainSize; ++i) {
        BlockEntity* blockEntity = world.getBlockEntity(chainBlocks[static_cast<std::size_t>(i)]);
        if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::Shelf) {
            continue;
        }
        auto* shelfEntity = static_cast<blockentity::ShelfBlockEntity*>(blockEntity);
        IInventory* shelfInventory = shelfEntity->getInventory();
        if (shelfInventory == nullptr) {
            continue;
        }

        for (i32 j = 0; j < shelfInventory->getContainerSize(); ++j) {
            // 热栏映射：左侧书架 → 热栏低位，右侧书架 → 热栏高位
            // 公式：hotbarSlot = 9 - chainSize * containerSize + i * containerSize + j
            i32 hotbarSlot =
                9 - chainSize * shelfInventory->getContainerSize() + i * shelfInventory->getContainerSize() + j;
            if (hotbarSlot < 0 || hotbarSlot >= inventory.getContainerSize()) {
                continue;
            }

            ItemStack hotbarItem = inventory.removeItemNoUpdate(hotbarSlot);
            ItemStack shelfItem = shelfEntity->swapItemNoUpdate(j, hotbarItem);

            if (!hotbarItem.isEmpty() || !shelfItem.isEmpty()) {
                inventory.setItem(hotbarSlot, shelfItem);
                anySwapped = true;
            }
        }

        inventory.setChanged();
        shelfEntity->markChanged();
    }

    return anySwapped;
}

void ShelfBlock::playSound(IWorld& world, const BlockPos& pos, const ResourceLocation& soundEvent)
{
    world.playSound(soundEvent, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
}

// ============================================================================
// 旋转和镜像
// ============================================================================

const BlockState& ShelfBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(FACING());
    Direction newFacing = Directions::rotateDirection(facing, rotation);
    return state.with(FACING(), newFacing);
}

const BlockState& ShelfBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(FACING());
    Direction newFacing = facing;

    switch (mirror) {
        case Mirror::LeftRight:
            if (facing == Direction::East) {
                newFacing = Direction::West;
            } else if (facing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            if (facing == Direction::North) {
                newFacing = Direction::South;
            } else if (facing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(FACING(), newFacing);
}

// ============================================================================
// 状态容器
// ============================================================================

void ShelfBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

} // namespace blocks
} // namespace mc
