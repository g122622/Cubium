#include "world/block/blocks/AbstractFurnaceBlock.hpp"
#include "world/blockentity/processing/AbstractFurnaceEntity.hpp"
#include "world/IWorld.hpp"
#include "world/World.hpp"
#include "entity/Player.hpp"
#include "util/Direction.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

AbstractFurnaceBlock::AbstractFurnaceBlock(const BlockProperties& properties)
    : Block(properties) {
}

// ========== 方块状态 ==========

void AbstractFurnaceBlock::fillStateContainer(StateContainer<Block, BlockState>& container) {
    container.add(BlockStateProperties::HORIZONTAL_FACING());
    container.add(BlockStateProperties::LIT());
}

const BlockState& AbstractFurnaceBlock::getDefaultState() const {
    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
        .with(BlockStateProperties::LIT(), false);
}

// ========== 放置和更新 ==========

BlockState AbstractFurnaceBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 熔炉朝向玩家的反方向
    Direction facing = context.getHorizontalDirection();
    Direction opposite = Directions::opposite(facing);

    return defaultState()
        .with(BlockStateProperties::HORIZONTAL_FACING(), opposite)
        .with(BlockStateProperties::LIT(), false);
}

// ========== 交互 ==========

ActionResult AbstractFurnaceBlock::onBlockActivated(
    const BlockState& state,
    World& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {

    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 客户端直接返回成功
    // TODO: 检查 world.isRemote()
    // if (world.isRemote()) {
    //     return ActionResult::SUCCESS;
    // }

    // 打开熔炉GUI
    interactWith(world, pos, player);
    return ActionResult::CONSUME;
}

// ========== 红石 ==========

i32 AbstractFurnaceBlock::getComparatorInputOverride(
    const BlockState& state,
    World& world,
    const BlockPos& pos) const {

    MC_UNUSED(state);

    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr) {
        return 0;
    }

    // 检查是否是熔炉实体
    if (blockEntity->getType() != BlockEntityType::Furnace &&
        blockEntity->getType() != BlockEntityType::BlastFurnace &&
        blockEntity->getType() != BlockEntityType::Smoker) {
        return 0;
    }

    auto* furnace = static_cast<blockentity::AbstractFurnaceEntity*>(blockEntity);
    return furnace->getComparatorSignal();
}

// ========== 旋转和镜像 ==========

const BlockState& AbstractFurnaceBlock::rotate(const BlockState& state, Rotation rotation) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& AbstractFurnaceBlock::mirror(const BlockState& state, Mirror mirror) const {
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = mirrorToRotation(mirror, facing);
    return rotate(state, rotation);
}

// ========== 静态工具方法 ==========

bool AbstractFurnaceBlock::isLit(const BlockState& state) {
    return state.get(BlockStateProperties::LIT());
}

} // namespace blocks
} // namespace mc
