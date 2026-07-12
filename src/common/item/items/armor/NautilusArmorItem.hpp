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

#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 鹦鹉螺铠甲物品
 *
 * 用于装备鹦鹉螺类实体（Nautilus, ZombieNautilus）的护甲。
 * 鹦鹉螺铠甲没有耐久度，不会损坏，不支持染色。
 * 有五种材质：铜、铁、金、钻石、下界合金。
 *
 * 与马铠（HorseArmorItem）类似，鹦鹉螺铠甲是一种简单的护甲物品，
 * 不继承自 ArmorItem，因为它不参与玩家的盔甲装备系统、不可破坏。
 * 护甲值从 `ArmorMaterial::getDefense(ArmorSlot::Body)` 推导，
 * 与 MC 1.21.11 `Item.Properties.nautilusArmor(ArmorMaterial)` 通过
 * `ArmorMaterial.createAttributes(ArmorType.BODY)` 取护甲值的语义一致。
 *
 * 实体侧集成已就绪（AbstractNautilusEntity）：
 * - IEquipable 槽位 1 = Body 装备槽位，用于装备鹦鹉螺铠甲
 * - interactMob 中处理右键对鹦鹉螺使用铠甲的装备交互逻辑
 * - ZombieNautilusEntity::sunProtectionSlot() 返回 Body，鹦鹉螺铠甲可替代亡灵燃烧
 * - 装备音效通过 AbstractNautilusEntity::getEquipSound(EquipmentSlot::Body) 播放
 *
 * TODO: 鹦鹉螺铠甲渲染层（显示铠甲模型）尚未实现，等待客户端实体渲染器扩展。
 *
 * 下界合金鹦鹉螺铠甲额外具备防火属性（FIRE_RESISTANT 标签），与下界合金马铠机制一致，
 * 免疫火焰/岩浆伤害。
 *
 * 参考: net.minecraft.world.item.Item.Properties.nautilusArmor(ArmorMaterial) (MC 1.21.11)
 */
class NautilusArmorItem : public Item {
public:
    /**
     * @brief 构造鹦鹉螺铠甲
     * @param properties 物品属性（maxStackSize=1 应由调用者设置）
     * @param material 盔甲材质（用于获取装备音效、修复材料和 Body 槽位护甲值）
     */
    NautilusArmorItem(const ItemProperties& properties, const armor::ArmorMaterial& material);

    /**
     * @brief 获取护甲值
     * @return 护甲值（由材质的 Body 槽位防御值提供，等价于 material.getDefense(ArmorSlot::Body)）
     */
    [[nodiscard]] i32 getArmorValue() const { return m_armorValue; }

    /**
     * @brief 获取装备音效
     * @return 音效事件
     */
    [[nodiscard]] sound::SoundEvent getEquipSound() const { return m_equipSound; }

    /**
     * @brief 获取盔甲材质
     * @return 盔甲材质引用
     */
    [[nodiscard]] const armor::ArmorMaterial& getMaterial() const { return m_material; }

private:
    i32 m_armorValue;
    sound::SoundEvent m_equipSound;
    const armor::ArmorMaterial& m_material;
};

} // namespace item::items
} // namespace mc
