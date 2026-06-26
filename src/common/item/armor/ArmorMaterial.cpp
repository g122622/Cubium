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

#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/Items.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/sound/SoundEvent.hpp"

namespace mc {
namespace item::armor {

// ============================================================================
// ArmorMaterial 工具方法
// ============================================================================

i32 ArmorMaterial::getDurabilityMultiplier(ArmorSlot slot)
{
    switch (slot) {
        case ArmorSlot::Head:
            return 11;
        case ArmorSlot::Chest:
            return 16;
        case ArmorSlot::Legs:
            return 15;
        case ArmorSlot::Feet:
            return 13;
        default:
            return 11;
    }
}

i32 ArmorMaterial::toEquipmentSlotIndex(ArmorSlot slot)
{
    return static_cast<i32>(slot);
}

// ============================================================================
// LeatherArmorMaterial
// ============================================================================

i32 LeatherArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 5
    return 5 * getDurabilityMultiplier(slot);
}

i32 LeatherArmorMaterial::getDefense(ArmorSlot slot) const
{
    switch (slot) {
        case ArmorSlot::Head:
            return 1;
        case ArmorSlot::Chest:
            return 3;
        case ArmorSlot::Legs:
            return 2;
        case ArmorSlot::Feet:
            return 1;
        default:
            return 0;
    }
}

sound::SoundEvent LeatherArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_leather"));
}

crafting::Ingredient LeatherArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::LEATHER);
}

// ============================================================================
// ChainArmorMaterial
// ============================================================================

i32 ChainArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 15
    return 15 * getDurabilityMultiplier(slot);
}

i32 ChainArmorMaterial::getDefense(ArmorSlot slot) const
{
    switch (slot) {
        case ArmorSlot::Head:
            return 2;
        case ArmorSlot::Chest:
            return 5;
        case ArmorSlot::Legs:
            return 4;
        case ArmorSlot::Feet:
            return 1;
        default:
            return 0;
    }
}

sound::SoundEvent ChainArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_chain"));
}

crafting::Ingredient ChainArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::IRON_INGOT);
}

// ============================================================================
// CopperArmorMaterial
// ============================================================================

i32 CopperArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 11（MC 1.21.11 原版数据）
    return 11 * getDurabilityMultiplier(slot);
}

i32 CopperArmorMaterial::getDefense(ArmorSlot slot) const
{
    // MC 1.21.11 原版防御值：头盔=2, 胸甲=4, 护腿=3, 靴子=1
    switch (slot) {
        case ArmorSlot::Head:
            return 2;
        case ArmorSlot::Chest:
            return 4;
        case ArmorSlot::Legs:
            return 3;
        case ArmorSlot::Feet:
            return 1;
        default:
            return 0;
    }
}

sound::SoundEvent CopperArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_copper"));
}

crafting::Ingredient CopperArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::COPPER_INGOT);
}

// ============================================================================
// IronArmorMaterial
// ============================================================================

i32 IronArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 15
    return 15 * getDurabilityMultiplier(slot);
}

i32 IronArmorMaterial::getDefense(ArmorSlot slot) const
{
    switch (slot) {
        case ArmorSlot::Head:
            return 2;
        case ArmorSlot::Chest:
            return 6;
        case ArmorSlot::Legs:
            return 5;
        case ArmorSlot::Feet:
            return 2;
        default:
            return 0;
    }
}

sound::SoundEvent IronArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_iron"));
}

crafting::Ingredient IronArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::IRON_INGOT);
}

// ============================================================================
// GoldArmorMaterial
// ============================================================================

i32 GoldArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 7
    return 7 * getDurabilityMultiplier(slot);
}

i32 GoldArmorMaterial::getDefense(ArmorSlot slot) const
{
    switch (slot) {
        case ArmorSlot::Head:
            return 2;
        case ArmorSlot::Chest:
            return 5;
        case ArmorSlot::Legs:
            return 3;
        case ArmorSlot::Feet:
            return 1;
        default:
            return 0;
    }
}

sound::SoundEvent GoldArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_gold"));
}

crafting::Ingredient GoldArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::GOLD_INGOT);
}

// ============================================================================
// DiamondArmorMaterial
// ============================================================================

i32 DiamondArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 33
    return 33 * getDurabilityMultiplier(slot);
}

i32 DiamondArmorMaterial::getDefense(ArmorSlot slot) const
{
    switch (slot) {
        case ArmorSlot::Head:
            return 3;
        case ArmorSlot::Chest:
            return 8;
        case ArmorSlot::Legs:
            return 6;
        case ArmorSlot::Feet:
            return 3;
        default:
            return 0;
    }
}

sound::SoundEvent DiamondArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_diamond"));
}

crafting::Ingredient DiamondArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::DIAMOND);
}

// ============================================================================
// TurtleArmorMaterial
// ============================================================================

i32 TurtleArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 25（只有头盔有意义）
    return 25 * getDurabilityMultiplier(slot);
}

i32 TurtleArmorMaterial::getDefense(ArmorSlot slot) const
{
    switch (slot) {
        case ArmorSlot::Head:
            return 2;
        default:
            return 0; // 海龟壳只有头盔
    }
}

sound::SoundEvent TurtleArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_turtle"));
}

crafting::Ingredient TurtleArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::SCUTE);
}

// ============================================================================
// NetheriteArmorMaterial
// ============================================================================

i32 NetheriteArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 37
    return 37 * getDurabilityMultiplier(slot);
}

i32 NetheriteArmorMaterial::getDefense(ArmorSlot slot) const
{
    switch (slot) {
        case ArmorSlot::Head:
            return 3;
        case ArmorSlot::Chest:
            return 8;
        case ArmorSlot::Legs:
            return 6;
        case ArmorSlot::Feet:
            return 3;
        default:
            return 0;
    }
}

sound::SoundEvent NetheriteArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_netherite"));
}

crafting::Ingredient NetheriteArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::NETHERITE_INGOT);
}

// ============================================================================
// 材质全局实例
// ============================================================================

namespace ArmorMaterials {

const LeatherArmorMaterial LEATHER;
const CopperArmorMaterial COPPER;
const ChainArmorMaterial CHAIN;
const IronArmorMaterial IRON;
const GoldArmorMaterial GOLD;
const DiamondArmorMaterial DIAMOND;
const TurtleArmorMaterial TURTLE;
const NetheriteArmorMaterial NETHERITE;

void initialize()
{
    // 静态实例已创建，无需额外初始化
}

} // namespace ArmorMaterials

} // namespace item::armor
} // namespace mc
