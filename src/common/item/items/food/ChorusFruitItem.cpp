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

#include "ChorusFruitItem.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/food/Food.hpp"
#include "common/item/items/food/FoodItem.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include <utility>

namespace mc {
namespace item::items {

ChorusFruitItem::ChorusFruitItem(const food::Food* food, ItemProperties properties)
    : FoodItem(food, std::move(properties))
{}

ItemStack ChorusFruitItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    // 调用父类方法处理基本的食用逻辑
    ItemStack result = FoodItem::onItemUseFinish(stack, world, entity);

    // 随机传送逻辑（仅服务端执行）
    if (!world.isClientSide()) {
        // 记录原始位置用于播放音效
        Vector3 originalPos = entity.position();
        f64 d0 = originalPos.x;
        f64 d1 = originalPos.y;
        f64 d2 = originalPos.z;

        bool teleported = false;

        // 尝试随机传送
        // 紫颂果传送范围：水平方向 ±8 格，垂直方向 ±8 格
        teleported = entity.randomTeleport(16.0, false, true);

        if (teleported) {
            // 播放传送音效（狐狸使用特殊音效）
            const bool isFox = dynamic_cast<FoxEntity*>(&entity) != nullptr;
            const auto& soundEvent = isFox ? SoundEvents::ENTITY_FOX_TELEPORT : SoundEvents::ITEM_CHORUS_FRUIT_TELEPORT;

            // 在原位置播放音效
            world.playSound(soundEvent,
                sound::SoundCategory::Players,
                Vector3(static_cast<f32>(d0), static_cast<f32>(d1), static_cast<f32>(d2)),
                1.0f,
                1.0f);

            // 在新位置播放音效（实体自己播放）
            entity.playSound(soundEvent, 1.0f, 1.0f);
        }

        // 设置冷却时间（仅玩家）
        // 冷却时间：20 ticks = 1 秒
        if (auto* player = dynamic_cast<Player*>(&entity)) {
            player->cooldownTracker().setCooldown(this, 20);
        }
    }

    return result;
}

} // namespace item::items
} // namespace mc
