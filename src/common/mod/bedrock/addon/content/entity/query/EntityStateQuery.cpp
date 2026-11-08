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

#include "common/mod/bedrock/addon/content/entity/query/EntityStateQuery.hpp"

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/basic/CreeperEntity.hpp"
#include "common/entity/entities/passive/basic/MooshroomEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/tamable/TameableEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/vehicle/ChestBoatEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/mod/bedrock/addon/content/entity/state/AddonEntityComponents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <array>
#include <string_view>

namespace mc::mod::bedrock::addon {
namespace {

bool _matches(const mc::ItemStack& stack, const mc::ResourceLocation& item)
{
    return !stack.isEmpty() && stack.getItem()->itemLocation() == item;
}

bool _inventoryMatches(const mc::IInventory& inventory, i32 begin, i32 end, const mc::ResourceLocation& item)
{
    MC_ASSERT_RELEASE(begin >= 0 && begin <= end && end <= inventory.getContainerSize());
    for (i32 slot = begin; slot < end; ++slot) {
        if (_matches(inventory.getItem(slot), item)) {
            return true;
        }
    }
    return false;
}

Result<bool> _inventory(const mc::Entity& entity, const mc::ResourceLocation& item)
{
    if (const auto* player = dynamic_cast<const mc::Player*>(&entity)) {
        // 基岩 inventory 域只查询快捷栏和主背包，不包含护甲、副手及光标物品。
        return _inventoryMatches(
            player->inventory(), 0, mc::PlayerInventory::HOTBAR_SIZE + mc::PlayerInventory::MAIN_SIZE, item);
    }
    if (const auto* horse = dynamic_cast<const mc::AbstractHorseEntity*>(&entity)) {
        // 马类前两个槽位承载装备，额外槽位才是装箱后的储物空间。
        for (i32 slot = 2; slot < horse->getInventorySize(); ++slot) {
            if (_matches(horse->getEquipment(slot), item)) {
                return true;
            }
        }
        return false;
    }
    if (const auto* villager = dynamic_cast<const mc::entity::AbstractVillagerEntity*>(&entity)) {
        const auto& inventory = villager->inventory();
        return _inventoryMatches(inventory, 0, inventory.getContainerSize(), item);
    }
    // 载具容器：矿车与箱船都提供了不修改库存的只读访问接口（getContainerSize +
    // getInventoryItem），逐个槽位比对即可，与商人/马类走同一套 _matches 判定。
    // 这里按实体类型判定而非 getTypeId() 字符串：矿车/船实体并不各自覆写 getTypeId，
    // 依赖字符串会漏判。
    if (const auto* chestMinecart = dynamic_cast<const entity::ChestMinecartEntity*>(&entity)) {
        const i32 size = chestMinecart->getContainerSize();
        for (i32 slot = 0; slot < size; ++slot) {
            if (_matches(chestMinecart->getInventoryItem(slot), item)) {
                return true;
            }
        }
        return false;
    }
    if (const auto* hopperMinecart = dynamic_cast<const entity::HopperMinecartEntity*>(&entity)) {
        const i32 size = hopperMinecart->getContainerSize();
        for (i32 slot = 0; slot < size; ++slot) {
            if (_matches(hopperMinecart->getInventoryItem(slot), item)) {
                return true;
            }
        }
        return false;
    }
    if (const auto* chestBoat = dynamic_cast<const entity::ChestBoatEntity*>(&entity)) {
        // ChestBoatEntity::getInventoryItem 会先触发战利品表懒解包（首次访问时填充库存），
        // 因此不是 const 成员；这里只是读取，用 const_cast 是安全的。
        auto& mutableChestBoat = const_cast<entity::ChestBoatEntity&>(*chestBoat);
        const i32 size = mutableChestBoat.getContainerSize();
        for (i32 slot = 0; slot < size; ++slot) {
            if (_matches(mutableChestBoat.getInventoryItem(slot), item)) {
                return true;
            }
        }
        return false;
    }
    // TODO: 随其他原生携物生物和 addon inventory 组件接入统一背包查询。
    return false;
}

} // namespace

Result<bool> EntityStateQuery::hasComponent(const mc::Entity& entity, const std::string& name)
{
    if (const auto* components = entity.tryGetComponent<AddonEntityComponents>()) {
        return components->has(name);
    }
    if (name == "minecraft:health" || name == "minecraft:movement" || name == "minecraft:equippable") {
        return dynamic_cast<const mc::LivingEntity*>(&entity) != nullptr;
    }
    if (name == "minecraft:is_tamed") {
        if (const auto* tameable = dynamic_cast<const mc::TameableEntity*>(&entity)) {
            return tameable->isTamed();
        }
        const auto* horse = dynamic_cast<const mc::AbstractHorseEntity*>(&entity);
        return horse != nullptr && horse->isTame();
    }
    if (name == "minecraft:is_charged") {
        const auto* creeper = dynamic_cast<const mc::CreeperEntity*>(&entity);
        return creeper != nullptr && creeper->isPowered();
    }
    if (name == "minecraft:onfire") {
        return entity.isOnFire();
    }
    if (name == "minecraft:inventory") {
        // 具备"可查询库存"这一组件的实体：玩家、马类、商人，以及承载物品的载具
        // （箱子矿车/漏斗矿车/箱子船）。判定口径与 _inventory 的查询分支保持一致，
        // 否则会出现"组件报告存在、却查不到库存"或反之的不一致。
        if (dynamic_cast<const mc::Player*>(&entity) != nullptr ||
            dynamic_cast<const mc::AbstractHorseEntity*>(&entity) != nullptr ||
            dynamic_cast<const mc::entity::AbstractVillagerEntity*>(&entity) != nullptr ||
            dynamic_cast<const entity::ChestMinecartEntity*>(&entity) != nullptr ||
            dynamic_cast<const entity::HopperMinecartEntity*>(&entity) != nullptr ||
            dynamic_cast<const entity::ChestBoatEntity*>(&entity) != nullptr) {
            return true;
        }
        return Error(
            ErrorCode::Unsupported, "Native inventory component mapping is not implemented: " + entity.getTypeId());
    }
    if (name == "minecraft:variant" && dynamic_cast<const mc::MooshroomEntity*>(&entity) != nullptr) {
        // 哞菇的红/棕是**主变种**（基岩 `minecraft:variant`，0=红、1=棕），
        // 不是 `minecraft:mark_variant`（那是"附加变种"，用于进一步区分同一主变种，
        // 如马的花纹）。此前占用 mark_variant 承载它，会让脚本按官方组件名读到 undefined。
        return true;
    }
    // TODO: 将其余原版实体组件的存在条件接入统一描述表。
    return Error(ErrorCode::Unsupported, "Native Bedrock component mapping is not implemented: " + name);
}

Result<i32> EntityStateQuery::variant(const mc::Entity& entity, const std::string& name)
{
    MC_ASSERT_RELEASE(name == "minecraft:variant" || name == "minecraft:mark_variant");
    if (const auto* components = entity.tryGetComponent<AddonEntityComponents>()) {
        const auto* value = components->component(name);
        return value != nullptr ? value->value("value", i32{0}) : i32{0};
    }
    if (name == "minecraft:variant") {
        if (const auto* mooshroom = dynamic_cast<const mc::MooshroomEntity*>(&entity)) {
            return static_cast<i32>(mooshroom->getMooshroomType());
        }
    }
    // TODO: 用基岩各类型的变种编号映射原生实体，不能直接套用 Java 枚举编号。
    return Error(ErrorCode::Unsupported, "Native Bedrock variant mapping is not implemented: " + entity.getTypeId());
}

Result<bool> EntityStateQuery::hasEquipment(
    const mc::Entity& entity, const std::string& domain, const std::string& itemName)
{
    const auto item = mc::ResourceLocation::parse(itemName);
    MC_ASSERT_RELEASE(item.isValid());
    const auto* living = dynamic_cast<const mc::LivingEntity*>(&entity);
    if (living != nullptr) {
        const auto slotMatches = [&](mc::EquipmentSlot slot) {
            return _matches(living->getEquipment(slot), item);
        };
        if ((domain == "hand" || domain == "main_hand" || domain == "any") &&
            slotMatches(mc::EquipmentSlot::MainHand)) {
            return true;
        }
        if ((domain == "hand" || domain == "any") && slotMatches(mc::EquipmentSlot::OffHand)) {
            return true;
        }
        const std::array armor = {std::pair{"head", mc::EquipmentSlot::Head},
            std::pair{"torso", mc::EquipmentSlot::Chest},
            std::pair{"leg", mc::EquipmentSlot::Legs},
            std::pair{"feet", mc::EquipmentSlot::Feet}};
        for (const auto& [name, slot] : armor) {
            if ((domain == name || domain == "armor" || domain == "any") && slotMatches(slot)) {
                return true;
            }
        }
        if (domain == "body" || domain == "any") {
            if (const auto* horse = dynamic_cast<const mc::AbstractHorseEntity*>(&entity)) {
                if (_matches(horse->getEquipment(1), item)) {
                    return true;
                }
            } else if (slotMatches(mc::EquipmentSlot::Body)) {
                return true;
            }
        }
    }
    if (domain == "inventory" || domain == "any") {
        return _inventory(entity, item);
    }
    return false;
}

bool EntityStateQuery::isOwner(const mc::Entity& caller, const mc::Entity& subject)
{
    const auto id = mc::util::uuidFromString(subject.uuid());
    if (const auto* tameable = dynamic_cast<const mc::TameableEntity*>(&caller)) {
        return tameable->isOwner(id);
    }
    if (const auto* horse = dynamic_cast<const mc::AbstractHorseEntity*>(&caller)) {
        return horse->hasOwner() && mc::util::uuidFromString(horse->getOwnerUuid()) == id;
    }
    // TODO: addon tameable 组件接入后从实例的真实归属状态读取主人。
    return false;
}

} // namespace mc::mod::bedrock::addon
