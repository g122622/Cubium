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

#include "PotionItem.hpp"

#include "../../../core/Types.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/effect/EffectType.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../Items.hpp"
#include "../../potion/PotionUtils.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/util/math/random/Random.hpp"
#include <string>
#include <utility>

namespace mc {
namespace item {

/**
 * @brief 构造药水物品
 */
PotionItem::PotionItem(const ItemProperties& properties)
    : Item(properties)
{}

/**
 * @brief 获取使用时长
 */
i32 PotionItem::getUseDuration(const ItemStack& /*stack*/) const
{
    // 药水饮用需要 32 ticks
    return 32;
}

/**
 * @brief 获取使用动作
 */
UseAction PotionItem::getUseAction(const ItemStack& /*stack*/) const
{
    return UseAction::Drink;
}

/**
 * @brief 使用完成
 *
 * 应用药水效果，消耗物品，返回玻璃瓶
 */
ItemStack PotionItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity)
{
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);

    // 应用效果
    if (potion != nullptr) {
        _applyEffects(potion, entity, world);
    }

    // 检查是否是玩家
    Player* player = dynamic_cast<Player*>(&entity);

    // 消耗物品
    if (player != nullptr && !player->isCreative()) {
        stack.shrink(1);
    } else if (player == nullptr) {
        // 非玩家实体也消耗物品
        stack.shrink(1);
    }

    // 返回玻璃瓶（对齐 vanilla Item#finishUsingItem 药水分支 + MilkBucketItem 范式）。
    // shrink 后空（持1个）：返回玻璃瓶替换主手。
    // shrink 后非空（持多个）：返回 shrink 后 stack（剩余药水留主手）+ 玻璃瓶 inventory.add 入背包；
    //   背包满（remaining>0）则玻璃瓶掉落地面（spawnItemAtEntity），不覆盖主手剩药水。
    //   此前背包满返回 ItemStack(GLASS_BOTTLE, remaining) 覆盖主手剩药水是偏差（vanilla 不会覆盖），
    //   已改为掉落玻璃瓶（对齐 MilkBucketItem）。
    if (player != nullptr && !player->isCreative()) {
        if (stack.isEmpty()) {
            return ItemStack(Items::GLASS_BOTTLE, 1);
        }
        // 持多个：玻璃瓶入背包，背包满掉落。
        ItemStack glassBottle(Items::GLASS_BOTTLE, 1);
        const i32 remaining = player->inventory().add(glassBottle);
        if (remaining > 0 && !glassBottle.isEmpty()) {
            // 背包满，玻璃瓶掉落到地面（对齐 MilkBucketItem::onItemUseFinish 范式）。
            math::Random rng;
            ItemDropHelper::spawnItemAtEntity(player, glassBottle, 0.5f, rng);
        }
        return stack;
    }

    return stack;
}

/**
 * @brief 右键使用物品
 *
 * 药水可以随时饮用，不像食物需要饥饿
 */
ItemActionResult PotionItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand)
{
    player.setActiveHand(hand);
    return ItemActionResult(ActionResultType::Success, player.getHeldItem(hand));
}

/**
 * @brief 是否有药水效果
 */
bool PotionItem::hasEffect(const ItemStack& stack) const
{
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

/**
 * @brief 获取翻译键
 */
std::string PotionItem::getTranslationKey(const ItemStack& stack) const
{
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return std::string("item.minecraft.potion.effect.") + potion->baseName();
    }
    return Item::getTranslationKey(stack);
}

/**
 * @brief 将药水效果应用到实体
 *
 * 瞬间效果（瞬间治疗、瞬间伤害）直接应用治疗/伤害逻辑。
 * 饱和效果通过 addEffect 路径处理（EffectManager 会对瞬间效果立即执行后移除）。
 * 其他非瞬间效果通过 addEffect 添加到实体的效果列表。
 */
void PotionItem::_applyEffects(const potion::Potion* potion, Entity& entity, IWorld& /*world*/)
{
    if (potion == nullptr) {
        return;
    }

    LivingEntity* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity == nullptr) {
        return;
    }

    for (const auto& effect : potion->effects()) {
        // 瞬间治疗/伤害效果直接应用（不通过 addEffect 系统）。
        // 复用 EffectInstance::applyInstantly 统一公式与反转判定（4<<level 治疗 / 6<<level 伤害，
        // INVERTED_HEALING_AND_HARM 标签反转），避免与 EffectInstance/EffectEntities 路径产生行为分叉。
        if (effect.type() == entity::effect::EffectType::InstantHealth ||
            effect.type() == entity::effect::EffectType::InstantDamage) {
            effect.applyInstantly(*livingEntity);
            continue;
        }

        // 其他效果（包括饱和效果）通过 addEffect 添加
        // EffectManager::addEffect 对瞬间效果（如饱和）会立即执行效果逻辑后移除
        entity::effect::EffectInstance newEffect(effect);
        livingEntity->addEffect(std::move(newEffect));
    }
}

} // namespace item
} // namespace mc
