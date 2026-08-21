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

#include "FishingRodItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/OtherProjectiles.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/enchantment/enchantments/AllEnchantments.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace item {

// 钓鱼竿常量
namespace {
constexpr f32 BOBBER_VELOCITY = 1.5f;   // 浮标初速度
constexpr f32 BOBBER_INACCURACY = 0.0f; // 浮标准确度（0 = 完美准确）
} // namespace

// ========== 构造函数 ==========

FishingRodItem::FishingRodItem(const ItemProperties& properties)
    : Item(properties)
{}

// ========== Item 接口重写 ==========

// 钓鱼竿没有重写 getUseDuration() 和 getUseAction()
// 使用默认值：getUseDuration() 返回 0，getUseAction() 返回 NONE
// 这意味着钓鱼竿是即时使用物品，没有使用动画

i32 FishingRodItem::getItemEnchantability() const
{
    // 钓鱼竿附魔能力为 1
    return 1;
}

ItemActionResult FishingRodItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
    ItemStack& rodStack = player.getHeldItem(hand);

    // 检查玩家是否已经有浮标
    if (hasBobber(player)) {
        // 收杆
        entity::FishingBobberEntity* bobber = getBobber(player);
        if (bobber != nullptr) {
            i32 damage = bobber->reelIn();
            // 消耗耐久度，若物品损坏则触发 onEquippedItemBroken 回调
            LivingEntity::hurtAndBreak(rodStack, damage, &player, LivingEntity::handToEquipmentSlot(hand));
            // 播放收杆音效
            player.playSound(
                SoundEvents::ENTITY_FISHING_BOBBER_RETRIEVE, 0.5f, 0.4f / (math::Random().nextFloat() * 0.4f + 0.8f));
        }
        // 清除玩家的浮标引用
        player.setFishingBobber(0);
    } else {
        // 抛杆
        // 获取钓鱼附魔
        i32 luckBonus =
            enchant::EnchantmentHelper::getEnchantmentLevel(rodStack, &enchant::AllEnchantments::LUCK_OF_THE_SEA);
        i32 speedBonus = enchant::EnchantmentHelper::getEnchantmentLevel(rodStack, &enchant::AllEnchantments::LURE);

        // 创建浮标实体
        // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
        auto* registry = world.entityRegistry();
        if (registry == nullptr) {
            return ItemActionResult::fail(rodStack);
        }
        auto bobber = std::make_unique<entity::FishingBobberEntity>(EntityInstanceId(0), *registry);
        bobber->setTypeId(entity::EntityTypeKeys::FISHING_BOBBER);
        bobber->setWorld(&world);
        bobber->setPosition(player.x(), player.y() + player.eyeHeight() - 0.1f, player.z());
        bobber->setShooter(&player);

        // 设置钓鱼参数（通过NBT或实体方法）
        bobber->setFishingBonus(luckBonus, speedBonus);

        // 发射浮标
        bobber->shootFrom(player, player.pitch(), player.yaw(), 0.0f, BOBBER_VELOCITY, BOBBER_INACCURACY);

        // 生成实体并记录 ID。必须用 spawnEntity 的返回值（addEntity 分配的真实 id），
        // 不能用 bobber->id()——make_unique 构造时传入 EntityInstanceId(0)，spawnEntity 前实体尚未
        // 经 EntityManager::addEntity 分配 id，bobber->id() 返回 0。此前用 bobber->id() 致
        // setFishingBobber(0)，而 isFishing() = (m_fishingBobber != 0) 恒为 false，FishingBobberEntity
        // ::tick 检测 angler->fishingBobber() != id()（0 != 真实id）立即 remove 浮标——抛杆生成的
        // 浮标首 tick 即被移除，钓鱼链路完全断裂。对齐 VillagerBreedGoal/HangingEntity/ZombieEntity
        // 等用 spawnEntity 返回值取 id 的范式。
        EntityInstanceId bobberId = world.spawnEntity(std::move(bobber));
        player.setFishingBobber(bobberId);

        // 播放抛杆音效
        player.playSound(
            SoundEvents::ENTITY_FISHING_BOBBER_THROW, 0.5f, 0.4f / (math::Random().nextFloat() * 0.4f + 0.8f));
    }

    return ItemActionResult::success(rodStack);
}

// ========== 钓鱼竿特有方法 ==========

bool FishingRodItem::hasBobber(Player& player)
{
    return player.isFishing();
}

entity::FishingBobberEntity* FishingRodItem::getBobber(Player& player)
{
    EntityInstanceId bobberId = player.fishingBobber();
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
