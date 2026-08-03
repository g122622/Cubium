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

#include "ComposterBlock.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../sound/SoundCategory.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../WorldEvents.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "CompostableItems.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/ISidedInventory.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== ComposterBlock 实现 ==========

ComposterBlock::ComposterBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LEVEL_0_8())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::LEVEL_0_8(), 0));

    // 预计算各等级的形状
    // 堆肥桶形状 = 完整方块 - 内部12像素宽的柱体（从 fillHeight 到顶部）
    // 由于 CollisionShape 暂不支持布尔减法，采用与 CauldronBlock 相同的方式手动拼接外壁：
    // 底板 + 四面墙壁（2像素厚），内部柱体区域为空心
    constexpr f32 P = 1.0f / 16.0f;

    // 各等级的渲染形状：
    // 底板厚度随等级增加（内部柱体底部上移，即空心区域减小）
    // 内部柱体宽度 = 12像素，从 y = clamp(1+level*2, 2, 16) 到 y = 16
    // 外壁 = 底板（y: 0 ~ fillHeight）+ 四面墙壁（y: fillHeight ~ 16, 2像素厚）
    for (i32 i = 0; i < 8; ++i) {
        i32 fillHeightPixels = std::max(2, 1 + i * 2);
        f32 fillHeight = static_cast<f32>(fillHeightPixels) * P;
        f32 innerMin = 2.0f * P;  // 内壁起始 X/Z
        f32 innerMax = 14.0f * P; // 内壁结束 X/Z
        f32 top = 1.0f;           // 方块顶部

        // 底板：完整方块，从 y=0 到 y=fillHeight
        CollisionShape base = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, fillHeight, 1.0f);

        // 北墙：x: 0~16, y: fillHeight~16, z: 0~2
        CollisionShape northWall = CollisionShape::box(0.0f, fillHeight, 0.0f, 1.0f, top, innerMin);
        // 南墙：x: 0~16, y: fillHeight~16, z: 14~16
        CollisionShape southWall = CollisionShape::box(0.0f, fillHeight, innerMax, 1.0f, top, 1.0f);
        // 西墙：x: 0~2, y: fillHeight~16, z: 2~14
        CollisionShape westWall = CollisionShape::box(0.0f, fillHeight, innerMin, innerMin, top, innerMax);
        // 东墙：x: 14~16, y: fillHeight~16, z: 2~14
        CollisionShape eastWall = CollisionShape::box(innerMax, fillHeight, innerMin, 1.0f, top, innerMax);

        // 合并所有部分
        m_shapesByLevel[i] = CollisionShape::combine(
            CollisionShape::combine(
                CollisionShape::combine(CollisionShape::combine(base, northWall), southWall), westWall),
            eastWall);
    }
    // 等级7和8形状相同
    m_shapesByLevel[8] = m_shapesByLevel[7];
}

BlockState ComposterBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState();
}

void ComposterBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    i32 level = getLevel(state);
    if (level == 7) {
        // 等级7时，经过20 tick后变成等级8（可以收获骨粉）
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), 8);
        world.setBlockState(pos, &newState, 3);

        // 播放堆肥完成音效
        if (!world.isClientSide()) {
            world.playSound(SoundEvents::BLOCK_COMPOSTER_READY, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        }
    }
}

const CollisionShape& ComposterBlock::getShape(const BlockState& state) const
{
    i32 level = getLevel(state);
    MC_ASSERT(level >= 0 && level <= 8);
    return m_shapesByLevel[level];
}

const CollisionShape& ComposterBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 碰撞形状始终为等级0的外壳形状（底板2像素 + 四面墙壁）
    return m_shapesByLevel[0];
}

i32 ComposterBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 等级
    return getLevel(state);
}

BlockState ComposterBlock::attemptCompost(
    const BlockState& state, IWorld& world, const BlockPos& pos, const Block& block, u32 itemId)
{

    i32 level = getLevel(state);
    if (level >= 7) {
        return state; // 已满或正在完成
    }

    // 从 CompostableItems 注册表获取堆肥概率
    const Item* item = Item::getItem(itemId);
    if (item == nullptr) {
        return state;
    }

    f32 chance = CompostableItems::getCompostChance(item);
    if (chance <= 0.0f) {
        return state; // 不可堆肥
    }

    // 概率性增加等级
    // MC 原版使用 world.getRandom() 获取随机数，确保每次调用结果不同
    math::IRandom& random = world.getRandom();
    if (random.nextFloat() < chance) {
        i32 newLevel = level + 1;
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), newLevel);
        world.setBlockState(pos, &newState, 3);

        // 通过 WorldEvent 广播堆肥成功事件（客户端同时播放音效和粒子效果）
        // data=1 表示成功升级，data=0 表示仅填充未升级
        if (!world.isClientSide()) {
            world.playEvent(world::WorldEvents::COMPOSTER_FILLED_UP, pos, 1);
        }

        // 如果达到等级7，调度 20 tick 后的转变
        if (newLevel == 7) {
            world.tickManager().scheduleBlockTick(pos, block, 20);
        }

        return newState;
    }

    // 堆肥失败（尝试堆肥但没增加等级）
    // 通过 WorldEvent 广播堆肥失败事件
    if (!world.isClientSide()) {
        world.playEvent(world::WorldEvents::COMPOSTER_FILLED_UP, pos, 0);
    }

    return state;
}

BlockState ComposterBlock::empty(IWorld& world, const BlockPos& pos, BlockState& state)
{
    // 生成骨粉物品
    // 只有等级为 8 时才能收获
    i32 level = getLevel(state);
    if (level != 8) {
        return state;
    }

    // 掉落骨粉物品
    if (!world.isClientSide() && Items::BONE_MEAL != nullptr) {
        // 创建骨粉物品堆
        ItemStack boneMealStack(Items::BONE_MEAL, 1);

        // 使用 ItemDropHelper 生成物品实体
        math::Random random;
        ItemDropHelper::spawnItemEntity(&world,
            boneMealStack,
            static_cast<f64>(pos.x) + 0.5,
            static_cast<f64>(pos.y) + 1.0, // 在堆肥桶上方生成
            static_cast<f64>(pos.z) + 0.5,
            random,
            ItemDropHelper::DEFAULT_PICKUP_DELAY,
            "" // 无所有者
        );
    }

    // 重置为等级0
    BlockState newState = state.with(BlockStateProperties::LEVEL_0_8(), 0);
    world.setBlockState(pos, &newState, 3);

    // 播放清空音效
    if (!world.isClientSide()) {
        world.playSound(SoundEvents::BLOCK_COMPOSTER_EMPTY, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
    }

    return newState;
}

bool ComposterBlock::isCompostable(u32 itemId)
{
    const Item* item = Item::getItem(itemId);
    return CompostableItems::isCompostable(item);
}

f32 ComposterBlock::getCompostChance(u32 itemId)
{
    const Item* item = Item::getItem(itemId);
    return CompostableItems::getCompostChance(item);
}

BlockActionResult ComposterBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(hand);
    MC_UNUSED(hit);
    i32 level = getLevel(state);

    // 如果等级为8，取出骨粉
    if (level == 8) {
        empty(world, pos, const_cast<BlockState&>(state));
        return ActionResultType::Success;
    }

    // 检查玩家手持物品
    ItemStack& heldItem = player.getHeldItem(hand);
    if (heldItem.isEmpty()) {
        return ActionResultType::Pass;
    }

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查物品是否可堆肥
    f32 chance = CompostableItems::getCompostChance(item);
    if (chance <= 0.0f) {
        return ActionResultType::Pass;
    }

    // 尝试堆肥
    BlockState newState = attemptCompost(state, world, pos, *this, static_cast<u32>(item->itemId()));

    // 如果堆肥成功（状态改变了），消耗物品
    if (newState.get(BlockStateProperties::LEVEL_0_8()) > level) {
        // 非创造模式消耗物品
        if (!player.abilities().creativeMode) {
            heldItem.shrink(1);
            player.inventory().setChanged();
        }
        return ActionResultType::Success;
    }

    // 堆肥失败但仍播放了音效
    return ActionResultType::Success;
}

// ========== ISidedInventoryProvider 接口 ==========

std::unique_ptr<ISidedInventory> ComposterBlock::createInventory(
    const BlockState& state, IWorld& world, const BlockPos& pos)
{
    i32 level = getLevel(state);
    if (level == 8) {
        return std::make_unique<OutputContainer>(state, world, pos);
    } else if (level < 7) {
        return std::make_unique<InputContainer>(state, world, pos);
    } else {
        // 等级 7：正在转变中，不允许任何交互
        return std::make_unique<EmptyContainer>();
    }
}

// ========== EmptyContainer 实现 ==========

ItemStack ComposterBlock::EmptyContainer::getItem(i32 slot) const
{
    MC_UNUSED(slot);
    return ItemStack::EMPTY;
}

void ComposterBlock::EmptyContainer::setItem(i32 slot, const ItemStack& stack)
{
    MC_UNUSED(slot);
    MC_UNUSED(stack);
}

ItemStack ComposterBlock::EmptyContainer::removeItem(i32 slot, i32 count)
{
    MC_UNUSED(slot);
    MC_UNUSED(count);
    return ItemStack::EMPTY;
}

ItemStack ComposterBlock::EmptyContainer::removeItemNoUpdate(i32 slot)
{
    MC_UNUSED(slot);
    return ItemStack::EMPTY;
}

std::vector<i32> ComposterBlock::EmptyContainer::getSlotsForFace(Direction side) const
{
    MC_UNUSED(side);
    return {};
}

bool ComposterBlock::EmptyContainer::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    MC_UNUSED(stack);
    MC_UNUSED(direction);
    return false;
}

bool ComposterBlock::EmptyContainer::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    MC_UNUSED(stack);
    MC_UNUSED(direction);
    return false;
}

// ========== InputContainer 实现 ==========

ComposterBlock::InputContainer::InputContainer(const BlockState& state, IWorld& world, const BlockPos& pos)
    : m_state(state)
    , m_world(world)
    , m_pos(pos)
{}

bool ComposterBlock::InputContainer::isEmpty() const noexcept
{
    return m_item.isEmpty();
}

ItemStack ComposterBlock::InputContainer::getItem(i32 slot) const
{
    MC_UNUSED(slot);
    return m_item;
}

void ComposterBlock::InputContainer::setItem(i32 slot, const ItemStack& stack)
{
    MC_UNUSED(slot);
    m_item = stack;
    setChanged();
}

ItemStack ComposterBlock::InputContainer::removeItem(i32 slot, i32 count)
{
    MC_UNUSED(slot);
    MC_UNUSED(count);
    // 输入容器不允许提取
    return ItemStack::EMPTY;
}

ItemStack ComposterBlock::InputContainer::removeItemNoUpdate(i32 slot)
{
    MC_UNUSED(slot);
    return ItemStack::EMPTY;
}

void ComposterBlock::InputContainer::clear()
{
    m_item = ItemStack::EMPTY;
}

void ComposterBlock::InputContainer::setChanged()
{
    if (m_changed) {
        return;
    }

    // 当物品被放入槽位时，自动执行堆肥逻辑
    if (!m_item.isEmpty()) {
        m_changed = true;
        const Item* item = m_item.getItem();
        if (item != nullptr) {
            attemptCompost(m_state, m_world, m_pos, m_state.getBlock(), static_cast<u32>(item->itemId()));
        }
        // 处理完毕后清空槽位
        m_item = ItemStack::EMPTY;
    }
}

bool ComposterBlock::InputContainer::canPlaceItem(i32 slot, const ItemStack& stack) const
{
    MC_UNUSED(slot);
    // 仅当未处理过且物品可堆肥时允许放入
    if (m_changed) {
        return false;
    }
    const Item* item = stack.getItem();
    return item != nullptr && CompostableItems::isCompostable(item);
}

std::vector<i32> ComposterBlock::InputContainer::getSlotsForFace(Direction side) const
{
    // 仅允许从上方访问槽位 0
    if (side == Direction::Up) {
        return {0};
    }
    return {};
}

bool ComposterBlock::InputContainer::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    // 仅允许从上方插入可堆肥物品，且未处理过
    if (m_changed || direction != Direction::Up) {
        return false;
    }
    const Item* item = stack.getItem();
    return item != nullptr && CompostableItems::isCompostable(item);
}

bool ComposterBlock::InputContainer::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    MC_UNUSED(stack);
    MC_UNUSED(direction);
    // 输入容器不允许提取
    return false;
}

// ========== OutputContainer 实现 ==========

ComposterBlock::OutputContainer::OutputContainer(const BlockState& state, IWorld& world, const BlockPos& pos)
    : m_state(state)
    , m_world(world)
    , m_pos(pos)
{
    // 初始化为 1 个骨粉
    if (Items::BONE_MEAL != nullptr) {
        m_item = ItemStack(Items::BONE_MEAL, 1);
    }
}

bool ComposterBlock::OutputContainer::isEmpty() const noexcept
{
    return m_item.isEmpty();
}

ItemStack ComposterBlock::OutputContainer::getItem(i32 slot) const
{
    MC_UNUSED(slot);
    return m_item;
}

void ComposterBlock::OutputContainer::setItem(i32 slot, const ItemStack& stack)
{
    MC_UNUSED(slot);
    m_item = stack;
    setChanged();
}

ItemStack ComposterBlock::OutputContainer::removeItem(i32 slot, i32 count)
{
    if (slot != 0 || m_item.isEmpty()) {
        return ItemStack::EMPTY;
    }
    i32 toRemove = std::min(count, m_item.getCount());
    ItemStack result = m_item.copy();
    result.setCount(toRemove);
    m_item.shrink(toRemove);
    if (m_item.isEmpty()) {
        m_item = ItemStack::EMPTY;
    }
    setChanged();
    return result;
}

ItemStack ComposterBlock::OutputContainer::removeItemNoUpdate(i32 slot)
{
    if (slot != 0) {
        return ItemStack::EMPTY;
    }
    ItemStack result = std::move(m_item);
    m_item = ItemStack::EMPTY;
    return result;
}

void ComposterBlock::OutputContainer::clear()
{
    m_item = ItemStack::EMPTY;
    setChanged();
}

void ComposterBlock::OutputContainer::setChanged()
{
    if (m_changed) {
        return;
    }

    // 当骨粉被取出时，清空堆肥桶并重置为等级 0
    if (m_item.isEmpty()) {
        m_changed = true;
        BlockState stateCopy = m_state;
        empty(m_world, m_pos, stateCopy);
    }
}

std::vector<i32> ComposterBlock::OutputContainer::getSlotsForFace(Direction side) const
{
    // 仅允许从下方访问槽位 0
    if (side == Direction::Down) {
        return {0};
    }
    return {};
}

bool ComposterBlock::OutputContainer::canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    MC_UNUSED(stack);
    MC_UNUSED(direction);
    // 输出容器不允许插入
    return false;
}

bool ComposterBlock::OutputContainer::canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const
{
    MC_UNUSED(slot);
    // 仅允许从下方提取骨粉，且未处理过
    if (m_changed || direction != Direction::Down) {
        return false;
    }
    // 检查提取的物品是否为骨粉
    if (Items::BONE_MEAL != nullptr && stack.getItem() == Items::BONE_MEAL) {
        return true;
    }
    return false;
}

} // namespace blocks
} // namespace mc
