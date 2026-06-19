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
#include "../../../tick/manager/TickManager.hpp"
#include "CompostableItems.hpp"

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
    // MC Java: Block.column(12.0, clamp(1 + level * 2, 2, 16), 16.0)
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
    // MC Java: 碰撞形状始终为等级0的外壳形状（底板2像素 + 四面墙壁）
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
    const BlockState& state, IWorld& world, const BlockPos& pos, Block& block, u32 itemId)
{

    i32 level = getLevel(state);
    if (level >= 7) {
        return state; // 已满或正在完成
    }

    // 从 CompostableItems 注册表获取堆肥概率
    const Item* item = Item::getItem(static_cast<ItemId>(itemId));
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

        // 播放成功音效
        if (!world.isClientSide()) {
            world.playSound(
                SoundEvents::BLOCK_COMPOSTER_FILL_SUCCESS, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
        }

        // 如果达到等级7，调度 20 tick 后的转变
        if (newLevel == 7) {
            world.tickManager().scheduleBlockTick(pos, block, 20);
        }

        return newState;
    }

    // 播放失败音效（尝试堆肥但没增加等级）
    if (!world.isClientSide()) {
        world.playSound(SoundEvents::BLOCK_COMPOSTER_FILL, sound::SoundCategory::Blocks, pos.center(), 1.0f, 1.0f);
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
    const Item* item = Item::getItem(static_cast<ItemId>(itemId));
    return CompostableItems::isCompostable(item);
}

f32 ComposterBlock::getCompostChance(u32 itemId)
{
    const Item* item = Item::getItem(static_cast<ItemId>(itemId));
    return CompostableItems::getCompostChance(item);
}

ActionResultType ComposterBlock::onBlockActivated(const BlockState& state,
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
    ItemStack heldItem = player.inventory().getSelectedStack();
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
    // TODO: 堆肥成功时需要播放粒子效果（参考 MC Java: ComposterBlock.animateTick）
}

} // namespace blocks
} // namespace mc
