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

#include "ArmorStandItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {
namespace item::items {

ArmorStandItem::ArmorStandItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType ArmorStandItem::onItemUse(ItemUseContext& context)
{
    // 对齐 vanilla ArmorStandItem#useOn：不能放置在方块底面
    const Direction face = context.getFace();
    if (face == Direction::Down) {
        return ActionResultType::Fail;
    }

    IWorld& world = context.getWorld();
    Player* player = context.getPlayer();

    // 放置位置为点击面外侧的方块
    const BlockPos placePos = context.blockPos().offset(face);

    // 对齐 vanilla：按 yaw 对齐到 45 度取整
    const f32 yaw =
        static_cast<f32>(math::floorTo<i32>((math::wrapDegrees(context.getPlayerYaw() - 180.0f) + 22.5f) / 45.0f)) *
        45.0f;

    // 对齐 vanilla：检查目标位置的实体碰撞（有实体占据则不放置）
    const AxisAlignedBB aabb(static_cast<f32>(placePos.x),
        static_cast<f32>(placePos.y),
        static_cast<f32>(placePos.z),
        static_cast<f32>(placePos.x) + 0.5f,
        static_cast<f32>(placePos.y) + 1.975f,
        static_cast<f32>(placePos.z) + 0.5f);
    if (!world.getEntitiesInAABB(aabb, nullptr).empty()) {
        return ActionResultType::Fail;
    }

    if (!world.isClientSide()) {
        auto* registry = world.entityRegistry();
        if (registry == nullptr) {
            return ActionResultType::Fail;
        }
        const entity::EntityType* armorStandType = entity::EntityRegistry::instance().getType("minecraft:armor_stand");
        if (armorStandType == nullptr) {
            return ActionResultType::Fail;
        }

        auto armorStand = armorStandType->create(&world, *registry);
        if (armorStand == nullptr) {
            return ActionResultType::Fail;
        }

        armorStand->setPosition(
            static_cast<f32>(placePos.x) + 0.5f, static_cast<f32>(placePos.y), static_cast<f32>(placePos.z) + 0.5f);
        armorStand->setRotation(yaw, 0.0f);
        world.spawnEntity(std::move(armorStand));

        world.playSound(SoundEvents::ENTITY_ARMOR_STAND_PLACE,
            sound::SoundCategory::Blocks,
            Vector3(
                static_cast<f32>(placePos.x) + 0.5f, static_cast<f32>(placePos.y), static_cast<f32>(placePos.z) + 0.5f),
            0.75f,
            0.8f);
    }

    // 消耗物品（创造模式由外层统一处理）
    if (player != nullptr && !player->isCreative()) {
        player->getHeldItem(context.getHand()).shrink(1);
    }

    return ActionResultType::Success;
}

} // namespace item::items
} // namespace mc
