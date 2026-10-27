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
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include <string>
#include <utility>

namespace mc {
namespace item::items {

GoatHornItem::GoatHornItem(ItemProperties properties)
    : Item(std::move(properties))
{}

const instrument::InstrumentDefinition* GoatHornItem::getInstrument(const ItemStack& stack)
{
    const std::string instrumentId = stack.getInstrument();
    if (instrumentId.empty()) {
        return nullptr;
    }
    return instrument::InstrumentRegistry::instance().get(ResourceLocation(instrumentId));
}

ItemStack GoatHornItem::createStack(const instrument::InstrumentDefinition& instrument)
{
    // 乐器资源名由注册表反查得到：调用方按指针持有定义，这里通过指针身份定位其资源名。
    const auto& registry = instrument::InstrumentRegistry::instance();
    for (const auto& [id, registered] : registry.getAll()) {
        if (registered == &instrument) {
            ItemStack stack(*Items::GOAT_HORN, 1);
            stack.setInstrument(id.toString());
            return stack;
        }
    }
    MC_ASSERT_RELEASE_MSG(false, "GoatHornItem::createStack: instrument is not registered");
    return ItemStack(*Items::GOAT_HORN, 1);
}

i32 GoatHornItem::getUseDuration(const ItemStack& stack) const
{
    const instrument::InstrumentDefinition* instrumentDefinition = getInstrument(stack);
    return instrumentDefinition != nullptr ? instrumentDefinition->useDurationTicks() : 0;
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

    // 无乐器定义（缺 instrument 组件或资源名未注册）时不可吹奏。
    const instrument::InstrumentDefinition* instrumentDefinition = getInstrument(held);
    if (instrumentDefinition == nullptr) {
        return ItemActionResult::fail(held);
    }

    // 冷却中不可吹奏（对应 vanilla ItemCooldowns 门控）
    if (player.cooldownTracker().hasCooldown(held.getItem())) {
        return ItemActionResult::fail(held);
    }

    player.setActiveHand(hand);

    // 以玩家为音源播放乐器音效，音量取 range/16，音调 1.0。
    world.playSound(instrumentDefinition->soundEvent(),
        sound::SoundCategory::Records,
        player.position(),
        instrumentDefinition->volume(),
        1.0f);

    player.gameEvent(gameevent::GameEvents::INSTRUMENT_PLAY);

    // 冷却时长 = 吹奏时长（秒）× 20（vanilla Mth.floor(useDuration * 20.0F)）
    player.setItemCooldown(held.getItem(), instrumentDefinition->useDurationTicks());

    return ItemActionResult::success(held);
}

} // namespace item::items
} // namespace mc
