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
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvent.hpp"
#include <string>

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
        case ArmorSlot::Body:
            return 16; // 与 Chest 相同（MC 1.21.11 ArmorType.BODY unitDurability=16）
        default:
            return 11;
    }
}

i32 ArmorMaterial::toEquipmentSlotIndex(ArmorSlot slot)
{
    // ArmorSlot 与 EquipmentSlot 的映射：
    // Head=0 -> EquipmentSlot::Head=5
    // Chest=1 -> EquipmentSlot::Chest=4
    // Legs=2 -> EquipmentSlot::Legs=3
    // Feet=3 -> EquipmentSlot::Feet=2
    // Body=4 -> EquipmentSlot::Body=6
    switch (slot) {
        case ArmorSlot::Head:
            return static_cast<i32>(EquipmentSlot::Head);
        case ArmorSlot::Chest:
            return static_cast<i32>(EquipmentSlot::Chest);
        case ArmorSlot::Legs:
            return static_cast<i32>(EquipmentSlot::Legs);
        case ArmorSlot::Feet:
            return static_cast<i32>(EquipmentSlot::Feet);
        case ArmorSlot::Body:
            return static_cast<i32>(EquipmentSlot::Body);
        default:
            return static_cast<i32>(EquipmentSlot::Head);
    }
}

ResourceLocation ArmorMaterial::getArmorTexturePath(const std::string& assetId, ArmorSlot slot)
{
    if (slot == ArmorSlot::Legs) {
        return ResourceLocation("minecraft", "textures/entity/equipment/humanoid_leggings/" + assetId + ".png");
    }
    return ResourceLocation("minecraft", "textures/entity/equipment/humanoid/" + assetId + ".png");
}

ResourceLocation ArmorMaterial::getLeatherOverlayTexturePath(ArmorSlot slot)
{
    if (slot == ArmorSlot::Legs) {
        return ResourceLocation("minecraft", "textures/entity/equipment/humanoid_leggings/leather_overlay.png");
    }
    return ResourceLocation("minecraft", "textures/entity/equipment/humanoid/leather_overlay.png");
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
        case ArmorSlot::Body:
            return 3; // MC 1.21.11 ArmorMaterials.LEATHER defense.body=3
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
        case ArmorSlot::Body:
            return 4; // MC 1.21.11 ArmorMaterials.CHAINMAIL defense.body=4
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
    // MC 1.21.11 原版防御值：头盔=2, 胸甲=4, 护腿=3, 靴子=1, 身体=4
    switch (slot) {
        case ArmorSlot::Head:
            return 2;
        case ArmorSlot::Chest:
            return 4;
        case ArmorSlot::Legs:
            return 3;
        case ArmorSlot::Feet:
            return 1;
        case ArmorSlot::Body:
            return 4;
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
        case ArmorSlot::Body:
            return 5; // MC 1.21.11 ArmorMaterials.IRON defense.body=5
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
        case ArmorSlot::Body:
            return 7; // MC 1.21.11 ArmorMaterials.GOLD defense.body=7
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
        case ArmorSlot::Body:
            return 11; // MC 1.21.11 ArmorMaterials.DIAMOND defense.body=11
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
        case ArmorSlot::Body:
            return 5; // MC 1.21.11 ArmorMaterials.TURTLE_SCUTE defense.body=5
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
    return crafting::Ingredient::fromItem(Items::TURTLE_SCUTE);
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
        case ArmorSlot::Body:
            return 19; // MC 1.21.11 ArmorMaterials.NETHERITE defense.body=19
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
// ArmadilloScuteArmorMaterial（犰狳鳞甲材质 - 狼铠）
// ============================================================================

i32 ArmadilloScuteArmorMaterial::getDurability(ArmorSlot slot) const
{
    // 基础耐久度: 4
    // 狼铠使用 BODY 槽位，耐久度 = 4 * 16 = 64
    return 4 * getDurabilityMultiplier(slot);
}

i32 ArmadilloScuteArmorMaterial::getDefense(ArmorSlot slot) const
{
    // MC 1.21.11: ARMADILLO_SCUTE 材质防御值
    // HEAD=3, CHEST=6, LEGS=8, FEET=3, BODY=11
    // 狼铠仅使用 BODY 槽位的 11 防御值，其他槽位不实际使用
    switch (slot) {
        case ArmorSlot::Head:
            return 3;
        case ArmorSlot::Chest:
            return 6;
        case ArmorSlot::Legs:
            return 8;
        case ArmorSlot::Feet:
            return 3;
        case ArmorSlot::Body:
            return 11;
        default:
            return 0;
    }
}

sound::SoundEvent ArmadilloScuteArmorMaterial::getEquipSound() const
{
    return sound::SoundEvent(ResourceLocation("minecraft:item.armor.equip_wolf"));
}

crafting::Ingredient ArmadilloScuteArmorMaterial::getRepairMaterial() const
{
    return crafting::Ingredient::fromItem(Items::ARMADILLO_SCUTE);
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
const ArmadilloScuteArmorMaterial ARMADILLO_SCUTE;

void initialize()
{
    // 静态实例已创建，无需额外初始化
}

} // namespace ArmorMaterials

} // namespace item::armor
} // namespace mc
