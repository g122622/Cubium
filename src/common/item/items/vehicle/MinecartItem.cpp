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

#include "common/item/items/vehicle/MinecartItem.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <cmath>

namespace mc {
namespace item {

MinecartItem::MinecartItem(entity::AbstractMinecartEntity::Type minecartType, const ItemProperties& properties)
    : Item(properties)
    , m_minecartType(minecartType)
{}

ActionResultType MinecartItem::onItemUse(ItemUseContext& context)
{
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
    // X 和 Z 为方块中心，Y 为方块底部 + 0.0625 (1/16)
    f64 x = static_cast<f64>(pos.x) + 0.5;
    f64 y = static_cast<f64>(pos.y) + 0.0625;
    f64 z = static_cast<f64>(pos.z) + 0.5;

    // 如果是斜坡铁轨，额外增加 Y 偏移 0.5
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
            minecart = std::make_unique<entity::RideableMinecartEntity>(EntityInstanceId(0));
            minecart->setTypeId(entity::EntityTypeKeys::MINECART);
            break;
        case entity::AbstractMinecartEntity::Type::Chest:
            minecart = std::make_unique<entity::ChestMinecartEntity>(EntityInstanceId(0));
            minecart->setTypeId(entity::EntityTypeKeys::CHEST_MINECART);
            break;
        case entity::AbstractMinecartEntity::Type::Furnace:
            minecart = std::make_unique<entity::FurnaceMinecartEntity>(EntityInstanceId(0));
            minecart->setTypeId(entity::EntityTypeKeys::FURNACE_MINECART);
            break;
        case entity::AbstractMinecartEntity::Type::TNT:
            minecart = std::make_unique<entity::TNTMinecartEntity>(EntityInstanceId(0));
            minecart->setTypeId(entity::EntityTypeKeys::TNT_MINECART);
            break;
        case entity::AbstractMinecartEntity::Type::Hopper:
            minecart = std::make_unique<entity::HopperMinecartEntity>(EntityInstanceId(0));
            minecart->setTypeId(entity::EntityTypeKeys::HOPPER_MINECART);
            break;
        case entity::AbstractMinecartEntity::Type::CommandBlock:
            minecart = std::make_unique<entity::CommandBlockMinecartEntity>(EntityInstanceId(0));
            break;
        case entity::AbstractMinecartEntity::Type::Spawner:
            minecart = std::make_unique<entity::SpawnerMinecartEntity>(EntityInstanceId(0));
            minecart->setTypeId(entity::EntityTypeKeys::SPAWNER_MINECART);
            break;
        default:
            minecart = std::make_unique<entity::RideableMinecartEntity>(EntityInstanceId(0));
            minecart->setTypeId(entity::EntityTypeKeys::MINECART);
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
