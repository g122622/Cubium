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

#include "BundleItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/items/special/bundle/BundleContents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item::items {

// ============================================================================
// 常量
// ============================================================================

namespace {
/// 首次丢出后的间隔（对应 MC TICKS_AFTER_FIRST_THROW = 10）
constexpr i32 TICKS_AFTER_FIRST_THROW = 10;
/// 两次丢出之间的间隔（对应 MC TICKS_BETWEEN_THROWS = 2）
constexpr i32 TICKS_BETWEEN_THROWS = 2;
/// 最大使用时长（对应 MC TICKS_MAX_THROW_DURATION = 200）
constexpr i32 TICKS_MAX_THROW_DURATION = 200;
} // namespace

// ============================================================================
// 构造
// ============================================================================

BundleItem::BundleItem(ItemProperties properties, DyeColor color)
    : Item(std::move(properties))
    , m_color(color)
{}

// ============================================================================
// Item 接口实现
// ============================================================================

ItemActionResult BundleItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    // 对应 MC 1.21.11 BundleItem#use
    player.setActiveHand(hand);
    ItemStack& held = player.getHeldItem(hand);
    return ItemActionResult::success(held);
}

void BundleItem::onUseTick(ItemStack& stack, IWorld& /*world*/, LivingEntity& entity, i32 elapsedTicks)
{
    // 对应 MC 1.21.11 BundleItem#onUseTick
    // elapsedTicks 为从开始到当前已过的 tick 数（1-based）
    // MC 源码：boolean flag = p_371432_ == i; (i = totalDuration = 200)
    //         if (flag || p_371432_ < i - 10 && p_371432_ % 2 == 0) dropContent(...)
    //
    // 注意：MC 的 p_371432_ 实际是剩余使用时间（getUseRemaining），而非已用时间。
    // 但本项目的 onUseTick 传入的是 elapsedTicks（已用时间）。
    // 转换：remaining = totalDuration - elapsedTicks
    // - flag = (remaining == totalDuration) => elapsedTicks == 0
    //   但 onUseTick 在第 1 tick 已调用，所以 elapsedTicks == 1 时触发首次？
    //   实际 MC 在 tick 末尾调用 onUseTick，elapsedTicks 从 1 开始。
    //   为保持行为一致：当 elapsedTicks == 1（首次）或 (elapsedTicks < totalDuration - 10 && elapsedTicks % 2 == 0)
    //   时丢出。
    //
    // 重新核对 MC 源码：
    //   p_371432_ 是 LivingEntity.getUseItemRemainingTicks()，即剩余时间。
    //   i = getUseDuration(stack, entity) = 200
    //   flag = (p_371432_ == i) 表示刚开始使用（剩余 == 总时长）
    //   p_371432_ < i - 10 表示剩余时间 < 190（即已用 > 10）
    //   p_371432_ % 2 == 0 表示剩余时间为偶数
    //
    // 本项目 elapsedTicks = totalDuration - remaining
    // - flag => elapsedTicks == 0（刚开始）
    //   但 onUseTick 在 elapsedTicks >= 1 时调用，所以 flag 实际不会触发。
    //   为匹配 MC 行为，我们在 elapsedTicks == 1 时触发首次（即 tick 开始后立即丢出）。
    // - remaining < 190 && remaining % 2 == 0
    //   => elapsedTicks > 10 && (totalDuration - elapsedTicks) % 2 == 0
    //   => elapsedTicks > 10 && elapsedTicks % 2 == 0 （因为 200 是偶数）
    //
    // 简化：在 elapsedTicks == 1 || (elapsedTicks > 10 && elapsedTicks % 2 == 0) 时丢出。

    auto* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        return;
    }

    bool isFirstTick = (elapsedTicks == 1);
    bool isPeriodic = (elapsedTicks > TICKS_AFTER_FIRST_THROW) && (elapsedTicks % TICKS_BETWEEN_THROWS == 0);
    if (isFirstTick || isPeriodic) {
        dropContent(stack, *player);
    }
}

i32 BundleItem::getUseDuration(const ItemStack& /*stack*/) const
{
    return TICKS_MAX_THROW_DURATION;
}

UseAction BundleItem::getUseAction(const ItemStack& /*stack*/) const
{
    return UseAction::Bundle;
}

void BundleItem::onDestroyed(ItemStack& stack, IWorld& world, Entity& entity)
{
    // 对应 MC 1.21.11 BundleItem#onDestroyed(ItemEntity)
    // 清空内容物并在被销毁实体位置生成物品实体
    BundleContents contents = getContents(stack);
    if (contents.isEmpty()) {
        Item::onDestroyed(stack, world, entity);
        return;
    }

    // 清空内容物
    setContents(stack, BundleContents::EMPTY);

    // 在被销毁实体位置生成内容物物品实体
    // 对应 MC 1.21.11 ItemUtils.onContainerDestroyed(ItemEntity, Iterable<ItemStack>)
    // MC 原版在被销毁的 ItemEntity 精确位置生成新物品实体（无随机速度）
    // 本项目使用 ItemDropHelper::spawnItemAtEntity 复刻此行为
    // 注意：调用方 entity 永远是 ItemEntity（见 ItemEntity::hurt），而非 Player
    math::Random& rng = entity.getRandom();
    for (const auto& item : contents.itemsCopy()) {
        if (item.isEmpty()) {
            continue;
        }
        ItemDropHelper::spawnItemAtEntity(&entity, item, 0.0f, rng);
    }

    Item::onDestroyed(stack, world, entity);
}

// ============================================================================
// 槽位覆写协议
// ============================================================================

bool BundleItem::overrideStackedOnOther(ItemStack& heldStack, Slot& slot, SlotClickAction clickAction, Player& player)
{
    // 对应 MC 1.21.11 BundleItem#overrideStackedOnOther
    BundleContents contents = getContents(heldStack);
    BundleContents::Mutable mutableContents(contents);

    ItemStack slotStack = slot.getItem();

    if (clickAction == SlotClickAction::Primary && !slotStack.isEmpty()) {
        // 左键 + 槽位有物品：尝试转入收纳袋
        i32 transferred = mutableContents.tryTransfer(slot, player);
        if (transferred > 0) {
            playInsertSound(player);
        } else {
            playInsertFailSound(player);
        }
        setContents(heldStack, mutableContents.toImmutable());
        return true;
    }

    if (clickAction == SlotClickAction::Secondary && slotStack.isEmpty()) {
        // 右键 + 槽位为空：从收纳袋取出一项放入槽位
        auto removed = mutableContents.removeOne();
        if (removed.has_value()) {
            ItemStack remaining = slot.safeInsert(std::move(removed).value());
            if (remaining.getCount() > 0) {
                // 槽位放不下，重新放回收纳袋
                mutableContents.tryInsert(remaining);
            } else {
                playRemoveOneSound(player);
            }
        }
        setContents(heldStack, mutableContents.toImmutable());
        return true;
    }

    return false;
}

bool BundleItem::overrideOtherStackedOnMe(
    ItemStack& bundleStack, ItemStack& cursorStack, Slot& slot, SlotClickAction clickAction, Player& player)
{
    // 对应 MC 1.21.11 BundleItem#overrideOtherStackedOnMe
    if (clickAction == SlotClickAction::Primary && cursorStack.isEmpty()) {
        // 左键 + 光标为空：切换选中项（不处理）
        toggleSelectedItem(bundleStack, -1);
        return false;
    }

    BundleContents contents = getContents(bundleStack);
    BundleContents::Mutable mutableContents(contents);

    if (clickAction == SlotClickAction::Primary && !cursorStack.isEmpty()) {
        // 左键 + 光标有物品：尝试插入收纳袋
        if (slot.allowModification(player)) {
            i32 inserted = mutableContents.tryInsert(cursorStack);
            if (inserted > 0) {
                playInsertSound(player);
            } else {
                playInsertFailSound(player);
            }
        } else {
            playInsertFailSound(player);
        }
        setContents(bundleStack, mutableContents.toImmutable());
        return true;
    }

    if (clickAction == SlotClickAction::Secondary && cursorStack.isEmpty()) {
        // 右键 + 光标为空：从收纳袋取出一项到光标
        if (slot.allowModification(player)) {
            auto removed = mutableContents.removeOne();
            if (removed.has_value()) {
                playRemoveOneSound(player);
                cursorStack = std::move(removed).value();
            }
        }
        setContents(bundleStack, mutableContents.toImmutable());
        return true;
    }

    toggleSelectedItem(bundleStack, -1);
    return false;
}

// ============================================================================
// 静态工具方法
// ============================================================================

void BundleItem::toggleSelectedItem(ItemStack& stack, i32 index)
{
    BundleContents contents = getContents(stack);
    BundleContents::Mutable mutableContents(contents);
    mutableContents.toggleSelectedItem(index);
    setContents(stack, mutableContents.toImmutable());
}

bool BundleItem::hasSelectedItem(const ItemStack& stack)
{
    return getContents(stack).hasSelectedItem();
}

i32 BundleItem::getSelectedItem(const ItemStack& stack)
{
    return getContents(stack).selectedItem();
}

ItemStack BundleItem::getSelectedItemStack(const ItemStack& stack)
{
    BundleContents contents = getContents(stack);
    if (!contents.hasSelectedItem()) {
        return ItemStack::EMPTY;
    }
    return contents.getItemUnsafe(contents.selectedItem()).copy();
}

i32 BundleItem::getNumberOfItemsToShow(const ItemStack& stack)
{
    return getContents(stack).numberOfItemsToShow();
}

f32 BundleItem::getFullnessDisplay(const ItemStack& stack)
{
    BundleContents contents = getContents(stack);
    // 权重 / MAX_WEIGHT = 满度（0.0~1.0）
    return static_cast<f32>(contents.weight()) / static_cast<f32>(BundleContents::MAX_WEIGHT);
}

bool BundleItem::isBundleItem(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    const ResourceLocation& id = stack.getItem()->itemLocation();
    const std::string& path = id.path();
    if (path == "bundle") {
        return true;
    }
    return path.size() > 7 && path.compare(path.size() - 7, 7, "_bundle") == 0;
}

Item* BundleItem::getByColor(DyeColor color)
{
    auto& registry = ItemRegistry::instance();
    switch (color) {
        case DyeColor::White:
            return registry.getItem(ResourceLocation("minecraft", "white_bundle"));
        case DyeColor::Orange:
            return registry.getItem(ResourceLocation("minecraft", "orange_bundle"));
        case DyeColor::Magenta:
            return registry.getItem(ResourceLocation("minecraft", "magenta_bundle"));
        case DyeColor::LightBlue:
            return registry.getItem(ResourceLocation("minecraft", "light_blue_bundle"));
        case DyeColor::Yellow:
            return registry.getItem(ResourceLocation("minecraft", "yellow_bundle"));
        case DyeColor::Lime:
            return registry.getItem(ResourceLocation("minecraft", "lime_bundle"));
        case DyeColor::Pink:
            return registry.getItem(ResourceLocation("minecraft", "pink_bundle"));
        case DyeColor::Gray:
            return registry.getItem(ResourceLocation("minecraft", "gray_bundle"));
        case DyeColor::LightGray:
            return registry.getItem(ResourceLocation("minecraft", "light_gray_bundle"));
        case DyeColor::Cyan:
            return registry.getItem(ResourceLocation("minecraft", "cyan_bundle"));
        case DyeColor::Purple:
            return registry.getItem(ResourceLocation("minecraft", "purple_bundle"));
        case DyeColor::Blue:
            return registry.getItem(ResourceLocation("minecraft", "blue_bundle"));
        case DyeColor::Brown:
            return registry.getItem(ResourceLocation("minecraft", "brown_bundle"));
        case DyeColor::Green:
            return registry.getItem(ResourceLocation("minecraft", "green_bundle"));
        case DyeColor::Red:
            return registry.getItem(ResourceLocation("minecraft", "red_bundle"));
        case DyeColor::Black:
            return registry.getItem(ResourceLocation("minecraft", "black_bundle"));
        case DyeColor::Count:
            return registry.getItem(ResourceLocation("minecraft", "bundle"));
    }
    return registry.getItem(ResourceLocation("minecraft", "bundle"));
}

// ============================================================================
// 私有方法
// ============================================================================

bool BundleItem::dropContent(ItemStack& bundleStack, Player& player)
{
    BundleContents contents = getContents(bundleStack);
    if (contents.isEmpty()) {
        return false;
    }

    BundleContents::Mutable mutableContents(contents);
    auto removed = mutableContents.removeOne();
    if (!removed.has_value()) {
        return false;
    }

    playRemoveOneSound(player);
    setContents(bundleStack, mutableContents.toImmutable());

    // 丢弃物品到世界
    ItemStack toDrop = std::move(removed).value();
    player.dropItem(toDrop, false, false);
    return true;
}

void BundleItem::playRemoveOneSound(Entity& entity)
{
    entity.playSound(SoundEvents::ITEM_BUNDLE_REMOVE_ONE, 0.8f, 0.8f);
}

void BundleItem::playInsertSound(Entity& entity)
{
    entity.playSound(SoundEvents::ITEM_BUNDLE_INSERT, 0.8f, 0.8f);
}

void BundleItem::playInsertFailSound(Entity& entity)
{
    entity.playSound(SoundEvents::ITEM_BUNDLE_INSERT_FAIL, 1.0f, 1.0f);
}

void BundleItem::playDropContentsSound(Entity& entity)
{
    entity.playSound(SoundEvents::ITEM_BUNDLE_DROP_CONTENTS, 0.8f, 0.8f);
}

BundleContents BundleItem::getContents(const ItemStack& stack)
{
    return BundleContents::fromItemStack(stack);
}

void BundleItem::setContents(ItemStack& stack, const BundleContents& contents)
{
    // NBT 路径：tag.BundleContents（与 BundleContents::fromItemStack 一致）
    if (contents.isEmpty()) {
        // 空内容物：移除标签
        nlohmann::json* tag = stack.getTag();
        if (tag != nullptr) {
            tag->erase("BundleContents");
        }
        return;
    }
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["BundleContents"] = contents.toJson();
}

} // namespace item::items
} // namespace mc
