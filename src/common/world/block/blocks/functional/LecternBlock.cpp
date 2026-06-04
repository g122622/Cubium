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

#include "LecternBlock.hpp"

#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/utils/ItemDropHelper.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/Item.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntityType.hpp"
#include "../../../blockentity/interactive/LecternEntity.hpp"
#include "../../../tick/manager/TickManager.hpp"

namespace mc {
namespace blocks {

namespace {

/**
 * @brief 判断物品是否可放入讲台。
 * @param itemId 物品ID。
 * @return true 表示支持放入。
 */
[[nodiscard]] bool isLecternBookItem(ItemId itemId)
{
    const Item* item = Item::getItem(itemId);
    if (item == nullptr) {
        return false;
    }

    const std::string& path = item->itemLocation().path();
    return path == "book" || path == "written_book" || path == "writable_book" || path == "enchanted_book";
}

} // namespace

LecternBlock::LecternBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::HAS_BOOK())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::HAS_BOOK(), false));

    constexpr f32 p = 1.0f / 16.0f;

    const CollisionShape base = CollisionShape::box(0.0f, 0.0f, 0.0f, 16.0f * p, 2.0f * p, 16.0f * p);
    const CollisionShape post = CollisionShape::box(4.0f * p, 2.0f * p, 4.0f * p, 12.0f * p, 14.0f * p, 12.0f * p);
    const CollisionShape top = CollisionShape::box(0.0f, 15.0f * p, 0.0f, 16.0f * p, 15.0f * p, 16.0f * p);

    m_collisionShape = CollisionShape::combine(CollisionShape::combine(base, post), top);

    const CollisionShape slopeN = CollisionShape::box(1.0f * p, 10.0f * p, 0.0f, 14.0f * p, 18.0f * p, 16.0f * p);
    m_shapesByFacing[static_cast<size_t>(Direction::North)] = CollisionShape::combine(m_collisionShape, slopeN);

    const CollisionShape slopeS = CollisionShape::box(1.0f * p, 10.0f * p, 0.0f, 14.0f * p, 18.0f * p, 16.0f * p);
    m_shapesByFacing[static_cast<size_t>(Direction::South)] = CollisionShape::combine(m_collisionShape, slopeS);

    const CollisionShape slopeW = CollisionShape::box(0.0f, 10.0f * p, 1.0f * p, 16.0f * p, 18.0f * p, 14.0f * p);
    m_shapesByFacing[static_cast<size_t>(Direction::West)] = CollisionShape::combine(m_collisionShape, slopeW);

    const CollisionShape slopeE = CollisionShape::box(0.0f, 10.0f * p, 1.0f * p, 16.0f * p, 18.0f * p, 14.0f * p);
    m_shapesByFacing[static_cast<size_t>(Direction::East)] = CollisionShape::combine(m_collisionShape, slopeE);
}

BlockState LecternBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing));
}

const BlockState& LecternBlock::rotate(const BlockState& state, Rotation rotation) const
{
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& LecternBlock::mirror(const BlockState& state, Mirror mirror) const
{
    if (mirror == Mirror::None) {
        return state;
    }

    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

void LecternBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    if (!state.get(BlockStateProperties::POWERED())) {
        return;
    }

    state = state.with(BlockStateProperties::POWERED(), false);
    world.setBlockState(pos, &state, 3);
}

const CollisionShape& LecternBlock::getShape(const BlockState& state) const
{
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

const CollisionShape& LecternBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_collisionShape;
}

i32 LecternBlock::getWeakPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return state.get(BlockStateProperties::POWERED()) ? 15 : 0;
}

i32 LecternBlock::getStrongPower(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    if (side == Direction::Up && state.get(BlockStateProperties::POWERED())) {
        return 15;
    }
    return 0;
}

i32 LecternBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    if (!state.get(BlockStateProperties::HAS_BOOK())) {
        return 0;
    }

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Lectern) {
        return static_cast<blockentity::LecternEntity*>(blockEntity)->getComparatorSignal();
    }

    return 1;
}

std::unique_ptr<BlockEntity> LecternBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::LecternEntity>(pos);
}

ActionResultType LecternBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    if (state.get(BlockStateProperties::HAS_BOOK())) {
        if (!world.isClientSide()) {
            // 打开讲台GUI
            // TODO: 实现容器打开
            // player.openContainer(lecternEntity);
            // player.addStat(Stats::INTERACT_WITH_LECTERN);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

void LecternBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 如果有书，掉落书本
    if (state.get(BlockStateProperties::HAS_BOOK())) {
        _dropBook(world, pos, state);
    }

    // 如果处于激活状态，通知下方方块更新红石
    if (state.get(BlockStateProperties::POWERED())) {
        // TODO: 实现红石更新
        // world.notifyNeighborsOfStateChange(pos.down(), this);
    }

    Block::onBlockRemoved(world, pos, state);
}

void LecternBlock::_dropBook(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Lectern) {
        auto* lectern = static_cast<blockentity::LecternEntity*>(entity);
        ItemStack book = lectern->removeBook();

        if (!book.isEmpty()) {
            // 根据朝向计算掉落位置
            Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
            f32 offsetX = 0.5f + 0.25f * static_cast<f32>(Directions::xOffset(facing));
            f32 offsetZ = 0.5f + 0.25f * static_cast<f32>(Directions::zOffset(facing));

            math::Random rng;
            ItemDropHelper::spawnItemEntity(&world, book, pos.x + offsetX, pos.y + 1.0, pos.z + offsetZ, rng);
        }
    }
}

bool LecternBlock::tryPlaceBook(IWorld& world, const BlockPos& pos, BlockState& state, u32 itemId)
{
    if (state.get(BlockStateProperties::HAS_BOOK())) {
        return false;
    }

    if (!isLecternBookItem(static_cast<ItemId>(itemId))) {
        return false;
    }

    setHasBook(world, pos, state, true);
    return true;
}

void LecternBlock::setHasBook(IWorld& world, const BlockPos& pos, BlockState& state, bool hasBook)
{
    state = state.with(BlockStateProperties::HAS_BOOK(), hasBook).with(BlockStateProperties::POWERED(), false);
    world.setBlockState(pos, &state, 3);

    if (BlockEntity* blockEntity = world.getBlockEntity(pos);
        blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Lectern) {
        static_cast<blockentity::LecternEntity*>(blockEntity)->setChanged();
    }
}

void LecternBlock::pulse(IWorld& world, const BlockPos& pos, BlockState& state)
{
    state = state.with(BlockStateProperties::POWERED(), true);
    world.setBlockState(pos, &state, 3);
    world.tickManager().scheduleBlockTick(
        pos, const_cast<Block&>(state.getBlock()), 2, world::tick::TickPriority::High);
}

} // namespace blocks
} // namespace mc