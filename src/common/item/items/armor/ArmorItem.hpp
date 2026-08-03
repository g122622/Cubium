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

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"

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
    ArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties);

    // ========== 物品重写方法 ==========

    /**
     * @brief 是否为护甲物品
     * @return true，盔甲物品始终返回 true
     */
    [[nodiscard]] bool isArmor() const override { return true; }

    /**
     * @brief 获取附魔能力
     * @return 盔甲材质的附魔能力
     */
    [[nodiscard]] i32 getItemEnchantability() const override { return m_material.getEnchantability(); }

    /**
     * @brief 检查物品堆是否可以用作修复材料
     *
     * 盔甲可以使用对应材质的材料修复：
     * - 皮革盔甲：皮革
     * - 铁盔甲：铁锭
     * - 金盔甲：金锭
     * - 钻石盔甲：钻石
     * - 下界合金盔甲：下界合金锭
     * - 锁链盔甲：铁锭
     * - 海龟壳：鳞甲
     *
     * 参考: net.minecraft.item.ArmorItem#getIsRepairable
     *
     * @param toRepair 待修复的物品堆
     * @param repair 修复材料物品堆
     * @return 是否可以修复
     */
    [[nodiscard]] bool getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const override;

    /**
     * @brief 获取挖掘速度
     *
     * 盔甲不是工具，返回默认速度1.0。
     */
    [[nodiscard]] f32 getDestroySpeed(const ItemStack& stack, const BlockState& state) const noexcept override;

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
    [[nodiscard]] const armor::ArmorMaterial& getMaterial() const noexcept { return m_material; }

    /**
     * @brief 获取盔甲槽位
     */
    [[nodiscard]] armor::ArmorSlot getSlot() const noexcept { return m_slot; }

    /**
     * @brief 获取防御值
     */
    [[nodiscard]] virtual i32 getDefense() const noexcept { return m_material.getDefense(m_slot); }

    /**
     * @brief 获取韧性
     */
    [[nodiscard]] f32 getToughness() const noexcept { return m_material.getToughness(); }

    /**
     * @brief 获取击退抗性
     */
    [[nodiscard]] f32 getKnockbackResistance() const noexcept { return m_material.getKnockbackResistance(); }

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
    [[nodiscard]] const ItemAttributeModifiers& getAttributeModifiers() const noexcept { return m_attributeModifiers; }

    /**
     * @brief 获取指定装备槽位的属性修饰符
     *
     * 重写 Item::getAttributeModifiers(i32)，当槽位匹配盔甲槽位时
     * 返回预构建的属性修饰符，否则返回空修饰符。
     * 对应 MC 原版 ArmorItem.getAttributeModifiers(EquipmentSlot)。
     *
     * @param equipmentSlot 装备槽位索引
     * @return 该槽位的属性修饰符
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const override;

    /**
     * @brief 检查是否为头盔
     */
    [[nodiscard]] bool isHelmet() const noexcept { return m_slot == armor::ArmorSlot::Head; }

    /**
     * @brief 检查是否为胸甲
     */
    [[nodiscard]] bool isChestplate() const noexcept { return m_slot == armor::ArmorSlot::Chest; }

    /**
     * @brief 检查是否为护腿
     */
    [[nodiscard]] bool isLeggings() const noexcept { return m_slot == armor::ArmorSlot::Legs; }

    /**
     * @brief 检查是否为靴子
     */
    [[nodiscard]] bool isBoots() const noexcept { return m_slot == armor::ArmorSlot::Feet; }

    /**
     * @brief 检查是否为身体护甲（非玩家实体护甲，如狼铠、鹦鹉螺铠甲、马铠）
     */
    [[nodiscard]] bool isBodyArmor() const noexcept { return m_slot == armor::ArmorSlot::Body; }

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
    ItemAttributeModifiers m_attributeModifiers; ///< 属性修饰符

    /**
     * @brief 构建属性修饰符
     *
     * 根据盔甲材质和槽位构建护甲值、韧性和击退抗性修饰符。
     * 子类可在构造函数中调用此方法重建属性修饰符（如需使用非标准防御值）。
     *
     * @param defense 护甲值修饰符使用的防御值
     */
    void _buildAttributeModifiers(i32 defense);
};

} // namespace item::items
} // namespace mc
