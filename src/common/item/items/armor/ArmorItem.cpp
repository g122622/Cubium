#include "ArmorItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/World.hpp"
#include "../../../world/block/BlockState.hpp"

namespace mc {
namespace item::items {

ArmorItem::ArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot,
                     ItemProperties properties)
    : Item(std::move(properties))
    , m_material(material)
    , m_slot(slot) {
}

f32 ArmorItem::getDestroySpeed(const ItemStack& /*stack*/, const BlockState& /*state*/) const {
    // 盔甲不是工具，返回默认速度
    return 1.0f;
}

ItemActionResult ArmorItem::onItemRightClick(World& world, Player& player, Hand hand) {
    // 获取对应装备槽的当前物品
    i32 equipmentSlot = armor::ArmorMaterial::toEquipmentSlotIndex(m_slot);
    ItemStack currentArmor = player.getArmorStack(equipmentSlot);

    // 如果槽位为空，则装备此盔甲
    if (currentArmor.isEmpty()) {
        // 从手中移除物品
        ItemStack heldItem = player.getHeldItem(hand);
        heldItem.shrink(1);

        // 装备到对应槽位
        player.setArmorStack(equipmentSlot, heldItem.copy());

        // TODO: 播放装备音效
        // world.playSound(player, player.getPos(), m_material.getEquipSound(), ...)

        return ItemActionResult::consume(heldItem);
    }

    // 槽位已有盔甲，交换物品
    return ItemActionResult::pass(player.getHeldItem(hand));
}

i32 ArmorItem::getTotalArmorValue(const LivingEntity& entity) {
    i32 total = 0;

    // 遍历所有盔甲槽位
    for (i32 i = 0; i < 4; ++i) {
        ItemStack stack = entity.getArmorStack(i);
        if (!stack.isEmpty()) {
            const Item* item = stack.getItem();
            if (item != nullptr) {
                // 尝试转换为ArmorItem
                const ArmorItem* armor = dynamic_cast<const ArmorItem*>(item);
                if (armor != nullptr) {
                    total += armor->getDefense();
                }
            }
        }
    }

    return total;
}

f32 ArmorItem::getTotalToughness(const LivingEntity& entity) {
    f32 total = 0.0f;

    for (i32 i = 0; i < 4; ++i) {
        ItemStack stack = entity.getArmorStack(i);
        if (!stack.isEmpty()) {
            const Item* item = stack.getItem();
            if (item != nullptr) {
                const ArmorItem* armor = dynamic_cast<const ArmorItem*>(item);
                if (armor != nullptr) {
                    total += armor->getToughness();
                }
            }
        }
    }

    return total;
}

f32 ArmorItem::getTotalKnockbackResistance(const LivingEntity& entity) {
    f32 total = 0.0f;

    for (i32 i = 0; i < 4; ++i) {
        ItemStack stack = entity.getArmorStack(i);
        if (!stack.isEmpty()) {
            const Item* item = stack.getItem();
            if (item != nullptr) {
                const ArmorItem* armor = dynamic_cast<const ArmorItem*>(item);
                if (armor != nullptr) {
                    total += armor->getKnockbackResistance();
                }
            }
        }
    }

    return std::min(total, 1.0f);  // 上限为1.0
}

} // namespace item::items
} // namespace mc
