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
#include "common/item/tier/IItemTier.hpp"

namespace mc {
namespace item {
namespace tool {

/**
 * @brief 层级物品基类
 *
 * 所有具有材质层级的物品（工具、剑、护甲）的基类。
 * 提供层级相关的基础属性：耐久度、附魔能力、修复材料。
 *
 * 继承关系：
 * - TieredItem → ToolItem → PickaxeItem/AxeItem/ShovelItem/HoeItem
 * - TieredItem → SwordItem
 *
 * 参考: net.minecraft.item.TieredItem
 */
class TieredItem : public Item {
public:
    /**
     * @brief 构造层级物品
     * @param tier 工具层级（木、石、铁、金、钻石、下界合金）
     * @param properties 物品属性（耐久度会从层级自动设置）
     */
    TieredItem(const tier::IItemTier& tier, ItemProperties properties);

    ~TieredItem() override = default;

    /**
     * @brief 获取工具层级
     * @return 层级引用
     */
    [[nodiscard]] const tier::IItemTier& getTier() const noexcept { return m_tier; }

    /**
     * @brief 获取物品附魔能力
     *
     * 返回层级的附魔能力值。
     * 金制品最高(22)，钻石最低(10)。
     *
     * @return 附魔能力值
     */
    [[nodiscard]] i32 getItemEnchantability() const noexcept override { return m_tier.getEnchantability(); }

    /**
     * @brief 检查物品堆是否可以用作修复材料
     *
     * 检查修复材料是否匹配层级的修复材料。
     * 用于铁砧修复机制：
     * - 木工具：木板
     * - 石工具：圆石
     * - 铁工具：铁锭
     * - 金工具：金锭
     * - 钻石工具：钻石
     * - 下界合金工具：下界合金锭
     *
     * 参考: net.minecraft.item.TieredItem#getIsRepairable
     *
     * @param toRepair 待修复的物品堆
     * @param repair 修复材料物品堆
     * @return 是否可以修复
     */
    [[nodiscard]] bool getIsRepairable(const ItemStack& toRepair, const ItemStack& repair) const override;

protected:
    const tier::IItemTier& m_tier;
};

} // namespace tool
} // namespace item
} // namespace mc
