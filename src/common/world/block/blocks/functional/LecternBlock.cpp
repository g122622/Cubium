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

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/stats/Stats.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/LecternEntity.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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

    // 脉冲到期，将 POWERED 设为 false 并通知下方方块红石更新
    changePowered(world, pos, state, false);
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

i32 LecternBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return state.get(BlockStateProperties::POWERED()) ? 15 : 0;
}

i32 LecternBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
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

BlockActionResult LecternBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    if (state.get(BlockStateProperties::HAS_BOOK())) {
        // 讲台上已有书，打开讲台GUI
        if (!world.isClientSide()) {
            BlockEntity* entity = world.getBlockEntity(pos);
            if (entity != nullptr && entity->getType() == BlockEntityType::Lectern) {
                auto* lectern = static_cast<blockentity::LecternEntity*>(entity);
                if (world.openContainer(ContainerType::Lectern, pos, player)) {
                    lectern->openContainer();
                    player.awardCustomStat(ResourceLocation(stats::INTERACT_WITH_LECTERN), 1);
                }
            }
        }
        return ActionResultType::Success;
    }

    // 讲台上没有书，检查玩家手中是否持有可放入讲台的书籍
    if (!world.isClientSide()) {
        ItemStack& heldItem = player.getHeldItem(hand);
        if (!heldItem.isEmpty()) {
            const Item* item = heldItem.getItem();
            if (item != nullptr && isLecternBookItem(item->itemId())) {
                if (tryPlaceBook(world, pos, heldItem)) {
                    // tryPlaceBook 已将书放入讲台实体，这里消耗玩家手中的一个物品
                    heldItem.shrink(1);
                    // 播放放书音效
                    world.playSound(ResourceLocation("minecraft:item.book.put"),
                        sound::SoundCategory::Blocks,
                        pos.center(),
                        1.0f,
                        1.0f);
                    return ActionResultType::Success;
                }
            }
        }
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
        updateBelow(world, pos, state.getBlockMutable());
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

bool LecternBlock::tryPlaceBook(IWorld& world, const BlockPos& pos, const ItemStack& bookStack)
{
    // 从世界获取当前方块状态
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr || currentState->get(BlockStateProperties::HAS_BOOK())) {
        return false;
    }

    if (bookStack.isEmpty()) {
        return false;
    }

    const Item* item = bookStack.getItem();
    if (item == nullptr || !isLecternBookItem(item->itemId())) {
        return false;
    }

    // 将书本设置到讲台实体中，然后更新方块状态
    BlockEntity* entity = world.getBlockEntity(pos);
    if (entity != nullptr && entity->getType() == BlockEntityType::Lectern) {
        auto* lectern = static_cast<blockentity::LecternEntity*>(entity);
        // 将书的一份副本放入讲台（数量为1）
        ItemStack bookForLectern = bookStack.copy();
        bookForLectern.setCount(1);
        if (!lectern->setBook(bookForLectern)) {
            return false;
        }
        // 物品消耗由 onBlockActivated 中的 heldItem.shrink(1) 处理
    }

    setHasBook(world, pos, true);
    return true;
}

void LecternBlock::setHasBook(IWorld& world, const BlockPos& pos, bool hasBook)
{
    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr) {
        return;
    }

    BlockState updated =
        currentState->with(BlockStateProperties::HAS_BOOK(), hasBook).with(BlockStateProperties::POWERED(), false);
    world.setBlockState(pos, &updated, 3);

    // 通知下方方块红石更新（POWERED 状态可能改变）
    updateBelow(world, pos, updated.getBlockMutable());

    if (BlockEntity* blockEntity = world.getBlockEntity(pos);
        blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Lectern) {
        static_cast<blockentity::LecternEntity*>(blockEntity)->setChanged();
    }
}

void LecternBlock::pulse(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    changePowered(world, pos, state, true);
    world.tickManager().scheduleBlockTick(pos, state.getBlockMutable(), 2, world::tick::TickPriority::High);
}

void LecternBlock::changePowered(IWorld& world, const BlockPos& pos, const BlockState& state, bool powered)
{
    BlockState updated = state.with(BlockStateProperties::POWERED(), powered);
    world.setBlockState(pos, &updated, 3);
    updateBelow(world, pos, state.getBlockMutable());
}

void LecternBlock::updateBelow(IWorld& world, const BlockPos& pos, Block& block)
{
    // 讲台向所有方向输出弱信号，仅在上方输出强信号，
    // 红石更新只需通知正下方位置的方块即可，
    // 因为 setBlockState 已经会通知6个方向的邻居执行 updatePostPlacement 和 neighborChanged，
    // 但下方方块作为强信号接收者需要额外的红石更新通知。
    world.updateNeighbors(pos.down(), block);
}

} // namespace blocks
} // namespace mc