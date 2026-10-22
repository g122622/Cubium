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

#include "HarnessItem.hpp"

#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/world/IWorld.hpp"

#include <utility>

namespace mc {
namespace item::items {

HarnessItem::HarnessItem(ItemProperties properties, DyeColor color)
    : Item(std::move(properties))
    , m_color(color)
{}

bool HarnessItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // stack 在 Player::interactOn 路径下是值拷贝，shrink 不回写权威手持；统一以
    // player.getHeldItem(hand) 为权威源（同 SaddleItem/NameTagItem 修复范式）。
    (void)stack;

    if (!target.isAlive()) {
        return false;
    }

    // 对齐 vanilla HappyGhast.canUseSlot(BODY)：isAlive() && !isBaby()。挽具只装备成体。
    if (target.isChild()) {
        return false;
    }

    // 对齐 vanilla Equippable.harness 的 allowedEntities = EntityTypeTags.CAN_EQUIP_HARNESS 与
    // equipOnInteract=true：由目标实体侧判定能否以挽具右键装备到 Body 槽
    // （Cubium 无数据组件 Equippable 体系，判定落在 MobEntity::canEquipItemOnInteract）。
    const auto* mob = dynamic_cast<const MobEntity*>(&target);
    if (mob == nullptr || !mob->canEquipItemOnInteract(ItemStack(this, 1), EquipmentSlot::Body)) {
        return false;
    }

    // 已装备挽具时 vanilla 不会替换（HappyGhast.mobInteract 转而让玩家骑乘）。
    if (!target.getEquipment(EquipmentSlot::Body).isEmpty()) {
        return false;
    }

    target.setEquipment(EquipmentSlot::Body, ItemStack(this, 1));

    // 装备音效（vanilla Equippable.harness: setEquipSound(HARNESS_EQUIP)，Neutral 音源）
    if (IWorld* world = target.world(); world != nullptr && !target.isSilent()) {
        world->playSound(SoundEvents::HARNESS_EQUIP, sound::SoundCategory::Neutral, target.position(), 1.0f, 1.0f);
    }

    // 消耗一个物品：操作玩家权威手持引用（创造模式不消耗）。
    if (!player.isCreative()) {
        player.getHeldItem(hand).shrink(1);
    }

    return true;
}

} // namespace item::items
} // namespace mc
