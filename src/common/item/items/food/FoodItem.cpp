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

#include "FoodItem.hpp"

#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/effect/EffectInstance.hpp"
#include "../../../entity/effect/EffectType.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../sound/SoundEvents.hpp"
#include "../../../world/IWorld.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"
#include "common/core/Types.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/food/Food.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentEvents.hpp"
#include "common/mod/bedrock/addon/component/ItemComponentRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include <chrono>
#include <string>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace item::items {

using namespace entity::effect;

/**
 * @brief 构造食物物品
 */
FoodItem::FoodItem(const food::Food* food, ItemProperties properties)
    : Item(std::move(properties))
    , m_food(food)
{}

/**
 * @brief 获取使用时长
 *
 * 快速食用：16 ticks (0.8秒)
 * 普通食用：32 ticks (1.6秒)
 */
i32 FoodItem::getUseDuration(const ItemStack& /*stack*/) const
{
    if (m_food != nullptr && m_food->isFastEat()) {
        return 16;
    }
    return 32;
}

/**
 * @brief 获取使用动作
 *
 * 所有食物都返回 EAT 动作，
 * isMeat() 标记仅用于狼是否能食用，不影响使用动作。
 */
UseAction FoodItem::getUseAction(const ItemStack& /*stack*/) const
{
    if (m_food != nullptr) {
        return UseAction::Eat;
    }
    return UseAction::None;
}

/**
 * @brief 右键使用物品
 */
ItemActionResult FoodItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand)
{
    if (m_food == nullptr) {
        return ItemActionResult::pass(player.getHeldItem(hand));
    }

    ItemStack stack = player.getHeldItem(hand);

    // 检查是否可以食用
    // 创造模式下玩家可以吃任何食物（abilities.disableDamage 检查）
    bool canEat = player.abilities().invulnerable || m_food->canAlwaysEat() || player.foodStats().needsFood();

    if (canEat) {
        // 设置活跃手，开始进食（对齐基础 Item::onItemRightClick 的 isFood 分支与
        // BowItem/PotionItem/MilkBucketItem 范式）。setActiveHand 设置 m_activeItem +
        // m_activeItemUseCount=getUseDuration，后续 LivingEntity::tick → updatingUsingItem
        // 递减计时器，到 0 时调 onItemUseFinish 完成食用。此前此处仅返回 Consume 未调
        // setActiveHand，食物无法进入使用状态，onItemUseFinish 永不触发（食用完成链路断裂）。
        player.setActiveHand(hand);
        return ItemActionResult::consume(stack);
    }

    // 饱食时返回 Fail（不是 Pass）
    return ItemActionResult::fail(stack);
}

/**
 * @brief 使用完成（食用完成）
 *
 * 处理逻辑：
 * 1. 恢复饥饿值和饱和度
 * 2. 应用药水效果（带概率）
 * 3. 播放进食音效
 * 4. 播放打嗝音效（玩家专用）
 * 5. 减少物品数量（创造模式不减）
 * 6. 返回容器物品
 * 7. 触发消耗物品事件（进度系统）
 */
ItemStack FoodItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    if (m_food == nullptr) {
        return stack;
    }

    // 尝试转换为玩家和生物实体
    Player* player = dynamic_cast<Player*>(&entity);
    LivingEntity* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    bool isCreativePlayer = (player != nullptr && player->abilities().creativeMode);

    // 保存消耗前的物品副本（用于事件）
    ItemStack consumedItem = stack.copy();

    // 使用实体ID和时间生成随机数（用于效果概率和音调变化）
    math::Random rng(static_cast<u64>(entity.id()) ^
        static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

    // 应用药水效果（玩家和生物实体均可获得食物效果）
    // 优先应用食物自身定义的效果（如金苹果、河豚等）
    if (m_food->hasEffects()) {
        for (const auto& effect : m_food->getEffects()) {
            if (rng.nextFloat() < effect.probability) {
                EffectInstance instance(effect.type,
                    effect.duration,
                    effect.amplifier,
                    false, // 非环境效果
                    true,  // 显示粒子
                    true   // 显示图标
                );
                if (player != nullptr) {
                    player->addEffect(instance);
                } else if (livingEntity != nullptr) {
                    livingEntity->addEffect(std::move(instance));
                }
            }
        }
    }

    // 应用迷之炖菜效果（从物品 NBT 标签读取）
    // NBT 格式: {Effects: [{EffectId: byte, EffectDuration: int}, ...]}
    if (stack.hasTag()) {
        const nlohmann::json* tag = stack.getTag();
        if (tag != nullptr && tag->contains("Effects") && (*tag)["Effects"].is_array()) {
            for (const auto& effectEntry : (*tag)["Effects"]) {
                if (!effectEntry.contains("EffectId") || !effectEntry.contains("EffectDuration")) {
                    continue;
                }
                auto effectIdIt = effectEntry.find("EffectId");
                auto durationIt = effectEntry.find("EffectDuration");
                if (effectIdIt == effectEntry.end() || durationIt == effectEntry.end()) {
                    continue;
                }
                i32 effectId = 0;
                if (effectIdIt->is_number_integer()) {
                    effectId = effectIdIt->get<i32>();
                }
                i32 durationTicks = 0;
                if (durationIt->is_number_integer()) {
                    durationTicks = durationIt->get<i32>();
                }
                auto effectType = getEffectById(effectId);
                if (!effectType.has_value()) {
                    continue;
                }
                EffectInstance instance(effectType.value(),
                    durationTicks,
                    0,     // amplifier = 0（迷之炖菜效果等级固定为 I）
                    false, // 非环境效果
                    true,  // 显示粒子
                    true   // 显示图标
                );
                if (player != nullptr) {
                    player->addEffect(instance);
                } else if (livingEntity != nullptr) {
                    livingEntity->addEffect(std::move(instance));
                }
            }
        }
    }

    // 玩家专用逻辑：饥饿恢复、进食/打嗝音效、进度事件
    if (player != nullptr) {
        // 恢复饥饿值和饱和度
        player->foodStats().addStats(m_food->getHunger(), m_food->getSaturationModifier());

        // 重置食物计时器（用于生命恢复计时）
        player->foodStats().setFoodTimer(0);

        // 播放进食音效
        f32 pitch = 0.8f + (rng.nextFloat() * 0.4f);
        f32 volume = 0.5f + (rng.nextFloat() * 0.5f);
        player->playSound(SoundEvents::ENTITY_GENERIC_EAT, volume, pitch);

        // 播放打嗝音效（玩家专用，进食完成）
        player->playSound(SoundEvents::ENTITY_PLAYER_BURP, 0.5f, pitch);
    }

    // 减少物品数量（创造模式不减）
    if (!isCreativePlayer) {
        stack.shrink(1);
    }

    // 触发消耗物品事件（进度系统，仅玩家）
    if (player != nullptr) {
        world.onConsumeItem(player->id(), consumedItem);
    }

    // 派发自定义物品组件回调 - onConsume
    auto& itemCompReg = mc::mod::bedrock::addon::ItemComponentRegistry::instance();
    std::string itemTypeId = itemLocation().toString();
    if (itemCompReg.hasConsumeCallback(itemTypeId)) {
        mc::mod::bedrock::addon::ItemComponentConsumeEvent consumeEvent;
        consumeEvent.itemTypeId = itemTypeId;
        consumeEvent.sourceId = entity.id();
        consumeEvent.itemStackAmount = consumedItem.getCount();
        itemCompReg.dispatchConsume(itemTypeId, consumeEvent);
    }

    // 返回容器物品（对齐 vanilla：食用容器食物后返回容器物品）。
    // 持多个容器食物（如2个蘑菇煲）时，shrink 后 stack 非空（剩1个），应返回 shrink 后的 stack
    // （剩余食物留主手）+ 容器物品（碗/玻璃瓶/空桶）放背包；仅持1个（stack 空时）才返回容器物品
    // 替换主手。此前无条件返回容器物品致持多个容器食物食用时剩余食物丢失（主手被容器物品覆盖）。
    // 参考 vanilla LivingEntity#eat / Item#finishUsingItem 容器物品处理范式。
    if (hasContainerItem()) {
        if (stack.isEmpty()) {
            // 数量归零（持1个）：返回容器物品替换主手。
            return ItemStack(containerItem(), 1);
        }
        // 数量>0（持多个）：返回 shrink 后的 stack（剩余食物留主手），容器物品放背包。
        if (player != nullptr && !isCreativePlayer) {
            // 创造模式不返还容器物品（vanilla 创造食用不返还容器）。
            ItemStack container(containerItem(), 1);
            const i32 remaining = player->inventory().add(container);
            if (remaining > 0 && !container.isEmpty()) {
                // 背包满，容器物品掉落到地面（对齐 MilkBucketItem/PotionItem 范式）。
                math::Random rng;
                ItemDropHelper::spawnItemAtEntity(player, container, 0.5f, rng);
            }
        }
        return stack;
    }

    return stack;
}

/**
 * @brief 是否可以食用
 */
bool FoodItem::canEat(const ItemStack& /*stack*/, const Player& player) const
{
    if (m_food == nullptr) {
        return false;
    }

    // 创造模式无敌状态可以吃任何食物
    if (player.abilities().invulnerable) {
        return true;
    }

    // 可以在饱食时食用（如金苹果）
    if (m_food->canAlwaysEat()) {
        return true;
    }

    // 需要食物
    return player.foodStats().needsFood();
}

} // namespace item::items
} // namespace mc
