#include "BowItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../enchantment/EnchantmentHelper.hpp"
#include "../../enchantment/enchantments/AllEnchantments.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/AbstractArrowEntity.hpp"
#include "../../../entity/damage/DamageSource.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 常量 ==========
namespace {
    constexpr i32 MAX_USE_DURATION = 72000;   // MC 1.16.5: 几乎无限制
    constexpr f32 MIN_VELOCITY = 0.1f;         // 最小发射速度
    constexpr f32 MAX_VELOCITY = 3.0f;         // 最大箭矢速度
    constexpr i32 FULL_CHARGE_TICKS = 20;      // 满蓄力 tick 数
    constexpr i32 FLAME_DURATION = 100;        // 火矢持续时间 (5 秒 = 100 tick)
}

// ========== 构造函数 ==========

BowItem::BowItem(const ItemProperties& properties)
    : Item(properties)
{
}

// ========== Item 接口重写 ==========

i32 BowItem::getUseDuration(const ItemStack& stack) const {
    (void)stack;
    return MAX_USE_DURATION;
}

UseAction BowItem::getUseAction(const ItemStack& stack) const {
    (void)stack;
    return UseAction::Bow;
}

ActionResult BowItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    (void)world;

    ItemStack bowStack = player.getHeldItem(hand);

    // 检查是否有箭矢或无限附魔
    bool hasInfinity = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        bowStack, &item::enchant::AllEnchantments::INFINITY) > 0;
    bool isCreative = player.isCreativeMode();

    ItemStack ammoStack = findAmmo(player, bowStack);
    bool hasAmmo = !ammoStack.isEmpty();

    // 创造模式或有箭矢或无限附魔时才能使用
    if (isCreative || hasAmmo || hasInfinity) {
        player.setActiveHand(hand);
        return ActionResult::success(bowStack);
    }

    return ActionResult::fail(bowStack);
}

void BowItem::onPlayerStoppedUse(
    ItemStack& stack,
    IWorld& world,
    LivingEntity& entity,
    i32 timeLeft)
{
    // 检查是否是玩家
    Player* player = dynamic_cast<Player*>(&entity);
    if (player == nullptr) {
        return;
    }

    // 计算蓄力时间
    i32 chargeTicks = getUseDuration(stack) - timeLeft;
    if (chargeTicks < 0) {
        return;
    }

    // 计算箭矢速度
    f32 velocity = getArrowVelocity(chargeTicks);
    if (velocity < MIN_VELOCITY) {
        // 蓄力不足，不发射
        return;
    }

    // 查找箭矢
    ItemStack ammoStack = findAmmo(*player, stack);
    bool hasInfinity = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        stack, &item::enchant::AllEnchantments::INFINITY) > 0;
    bool isCreative = player->isCreativeMode();

    // 无箭矢且无无限附魔时使用默认箭
    if (ammoStack.isEmpty()) {
        if (hasInfinity || isCreative) {
            // 使用默认箭矢
            ammoStack = ItemStack();  // TODO: Items::ARROW
        } else {
            return;  // 无箭矢
        }
    }

    // 检查箭矢是否无限（不被消耗）
    bool isArrowInfinite = isInfiniteArrow(ammoStack, stack, *player);

    // 创建箭矢实体
    // TODO: 需要实现 ArrowItem::createArrow
    // AbstractArrowEntity* arrow = ...;

    // 暂时使用占位符，等待 ArrowItem 实现
    // arrow->shoot(player, player->pitch(), player->yaw(), 0.0f, velocity * MAX_VELOCITY, 1.0f);

    // 设置暴击（满蓄力时）
    if (velocity >= 1.0f) {
        // arrow->setCritical(true);
    }

    // 应用力量附魔
    i32 powerLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        stack, &item::enchant::AllEnchantments::POWER);
    if (powerLevel > 0) {
        // MC 1.16.5: 每级 +0.5 伤害 + 0.5
        // arrow->setDamage(arrow->damage() + static_cast<f32>(powerLevel) * 0.5f + 0.5f);
    }

    // 应用冲击附魔
    i32 punchLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        stack, &item::enchant::AllEnchantments::PUNCH);
    if (punchLevel > 0) {
        // arrow->setKnockbackStrength(punchLevel);
    }

    // 应用火矢附魔
    i32 flameLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        stack, &item::enchant::AllEnchantments::FLAME);
    if (flameLevel > 0) {
        // arrow->setFire(FLAME_DURATION);
    }

    // 消耗弓的耐久度
    if (!isCreative) {
        stack.damageItem(1, *player);
    }

    // 设置箭矢拾取状态
    // if (isArrowInfinite || isCreative) {
    //     arrow->setPickupStatus(PickupStatus::CreativeOnly);
    // } else {
    //     arrow->setPickupStatus(PickupStatus::Allowed);
    // }

    // 生成箭矢实体
    // world.spawnEntity(arrow);

    // 播放音效
    // TODO: 播放弓箭发射音效，音调基于蓄力时间

    // 消耗箭矢
    if (!isArrowInfinite && !isCreative && !ammoStack.isEmpty()) {
        ammoStack.shrink(1);
    }

    // TODO: 添加统计信息
}

// ========== 弓特有方法 ==========

std::function<bool(const ItemStack&)> BowItem::getAmmoPredicate() const {
    // TODO: 实现 ItemTags::ARROWS 检查
    return [](const ItemStack& stack) -> bool {
        if (stack.isEmpty()) {
            return false;
        }
        // 暂时检查是否为箭矢物品
        // return stack.getItem()->isArrow();
        (void)stack;
        return false;
    };
}

std::function<bool(const ItemStack&)> BowItem::getInventoryAmmoPredicate() const {
    return getAmmoPredicate();
}

f32 BowItem::getArrowVelocity(i32 chargeTicks) {
    // MC 1.16.5 BowItem.getArrowVelocity()
    // f = charge / 20.0
    // f = (f * f + f * 2.0) / 3.0
    // 最大 1.0
    if (chargeTicks <= 0) {
        return 0.0f;
    }

    f32 f = static_cast<f32>(chargeTicks) / static_cast<f32>(FULL_CHARGE_TICKS);
    f = (f * f + f * 2.0f) / 3.0f;

    return math::clamp(f, 0.0f, 1.0f);
}

AbstractArrowEntity* BowItem::customArrow(AbstractArrowEntity* arrow) {
    return arrow;
}

// ========== 私有方法 ==========

ItemStack BowItem::findAmmo(Player& player, const ItemStack& bowStack) const {
    (void)bowStack;

    // 检查副手
    ItemStack offhand = player.getHeldItem(Hand::OffHand);
    if (getAmmoPredicate()(offhand)) {
        return offhand;
    }

    // 检查主手
    ItemStack mainhand = player.getHeldItem(Hand::MainHand);
    if (getAmmoPredicate()(mainhand)) {
        return mainhand;
    }

    // TODO: 检查背包槽位

    return ItemStack();
}

bool BowItem::isInfiniteArrow(const ItemStack& arrowStack,
                               const ItemStack& bowStack,
                               Player& player) const {
    if (player.isCreativeMode()) {
        return true;
    }

    // 检查无限附魔
    i32 infinityLevel = item::enchant::EnchantmentHelper::getEnchantmentLevel(
        bowStack, &item::enchant::AllEnchantments::INFINITY);

    if (infinityLevel <= 0) {
        return false;
    }

    // MC 1.16.5: 只有普通箭矢受益于无限附魔
    // 光灵箭和药水箭不受无限影响
    // TODO: 实现 ArrowItem::isInfinite
    // if (const ArrowItem* arrowItem = dynamic_cast<const ArrowItem*>(arrowStack.getItem())) {
    //     return arrowItem->isInfinite(arrowStack, bowStack, player);
    // }

    (void)arrowStack;
    return false;
}

} // namespace item
} // namespace mc
