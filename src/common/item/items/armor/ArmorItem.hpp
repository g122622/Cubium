#pragma once

#include "../../core/Item.hpp"
#include "../../armor/ArmorMaterial.hpp"
#include "../../attribute/ItemAttributeModifiers.hpp"
#include "../../../entity/core/LivingEntity.hpp"

namespace mc {

// Forward declarations
class Player;

namespace item::items {

/**
 * @brief 盔甲基类
 *
 * 所有盔甲物品的基类。
 * 参考: net.minecraft.item.ArmorItem
 *
 * 用法示例:
 * @code
 * // 注册铁头盔
 * auto& ironHelmet = ItemRegistry::instance().registerItem<ArmorItem>(
 *     ResourceLocation("minecraft:iron_helmet"),
 *     ArmorMaterials::IRON,
 *     ArmorSlot::Head,
 *     ItemProperties().maxDamage(IRON.getDurability(ArmorSlot::Head))
 * );
 * @endcode
 */
class ArmorItem : public Item {
public:
    /**
     * @brief 构造盔甲物品
     * @param material 盔甲材质
     * @param slot 盔甲槽位
     * @param properties 物品属性
     */
    ArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot,
              ItemProperties properties);

    // ========== 物品重写方法 ==========

    /**
     * @brief 获取附魔能力
     * @return 盔甲材质的附魔能力
     */
    [[nodiscard]] i32 getItemEnchantability() const override {
        return m_material.getEnchantability();
    }

    /**
     * @brief 是否可修复
     * @return 盔甲材质有修复材料时可修复
     */
    [[nodiscard]] bool isRepairable() const { return true; }

    /**
     * @brief 获取挖掘速度
     *
     * 盔甲不是工具，返回默认速度1.0。
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack,
                                       const BlockState& state) const override;

    /**
     * @brief 右键使用物品
     *
     * 如果玩家当前槽位没有盔甲，则装备此盔甲。
     *
     * @param world 世界
     * @param player 玩家
     * @param hand 使用的手
     * @return 动作结果
     */
    ItemActionResult onItemRightClick(IWorld& world, Player& player, Hand hand) override;

    // ========== 盔甲特有方法 ==========

    /**
     * @brief 获取盔甲材质
     */
    [[nodiscard]] const armor::ArmorMaterial& getMaterial() const { return m_material; }

    /**
     * @brief 获取盔甲槽位
     */
    [[nodiscard]] armor::ArmorSlot getSlot() const { return m_slot; }

    /**
     * @brief 获取防御值
     */
    [[nodiscard]] i32 getDefense() const { return m_material.getDefense(m_slot); }

    /**
     * @brief 获取韧性
     */
    [[nodiscard]] f32 getToughness() const { return m_material.getToughness(); }

    /**
     * @brief 获取击退抗性
     */
    [[nodiscard]] f32 getKnockbackResistance() const {
        return m_material.getKnockbackResistance();
    }

    /**
     * @brief 获取属性修饰符
     *
     * 返回此盔甲物品在装备槽位上应用的属性修饰符。
     * 包含护甲值、护甲韧性和击退抗性修饰符。
     *
     * 参考: net.minecraft.item.ArmorItem#getAttributeModifiers
     *
     * @return 属性修饰符管理器
     */
    [[nodiscard]] const ItemAttributeModifiers& getAttributeModifiers() const {
        return m_attributeModifiers;
    }

    /**
     * @brief 检查是否为头盔
     */
    [[nodiscard]] bool isHelmet() const { return m_slot == armor::ArmorSlot::Head; }

    /**
     * @brief 检查是否为胸甲
     */
    [[nodiscard]] bool isChestplate() const { return m_slot == armor::ArmorSlot::Chest; }

    /**
     * @brief 检查是否为护腿
     */
    [[nodiscard]] bool isLeggings() const { return m_slot == armor::ArmorSlot::Legs; }

    /**
     * @brief 检查是否为靴子
     */
    [[nodiscard]] bool isBoots() const { return m_slot == armor::ArmorSlot::Feet; }

    // ========== 静态辅助方法 ==========

    /**
     * @brief 计算总护甲值
     * @param entity 实体
     * @return 总护甲值 (0-20)
     */
    [[nodiscard]] static i32 getTotalArmorValue(const LivingEntity& entity);

    /**
     * @brief 计算总韧性
     * @param entity 实体
     * @return 总韧性值
     */
    [[nodiscard]] static f32 getTotalToughness(const LivingEntity& entity);

    /**
     * @brief 计算总击退抗性
     * @param entity 实体
     * @return 总击退抗性 (0.0-1.0)
     */
    [[nodiscard]] static f32 getTotalKnockbackResistance(const LivingEntity& entity);

protected:
    const armor::ArmorMaterial& m_material;
    armor::ArmorSlot m_slot;
    ItemAttributeModifiers m_attributeModifiers;  ///< 属性修饰符

private:
    /**
     * @brief 构建属性修饰符
     *
     * 在构造函数中调用，根据盔甲材质和槽位构建护甲值、韧性和击退抗性修饰符。
     * 参考: net.minecraft.item.ArmorItem 构造函数
     */
    void buildAttributeModifiers();
};

} // namespace item::items
} // namespace mc
