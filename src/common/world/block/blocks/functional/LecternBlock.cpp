#include "LecternBlock.hpp"

#include "../../../IWorld.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../../blockentity/interactive/LecternEntity.hpp"
#include "../../../../item/core/Item.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

namespace {

/**
 * @brief 判断物品是否可放入讲台。
 * @param itemId 物品ID。
 * @return true 表示支持放入。
 */
[[nodiscard]] bool isLecternBookItem(ItemId itemId) {
    const Item* item = Item::getItem(itemId);
    if (item == nullptr) {
        return false;
    }

    const String& path = item->itemLocation().path();
    return path == "book" ||
           path == "written_book" ||
           path == "writable_book" ||
           path == "enchanted_book";
}

} // namespace

LecternBlock::LecternBlock(const BlockProperties& properties)
    : Block(properties) {
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::HORIZONTAL_FACING())
        .add(BlockStateProperties::POWERED())
        .add(BlockStateProperties::HAS_BOOK())
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
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

BlockState LecternBlock::getStateForPlacement(BlockItemUseContext& context) {
    const Direction facing = context.horizontalDirection();
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(facing));
}

const BlockState& LecternBlock::rotate(const BlockState& state, Rotation rotation) const {
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& LecternBlock::mirror(const BlockState& state, Mirror mirror) const {
    if (mirror == Mirror::None) {
        return state;
    }

    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

void LecternBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state) {
    if (!state.get(BlockStateProperties::POWERED())) {
        return;
    }

    state = state.with(BlockStateProperties::POWERED(), false);
    world.setBlockState(pos, &state, 3);
}

const CollisionShape& LecternBlock::getShape(const BlockState& state) const {
    const Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    const size_t index = static_cast<size_t>(facing);
    MC_ASSERT(index < Directions::COUNT && Directions::isHorizontal(facing));
    return m_shapesByFacing[index];
}

const CollisionShape& LecternBlock::getCollisionShape(const BlockState& state) const {
    MC_UNUSED(state);
    return m_collisionShape;
}

i32 LecternBlock::getWeakPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    return state.get(BlockStateProperties::POWERED()) ? 15 : 0;
}

i32 LecternBlock::getStrongPower(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction side) const {
    MC_UNUSED(world);
    MC_UNUSED(pos);

    if (side == Direction::Up && state.get(BlockStateProperties::POWERED())) {
        return 15;
    }
    return 0;
}

int LecternBlock::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {
    if (!state.get(BlockStateProperties::HAS_BOOK())) {
        return 0;
    }

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Lectern) {
        return static_cast<blockentity::LecternEntity*>(blockEntity)->getComparatorSignal();
    }

    return 1;
}

bool LecternBlock::tryPlaceBook(IWorld& world, const BlockPos& pos, BlockState& state, u32 itemId) {
    if (state.get(BlockStateProperties::HAS_BOOK())) {
        return false;
    }

    if (!isLecternBookItem(static_cast<ItemId>(itemId))) {
        return false;
    }

    setHasBook(world, pos, state, true);
    return true;
}

void LecternBlock::setHasBook(IWorld& world, const BlockPos& pos, BlockState& state, bool hasBook) {
    state = state
        .with(BlockStateProperties::HAS_BOOK(), hasBook)
        .with(BlockStateProperties::POWERED(), false);
    world.setBlockState(pos, &state, 3);

    if (BlockEntity* blockEntity = world.getBlockEntity(pos);
        blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Lectern) {
        static_cast<blockentity::LecternEntity*>(blockEntity)->setChanged();
    }
}

void LecternBlock::pulse(IWorld& world, const BlockPos& pos, BlockState& state) {
    state = state.with(BlockStateProperties::POWERED(), true);
    world.setBlockState(pos, &state, 3);
    world.tickManager().scheduleBlockTick(pos, const_cast<Block&>(state.getBlock()), 2, world::tick::TickPriority::High);
}

} // namespace blocks
} // namespace mc