#include "PotionItem.hpp"

#include "../../potion/PotionUtils.hpp"
#include "../../potion/Potions.hpp"
#include "../../Items.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/effect/EffectType.hpp"

namespace mc {
namespace item {

/**
 * @brief 构造药水物品
 */
PotionItem::PotionItem(const ItemProperties& properties)
    : Item(properties) {
}

/**
 * @brief 获取使用时长
 */
i32 PotionItem::getUseDuration(const ItemStack& /*stack*/) const {
    // MC 1.16.5: 32 ticks
    return 32;
}

/**
 * @brief 获取使用动作
 */
UseAction PotionItem::getUseAction(const ItemStack& /*stack*/) const {
    return UseAction::Drink;
}

/**
 * @brief 使用完成
 *
 * MC 1.16.5 对齐:
 * 1. 应用药水效果（瞬间效果直接应用，非瞬间效果添加到实体）
 * 2. 消耗物品（非创造模式）
 * 3. 返回玻璃瓶
 */
ItemStack PotionItem::onItemUseFinish(ItemStack& stack, IWorld& world, Entity& entity) {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);

    // 应用效果
    if (potion != nullptr) {
        applyEffects(potion, entity, world);
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

    // 返回玻璃瓶
    // MC 1.16.5: 如果物品堆为空，返回玻璃瓶；否则将玻璃瓶添加到背包
    if (player != nullptr && !player->isCreative()) {
        if (stack.isEmpty()) {
            return ItemStack(Items::GLASS_BOTTLE, 1);
        } else {
            // 尝试将玻璃瓶添加到背包
            // TODO: 实现 player->inventory().addItem()
            // 暂时直接返回玻璃瓶
            return ItemStack(Items::GLASS_BOTTLE, 1);
        }
    }

    return stack;
}

/**
 * @brief 右键使用物品
 */
ItemActionResult PotionItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    // MC 1.16.5: 使用 DrinkHelper.startDrinking()
    // 药水可以随时饮用，不像食物需要饥饿
    // 检查玩家是否可以饮用（不是正在使用其他物品）
    player.setActiveHand(hand);
    return ItemActionResult(ActionResultType::Success, player.getHeldItem(hand));
}

/**
 * @brief 是否有药水效果
 */
bool PotionItem::hasEffect(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    return potion != nullptr && potion->hasEffects();
}

/**
 * @brief 获取翻译键
 */
String PotionItem::getTranslationKey(const ItemStack& stack) const {
    const potion::Potion* potion = potion::PotionUtils::getPotion(stack);
    if (potion != nullptr && potion->hasEffects()) {
        return String("item.minecraft.potion.effect.") + potion->baseName();
    }
    return Item::getTranslationKey(stack);
}

/**
 * @brief 将药水效果应用到实体
 *
 * MC 1.16.5 对齐:
 * - 瞬间效果：直接应用
 * - 非瞬间效果：添加到实体
 */
void PotionItem::applyEffects(const potion::Potion* potion, Entity& entity, IWorld& /*world*/) {
    if (potion == nullptr) {
        return;
    }

    LivingEntity* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity == nullptr) {
        return;
    }

    for (const auto& effect : potion->effects()) {
        // MC 1.16.5: 瞬间效果直接应用
        if (effect.type() == entity::effect::EffectType::InstantHealth ||
            effect.type() == entity::effect::EffectType::InstantDamage) {
            // TODO: 实现瞬间效果的应用
            // affectEntity(player, player, entity, amplifier, 1.0D)
            continue;
        }

        // 非瞬间效果添加到实体
        entity::effect::EffectInstance newEffect(effect);
        livingEntity->addEffect(std::move(newEffect));
    }
}

} // namespace item
} // namespace mc
