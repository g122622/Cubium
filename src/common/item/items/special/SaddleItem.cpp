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

#include "SaddleItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/special/StriderEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/interfaces/IEquipable.hpp"
#include "common/entity/interfaces/IRideable.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace {

/**
 * @brief 根据实体类型选择鞍音效
 *
 * 不同实体装备鞍时播放不同的音效：
 * - 猪: ENTITY_PIG_SADDLE
 * - 炽足兽: ENTITY_STRIDER_SADDLE
 * - 马类及其他: ENTITY_HORSE_SADDLE
 */
const ResourceLocation& getSaddleSound(const LivingEntity& target)
{
    // 通过 dynamic_cast 判断实体类型，选择对应的音效
    // 猪使用猪专属音效
    if (dynamic_cast<const PigEntity*>(&target) != nullptr) {
        return SoundEvents::ENTITY_PIG_SADDLE;
    }
    // 炽足兽使用炽足兽专属音效
    if (dynamic_cast<const StriderEntity*>(&target) != nullptr) {
        return SoundEvents::ENTITY_STRIDER_SADDLE;
    }
    // 马类及其他实体使用马鞍音效
    return SoundEvents::ENTITY_HORSE_SADDLE;
}

} // namespace

namespace item::items {

SaddleItem::SaddleItem(const ItemProperties& properties)
    : Item(properties)
{}

bool SaddleItem::itemInteractionForEntity(ItemStack& stack, Player& player, LivingEntity& target, Hand hand)
{
    // stack 参数由调用方传入（Player::interactOn 传 getHeldItem 值拷贝 Player.cpp:2858，
    // MobEntity 传权威手持引用），为统一两路径行为，本方法内部直接以 player.getHeldItem(hand)
    // 为权威手持源操作（同 GoldenAppleItem/BucketItem/ShearsItem 修复范式）。
    (void)stack;

    // 检查目标实体是否实现了 IEquipable 接口（猪/炽足兽/马类均实现，鞍装备到装备槽 0）。
    auto* equipable = dynamic_cast<entity::IEquipable*>(&target);
    if (equipable == nullptr) {
        return false;
    }

    // 确保实体存活
    if (!target.isAlive()) {
        return false;
    }

    // 幼年实体不能装备鞍
    if (target.isChild()) {
        return false;
    }

    // 鞍状态接口分两类（对齐 vanilla Saddleable 接口，hasSaddle 不统一）：
    //   1) 实现 mc::entity::IRideable 的可骑乘实体：PigEntity/StriderEntity（IRideable::hasSaddle/setSaddle
    //      纯虚，dynamic_cast<IRideable*> 命中）。猪/炽足兽 isSaddleable 恒 true（vanilla Saddleable.isSaddleable
    //      对猪/炽足兽返 true），可直接装鞍。
    //   2) 马类 AbstractHorseEntity：不实现 IRideable（控制逻辑经乘客系统非 ride()），但有独立
    //      hasSaddle()/setSaddle()（HorseStatusComponent.m_saddled）。马类 isSaddleable=isTame()（vanilla
    //      AbstractHorse.isSaddleable 返 isTame()，未驯服马不可装鞍）。
    // 两路任一命中取 hasSaddle()/setSaddle()；isSaddleable 守卫：IRideable 恒 true，马类需 isTame()。
    // 修复前本方法单路 dynamic_cast<IRideable*>，把马类（不实现 IRideable）排除，致马永远装不上鞍
    // （对齐缺陷：vanilla Saddleable 接口不强制 IRideable，马类实现 Saddleable 但不实现 IRideable）。
    entity::IRideable* rideable = dynamic_cast<entity::IRideable*>(&target);
    AbstractHorseEntity* horse = dynamic_cast<AbstractHorseEntity*>(&target);

    // 非鞍实体（既非 IRideable 也非马类）直接返回
    if (rideable == nullptr && horse == nullptr) {
        return false;
    }

    // isSaddleable 守卫：马类需先驯服（vanilla AbstractHorse.isSaddleable=isTame()）；
    // IRideable（猪/炽足兽）恒可装鞍。已装鞍则不重复装（vanilla Saddleable.isSaddled() 守卫）。
    if (horse != nullptr) {
        if (!horse->isTame()) {
            return false;
        }
        if (horse->hasSaddle()) {
            return false;
        }
    } else {
        // rideable != nullptr（horse==nullptr 分支）
        if (rideable->hasSaddle()) {
            return false;
        }
    }

    // 检查装备槽是否可用
    if (equipable->getEquipmentSlotCount() <= 0) {
        return false;
    }

    // 检查鞍槽是否为空
    const ItemStack saddleSlot = equipable->getEquipment(0);
    if (!saddleSlot.isEmpty()) {
        return false;
    }

    // 设置鞍状态（双路：IRideable::setSaddle 或 AbstractHorseEntity::setSaddle）
    if (horse != nullptr) {
        horse->setSaddle(true);
    } else {
        rideable->setSaddle(true);
    }

    // 装备鞍到实体（装备槽 0）
    ItemStack saddleStack(stack.getItem(), 1);
    equipable->setEquipment(0, saddleStack);

    // 根据实体类型播放不同的鞍音效
    IWorld* world = target.world();
    if (world != nullptr) {
        const ResourceLocation& soundEvent = getSaddleSound(target);
        world->playSound(soundEvent, sound::SoundCategory::Neutral, target.position(), 0.5f, 1.0f);
    }

    // 消耗一个物品（非创造模式）：直接操作玩家权威手持物（player.getHeldItem(hand) 返回引用），
    // 而非 stack 参数（Player 路径下是值拷贝，shrink 不回写权威物品栏——持多个鞍装备时不消耗
    // 的对齐缺陷）。MobEntity 路径下 stack 已是权威引用，与 getHeldItem 等价，不影响。
    if (!player.isCreative()) {
        ItemStack& heldItem = player.getHeldItem(hand);
        heldItem.shrink(1);
    }

    return true;
}

} // namespace item::items
} // namespace mc
