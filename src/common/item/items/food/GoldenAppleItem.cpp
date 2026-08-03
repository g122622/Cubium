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

#include "GoldenAppleItem.hpp"
#include "../../../entity/core/Entity.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/effect/EffectInstance.hpp"
#include "../../../entity/effect/EffectType.hpp"
#include "../../../entity/entities/monster/undead/ZombieVillagerEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/UseAction.hpp"
#include "common/item/food/Food.hpp"
#include <chrono>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc {
namespace item::items {

GoldenAppleItem::GoldenAppleItem(const food::Food* food, ItemProperties properties)
    : Item(std::move(properties))
    , m_food(food)
{}

i32 GoldenAppleItem::getUseDuration(const ItemStack& /*stack*/) const
{
    // 金苹果食用时间为 32 ticks
    return 32;
}

UseAction GoldenAppleItem::getUseAction(const ItemStack& /*stack*/) const
{
    return UseAction::Eat;
}

ItemActionResult GoldenAppleItem::onItemRightClick(IWorld& /*world*/, Player& player, Hand hand)
{
    // 金苹果可以在任何时候食用
    ItemStack stack = player.getHeldItem(hand);
    if (stack.isEmpty()) {
        return ItemActionResult::pass(stack);
    }

    // 金苹果总是可以吃（不要求饥饿）
    if (canEat(stack, player)) {
        return ItemActionResult::consume(stack);
    }

    return ItemActionResult::pass(stack);
}

ItemStack GoldenAppleItem::onItemUseFinish(ItemStack& stack, IWorld& /*world*/, Entity& entity)
{
    // 金苹果食用完成：恢复饥饿值和饱和度，应用药水效果

    // 尝试转换为玩家
    Player* player = dynamic_cast<Player*>(&entity);
    bool isCreativePlayer = (player != nullptr && player->abilities().creativeMode);

    if (m_food != nullptr && player != nullptr) {
        // 恢复饥饿值和饱和度
        player->foodStats().addStats(m_food->getHunger(), m_food->getSaturationModifier());
        player->foodStats().setFoodTimer(0);

        // 应用效果
        if (m_food->hasEffects()) {
            // 使用实体ID和时间生成随机数
            math::Random rng(static_cast<u64>(entity.id()) ^
                static_cast<u64>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

            for (const auto& effect : m_food->getEffects()) {
                if (rng.nextFloat() < effect.probability) {
                    entity::effect::EffectInstance instance(effect.type,
                        effect.duration,
                        effect.amplifier,
                        false, // 非环境效果
                        true,  // 显示粒子
                        true   // 显示图标
                    );
                    player->addEffect(std::move(instance));
                }
            }
        }
    }

    // 减少物品数量（非创造模式）
    if (!isCreativePlayer) {
        stack.shrink(1);
    }

    return stack;
}

bool GoldenAppleItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand /*hand*/)
{
    // 金苹果对僵尸村民使用

    // 检查目标是否为僵尸村民
    auto* zombieVillager = dynamic_cast<ZombieVillagerEntity*>(&target);
    if (zombieVillager == nullptr) {
        return false;
    }

    // 确保实体存活
    if (!zombieVillager->isAlive()) {
        return false;
    }

    // 检查僵尸村民是否有虚弱效果
    if (!zombieVillager->hasEffect(entity::effect::EffectType::Weakness)) {
        // 没有虚弱效果，返回 false 让金苹果被正常食用
        return false;
    }

    // 开始治愈过程，治愈时间为 3600-6000 ticks (3-5 分钟)
    math::Random rng(static_cast<u64>(player.ticksExisted()));
    i32 conversionTime = 3600 + rng.nextInt(2401);
    zombieVillager->startConverting(player.uuid(), conversionTime);

    // 消耗一个金苹果（非创造模式）
    if (!player.isCreative()) {
        stack.shrink(1);
    }

    spdlog::info("GoldenAppleItem: Started curing zombie villager at ({}, {}, {}), "
                 "conversion time: {} ticks, starter: {}",
        zombieVillager->x(),
        zombieVillager->y(),
        zombieVillager->z(),
        conversionTime,
        player.uuid());

    return true;
}

bool GoldenAppleItem::canEat(const ItemStack& /*stack*/, const Player& player) const
{
    // 金苹果可以在任何时候食用（不要求饥饿），但旁观模式除外
    if (player.isSpectator()) {
        return false;
    }
    return true;
}

} // namespace item::items
} // namespace mc
