#include "SwordItem.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/VanillaBlocks.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/attribute/Attributes.hpp"
#include "../../../entity/attribute/AttributeModifierUUIDs.hpp"
#include "../../../entity/core/LivingEntity.hpp"

namespace mc {
namespace item {
namespace tool {

SwordItem::SwordItem(const tier::IItemTier& tier,
                     i32 attackDamage,
                     f32 attackSpeed,
                     ItemProperties properties)
    : TieredItem(tier, properties)
    , m_attackDamage(static_cast<f32>(attackDamage) + tier.getAttackDamage())
    , m_attackSpeed(attackSpeed) {
}

f32 SwordItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    (void)stack;

    // 剑对蜘蛛网有极高的挖掘效率
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return 15.0f;
    }

    // 剑对植物、珊瑚、葫芦材质有轻微效率
    const Material& mat = state.getMaterial();
    if (mat == Material::PLANT ||
        mat == Material::REPLACEABLE_PLANT ||
        mat == Material::TALL_PLANTS ||
        mat == Material::LEAVES ||
        mat == Material::CORAL ||
        mat == Material::GOURD) {
        return 1.5f;
    }

    // 其他方块返回基础速度
    return 1.0f;
}

bool SwordItem::canHarvestBlock(const BlockState& state) const {
    // 剑只能采集蜘蛛网
    if (VanillaBlocks::COBWEB && &state.owner() == VanillaBlocks::COBWEB) {
        return true;
    }

    // 其他方块不能采集（除非不需要工具）
    return state.getHarvestTool() == TOOL_TYPE_NONE;
}

bool SwordItem::hitEntity(ItemStack& stack,
                          LivingEntity& target,
                          LivingEntity& attacker) {
    (void)target;
    (void)attacker;
    // 剑攻击实体只消耗 1 点耐久（其他工具消耗 2 点）
    stack.attemptDamageItem(1);
    return true;
}

bool SwordItem::onBlockDestroyed(ItemStack& stack,
                                 IWorld& world,
                                 const BlockState& state,
                                 const BlockPos& pos,
                                 LivingEntity& entity) {
    (void)world;
    (void)pos;
    (void)entity;
    // 剑破坏方块消耗 2 点耐久（其他工具消耗 1 点）
    if (state.hardness() > 0.0f) {
        stack.attemptDamageItem(2);
    }
    return true;
}

item::ItemAttributeModifiers SwordItem::getAttributeModifiers(i32 equipmentSlot) const {
    // MC 1.16.5: 剑在主手时提供攻击伤害和攻击速度修饰符
    // 参考: net.minecraft.item.SwordItem#getAttributeModifiers
    if (equipmentSlot == static_cast<i32>(EquipmentSlot::MainHand)) {
        item::ItemAttributeModifiers modifiers;
        std::string uuid = entity::attribute::uuids::fromString(
            entity::attribute::uuids::ATTACK_DAMAGE_MODIFIER_UUID);

        // 添加攻击伤害修饰符
        auto attackDamageAttr = entity::attribute::Attributes::attackDamage();
        auto attackDamageModifier = entity::attribute::AttributeModifier(
            uuid,
            "Weapon modifier",
            static_cast<f64>(m_attackDamage),
            entity::attribute::Operation::Addition
        );
        modifiers.add(attackDamageAttr.get(), attackDamageModifier, equipmentSlot);

        // 添加攻击速度修饰符
        auto attackSpeedAttr = entity::attribute::Attributes::attackSpeed();
        auto attackSpeedModifier = entity::attribute::AttributeModifier(
            entity::attribute::uuids::fromString(
                entity::attribute::uuids::ATTACK_SPEED_MODIFIER_UUID),
            "Weapon modifier",
            static_cast<f64>(m_attackSpeed),
            entity::attribute::Operation::Addition
        );
        modifiers.add(attackSpeedAttr.get(), attackSpeedModifier, equipmentSlot);

        return modifiers;
    }
    return Item::getAttributeModifiers(equipmentSlot);
}

} // namespace tool
} // namespace item
} // namespace mc
