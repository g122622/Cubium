#include "DriedKelpBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../blockentity/BlockEntity.hpp"
#include "../../../blockentity/core/BlockEntityRegistry.hpp"
#include "../../../blockentity/processing/ConduitEntity.hpp"
#include "../../VanillaBlocks.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// DriedKelpBlock 实现
// ============================================================================

DriedKelpBlock::DriedKelpBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 干海带块不需要特殊逻辑
}

// ============================================================================
// ConduitBlock 实现
// ============================================================================

namespace {
/// 潮涌核心的碰撞箱形状 (5x5x5 到 11x11x11)
static const CollisionShape CONDUIT_SHAPE = CollisionShape::box(5.0f, 5.0f, 5.0f, 11.0f, 11.0f, 11.0f);
}

ConduitBlock::ConduitBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .add(BlockStateProperties::WATERLOGGED())
        .create([this](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), true));
}

bool ConduitBlock::isWaterlogged(const BlockState& state) const {
    return state.get(BlockStateProperties::WATERLOGGED());
}

BlockState ConduitBlock::getStateForPlacement(BlockItemUseContext& context) {
    // 检查放置位置是否在水中
    // MC 1.16.5: 默认含水
    return defaultState().with(BlockStateProperties::WATERLOGGED(), true);
}

void ConduitBlock::onBlockAdded(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state)
{
    MC_UNUSED(state);

    // 创建方块实体
    auto& registry = blockentity::BlockEntityRegistry::instance();
    auto blockEntity = registry.create(BlockEntityType::Conduit, pos);
    if (blockEntity != nullptr) {
        // 设置世界引用并存储方块实体
        // 注意：setBlockEntity 会接管所有权并设置世界引用
        world.setBlockEntity(pos, blockEntity.release());
    }
}

void ConduitBlock::onBlockRemoved(
    IWorld& world,
    const BlockPos& pos,
    const BlockState& state)
{
    MC_UNUSED(state);

    // 移除方块实体
    world.removeBlockEntity(pos);
}

BlockState ConduitBlock::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 如果含水，调度流体tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 注意：潮涌核心的激活状态检测在 ConduitEntity::tick() 中自动完成
    // 当方块更新时不需要手动触发重新检测

    return state;
}

const CollisionShape& ConduitBlock::getShape(const BlockState& state) const {
    MC_UNUSED(state);
    return CONDUIT_SHAPE;
}

} // namespace blocks
} // namespace mc
