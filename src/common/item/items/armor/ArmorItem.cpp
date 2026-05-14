#include "ArmorItem.hpp"
#include "../../../entity/attribute/AttributeModifierUUIDs.hpp"
#include "../../../entity/attribute/Attributes.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../../entity/entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../attribute/ItemAttributeModifiers.hpp"
#include "../../core/ActionResult.hpp"
#include "../../core/ItemStack.hpp"

namespace mc {
namespace item::items {

namespace {

/**
 * @brief 获取盔甲槽位对应的装备槽位
 *
 * ArmorSlot (Head/Chest/Legs/Feet) 映射到 EquipmentSlot (Head/Chest/Legs/Feet)
 */
[[nodiscard]] i32 armorSlotToEquipmentSlot(armor::ArmorSlot slot)
{
    switch (slot) {
        case armor::ArmorSlot::Feet:
            return static_cast<i32>(EquipmentSlot::Feet);
        case armor::ArmorSlot::Legs:
            return static_cast<i32>(EquipmentSlot::Legs);
        case armor::ArmorSlot::Chest:
            return static_cast<i32>(EquipmentSlot::Chest);
        case armor::ArmorSlot::Head:
            return static_cast<i32>(EquipmentSlot::Head);
        default:
            return static_cast<i32>(EquipmentSlot::Head);
    }
}

/**
 * @brief 获取盔甲槽位对应的UUID
 *
 * 参考: net.minecraft.item.ArmorItem.ARMOR_MODIFIERS
 */
[[nodiscard]] const char* getArmorModifierUUID(armor::ArmorSlot slot)
{
    switch (slot) {
        case armor::ArmorSlot::Feet:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_FEET;
        case armor::ArmorSlot::Legs:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_LEGS;
        case armor::ArmorSlot::Chest:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_CHEST;
        case armor::ArmorSlot::Head:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_HEAD;
        default:
            return entity::attribute::uuids::ARMOR_MODIFIER_UUID_HEAD;
    }
}

[[nodiscard]] const ItemStack& getArmorEquipment(const LivingEntity& entity, armor::ArmorSlot slot)
{
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

ArmorItem::ArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties)
    : Item(std::move(properties))
    , m_material(material)
    , m_slot(slot)
{
    // MC 1.16.5: 构造函数中构建属性修饰符
    buildAttributeModifiers();
}

void ArmorItem::buildAttributeModifiers()
{
    // 参考: net.minecraft.item.ArmorItem 构造函数
    // 在构造函数中创建属性修饰符的多重映射

    i32 equipmentSlot = armorSlotToEquipmentSlot(m_slot);
    std::string uuid = entity::attribute::uuids::fromString(getArmorModifierUUID(m_slot));

    // 1. 护甲值修饰符 (generic.armor)
    auto armorAttr = entity::attribute::Attributes::armor();
    auto armorModifier = entity::attribute::AttributeModifier(
        uuid, "Armor modifier", static_cast<f64>(getDefense()), entity::attribute::Operation::Addition);
    m_attributeModifiers.add(armorAttr.get(), armorModifier, equipmentSlot);

    // 2. 护甲韧性修饰符 (generic.armor_toughness)
    auto toughnessAttr = entity::attribute::Attributes::armorToughness();
    auto toughnessModifier = entity::attribute::AttributeModifier(
        uuid, "Armor toughness", static_cast<f64>(getToughness()), entity::attribute::Operation::Addition);
    m_attributeModifiers.add(toughnessAttr.get(), toughnessModifier, equipmentSlot);

    // 3. 击退抗性修饰符 (generic.knockback_resistance) - 仅当下界合金有击退抗性时
    f32 knockbackRes = getKnockbackResistance();
    if (knockbackRes > 0.0f) {
        auto knockbackAttr = entity::attribute::Attributes::knockbackResistance();
        auto knockbackModifier = entity::attribute::AttributeModifier(
            uuid, "Armor knockback resistance", static_cast<f64>(knockbackRes), entity::attribute::Operation::Addition);
        m_attributeModifiers.add(knockbackAttr.get(), knockbackModifier, equipmentSlot);
    }
}

f32 ArmorItem::getDestroySpeed(const ItemStack& /*stack*/, const BlockState& /*state*/) const
{
    // 盔甲不是工具，返回默认速度
    return 1.0f;
}

ItemActionResult ArmorItem::onItemRightClick(IWorld& world, Player& player, Hand hand)
{
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

i32 ArmorItem::getTotalArmorValue(const LivingEntity& entity)
{
    i32 total = 0;

    for (armor::ArmorSlot slot :
        {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs, armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getDefense();
        }
    }

    return total;
}

f32 ArmorItem::getTotalToughness(const LivingEntity& entity)
{
    f32 total = 0.0f;

    for (armor::ArmorSlot slot :
        {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs, armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getToughness();
        }
    }

    return total;
}

f32 ArmorItem::getTotalKnockbackResistance(const LivingEntity& entity)
{
    f32 total = 0.0f;

    for (armor::ArmorSlot slot :
        {armor::ArmorSlot::Feet, armor::ArmorSlot::Legs, armor::ArmorSlot::Chest, armor::ArmorSlot::Head}) {
        const ItemStack& stack = getArmorEquipment(entity, slot);
        const auto* armor = dynamic_cast<const ArmorItem*>(stack.getItem());
        if (armor != nullptr) {
            total += armor->getKnockbackResistance();
        }
    }

    return std::min(total, 1.0f); // 上限为1.0
}

bool ArmorItem::getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const
{
    // MC 1.16.5: 使用材质的修复材料检查
    // 参考: net.minecraft.item.ArmorItem#getIsRepairable
    (void)toRepair; // 盔甲修复不依赖于待修复物品的状态
    return m_material.getRepairMaterial().test(repair);
}

} // namespace item::items
} // namespace mc
