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
#include "common/core/Types.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"

namespace mc {
namespace item::items {

/**
 * @brief 狼铠物品
 *
 * 用于装备狼（Wolf）的护甲，可染色、可修复。
 * 使用犰狳鳞甲（armadillo_scute）修复，耐久度64点。
 * 防御值 11（Body 槽位），耐久度 4 * 16 = 64。
 *
 * 狼铠的装备和修复交互逻辑在狼实体侧处理（WolfEntity::interactMob）：
 * - 装备：主人右键狼 + 手持狼铠 + 未装备 + 非幼年 → 装备狼铠
 * - 修复：主人右键坐下的狼 + 犰狳鳞甲 + 狼铠已受损 → 修复 12.5% 耐久
 * - 染色：主人右键狼 + 手持染料 + 已装备狼铠 → 改变狼铠颜色
 * - 剪切：主人右键狼 + 手持剪刀 + 已装备狼铠 → 剪下狼铠（MobEntity::canShearEquipment）
 * - 伤害吸收：狼穿戴狼铠时，非绕过护甲的伤害由狼铠吸收（WolfEntity::actuallyHurt）
 * - 裂纹：狼铠受损到不同阈值时播放裂纹音效和粒子（Crackiness::WOLF_ARMOR）
 *
 * 参考: net.minecraft.world.item.WolfArmorItem (MC 1.21.11)
 */
class WolfArmorItem : public DyeableArmorItem {
public:
    /**
     * @brief 构造狼铠
     * @param material 盔甲材质（ArmadilloScuteArmorMaterial）
     * @param slot 盔甲槽位（应为 ArmorSlot::Body）
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
    [[nodiscard]] u32 getDefaultColor() const override { return DEFAULT_COLOR; }

private:
    /// 狼铠默认颜色（犰狳鳞甲棕色）
    static constexpr u32 DEFAULT_COLOR = 0xA06540;
};

} // namespace item::items
} // namespace mc
