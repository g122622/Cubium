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

#include "SaddleItem.hpp"

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IEquipable.hpp"
#include "common/entity/interfaces/IRideable.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace item::items {

SaddleItem::SaddleItem(const ItemProperties& properties)
    : Item(properties)
{}

bool SaddleItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    MC_UNUSED(hand);

    // 检查目标实体是否实现了 IEquipable 接口
    auto* equipable = dynamic_cast<entity::IEquipable*>(&target);
    if (equipable == nullptr) {
        return false;
    }

    // 确保实体存活
    if (!target.isAlive()) {
        return false;
    }

    // 检查实体是否已经装备鞍
    // 通过 IRideable 接口检查鞍状态
    auto* rideable = dynamic_cast<entity::IRideable*>(&target);
    if (rideable == nullptr) {
        return false;
    }

    if (rideable->hasSaddle()) {
        // 已经有鞍了，不执行操作
        return false;
    }

    // 检查是否可以装备鞍
    // 对于马类，需要驯服后才能装备鞍
    // 对于猪和炽足兽，可以直接装备鞍
    // TODO: 目前简化实现：检查实体是否有鞍槽（装备槽数量 > 0），后续需要根据实体类型检查驯服状态
    if (equipable->getEquipmentSlotCount() <= 0) {
        return false;
    }

    // 检查鞍槽是否可用
    const ItemStack saddleSlot = equipable->getEquipment(0);
    if (!saddleSlot.isEmpty()) {
        // 鞍槽已有物品
        return false;
    }

    // 设置鞍状态
    rideable->setSaddle(true);

    // 装备鞍到实体
    // 创建鞍物品堆并放入鞍槽
    ItemStack saddleStack(stack.getItem(), 1);
    equipable->setEquipment(0, saddleStack);

    // 播放鞍音效
    // TODO: 根据实体类型播放不同音效：
    // - 猪: ENTITY_PIG_SADDLE
    // - 马: ENTITY_HORSE_SADDLE
    // - 炽足兽: ENTITY_STRIDER_SADDLE
    // 目前使用通用音效
    IWorld* world = target.world();
    if (world != nullptr) {
        world->playSound(SoundEvents::ENTITY_HORSE_SADDLE,
            sound::SoundCategory::Neutral,
            target.position(),
            0.5f, // 音量
            1.0f  // 音调
        );
    }

    // 消耗一个物品（非创造模式）
    if (!player.isCreative()) {
        stack.shrink(1);
    }

    return true;
}

} // namespace item::items
} // namespace mc
