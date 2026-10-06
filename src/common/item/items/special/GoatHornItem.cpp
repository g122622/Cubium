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

#include "GoatHornItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include <array>
#include <utility>

namespace mc {
namespace item::items {

namespace {

/// 8 种山羊角音色（对齐 vanilla SoundEvents.GOAT_HORN_SOUND_VARIANTS = item.goat_horn.sound.0-7）
const std::array<const ResourceLocation*, 8>& _goatHornSounds()
{
    static const std::array<const ResourceLocation*, 8> s_sounds = {&SoundEvents::ITEM_GOAT_HORN_SOUND_0,
        &SoundEvents::ITEM_GOAT_HORN_SOUND_1,
        &SoundEvents::ITEM_GOAT_HORN_SOUND_2,
        &SoundEvents::ITEM_GOAT_HORN_SOUND_3,
        &SoundEvents::ITEM_GOAT_HORN_SOUND_4,
        &SoundEvents::ITEM_GOAT_HORN_SOUND_5,
        &SoundEvents::ITEM_GOAT_HORN_SOUND_6,
        &SoundEvents::ITEM_GOAT_HORN_SOUND_7};
    return s_sounds;
}

} // namespace

GoatHornItem::GoatHornItem(i32 variantIndex, ItemProperties properties)
    : Item(std::move(properties))
    , m_variantIndex(variantIndex)
{}

i32 GoatHornItem::getUseDuration(const ItemStack& stack) const
{
    MC_UNUSED(stack);
    return USE_DURATION_TICKS;
}

UseAction GoatHornItem::getUseAction(const ItemStack& stack) const
{
    MC_UNUSED(stack);
    return UseAction::TootHorn;
}

ItemActionResult GoatHornItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    ItemStack& held = player.getHeldItem(hand);
    if (held.isEmpty()) {
        return ItemActionResult::fail(held);
    }

    // 冷却中不可吹奏（对齐 vanilla InstrumentItem#use 的 addCooldown 门控由上层检查）
    if (player.cooldownTracker().hasCooldown(held.getItem())) {
        return ItemActionResult::fail(held);
    }

    // 对齐 vanilla InstrumentItem#use：开始使用 + 立即播放音色 + 进入冷却
    player.setActiveHand(hand);

    const i32 index = (m_variantIndex >= 0 && m_variantIndex < 8) ? m_variantIndex : 0;
    // 对齐 vanilla：range/16 作为音量倍率（默认 range=16 → 音量 1.0）
    player.playSound(*_goatHornSounds()[index], 1.0f, 1.0f);

    // 对齐 vanilla：Mth.floor(useDuration * 20.0F) = 140 tick 冷却
    player.setItemCooldown(held.getItem(), USE_DURATION_TICKS);

    MC_UNUSED(world);
    return ItemActionResult::success(held);
}

} // namespace item::items
} // namespace mc
