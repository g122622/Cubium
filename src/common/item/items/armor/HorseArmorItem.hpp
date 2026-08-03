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
    [[nodiscard]] i32 getArmorValue() const { return m_armorValue; }

    /**
     * @brief 获取马铠材质路径
     * @return 材质路径
     */
    [[nodiscard]] const ResourceLocation& getTexturePath() const { return m_texturePath; }

private:
    i32 m_armorValue;
    ResourceLocation m_texturePath;
};

} // namespace item::items
} // namespace mc
