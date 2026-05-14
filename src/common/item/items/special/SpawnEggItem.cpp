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

#include "SpawnEggItem.hpp"
#include "../../../core/Direction.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/EntityRegistry.hpp"
#include "../../../entity/core/EntityType.hpp"
#include "../../../item/context/BlockItemUseContext.hpp"
#include "../../../player/Player.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/BlockPos.hpp"

namespace mc {
namespace item {

SpawnEggItem::SpawnEggItem(
    entity::EntityType entityType, u32 primaryColor, u32 secondaryColor, const ItemProperties& properties)
    : Item(properties)
    , m_entityType(entityType)
    , m_primaryColor(primaryColor)
    , m_secondaryColor(secondaryColor)
{}

ActionResultType SpawnEggItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    Direction face = context.getFace();

    // 检查是否在客户端
    if (world.isClientSide()) {
        return ActionResultType::Success;
    }

    // 计算生成位置 (方块面偏移)
    BlockPos spawnPos = pos.offset(face);

    // 检查位置是否有效
    const BlockState* state = world.getBlockState(spawnPos);
    if (state != nullptr && !state->isAir() && !state->getMaterial().isReplaceable()) {
        return ActionResultType::Fail;
    }

    // 生成实体
    if (spawnEntity(world, spawnPos, entity::SpawnReason::SpawnEgg)) {
        // 消耗物品 (非创造模式)
        if (!context.getPlayer().isCreative()) {
            context.getItemStack().shrink(1);
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Fail;
}

ItemActionResult SpawnEggItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 在玩家位置生成实体
    if (world.isClientSide()) {
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    BlockPos spawnPos(
        static_cast<i32>(player.getX()), static_cast<i32>(player.getY()), static_cast<i32>(player.getZ()));

    if (spawnEntity(world, spawnPos, entity::SpawnReason::SpawnEgg)) {
        if (!player.isCreative()) {
            player.getHeldItem(hand).shrink(1);
        }
        return ItemActionResult::success(player.getHeldItem(hand));
    }

    return ItemActionResult::pass(player.getHeldItem(hand));
}

bool SpawnEggItem::spawnEntity(IWorld& world, const BlockPos& pos, entity::SpawnReason spawnReason) const
{
    // 通过实体注册表创建实体
    auto entity = entity::EntityRegistry::instance().createEntity(m_entityType, world);
    if (!entity) {
        return false;
    }

    // 设置位置 (方块中心上方)
    f32 x = static_cast<f32>(pos.x) + 0.5f;
    f32 y = static_cast<f32>(pos.y);
    f32 z = static_cast<f32>(pos.z) + 0.5f;
    entity->setPosition(x, y, z);

    // 生成实体
    world.spawnEntity(std::move(entity));
    return true;
}

} // namespace item
} // namespace mc
