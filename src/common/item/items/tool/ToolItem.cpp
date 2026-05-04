#include "ToolItem.hpp"
#include "../../../world/block/Block.hpp"
#include "../../core/ItemStack.hpp"
#include "../../../entity/attribute/Attributes.hpp"
#include "../../../entity/attribute/AttributeModifierUUIDs.hpp"
#include "../../../entity/core/LivingEntity.hpp"
#include "../../enchantment/EnchantmentHelper.hpp"
#include "../../enchantment/enchantments/tool/EfficiencyEnchantment.hpp"

namespace mc {
namespace item {
namespace tool {

ToolItem::ToolItem(f32 attackDamage,
                   f32 attackSpeed,
                   const tier::IItemTier& tier,
                   std::unordered_set<const Block*> effectiveBlocks,
                   ToolType toolType,
                   ItemProperties properties)
    : TieredItem(tier, properties)
    , m_effectiveBlocks(std::move(effectiveBlocks))
    , m_toolType(toolType)
    , m_attackDamage(attackDamage + tier.getAttackDamage())
    , m_attackSpeed(attackSpeed)
    , m_efficiency(tier.getEfficiency()) {
}

f32 ToolItem::getDestroySpeed(const ItemStack& stack, const BlockState& state) const {
    // 1. 检查材质有效性
    f32 speed = 1.0f;
    if (isEffectiveMaterial(state.getMaterial())) {
        speed = m_efficiency;
    } else if (isEffectiveBlock(state.owner())) {
        // 2. 检查特定方块有效性
        speed = m_efficiency;
    }

    // 3. 应用效率附魔加成（MC 1.16.5）
    // 效率附魔只在工具对当前方块有效时才生效
    if (speed > 1.0f) {
        i32 efficiencyLevel = enchant::EnchantmentHelper::getEfficiencyLevel(stack);
        if (efficiencyLevel > 0) {
            speed += static_cast<f32>(enchant::EfficiencyEnchantment::getMiningSpeedBonus(efficiencyLevel));
        }
    }

    return speed;
}

bool ToolItem::canHarvestBlock(const BlockState& state) const {
    // 获取工具类型的u8值
    u8 toolTypeValue = static_cast<u8>(m_toolType);

    // 检查工具类型和挖掘等级
    if (state.getHarvestTool() == toolTypeValue) {
        return m_tier.getHarvestLevel() >= state.getHarvestLevel();
    }

    // 如果方块不需要工具，总是可以采集
    if (state.getHarvestTool() == TOOL_TYPE_NONE) {
        return true;
    }

    // 如果方块需要工具但我们没有匹配的类型
    // 默认情况下不能采集（但子类如镐可能有特殊逻辑）
    return false;
}

bool ToolItem::hitEntity(ItemStack& stack,
                         LivingEntity& target,
                         LivingEntity& attacker) {
    (void)target;
    (void)attacker;
    // 攻击实体消耗 2 点耐久
    stack.attemptDamageItem(2);
    return true;
}

bool ToolItem::onBlockDestroyed(ItemStack& stack,
                                IWorld& world,
                                const BlockState& state,
                                const BlockPos& pos,
                                LivingEntity& entity) {
    (void)world;
    (void)pos;
    (void)entity;
    // 只有硬度 > 0 的方块才消耗耐久
    if (state.hardness() > 0.0f) {
        stack.attemptDamageItem(1);
    }
    return true;
}

bool ToolItem::isEffectiveBlock(const Block& block) const {
    return m_effectiveBlocks.find(&block) != m_effectiveBlocks.end();
}

bool ToolItem::isEffectiveMaterial(const Material& material) const {
    (void)material;
    // 基类默认不检查材质，由子类重写
    return false;
}

item::ItemAttributeModifiers ToolItem::getAttributeModifiers(i32 equipmentSlot) const {
    // MC 1.16.5: 工具在主手时提供攻击伤害和攻击速度修饰符
    // 参考: net.minecraft.item.ToolItem#getAttributeModifiers
    if (equipmentSlot == static_cast<i32>(EquipmentSlot::MainHand)) {
        item::ItemAttributeModifiers modifiers;
        String uuid = entity::attribute::uuids::fromString(
            entity::attribute::uuids::ATTACK_DAMAGE_MODIFIER_UUID);

        // 添加攻击伤害修饰符
        auto attackDamageAttr = entity::attribute::Attributes::attackDamage();
        auto attackDamageModifier = entity::attribute::AttributeModifier(
            uuid,
            "Tool modifier",
            static_cast<f64>(m_attackDamage),
            entity::attribute::Operation::Addition
        );
        modifiers.add(attackDamageAttr.get(), attackDamageModifier, equipmentSlot);

        // 添加攻击速度修饰符
        auto attackSpeedAttr = entity::attribute::Attributes::attackSpeed();
        auto attackSpeedModifier = entity::attribute::AttributeModifier(
            entity::attribute::uuids::fromString(
                entity::attribute::uuids::ATTACK_SPEED_MODIFIER_UUID),
            "Tool modifier",
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
