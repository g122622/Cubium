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
#include "common/world/block/BlockUpdateFlags.hpp"
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

/**
 * @brief 水平朝向在形状表中的槽位。
 *
 * `Direction` 枚举里水平方向并不连续（North=2、South=3、West=4、East=5），
 * 直接拿枚举值当下标会越界或读到未初始化的槽位，因此显式映射到 0..3。
 */
[[nodiscard]] size_t _facingSlot(Direction facing)
{
    switch (facing) {
        case Direction::North:
            return 0;
        case Direction::East:
            return 1;
        case Direction::South:
            return 2;
        case Direction::West:
            return 3;
        default:
            MC_ASSERT_RELEASE_MSG(false, "LecternBlock: facing must be horizontal");
            return 0;
    }
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
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::HAS_BOOK(), false));

    // 原版（LecternBlock）：
    //   SHAPE_COLLISION = column(16.0, 0.0, 2.0) ∪ column(8.0, 2.0, 14.0)
    //   SHAPES = rotateHorizontal(boxZ(16.0, 10.0, 1.0, 5.333333)
    //                            ∪ boxZ(16.0, 12.0, 5.333333, 9.666667)
    //                            ∪ boxZ(16.0, 14.0, 9.666667, 14.0)
    //                            ∪ SHAPE_COLLISION)
    // column(w, y1, y2)：横截面 w×w 像素居中，y 从 y1 到 y2。
    // boxZ(width, height, y1, y2)：x 居中宽 width、**竖直区间为 y1..y2**、z 居中高 height。
    // 三条斜台横条因此是：y 1..10 / z 5.333..14、y 5.333..16 / z 9.667..16、y 9.667..18 / z 14..18。
    const CollisionShape base = CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 2.0f, 16.0f);
    const CollisionShape post = CollisionShape::fromPixelBox(4.0f, 2.0f, 4.0f, 12.0f, 14.0f, 12.0f);
    m_collisionShape = CollisionShape::combine(base, post);

    // 斜台由三级台阶叠成：x 方向整格宽，每级 y 区间固定、z 区间随台阶前移。
    const CollisionShape slope =
        CollisionShape::combine(CollisionShape::combine(CollisionShape::fromPixelBox(0.0f, 1.0f, 5.333333f, 16.0f, 14.0f, 10.0f),
                                   CollisionShape::fromPixelBox(0.0f, 5.333333f, 9.666667f, 16.0f, 16.0f, 12.0f)),
            CollisionShape::fromPixelBox(0.0f, 9.666667f, 14.0f, 16.0f, 18.0f, 14.0f));

    // 其余朝向由绕 Y 轴旋转 90/180/270 度得到。原版使用 OctahedralGroup.BLOCK_ROT_Y_90，
    // 它对体素下标的作用是 (x, z) → (z, 1 - x)，而不是常见的 (1 - z, x)；
    // 盒子的坐标变换同样按该映射进行（另加 +1 的索引偏移由取 min/max 抵消）。
    const auto rotateY = [](const CollisionShape& shape, i32 quarterTurns) -> CollisionShape {
        CollisionShape result = CollisionShape::empty();
        for (const auto& b : shape.boxes()) {
            f32 x0 = b.minX;
            f32 x1 = b.maxX;
            f32 z0 = b.minZ;
            f32 z1 = b.maxZ;
            for (i32 i = 0; i < quarterTurns; ++i) {
                // (x, z) → (z, 1 - x)
                const f32 nx0 = z0;
                const f32 nx1 = z1;
                const f32 nz0 = 1.0f - x1;
                const f32 nz1 = 1.0f - x0;
                x0 = nx0;
                x1 = nx1;
                z0 = nz0;
                z1 = nz1;
            }
            result.addBox(x0, b.minY, z0, x1, b.maxY, z1);
        }
        return result;
    };

    const CollisionShape northShape = CollisionShape::combine(m_collisionShape, slope);
    m_shapesByFacing[_facingSlot(Direction::North)] = northShape;
    m_shapesByFacing[_facingSlot(Direction::East)] = rotateY(northShape, 1);
    m_shapesByFacing[_facingSlot(Direction::South)] = rotateY(northShape, 2);
    m_shapesByFacing[_facingSlot(Direction::West)] = rotateY(northShape, 3);
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

    return m_shapesByFacing[_facingSlot(facing)];
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

BlockActionResult LecternBlock::useItemOn(ItemStack& stack,
    const BlockState& state,
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
        ItemStack& heldItem = stack;
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

    return ActionResultType::TryEmptyHandInteraction;
}

BlockActionResult LecternBlock::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    // 本方块无空手交互分支（对齐 vanilla 未重写 useWithoutItem 的方块）
    (void)state;
    (void)world;
    (void)pos;
    (void)player;
    (void)hit;
    return ActionResultType::Pass;
}

void LecternBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 书本掉落已迁至 LecternEntity::preRemoveSideEffects（对齐 vanilla LecternBlockEntity），
    // 由 ServerWorld::setBlockState 在方块实体被丢弃前调用，并受
    // UPDATE_SKIP_BLOCK_ENTITY_SIDEEFFECTS 门控——此处不再负责掉落，避免重复。
    // 如果处于激活状态，通知下方方块更新红石
    if (state.get(BlockStateProperties::POWERED())) {
        updateBelow(world, pos, state.getBlockMutable());
    }

    Block::onBlockRemoved(world, pos, state, movedByPiston);
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
        // 物品消耗由 useItemOn 中的 heldItem.shrink(1) 处理
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
    world.setBlockState(pos, &updated, world::BlockUpdateFlags::UPDATE_ALL);

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
    world.setBlockState(pos, &updated, world::BlockUpdateFlags::UPDATE_ALL);
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
