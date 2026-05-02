#include "TridentItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../enchantment/EnchantmentHelper.hpp"
#include "../../enchantment/enchantments/AllEnchantments.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/projectile/TridentEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../sound/SoundEvents.hpp"
#include <cmath>

namespace mc {
namespace item {

// ========== 常量 ==========
namespace {
    constexpr i32 MAX_USE_DURATION = 72000;   // MC 1.16.5: 几乎无限制
    constexpr i32 MIN_CHARGE_TICKS = 10;       // 最小投掷蓄力时间
    constexpr f32 THROW_VELOCITY = 2.5f;       // 基础投掷速度
}

// ========== 构造函数 ==========

TridentItem::TridentItem(const ItemProperties& properties)
    : Item(properties)
{
}

// ========== Item 接口重写 ==========

i32 TridentItem::getUseDuration(const ItemStack& stack) const {
    (void)stack;
    return MAX_USE_DURATION;
}

UseAction TridentItem::getUseAction(const ItemStack& stack) const {
    (void)stack;
    return UseAction::Spear;
}

ItemActionResult TridentItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    (void)world;

    ItemStack tridentStack = player.getHeldItem(hand);

    // 检查耐久度
    if (tridentStack.getDamage() >= tridentStack.getMaxDamage() - 1) {
        // 耐久度即将耗尽，无法使用
        return ItemActionResult::fail(tridentStack);
    }

    // 激流附魔检查：如果不在水中/雨中且没有激流，则不能使用
    i32 riptideLevel = getRiptideLevel(tridentStack);
    if (riptideLevel > 0 && !isWet(player)) {
        return ItemActionResult::fail(tridentStack);
    }

    player.setActiveHand(hand);
    return ItemActionResult::success(tridentStack);
}

void TridentItem::onPlayerStoppedUsing(
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
    if (chargeTicks < MIN_CHARGE_TICKS) {
        // 蓄力不足，不投掷
        return;
    }

    // 获取激流附魔等级
    i32 riptideLevel = getRiptideLevel(stack);

    // 激流附魔：需要潮湿才能使用
    if (riptideLevel > 0 && !isWet(*player)) {
        return;
    }

    // 消耗耐久度
    if (!player->isCreative()) {
        stack.attemptDamageItem(1);
    }

    // 激流模式：冲刺而不是投掷
    if (riptideLevel > 0) {
        // 计算冲刺速度
        f32 yaw = player->yaw();
        f32 pitch = player->pitch();
        constexpr f32 DEG_TO_RAD = math::DEG_TO_RAD;

        f32 f2 = -std::sin(yaw * DEG_TO_RAD) * std::cos(pitch * DEG_TO_RAD);
        f32 f3 = -std::sin(pitch * DEG_TO_RAD);
        f32 f4 = std::cos(yaw * DEG_TO_RAD) * std::cos(pitch * DEG_TO_RAD);
        f32 f5 = std::sqrt(f2 * f2 + f3 * f3 + f4 * f4);
        f32 f6 = 3.0f * ((1.0f + static_cast<f32>(riptideLevel)) / 4.0f);
        f2 = f2 * (f6 / f5);
        f3 = f3 * (f6 / f5);
        f4 = f4 * (f6 / f5);

        // 给玩家加速度
        player->addVelocity(f2, f3, f4);

        // 开始旋转攻击
        // player->startSpinAttack(20);

        // 如果在地面，额外提升
        if (player->isOnGround()) {
            player->addVelocity(0.0, 1.1999999, 0.0);
        }

        // 播放激流音效
        // MC 1.16.5: 根据激流等级播放不同音效
        math::Random rng;
        if (player != nullptr) {
            const ResourceLocation* soundEvent = nullptr;
            switch (riptideLevel) {
                case 1:
                    soundEvent = &SoundEvents::ITEM_TRIDENT_RIPTIDE_1;
                    break;
                case 2:
                    soundEvent = &SoundEvents::ITEM_TRIDENT_RIPTIDE_2;
                    break;
                default:
                    soundEvent = &SoundEvents::ITEM_TRIDENT_RIPTIDE_3;
                    break;
            }
            player->playSound(*soundEvent, 1.0f, 1.0f);
        }

        // 激流模式下不投掷三叉戟
        return;
    }

    // 正常投掷模式
    // 创建三叉戟实体
    auto tridentEntity = std::make_unique<entity::TridentEntity>(LegacyEntityType::Trident, EntityId(0));
    tridentEntity->setWorld(&world);
    tridentEntity->setPosition(player->x(), player->y() + player->eyeHeight() - 0.1f, player->z());
    tridentEntity->setShooter(player);

    // 设置发射参数
    f32 velocity = THROW_VELOCITY + static_cast<f32>(riptideLevel) * 0.5f;
    tridentEntity->shootFrom(*player, player->pitch(), player->yaw(), 0.0f, velocity, 1.0f);

    // 设置三叉戟物品
    tridentEntity->setItemStack(stack);

    // 设置忠诚附魔等级
    i32 loyaltyLevel = enchant::EnchantmentHelper::getEnchantmentLevel(
        stack, &enchant::AllEnchantments::LOYALTY);
    tridentEntity->setLoyaltyLevel(static_cast<u8>(loyaltyLevel));

    // 创造模式下设置拾取状态
    if (player->isCreative()) {
        tridentEntity->setPickupStatus(entity::PickupStatus::CreativeOnly);
    }

    // 生成实体
    world.spawnEntity(std::move(tridentEntity));

    // 播放投掷音效
    player->playSound(SoundEvents::ITEM_TRIDENT_THROW, 1.0f, 1.0f);

    // 非创造模式从背包移除三叉戟
    if (!player->isCreative()) {
        stack.shrink(1);
    }
}

bool TridentItem::hitEntity(ItemStack& stack, LivingEntity& target, LivingEntity& attacker) {
    (void)target;

    // 消耗耐久度
    stack.attemptDamageItem(1);
    return true;
}

bool TridentItem::onBlockDestroyed(
    ItemStack& stack,
    IWorld& world,
    const BlockState& state,
    const BlockPos& pos,
    LivingEntity& breaker)
{
    (void)world;
    (void)pos;
    (void)breaker;

    // 如果方块硬度不为0，消耗耐久度
    // MC 1.16.5: 三叉戟作为工具破坏方块消耗2点耐久
    if (state.hardness() > 0.0f) {
        stack.attemptDamageItem(2);
    }
    return true;
}

// ========== 私有方法 ==========

bool TridentItem::isWet(const Player& player) {
    // MC 1.16.5: isWet() = isInWater() || isInRain()
    // Entity 基类已实现 isWet() 方法
    return player.isWet();
}

i32 TridentItem::getRiptideLevel(const ItemStack& stack) {
    return enchant::EnchantmentHelper::getEnchantmentLevel(
        stack, &enchant::AllEnchantments::RIPTIDE);
}

} // namespace item
} // namespace mc
