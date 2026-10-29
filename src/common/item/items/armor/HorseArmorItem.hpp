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
#include "common/item/attribute/ItemAttributeModifiers.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 马铠物品
 *
 * 用于装备马类实体（仅限 HorseEntity）提供护甲值。
 * 马铠有六种类型：
 * - 皮革马铠：+3 护甲
 * - 铜马铠：+4 护甲
 * - 铁马铠：+5 护甲
 * - 金马铠：+7 护甲
 * - 钻石马铠：+11 护甲
 * - 下界合金马铠：+19 护甲，防火（FIRE_RESISTANT 标签）
 *
 * 参考: net.minecraft.item.HorseArmorItem
 */
class HorseArmorItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param properties 物品属性
     * @param armorValue 护甲值
     * @param texturePath 马铠材质路径（用于渲染）
     */
    HorseArmorItem(const ItemProperties& properties, i32 armorValue, const ResourceLocation& texturePath);

    /**
     * @brief 获取护甲值
     * @return 护甲值
     */
    [[nodiscard]] i32 getArmorValue() const
    {
        return m_armorValue;
    }

    /**
     * @brief 获取马铠材质路径
     * @return 材质路径
     */
    [[nodiscard]] const ResourceLocation& getTexturePath() const
    {
        return m_texturePath;
    }

    /**
     * @brief 身体护甲槽位提供的属性修饰符（护甲值 / 韧性 / 击退抗性）
     *
     * 对齐 vanilla 的护甲材质组件：马铠虽是普通物品，也照样登记一条韧性修饰符
     * （0.0 也登记），下界合金额外给击退抗性。
     */
    [[nodiscard]] item::ItemAttributeModifiers getAttributeModifiers(i32 equipmentSlot) const override;

    /**
     * @brief 设置韧性与击退抗性
     *
     * 马铠按护甲值单独注册，材质对象只在注册处短暂可用，故在注册后补写这两项。
     * 这是临时形态：原版由材质统一提供，后续把马铠改为按材质注册时删除本方法与上面的
     * 两个成员。
     */
    void setMaterialBonuses(f32 toughness, f32 knockbackResistance)
    {
        m_toughness = toughness;
        m_knockbackResistance = knockbackResistance;
    }

private:
    i32 m_armorValue;
    ResourceLocation m_texturePath;
    /// 材质给出的韧性与击退抗性（构造时取材质的默认值；马铠本身不带材质对象）。
    f32 m_toughness = 0.0f;
    f32 m_knockbackResistance = 0.0f;
};

} // namespace item::items
} // namespace mc
