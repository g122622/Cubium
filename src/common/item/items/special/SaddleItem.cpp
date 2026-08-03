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

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IEquipable.hpp"
#include "common/entity/interfaces/IRideable.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

/**
 * @brief 根据实体类型选择鞍音效
 *
 * 不同实体装备鞍时播放不同的音效：
 * - 猪: ENTITY_PIG_SADDLE
 * - 炽足兽: ENTITY_STRIDER_SADDLE
 * - 马类及其他: ENTITY_HORSE_SADDLE
 */
const ResourceLocation& getSaddleSound(const LivingEntity& target)
{
    // 通过 dynamic_cast 判断实体类型，选择对应的音效
    // 猪使用猪专属音效
    if (dynamic_cast<const PigEntity*>(&target) != nullptr) {
        return SoundEvents::ENTITY_PIG_SADDLE;
    }
    // 炽足兽使用炽足兽专属音效
    if (dynamic_cast<const StriderEntity*>(&target) != nullptr) {
        return SoundEvents::ENTITY_STRIDER_SADDLE;
    }
    // 马类及其他实体使用马鞍音效
    return SoundEvents::ENTITY_HORSE_SADDLE;
}

} // namespace

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

    // 幼年实体不能装备鞍
    if (target.isChild()) {
        return false;
    }

    // 检查实体是否已经装备鞍
    auto* rideable = dynamic_cast<entity::IRideable*>(&target);
    if (rideable == nullptr) {
        return false;
    }

    if (rideable->hasSaddle()) {
        return false;
    }

    // 检查是否可以装备鞍
    // 马类需要先驯服才能装备鞍；猪和炽足兽可以直接装备
    auto* horse = dynamic_cast<AbstractHorseEntity*>(&target);
    if (horse != nullptr && !horse->isTame()) {
        return false;
    }

    // 检查装备槽是否可用
    if (equipable->getEquipmentSlotCount() <= 0) {
        return false;
    }

    // 检查鞍槽是否为空
    const ItemStack saddleSlot = equipable->getEquipment(0);
    if (!saddleSlot.isEmpty()) {
        return false;
    }

    // 设置鞍状态
    rideable->setSaddle(true);

    // 装备鞍到实体
    ItemStack saddleStack(stack.getItem(), 1);
    equipable->setEquipment(0, saddleStack);

    // 根据实体类型播放不同的鞍音效
    IWorld* world = target.world();
    if (world != nullptr) {
        const ResourceLocation& soundEvent = getSaddleSound(target);
        world->playSound(soundEvent, sound::SoundCategory::Neutral, target.position(), 0.5f, 1.0f);
    }

    // 消耗一个物品（非创造模式）
    if (!player.isCreative()) {
        stack.shrink(1);
    }

    return true;
}

} // namespace item::items
} // namespace mc
