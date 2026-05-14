#include "MinecartItem.hpp"
#include "../../../entity/entities/vehicle/MinecartEntity.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockPos.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../../world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "../../context/ItemUseContext.hpp"
#include "../../core/ItemStack.hpp"
#include <cmath>

namespace mc {
namespace item {

MinecartItem::MinecartItem(entity::AbstractMinecartEntity::Type minecartType, const ItemProperties& properties)
    : Item(properties)
    , m_minecartType(minecartType)
{}

ActionResultType MinecartItem::onItemUse(ItemUseContext& context)
{
    // MC 1.16.5 MinecartItem.onItemUse
    IWorld* world = &context.world();
    if (!world) {
        return ActionResultType::Fail;
    }

    const BlockPos& pos = context.blockPos();
    const BlockState* state = world->getBlockState(pos);
    if (!state) {
        return ActionResultType::Fail;
    }

    // 检查是否为铁轨
    const Block& block = state->getBlock();
    const blocks::AbstractRailBlock* railBlock = dynamic_cast<const blocks::AbstractRailBlock*>(&block);
    bool isRail = (railBlock != nullptr);

    if (!isRail) {
        // 不在铁轨上，尝试在下方一格检查
        const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        const BlockState* belowState = world->getBlockState(belowPos);
        if (belowState) {
            const Block& belowBlock = belowState->getBlock();
            railBlock = dynamic_cast<const blocks::AbstractRailBlock*>(&belowBlock);
            if (railBlock != nullptr) {
                isRail = true;
                state = belowState;
                // 注意：此时 pos 需要更新为下方位置
                // 但 MC 源码中矿车放置在点击位置，不是下方位置
                // 这里保持 pos 不变，只是使用下方铁轨的状态来获取形状
            }
        }
    }

    if (!isRail) {
        return ActionResultType::Fail;
    }

    // 服务端才能创建实体
    if (world->isClientSide()) {
        return ActionResultType::Success;
    }

    // 计算矿车位置
    // MC 1.16.5: X 和 Z 为方块中心，Y 为方块底部 + 0.0625 (1/16)
    f64 x = static_cast<f64>(pos.x) + 0.5;
    f64 y = static_cast<f64>(pos.y) + 0.0625;
    f64 z = static_cast<f64>(pos.z) + 0.5;

    // MC 1.16.5: 如果是斜坡铁轨，额外增加 Y 偏移 0.5
    // 注意：此时 state 已确保非空（在前面检查过）
    if (railBlock != nullptr) {
        blocks::RailShape shape = railBlock->getRailShape(*state);
        if (blocks::isAscending(shape)) {
            y += 0.5;
        }
    }

    // 创建矿车实体
    std::unique_ptr<entity::AbstractMinecartEntity> minecart;

    switch (m_minecartType) {
        case entity::AbstractMinecartEntity::Type::Rideable:
            minecart = std::make_unique<entity::RideableMinecartEntity>(EntityId(0));
            break;
        case entity::AbstractMinecartEntity::Type::Chest:
            minecart = std::make_unique<entity::ChestMinecartEntity>(EntityId(0));
            break;
        case entity::AbstractMinecartEntity::Type::Furnace:
            minecart = std::make_unique<entity::FurnaceMinecartEntity>(EntityId(0));
            break;
        case entity::AbstractMinecartEntity::Type::TNT:
            minecart = std::make_unique<entity::TNTMinecartEntity>(EntityId(0));
            break;
        case entity::AbstractMinecartEntity::Type::Hopper:
            minecart = std::make_unique<entity::HopperMinecartEntity>(EntityId(0));
            break;
        case entity::AbstractMinecartEntity::Type::CommandBlock:
            minecart = std::make_unique<entity::CommandBlockMinecartEntity>(EntityId(0));
            break;
        case entity::AbstractMinecartEntity::Type::Spawner:
            // Spawner minecart not implemented yet
            minecart = std::make_unique<entity::RideableMinecartEntity>(EntityId(0));
            break;
        default:
            minecart = std::make_unique<entity::RideableMinecartEntity>(EntityId(0));
            break;
    }

    // 设置位置
    minecart->setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));

    // 设置自定义名称（如果有）
    const ItemStack& stack = context.itemStack();
    if (stack.hasCustomName()) {
        minecart->setCustomName(stack.getCustomName());
    }

    // 生成实体
    world->spawnEntity(std::move(minecart));

    // 消耗物品
    ItemStack& mutableStack = context.getItemStackMut();
    mutableStack.shrink(1);

    return ActionResultType::Success;
}

} // namespace item
} // namespace mc
