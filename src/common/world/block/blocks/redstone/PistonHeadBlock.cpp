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
 * LIABILITY, WHETHER IN TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "PistonHeadBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/redstone/MovingPistonBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc {

// EnumProperty Traits 实现 - 必须在 mc 命名空间
template <>
std::string EnumProperty<blocks::PistonHeadBlock::Type>::Traits::toString(const blocks::PistonHeadBlock::Type& value)
{
    switch (value) {
        case blocks::PistonHeadBlock::Type::Normal:
            return "default";
        case blocks::PistonHeadBlock::Type::Sticky:
            return "sticky";
        default:
            return "default";
    }
}

template <>
std::optional<blocks::PistonHeadBlock::Type> EnumProperty<blocks::PistonHeadBlock::Type>::Traits::fromName(
    std::string_view name)
{
    if (name == "default") return blocks::PistonHeadBlock::Type::Normal;
    if (name == "sticky") return blocks::PistonHeadBlock::Type::Sticky;
    return std::nullopt;
}

namespace blocks {

// 活塞头类型属性 - 使用静态函数返回引用
const EnumProperty<PistonHeadBlock::Type>& TYPE_PROP()
{
    static auto prop = EnumProperty<PistonHeadBlock::Type>::create(
        "type", {PistonHeadBlock::Type::Normal, PistonHeadBlock::Type::Sticky});
    return *prop;
}

// 静态方法实现 - 返回类型属性
const EnumProperty<PistonHeadBlock::Type>& PistonHeadBlock::getTypeProperty() noexcept
{
    return TYPE_PROP();
}

PistonHeadBlock::PistonHeadBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    // vanilla 1.21.11 piston_head 属性：facing + type + short
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::FACING())
            .add(BlockStateProperties::SHORT())
            .add(TYPE_PROP())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::FACING(), Direction::North)
            .with(BlockStateProperties::SHORT(), false)
            .with(TYPE_PROP(), Type::Normal));
}

Direction PistonHeadBlock::getFacing(const BlockState& state) noexcept
{
    return state.get(BlockStateProperties::FACING());
}

PistonHeadBlock::Type PistonHeadBlock::getType(const BlockState& state) noexcept
{
    return state.get(TYPE_PROP());
}

BlockState PistonHeadBlock::withType(BlockState state, Type type) noexcept
{
    return state.with(TYPE_PROP(), type);
}

bool PistonHeadBlock::isFittingBase(const BlockState& headState, const BlockState& baseState) noexcept
{
    // 根据活塞头类型判断应该对应哪种活塞
    Block* expectedPiston = (getType(headState) == Type::Sticky) ? VanillaBlocks::STICKY_PISTON : VanillaBlocks::PISTON;

    // 三个条件必须全部满足：
    // 1. 类型匹配：邻居方块是对应类型的活塞
    // 2. 已伸出：活塞处于 EXTENDED=true 状态
    // 3. 朝向一致：活塞的 FACING 与活塞头的 FACING 相同
    return baseState.is(expectedPiston) && baseState.get(BlockStateProperties::EXTENDED()) &&
        baseState.get(BlockStateProperties::FACING()) == getFacing(headState);
}

BlockState PistonHeadBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 检查更新是否来自活塞头朝向的反方向（即活塞主体方向）
    // 活塞主体位于活塞头朝向的反方向一格
    if (Directions::opposite(facing) == getFacing(state)) {
        // 更新来自活塞主体方向，检查活塞头是否仍能存活
        if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
            // 活塞主体不存在或未伸出，活塞头应消失
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

bool PistonHeadBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 活塞头能存活的条件：
    // 1. 反方向（朝活塞基座方向）有匹配的已伸出活塞基座（isFittingBase 返回 true）
    // 2. 或者反方向是方向匹配的 MOVING_PISTON（正在移动的活塞方块）
    Direction facing = getFacing(state);
    BlockPos basePos = pos.offset(Directions::opposite(facing));
    const BlockState* baseState = world.getBlockState(basePos);

    if (baseState == nullptr) {
        return false;
    }

    // 条件1：匹配的已伸出活塞基座
    if (isFittingBase(state, *baseState)) {
        return true;
    }

    // 条件2：方向匹配的 MOVING_PISTON
    if (baseState->is(VanillaBlocks::MOVING_PISTON) && baseState->get(BlockStateProperties::FACING()) == facing) {
        return true;
    }

    return false;
}

void PistonHeadBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    Block::neighborChanged(world, pos, neighborBlock, neighborPos, isMoving);

    // 如果活塞头自身仍能存活，将邻居变化通知转发到活塞主体方向
    // 这确保了活塞头区域的红石变化能传导到活塞基座
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState != nullptr && isValidPosition(*currentState, static_cast<IBlockReader&>(world), pos)) {
        Direction facing = getFacing(*currentState);
        BlockPos basePos = pos.offset(Directions::opposite(facing));
        const BlockState* baseState = world.getBlockState(basePos);
        if (baseState != nullptr && !baseState->isAir()) {
            Block& baseBlock = baseState->getBlockMutable();
            baseBlock.neighborChanged(world, basePos, neighborBlock, neighborPos, isMoving);
        }
    }
}

void PistonHeadBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    Block::onBlockRemoved(world, pos, state);

    // 活塞头被移除时，检查反方向是否有匹配的已伸出活塞基座
    // 如果有，级联销毁活塞基座并产生掉落物
    // 注意：当玩家在创造模式下破坏活塞头时，playerWillDestroy 已先将基座设为空气，
    // 因此此处 isFittingBase 检查会失败，不会重复销毁
    Direction facing = getFacing(state);
    BlockPos basePos = pos.offset(Directions::opposite(facing));
    const BlockState* baseState = world.getBlockState(basePos);

    if (baseState != nullptr && isFittingBase(state, *baseState)) {
        // 生成活塞基座的掉落物品
        const Block& baseBlock = baseState->getBlock();
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(baseBlock);
        if (blockItem != nullptr) {
            ItemStack dropStack(blockItem, 1);
            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world,
                dropStack,
                static_cast<f64>(basePos.x) + 0.5,
                static_cast<f64>(basePos.y) + 0.5,
                static_cast<f64>(basePos.z) + 0.5,
                rng);
        }

        // 将活塞基座设置为空气
        if (auto* airState = BlockRegistry::instance().airState()) {
            world.setBlockState(basePos, airState);
        }

        // 调用活塞基座的 spawnAfterBreak（如蠹虫生成等特殊逻辑）
        baseBlock.spawnAfterBreak(world, basePos, *baseState, nullptr, false);
    }
}

void PistonHeadBlock::playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player)
{
    // 创造模式下，破坏活塞头时应同时销毁匹配的活塞基座且不产生掉落物
    // 生存模式下，掉落物由 onBlockRemoved 中的一般逻辑处理
    if (!player.isCreative()) {
        return;
    }

    Direction facing = getFacing(state);
    BlockPos basePos = pos.offset(Directions::opposite(facing));
    const BlockState* baseState = world.getBlockState(basePos);

    if (baseState != nullptr && isFittingBase(state, *baseState)) {
        // 创造模式：销毁活塞基座但不产生掉落物
        if (auto* airState = BlockRegistry::instance().airState()) {
            world.setBlockState(basePos, airState);
        }
    }
}

} // namespace blocks
} // namespace mc
