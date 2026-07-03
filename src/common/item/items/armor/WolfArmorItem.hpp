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

#include "DyeableArmorItem.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 狼铠物品
 *
 * 用于装备狼（Wolf）的护甲，可染色、可修复。
 * 使用犰狳鳞甲（armadillo_scute）修复，耐久度64点。
 *
 * 狼铠的装备和修复交互逻辑在狼实体侧处理，
 * 此类仅负责物品本身的基础属性和染色功能。
 *
 * 参考: net.minecraft.item.WolfArmorItem (MC 1.21.11)
 */
class WolfArmorItem : public DyeableArmorItem {
public:
    /**
     * @brief 构造狼铠
     * @param material 盔甲材质（ArmadilloScuteArmorMaterial）
     * @param slot 盔甲槽位
     * @param properties 物品属性
     */
    WolfArmorItem(const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties);

    /**
     * @brief 获取默认颜色
     *
     * 狼铠的默认颜色为犰狳鳞甲棕色 (0xA06540)
     *
     * @return 默认颜色值
     */
    [[nodiscard]] u32 getDefaultColor() const { return DEFAULT_COLOR; }

private:
    /// 狼铠默认颜色（犰狳鳞甲棕色）
    static constexpr u32 DEFAULT_COLOR = 0xA06540;
};

} // namespace item::items
} // namespace mc
