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

#include "common/entity/serialization/components/LivingEntityComponentSerialization.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EquipmentSlot.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/ecs/components/HealthComponent.hpp"
#include "common/entity/ecs/components/HurtStateComponent.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/EquipmentSlotNames.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"

#include <array>

namespace mc::entity::serialization::components {

// ============================================================================
// HealthComponent — Health（生命值，f32）
// ============================================================================

static void saveHealth(const Entity& entity, nbt::tags::compound_tag& tag)
{
    auto* living = dynamic_cast<const LivingEntity*>(&entity);
    if (living == nullptr) {
        return;
    }
    tag.put(nbt_keys::HEALTH, living->health());
}

static Result<void> loadHealth(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living == nullptr) {
        return {};
    }
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::HEALTH)) {
        // NBT 加载的 health 是权威值，经 setHealth 完成组件真相源 + DataParameter 镜像双写。
        // 置 m_healthSynced 避免 tick 首帧 setHealth(maxHealth) 覆盖权威值。
        living->setHealth(*val);
        if (auto* c = entity.tryGetComponent<ecs::HealthComponent>()) {
            c->m_healthSynced = true;
        }
    }
    return {};
}

// ============================================================================
// HurtStateComponent — Absorption + HurtTime + DeathTime
// ============================================================================

static void saveHurtState(const Entity& entity, nbt::tags::compound_tag& tag)
{
    auto* living = dynamic_cast<const LivingEntity*>(&entity);
    if (living == nullptr) {
        return;
    }
    tag.put(nbt_keys::ABSORPTION_AMOUNT, living->absorptionAmount());
    tag.put(nbt_keys::HURT_TIME, static_cast<i16>(living->hurtTime()));
    tag.put(nbt_keys::DEATH_TIME, static_cast<i16>(living->deathTime()));
}

static Result<void> loadHurtState(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living == nullptr) {
        return {};
    }
    // AbsorptionAmount：setAbsorptionAmount 是 virtual，Player override 额外下发
    // DATA_PLAYER_ABSORPTION_PARAM 镜像，自动正确派发。
    if (auto val = nbt_helper::tryGetFloat(tag, nbt_keys::ABSORPTION_AMOUNT)) {
        living->setAbsorptionAmount(*val);
    }
    // HurtTime / DeathTime：无 setter，直写组件（与原 readAdditionalSaveData 一致）
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::HURT_TIME)) {
        if (auto* c = entity.tryGetComponent<ecs::HurtStateComponent>()) {
            c->m_hurtTime = *val;
        }
    }
    if (auto val = nbt_helper::tryGetShort(tag, nbt_keys::DEATH_TIME)) {
        if (auto* c = entity.tryGetComponent<ecs::HurtStateComponent>()) {
            c->m_deathTime = *val;
        }
    }
    return {};
}

// ============================================================================
// EquipmentComponent — Equipment（装备，MC 1.21.11 equipment 复合标签）
// ============================================================================

static void saveEquipment(const Entity& entity, nbt::tags::compound_tag& tag)
{
    auto* living = dynamic_cast<const LivingEntity*>(&entity);
    if (living == nullptr) {
        return;
    }
    // MC 1.21.11 新格式：使用 "equipment" 复合标签，键名为 EquipmentSlot 序列化名，
    // 空槽位不写入（与 MC 原版 EntityEquipment.CODEC 一致）。
    nbt::tags::compound_tag equipmentTag;
    for (u8 i = 0; i < static_cast<u8>(EquipmentSlot::Count); ++i) {
        auto slot = static_cast<EquipmentSlot>(i);
        const ItemStack& stack = living->getEquipment(slot);
        if (!stack.isEmpty()) {
            nbt::tags::compound_tag itemTag;
            stack.toNbt(itemTag);
            equipmentTag.value.emplace(
                EquipmentSlotNames::toName(slot), std::make_unique<nbt::tags::compound_tag>(std::move(itemTag)));
        }
    }
    if (!equipmentTag.value.empty()) {
        tag.value.emplace(nbt_keys::EQUIPMENT, std::make_unique<nbt::tags::compound_tag>(std::move(equipmentTag)));
    }
}

static Result<void> loadEquipment(Entity& entity, const nbt::tags::compound_tag& tag)
{
    auto* living = dynamic_cast<LivingEntity*>(&entity);
    if (living == nullptr) {
        return {};
    }
    // 新格式：从 equipment 复合标签读取
    if (auto* equipmentTag = nbt_helper::tryGetCompound(tag, nbt_keys::EQUIPMENT)) {
        for (u8 i = 0; i < static_cast<u8>(EquipmentSlot::Count); ++i) {
            auto slot = static_cast<EquipmentSlot>(i);
            const char* name = EquipmentSlotNames::toName(slot);
            if (auto* itemCompound = nbt_helper::tryGetCompound(*equipmentTag, name)) {
                auto stackResult = ItemStack::fromNbt(*itemCompound);
                if (stackResult.success()) {
                    living->setEquipment(slot, stackResult.value());
                }
            }
        }
        return {};
    }
    // 旧格式回退：从 HandItems/ArmorItems 列表读取（MC 1.21.11 之前的存档）
    if (auto* handItems = nbt_helper::tryGetList(tag, nbt_keys::HAND_ITEMS)) {
        if (handItems->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*handItems);
            if (!compoundList.value.empty()) {
                auto mainHandResult = ItemStack::fromNbt(compoundList.value[0]);
                if (mainHandResult.success()) {
                    living->setMainHandItem(mainHandResult.value());
                }
            }
            if (compoundList.value.size() > 1) {
                auto offHandResult = ItemStack::fromNbt(compoundList.value[1]);
                if (offHandResult.success()) {
                    living->setOffHandItem(offHandResult.value());
                }
            }
        }
    }
    if (auto* armorItems = nbt_helper::tryGetList(tag, nbt_keys::ARMOR_ITEMS)) {
        if (armorItems->element_id() == nbt::TagId::Compound) {
            auto& compoundList = dynamic_cast<const nbt::tags::compound_list_tag&>(*armorItems);
            constexpr std::array<EquipmentSlot, 4> armorOrder = {
                EquipmentSlot::Feet, EquipmentSlot::Legs, EquipmentSlot::Chest, EquipmentSlot::Head};
            for (size_t i = 0; i < armorOrder.size() && i < compoundList.value.size(); ++i) {
                auto armorResult = ItemStack::fromNbt(compoundList.value[i]);
                if (armorResult.success()) {
                    living->setEquipment(armorOrder[i], armorResult.value());
                }
            }
        }
    }
    return {};
}

// ============================================================================
// 注册
// ============================================================================

void registerLivingEntityComponentSerializers(ComponentSerializerRegistry& registry)
{
    registry.registerSerializer<ecs::HealthComponent>(saveHealth, loadHealth);
    registry.registerSerializer<ecs::HurtStateComponent>(saveHurtState, loadHurtState);
    registry.registerSerializer<ecs::EquipmentComponent>(saveEquipment, loadEquipment);
}

} // namespace mc::entity::serialization::components
