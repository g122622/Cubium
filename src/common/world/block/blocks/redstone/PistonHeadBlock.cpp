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
#include "common/util/Direction.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/redstone/MovingPistonBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <array>
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
    // vanilla piston_head/moving_piston 的 type 取值为 "normal"/"sticky"
    switch (value) {
        case blocks::PistonHeadBlock::Type::Normal:
            return "normal";
        case blocks::PistonHeadBlock::Type::Sticky:
            return "sticky";
        default:
            return "normal";
    }
}

template <>
std::optional<blocks::PistonHeadBlock::Type> EnumProperty<blocks::PistonHeadBlock::Type>::Traits::fromName(
    std::string_view name)
{
    if (name == "normal") {
        return blocks::PistonHeadBlock::Type::Normal;
    }
    if (name == "sticky") {
        return blocks::PistonHeadBlock::Type::Sticky;
    }
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
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

const CollisionShape& PistonHeadBlock::getShape(const BlockState& state) const
{
    // vanilla PistonHeadBlock：
    //   SHAPE_PLATFORM = boxZ(16.0, 0.0, 4.0)  = box(0, 0, 0, 16, 16, 4)
    //   SHAPES_SHORT   = SHAPE_PLATFORM | boxZ(4, 4, 16) = platform | box(6, 6, 4, 10, 10, 16)
    //   SHAPES         = SHAPE_PLATFORM | boxZ(4, 4, 20) = platform | box(6, 6, 4, 10, 10, 20)
    // 基准姿态朝 NORTH（平台贴在 +z 端、细杆向 -z 伸出），再按 FACING 旋转。
    // 平台 + 细杆的并集不覆盖整格，所以天空光可穿过（lightBlock=0）。早期未重写 getShape，
    // 退回整格立方体，活塞头被误判为完全挡光（lightBlock=15）。
    static const std::array<std::array<CollisionShape, 6>, 2> shapes = {
        std::array<CollisionShape, 6>{
            // 短臂
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 12, 0, 16, 16, 16),
                CollisionShape::fromPixelBox(6, 0, 6, 10, 12, 10)), // Down
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 0, 16, 4, 16),
                CollisionShape::fromPixelBox(6, 4, 6, 10, 16, 10)), // Up
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 4),
                CollisionShape::fromPixelBox(6, 6, 0, 10, 10, 12)), // North
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 12, 16, 16, 16),
                CollisionShape::fromPixelBox(6, 6, 4, 10, 10, 16)), // South
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 0, 4, 16, 16),
                CollisionShape::fromPixelBox(4, 6, 6, 16, 10, 10)), // West
            CollisionShape::combine(CollisionShape::fromPixelBox(12, 0, 0, 16, 16, 16),
                CollisionShape::fromPixelBox(0, 6, 6, 12, 10, 10)), // East
        },
        std::array<CollisionShape, 6>{
            // 长臂（细杆超出方块本地范围，与原版一致）
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 12, 0, 16, 16, 16),
                CollisionShape::fromPixelBox(6, -4, 6, 10, 12, 10)), // Down
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 0, 16, 4, 16),
                CollisionShape::fromPixelBox(6, 4, 6, 10, 20, 10)), // Up
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 4),
                CollisionShape::fromPixelBox(6, 6, -4, 10, 10, 12)), // North
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 12, 16, 16, 16),
                CollisionShape::fromPixelBox(6, 6, 4, 10, 10, 20)), // South
            CollisionShape::combine(CollisionShape::fromPixelBox(0, 0, 0, 4, 16, 16),
                CollisionShape::fromPixelBox(4, 6, 6, 20, 10, 10)), // West
            CollisionShape::combine(CollisionShape::fromPixelBox(12, 0, 0, 16, 16, 16),
                CollisionShape::fromPixelBox(-4, 6, 6, 12, 10, 10)), // East
        },
    };

    const bool shortArm = state.get(BlockStateProperties::SHORT());
    return shapes[shortArm ? 0 : 1][static_cast<size_t>(getFacing(state))];
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

void PistonHeadBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    Block::onBlockRemoved(world, pos, state, movedByPiston);

    // 活塞头被移除时，检查反方向是否有匹配的已伸出活塞基座
    // 如果有，级联销毁活塞基座并产生掉落物
    // 注意：当玩家在创造模式下破坏活塞头时，playerWillDestroy 已先将基座设为空气，
    // 因此此处 isFittingBase 检查会失败，不会重复销毁
    Direction facing = getFacing(state);
    BlockPos basePos = pos.offset(Directions::opposite(facing));
    const BlockState* baseState = world.getBlockState(basePos);

    if (baseState != nullptr && isFittingBase(state, *baseState)) {
        // 原版 affectNeighborsAfterRemoval 使用 serverLevel.destroyBlock(basePos, true)：
        // 掉落表、破坏粒子与 BLOCK_DESTROY 事件由统一接口处理。
        world.destroyBlock(basePos, true);
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
        // 创造模式：销毁活塞基座但不产生掉落物（原版 playerWillDestroy 的 destroyBlock(basePos, false)）
        world.destroyBlock(basePos, false);
    }
}

} // namespace blocks
} // namespace mc
