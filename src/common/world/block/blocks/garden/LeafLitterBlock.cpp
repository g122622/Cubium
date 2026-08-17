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

#include "LeafLitterBlock.hpp"

#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "common/core/Types.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

LeafLitterBlock::LeafLitterBlock(const BlockProperties& properties)
    : BushBlock(properties)
{
    // 创建状态容器：FACING + SEGMENT_AMOUNT 两个属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(SEGMENT_AMOUNT())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认状态：朝北、1段
    setDefaultState(defaultState().with(FACING(), Direction::North).with(SEGMENT_AMOUNT(), 1));

    _initShapes();
}

// ========== 放置逻辑 ==========

BlockState LeafLitterBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查目标位置是否已有同类型枯叶（堆叠逻辑）
    const BlockState* existingState = world.getBlockState(pos);
    if (existingState != nullptr && existingState->is(this)) {
        i32 currentAmount = existingState->get(SEGMENT_AMOUNT());
        if (currentAmount < 4) {
            // 增加枯叶段数，保持原朝向
            return existingState->with(SEGMENT_AMOUNT(), currentAmount + 1);
        }
        // 已满4段，无法继续堆叠
        return *existingState;
    }

    // 新放置：朝向为玩家水平朝向的反方向
    Direction facing = context.horizontalDirection();
    Direction oppositeFacing = Directions::opposite(facing);
    return defaultState().with(FACING(), oppositeFacing).with(SEGMENT_AMOUNT(), 1);
}

bool LeafLitterBlock::isReplaceable(const BlockState& state, const BlockItemUseContext& context) const
{
    // 条件1：玩家未潜行
    Player* player = context.getPlayer();
    if (player != nullptr && player->isSneaking()) {
        // 潜行时不堆叠，但仍然可被其他方块替换（回退到基类行为）
        return BushBlock::isReplaceable(state, context);
    }

    // 条件2：手持物品必须是此方块对应的物品
    const ItemStack& heldItem = context.getItemStack();
    if (heldItem.isEmpty()) {
        // 空手不能堆叠，但仍然可被其他方块替换
        return BushBlock::isReplaceable(state, context);
    }

    // 条件3：通过 BlockItemRegistry 检查手持物品是否为此方块的 BlockItem
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*this);
    if (blockItem == nullptr || heldItem.getItem() != blockItem) {
        // 手持非同类型物品不能堆叠，但仍然可被其他方块替换
        return BushBlock::isReplaceable(state, context);
    }

    // 条件4：当前 SEGMENT_AMOUNT < 4 时允许堆叠替换
    return state.get(SEGMENT_AMOUNT()) < 4;
}

bool LeafLitterBlock::canSustain(const BlockState& groundState, IWorld& world, const BlockPos& groundPos) const
{
    // 下方方块顶面支撑形状完整即可放置枯叶（草方块/泥土/石头/沙子/陶瓦/玻璃等顶面完整的方块）。
    // 不沿用 BushBlock 默认的 DIRT 标签判定（那只允许泥土类），改用顶面完整判定与原版行为一致。
    return groundState.isFaceSturdy(world, groundPos, Direction::Up, SupportType::Full);
}

// ========== 旋转/镜像 ==========

const BlockState& LeafLitterBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction currentFacing = _getFacing(state);
    Direction newFacing = currentFacing;

    switch (rotation) {
        case Rotation::Clockwise90:
            newFacing = Directions::rotateY(currentFacing);
            break;
        case Rotation::Clockwise180:
            newFacing = Directions::opposite(currentFacing);
            break;
        case Rotation::CounterClockwise90:
            newFacing = Directions::rotateYCCW(currentFacing);
            break;
        case Rotation::None:
        default:
            break;
    }

    return state.with(FACING(), newFacing);
}

const BlockState& LeafLitterBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction currentFacing = _getFacing(state);
    Direction newFacing = currentFacing;

    switch (mirror) {
        case Mirror::LeftRight:
            if (currentFacing == Direction::East) {
                newFacing = Direction::West;
            } else if (currentFacing == Direction::West) {
                newFacing = Direction::East;
            }
            break;
        case Mirror::FrontBack:
            if (currentFacing == Direction::North) {
                newFacing = Direction::South;
            } else if (currentFacing == Direction::South) {
                newFacing = Direction::North;
            }
            break;
        case Mirror::None:
        default:
            break;
    }

    return state.with(FACING(), newFacing);
}

// ========== 形状 ==========

const CollisionShape& LeafLitterBlock::getShape(const BlockState& state) const
{
    Direction facing = _getFacing(state);
    i32 amount = _getAmount(state);

    // 将 (facing, amount) 映射到索引
    i32 facingIndex = 0;
    switch (facing) {
        case Direction::North:
            facingIndex = 0;
            break;
        case Direction::East:
            facingIndex = 1;
            break;
        case Direction::South:
            facingIndex = 2;
            break;
        case Direction::West:
            facingIndex = 3;
            break;
        default:
            facingIndex = 0;
            break;
    }

    i32 index = facingIndex * 4 + (amount - 1);
    MC_ASSERT_DEBUG(index >= 0 && index < static_cast<i32>(m_shapes.size()));
    return m_shapes[index];
}

// ========== 私有方法 ==========

void LeafLitterBlock::_initShapes()
{
    // 预计算所有 4x4=16 种形状
    m_shapes.resize(16);

    static constexpr Direction FACINGS[4] = {
        Direction::North,
        Direction::East,
        Direction::South,
        Direction::West,
    };

    for (i32 facingIdx = 0; facingIdx < 4; ++facingIdx) {
        for (i32 amount = 1; amount <= 4; ++amount) {
            i32 index = facingIdx * 4 + (amount - 1);
            m_shapes[index] = _calculateShape(FACINGS[facingIdx], amount);
        }
    }
}

CollisionShape LeafLitterBlock::_calculateShape(Direction facing, i32 amount)
{
    // 枯叶形状：每个枯叶段为 8x1x8 像素盒子（高度 1/16）
    // 基础盒子 box(0,0,0, 8, height, 8) 旋转水平方向后得到各朝向的段0，
    // 然后逆时针旋转90度叠加段1、段2、段3。
    static constexpr f32 HEIGHT = 1.0f / 16.0f;

    struct Box {
        f32 x1, y1, z1, x2, y2, z2;
    };

    // 各朝向的段位置（通过旋转 NORTH 的段坐标得到）
    static constexpr Box SEGMENTS[4][4] = {
        // North: 段0(NW), 段1(SW), 段2(SE), 段3(NE)
        {{0.0f, 0.0f, 0.0f, 0.5f, HEIGHT, 0.5f},
            {0.0f, 0.0f, 0.5f, 0.5f, HEIGHT, 1.0f},
            {0.5f, 0.0f, 0.5f, 1.0f, HEIGHT, 1.0f},
            {0.5f, 0.0f, 0.0f, 1.0f, HEIGHT, 0.5f}},
        // East: 段0(NE), 段1(NW), 段2(SW), 段3(SE)
        {{0.5f, 0.0f, 0.0f, 1.0f, HEIGHT, 0.5f},
            {0.0f, 0.0f, 0.0f, 0.5f, HEIGHT, 0.5f},
            {0.0f, 0.0f, 0.5f, 0.5f, HEIGHT, 1.0f},
            {0.5f, 0.0f, 0.5f, 1.0f, HEIGHT, 1.0f}},
        // South: 段0(SE), 段1(NE), 段2(NW), 段3(SW)
        {{0.5f, 0.0f, 0.5f, 1.0f, HEIGHT, 1.0f},
            {0.5f, 0.0f, 0.0f, 1.0f, HEIGHT, 0.5f},
            {0.0f, 0.0f, 0.0f, 0.5f, HEIGHT, 0.5f},
            {0.0f, 0.0f, 0.5f, 0.5f, HEIGHT, 1.0f}},
        // West: 段0(SW), 段1(SE), 段2(NE), 段3(NW)
        {{0.0f, 0.0f, 0.5f, 0.5f, HEIGHT, 1.0f},
            {0.5f, 0.0f, 0.5f, 1.0f, HEIGHT, 1.0f},
            {0.5f, 0.0f, 0.0f, 1.0f, HEIGHT, 0.5f},
            {0.0f, 0.0f, 0.0f, 0.5f, HEIGHT, 0.5f}},
    };

    i32 facingIdx = 0;
    switch (facing) {
        case Direction::North:
            facingIdx = 0;
            break;
        case Direction::East:
            facingIdx = 1;
            break;
        case Direction::South:
            facingIdx = 2;
            break;
        case Direction::West:
            facingIdx = 3;
            break;
        default:
            facingIdx = 0;
            break;
    }

    // 构建形状：叠加各个段的盒子
    CollisionShape shape = CollisionShape::box(SEGMENTS[facingIdx][0].x1,
        SEGMENTS[facingIdx][0].y1,
        SEGMENTS[facingIdx][0].z1,
        SEGMENTS[facingIdx][0].x2,
        SEGMENTS[facingIdx][0].y2,
        SEGMENTS[facingIdx][0].z2);

    for (i32 i = 1; i < amount; ++i) {
        const Box& seg = SEGMENTS[facingIdx][i];
        shape.addBox(seg.x1, seg.y1, seg.z1, seg.x2, seg.y2, seg.z2);
    }

    return shape;
}

i32 LeafLitterBlock::_getAmount(const BlockState& state) const
{
    return state.get(SEGMENT_AMOUNT());
}

Direction LeafLitterBlock::_getFacing(const BlockState& state) const
{
    return state.get(FACING());
}

} // namespace blocks
} // namespace mc
