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

#include "EmptyMapItem.hpp"
#include "FilledMapItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/map/AbstractMapItem.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/map/MapData.hpp"
#include "common/world/map/MapDataManager.hpp"

namespace mc::item::items {

EmptyMapItem::EmptyMapItem(const ItemProperties& properties)
    : AbstractMapItem(properties)
{}

ItemActionResult EmptyMapItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 创建新的已填充地图
    ItemStack result = createFilledMap(world,
        static_cast<i32>(player.position().x),
        static_cast<i32>(player.position().z),
        0,    // 初始缩放级别0
        true, // 追踪玩家位置
        false // 不无限追踪
    );

    // 消耗空地图（创造模式不消耗）
    ItemStack& held = player.getHeldItem(hand);
    if (!player.isCreative()) {
        held.shrink(1);
    }

    // 如果消耗后物品栏有空位，将地图添加到物品栏
    // 否则返回给玩家
    if (!player.inventory().add(result)) {
        player.dropItem(result, false, true);
    }

    return ItemActionResult::success(result);
}

ItemStack EmptyMapItem::createFilledMap(
    IWorld& world, i32 x, i32 z, i32 scale, bool trackingPosition, bool unlimitedTracking)
{
    ItemStack result(ItemRegistry::instance().getItem(ResourceLocation("minecraft:filled_map")), 1);

    auto* manager = world.mapDataManager();
    if (manager != nullptr) {
        i32 mapId = manager->createMap(x, z, scale, trackingPosition, unlimitedTracking);
        result.getOrCreateTag()["map"] = mapId;
    }

    return result;
}

} // namespace mc::item::items
