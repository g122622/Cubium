#include "ArmorItem.hpp"
#include "../../core/ItemStack.hpp"
#include "../../core/ActionResult.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"

namespace mc {
namespace item::items {

namespace {

[[nodiscard]] const ItemStack& getArmorEquipment(const LivingEntity& entity, armor::ArmorSlot slot) {
    switch (slot) {
        case armor::ArmorSlot::Feet:
            return entity.getEquipment(EquipmentSlot::Feet);
        case armor::ArmorSlot::Legs:
            return entity.getEquipment(EquipmentSlot::Legs);
        case armor::ArmorSlot::Chest:
            return entity.getEquipment(EquipmentSlot::Chest);
        case armor::ArmorSlot::Head:
            return entity.getEquipment(EquipmentSlot::Head);
        default:
            return entity.getEquipment(EquipmentSlot::Head);
    }
}

} // namespace

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

ItemActionResult ArmorItem::onItemRightClick(IWorld& world, Player& player, Hand hand) {
    (void)world;

    ItemStack& heldStack = player.getHeldItem(hand);
    if (heldStack.isEmpty()) {
        return ItemActionResult::pass(heldStack);
    }

    PlayerInventory& inventory = player.inventory();
    switch (m_slot) {
        case armor::ArmorSlot::Head:
            if (!inventory.getHelmet().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setHelmet(heldStack);
            break;
        case armor::ArmorSlot::Chest:
            if (!inventory.getChestplate().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setChestplate(heldStack);
            break;
        case armor::ArmorSlot::Legs:
            if (!inventory.getLeggings().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setLeggings(heldStack);
            break;
        case armor::ArmorSlot::Feet:
            if (!inventory.getBoots().isEmpty()) {
                return ItemActionResult::pass(heldStack);
            }
            inventory.setBoots(heldStack);
            break;
    }

    heldStack = ItemStack();
    return ItemActionResult::consume(ItemStack());
}

i32 ArmorItem::getTotalArmorValue(const LivingEntity& entity) {
    i32 total = 0;

    for (armor::ArmorSlot slot : {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs,
                                  armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getDefense();
        }
    }

    return total;
}

f32 ArmorItem::getTotalToughness(const LivingEntity& entity) {
    f32 total = 0.0f;

    for (armor::ArmorSlot slot : {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs,
                                  armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getToughness();
        }
    }

    return total;
}

f32 ArmorItem::getTotalKnockbackResistance(const LivingEntity& entity) {
    f32 total = 0.0f;

    for (armor::ArmorSlot slot : {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs,
                                  armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getKnockbackResistance();
        }
    }

    return std::min(total, 1.0f);  // 上限为1.0
}

} // namespace item::items
} // namespace mc
