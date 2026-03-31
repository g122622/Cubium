#include "ArmorMaterial.hpp"
#include "../crafting/Ingredient.hpp"
#include "../../sound/SoundEvent.hpp"

namespace mc {
namespace item::armor {

// ============================================================================
// ArmorMaterial 工具方法
// ============================================================================

i32 ArmorMaterial::getDurabilityMultiplier(ArmorSlot slot) {
    switch (slot) {
        case ArmorSlot::Head:  return 11;
        case ArmorSlot::Chest: return 16;
        case ArmorSlot::Legs:  return 15;
        case ArmorSlot::Feet:  return 13;
        default: return 11;
    }
}

i32 ArmorMaterial::toEquipmentSlotIndex(ArmorSlot slot) {
    return static_cast<i32>(slot);
}

// ============================================================================
// LeatherArmorMaterial
// ============================================================================

i32 LeatherArmorMaterial::getDurability(ArmorSlot slot) const {
    // 基础耐久度: 5
    return 5 * getDurabilityMultiplier(slot);
}

i32 LeatherArmorMaterial::getDefense(ArmorSlot slot) const {
    switch (slot) {
        case ArmorSlot::Head:  return 1;
        case ArmorSlot::Chest: return 3;
        case ArmorSlot::Legs:  return 2;
        case ArmorSlot::Feet:  return 1;
        default: return 0;
    }
}

sound::SoundEvent LeatherArmorMaterial::getEquipSound() const {
    // TODO: 返回物品装备音效
    return sound::SoundEvent();  // item.armor.equip_leather
}

crafting::Ingredient LeatherArmorMaterial::getRepairMaterial() const {
    // TODO: 返回皮革
    return crafting::Ingredient();
}

// ============================================================================
// ChainArmorMaterial
// ============================================================================

i32 ChainArmorMaterial::getDurability(ArmorSlot slot) const {
    // 基础耐久度: 15
    return 15 * getDurabilityMultiplier(slot);
}

i32 ChainArmorMaterial::getDefense(ArmorSlot slot) const {
    switch (slot) {
        case ArmorSlot::Head:  return 2;
        case ArmorSlot::Chest: return 5;
        case ArmorSlot::Legs:  return 4;
        case ArmorSlot::Feet:  return 1;
        default: return 0;
    }
}

sound::SoundEvent ChainArmorMaterial::getEquipSound() const {
    return sound::SoundEvent();  // item.armor.equip_chain
}

crafting::Ingredient ChainArmorMaterial::getRepairMaterial() const {
    // 铁锭（原版如此）
    return crafting::Ingredient();
}

// ============================================================================
// IronArmorMaterial
// ============================================================================

i32 IronArmorMaterial::getDurability(ArmorSlot slot) const {
    // 基础耐久度: 15
    return 15 * getDurabilityMultiplier(slot);
}

i32 IronArmorMaterial::getDefense(ArmorSlot slot) const {
    switch (slot) {
        case ArmorSlot::Head:  return 2;
        case ArmorSlot::Chest: return 6;
        case ArmorSlot::Legs:  return 5;
        case ArmorSlot::Feet:  return 2;
        default: return 0;
    }
}

sound::SoundEvent IronArmorMaterial::getEquipSound() const {
    return sound::SoundEvent();  // item.armor.equip_iron
}

crafting::Ingredient IronArmorMaterial::getRepairMaterial() const {
    // 铁锭
    return crafting::Ingredient();
}

// ============================================================================
// GoldArmorMaterial
// ============================================================================

i32 GoldArmorMaterial::getDurability(ArmorSlot slot) const {
    // 基础耐久度: 7
    return 7 * getDurabilityMultiplier(slot);
}

i32 GoldArmorMaterial::getDefense(ArmorSlot slot) const {
    switch (slot) {
        case ArmorSlot::Head:  return 2;
        case ArmorSlot::Chest: return 5;
        case ArmorSlot::Legs:  return 3;
        case ArmorSlot::Feet:  return 1;
        default: return 0;
    }
}

sound::SoundEvent GoldArmorMaterial::getEquipSound() const {
    return sound::SoundEvent();  // item.armor.equip_gold
}

crafting::Ingredient GoldArmorMaterial::getRepairMaterial() const {
    // 金锭
    return crafting::Ingredient();
}

// ============================================================================
// DiamondArmorMaterial
// ============================================================================

i32 DiamondArmorMaterial::getDurability(ArmorSlot slot) const {
    // 基础耐久度: 33
    return 33 * getDurabilityMultiplier(slot);
}

i32 DiamondArmorMaterial::getDefense(ArmorSlot slot) const {
    switch (slot) {
        case ArmorSlot::Head:  return 3;
        case ArmorSlot::Chest: return 8;
        case ArmorSlot::Legs:  return 6;
        case ArmorSlot::Feet:  return 3;
        default: return 0;
    }
}

sound::SoundEvent DiamondArmorMaterial::getEquipSound() const {
    return sound::SoundEvent();  // item.armor.equip_diamond
}

crafting::Ingredient DiamondArmorMaterial::getRepairMaterial() const {
    // 钻石
    return crafting::Ingredient();
}

// ============================================================================
// TurtleArmorMaterial
// ============================================================================

i32 TurtleArmorMaterial::getDurability(ArmorSlot slot) const {
    // 基础耐久度: 25（只有头盔有意义）
    return 25 * getDurabilityMultiplier(slot);
}

i32 TurtleArmorMaterial::getDefense(ArmorSlot slot) const {
    switch (slot) {
        case ArmorSlot::Head:  return 2;
        default: return 0;  // 海龟壳只有头盔
    }
}

sound::SoundEvent TurtleArmorMaterial::getEquipSound() const {
    return sound::SoundEvent();  // item.armor.equip_turtle
}

crafting::Ingredient TurtleArmorMaterial::getRepairMaterial() const {
    // 海龟壳（鳞甲）
    return crafting::Ingredient();
}

// ============================================================================
// NetheriteArmorMaterial
// ============================================================================

i32 NetheriteArmorMaterial::getDurability(ArmorSlot slot) const {
    // 基础耐久度: 37
    return 37 * getDurabilityMultiplier(slot);
}

i32 NetheriteArmorMaterial::getDefense(ArmorSlot slot) const {
    switch (slot) {
        case ArmorSlot::Head:  return 3;
        case ArmorSlot::Chest: return 8;
        case ArmorSlot::Legs:  return 6;
        case ArmorSlot::Feet:  return 3;
        default: return 0;
    }
}

sound::SoundEvent NetheriteArmorMaterial::getEquipSound() const {
    return sound::SoundEvent();  // item.armor.equip_netherite
}

crafting::Ingredient NetheriteArmorMaterial::getRepairMaterial() const {
    // 下界合金锭
    return crafting::Ingredient();
}

// ============================================================================
// 材质全局实例
// ============================================================================

namespace ArmorMaterials {

const LeatherArmorMaterial LEATHER;
const ChainArmorMaterial CHAIN;
const IronArmorMaterial IRON;
const GoldArmorMaterial GOLD;
const DiamondArmorMaterial DIAMOND;
const TurtleArmorMaterial TURTLE;
const NetheriteArmorMaterial NETHERITE;

void initialize() {
    // 静态实例已创建，无需额外初始化
}

} // namespace ArmorMaterials

} // namespace item::armor
} // namespace mc
