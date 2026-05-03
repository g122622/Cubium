#include "FishingRodItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/entities/projectile/OtherProjectiles.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../enchantment/EnchantmentHelper.hpp"
#include "../../enchantment/enchantments/AllEnchantments.hpp"
#include "../../../sound/SoundEvents.hpp"
#include <cmath>

namespace mc {
namespace item {

// MC 1.16.5 钓鱼竿常量
namespace {
    constexpr f32 BOBBER_VELOCITY = 1.5f;      // 浮标初速度
    constexpr f32 BOBBER_INACCURACY = 0.0f;    // 浮标准确度（0 = 完美准确）
}

// ========== 构造函数 ==========

FishingRodItem::FishingRodItem(const ItemProperties& properties)
    : Item(properties)
{
}

// ========== Item 接口重写 ==========

// MC 1.16.5: 钓鱼竿没有重写 getUseDuration() 和 getUseAction()
// 使用默认值：getUseDuration() 返回 0，getUseAction() 返回 NONE
// 这意味着钓鱼竿是即时使用物品，没有使用动画

i32 FishingRodItem::getItemEnchantability() const {
    // MC 1.16.5: 钓鱼竿附魔能力为 1
    return 1;
}

ItemActionResult FishingRodItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    ItemStack rodStack = player.getHeldItem(hand);

    // 检查玩家是否已经有浮标
    if (hasBobber(player)) {
        // 收杆
        entity::FishingBobberEntity* bobber = getBobber(player);
        if (bobber != nullptr) {
            i32 damage = bobber->reelIn();
            rodStack.attemptDamageItem(damage);
            // 播放收杆音效
            player.playSound(SoundEvents::ENTITY_FISHING_BOBBER_RETRIEVE, 0.5f, 0.4f / (math::Random().nextFloat() * 0.4f + 0.8f));
        }
        // 清除玩家的浮标引用
        player.setFishingBobber(0);
    } else {
        // 抛杆
        // 获取钓鱼附魔
        i32 luckBonus = enchant::EnchantmentHelper::getEnchantmentLevel(
            rodStack, &enchant::AllEnchantments::LUCK_OF_THE_SEA);
        i32 speedBonus = enchant::EnchantmentHelper::getEnchantmentLevel(
            rodStack, &enchant::AllEnchantments::LURE);

        // 创建浮标实体
        auto bobber = std::make_unique<entity::FishingBobberEntity>(
            LegacyEntityType::FishingBobber, EntityId(0));
        bobber->setWorld(&world);
        bobber->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
        bobber->setShooter(&player);

        // 设置钓鱼参数（通过NBT或实体方法）
        bobber->setFishingBonus(luckBonus, speedBonus);

        // 发射浮标
        // MC 1.16.5: 浮标速度约为 1.5，不准确度 0
        bobber->shootFrom(player, player.pitch(), player.yaw(), 0.0f, BOBBER_VELOCITY, BOBBER_INACCURACY);

        // 生成实体并记录ID
        EntityId bobberId = bobber->id();
        world.spawnEntity(std::move(bobber));
        player.setFishingBobber(bobberId);

        // 播放抛杆音效
        player.playSound(SoundEvents::ENTITY_FISHING_BOBBER_THROW, 0.5f, 0.4f / (math::Random().nextFloat() * 0.4f + 0.8f));
    }

    return ItemActionResult::success(rodStack);
}

// ========== 钓鱼竿特有方法 ==========

bool FishingRodItem::hasBobber(Player& player) {
    return player.isFishing();
}

entity::FishingBobberEntity* FishingRodItem::getBobber(Player& player) {
    EntityId bobberId = player.fishingBobber();
    if (bobberId == 0) {
        return nullptr;
    }

    // 从世界获取浮标实体
    IWorld* world = player.world();
    if (world == nullptr) {
        return nullptr;
    }

    Entity* entity = world->getEntity(bobberId);
    if (entity == nullptr || !entity->isAlive()) {
        // 浮标不存在或已死亡，清除引用
        player.setFishingBobber(0);
        return nullptr;
    }

    // 类型转换
    return dynamic_cast<entity::FishingBobberEntity*>(entity);
}

} // namespace item
} // namespace mc
