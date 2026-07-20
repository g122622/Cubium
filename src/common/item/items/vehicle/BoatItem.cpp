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

#include "BoatItem.hpp"

#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/BoatEntity.hpp"
#include "common/entity/entities/vehicle/ChestBoatEntity.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {
namespace item {

BoatItem::BoatItem(entity::BoatEntity::Type boatType, bool hasChest, const ItemProperties& properties)
    : Item(std::move(properties))
    , m_boatType(boatType)
    , m_hasChest(hasChest)
{}

ActionResultType BoatItem::onItemUse(ItemUseContext& context)
{
    // 船物品使用右键点击放置，而非左键点击方块
    IWorld* world = &context.world();
    if (world == nullptr) {
        return ActionResultType::Fail;
    }

    // 服务端才能创建实体
    if (world->isClientSide()) {
        return ActionResultType::Success;
    }

    Player* player = context.player();
    if (player == nullptr) {
        return ActionResultType::Fail;
    }

    // 计算船的生成位置，船生成在击中点位置
    const Vector3& hitPos = context.hitPosition();
    f32 x = hitPos.x;
    f32 y = hitPos.y;
    f32 z = hitPos.z;

    // 创建船实体（带箱子的船使用 ChestBoatEntity，普通船使用 BoatEntity）
    std::unique_ptr<mc::Entity> boat;
    if (m_hasChest) {
        auto chestBoat = std::make_unique<entity::ChestBoatEntity>(m_boatType);
        chestBoat->setTypeId(entity::EntityTypeKeys::CHEST_BOAT);
        chestBoat->setPosition(x, y, z);
        chestBoat->setRotation(context.getPlayerYaw());
        boat = std::move(chestBoat);
    } else {
        auto normalBoat = std::make_unique<entity::BoatEntity>(m_boatType);
        normalBoat->setTypeId(entity::EntityTypeKeys::BOAT);
        normalBoat->setPosition(x, y, z);
        normalBoat->setRotation(context.getPlayerYaw());
        boat = std::move(normalBoat);
    }

    // 检查碰撞：如果船的碰撞箱（缩小0.1格）与其他实体碰撞，返回 Fail
    AxisAlignedBB boatBox = boat->boundingBox().grow(-0.1f);
    if (!world->hasNoCollisions(boatBox)) {
        return ActionResultType::Fail;
    }

    // 生成船实体
    world->spawnEntity(std::move(boat));

    // 非创造模式消耗物品
    ItemStack& stack = context.getItemStackMut();
    if (!player->isCreative()) {
        stack.shrink(1);
    }

    return ActionResultType::Success;
}

} // namespace item
} // namespace mc
