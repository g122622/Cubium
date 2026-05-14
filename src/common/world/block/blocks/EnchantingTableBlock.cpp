#include "EnchantingTableBlock.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/inventory/ContainerTypes.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../IWorld.hpp"
#include "../../blockentity/interactive/EnchantingTableEntity.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

EnchantingTableBlock::EnchantingTableBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 附魔台形状：桌面略大于基座
    // 底座: (1, 0, 1) -> (15, 2, 15)
    // 桌面: (0, 2, 0) -> (16, 14, 16)

    CollisionShape base =
        VoxelShapes::cube(1.0f / 16.0f, 0.0f, 1.0f / 16.0f, 15.0f / 16.0f, 2.0f / 16.0f, 15.0f / 16.0f);

    CollisionShape top = VoxelShapes::cube(0.0f, 2.0f / 16.0f, 0.0f, 1.0f, 14.0f / 16.0f, 1.0f);

    m_shape = CollisionShape::combine(base, top, CollisionShape::CombineOp::OR);
}

// ========== 方块实体 ==========

std::unique_ptr<BlockEntity> EnchantingTableBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::EnchantingTableEntity>(pos);
}

// ========== 交互 ==========

ActionResultType EnchantingTableBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // MC 1.16.5: 客户端直接返回成功
    // 参考: net.minecraft.block.EnchantingTableBlock.onBlockActivated
    if (world.asServerWorld() == nullptr) {
        return ActionResultType::Success;
    }

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::EnchantingTable) {
        return ActionResultType::Pass;
    }

    // 打开附魔台GUI
    // 参考: net.minecraft.block.EnchantingTableBlock.getContainer
    if (world.openContainer(ContainerType::Enchantment, pos, player)) {
        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

// ========== 形状 ==========

const CollisionShape& EnchantingTableBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EnchantingTableBlock::getOcclusionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 附魔台不阻挡光线（用于渲染）
    return VoxelShapes::empty();
}

// ========== 放置和更新 ==========

void EnchantingTableBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 当附魔台放置时，重新计算附魔力量
    if (world.getBlockEntity(pos) != nullptr) {
        // 注意：这里需要World引用来重新计算附魔力量
        // 在IWorld中可能无法直接调用，需要在World中处理
    }
}

} // namespace blocks
} // namespace mc
