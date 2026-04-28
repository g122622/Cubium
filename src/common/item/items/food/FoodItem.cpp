#include "FoodItem.hpp"

#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../entity/effect/EffectInstance.hpp"
#include "../../../sound/SoundEvents.hpp"

namespace mc {
namespace item::items {

using namespace entity::effect;

/**
 * @brief 构造食物物品
 */
FoodItem::FoodItem(const food::Food* food, ItemProperties properties)
    : Item(std::move(properties))
    , m_food(food) {
}

/**
 * @brief 获取使用时长
 *
 * 快速食用：16 ticks (0.8秒)
 * 普通食用：32 ticks (1.6秒)
 */
i32 FoodItem::getUseDuration(const ItemStack& /*stack*/) const {
    if (m_food != nullptr && m_food->isFastEat()) {
        return 16;
    }
    return 32;
}

/**
 * @brief 获取使用动作
 *
 * MC 1.16.5 中所有食物都返回 EAT 动作，
 * isMeat() 标记仅用于狼是否能食用，不影响使用动作。
 */
UseAction FoodItem::getUseAction(const ItemStack& /*stack*/) const {
    if (m_food != nullptr) {
        return UseAction::Eat;
    }
    return UseAction::None;
}

/**
 * @brief 右键使用物品
 */
ItemActionResult FoodItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand) {
    if (m_food == nullptr) {
        return ItemActionResult::pass(player.getHeldItem(hand));
    }

    ItemStack stack = player.getHeldItem(hand);

    // 检查是否可以食用
    // 创造模式下玩家可以吃任何食物（abilities.disableDamage 检查）
    bool canEat = player.abilities().invulnerable || m_food->canAlwaysEat() || player.foodStats().needsFood();

    if (canEat) {
        // 设置活跃手，开始进食
        // NOTE: setActiveHand() 需要在 Player 动画系统完成后实现
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
 */
ItemStack FoodItem::onItemUseFinish(ItemStack& stack, IWorld& /*world*/, Entity& entity) {
    if (m_food == nullptr) {
        return stack;
    }

    // 尝试转换为玩家
    Player* player = dynamic_cast<Player*>(&entity);
    bool isCreativePlayer = (player != nullptr && player->abilities().creativeMode);

    // 只有玩家才处理饥饿恢复
    if (player != nullptr) {
        // 恢复饥饿值和饱和度
        // 饱和度计算：saturation += food * saturationModifier * 2.0
        player->foodStats().addStats(m_food->getHunger(), m_food->getSaturationModifier());

        // 重置食物计时器（用于生命恢复计时）
        player->foodStats().setFoodTimer(0);

        // 使用实体ID和时间生成随机数（用于效果概率和音调变化）
        math::Random rng(static_cast<u64>(entity.id()) ^
                        static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

        // 应用药水效果
        if (m_food->hasEffects()) {
            for (const auto& effect : m_food->getEffects()) {
                if (rng.nextFloat() < effect.probability) {
                    EffectInstance instance(
                        effect.type,
                        effect.duration,
                        effect.amplifier,
                        false,  // 非环境效果
                        true,   // 显示粒子
                        true    // 显示图标
                    );
                    player->addEffect(instance);
                }
            }
        }

        // 播放进食音效
        // MC 1.16.5: 所有食物使用通用进食音效
        // 音调在 0.8-1.2 范围内随机变化
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

    // 返回容器物品
    if (hasContainerItem()) {
        return ItemStack(containerItem(), 1);
    }

    return stack;
}

/**
 * @brief 是否可以食用
 */
bool FoodItem::canEat(const ItemStack& /*stack*/, const Player& player) const {
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
