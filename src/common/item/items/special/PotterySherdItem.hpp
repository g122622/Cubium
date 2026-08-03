/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "item/core/Item.hpp"
#include "world/blockentity/interactive/DecoratedPotPattern.hpp"
#include <string>
#include <vector>

namespace mc {
namespace item {

/**
 * @brief 陶片物品
 *
 * 用于在饰纹陶罐合成配方中指定罐身每一面图案的物品。
 * 每种陶片对应一种饰纹陶罐图案，陶片与砖块可按十字形排列
 * 合成饰纹陶罐。
 *
 * 可用陶片（1.20 考古学）：
 * - Angler, Archer, ArmsUp, Blade, Brewer, Burn, Danger, Explorer,
 *   Friend, Heart, Heartbreak, Howl, Miner, Mourner, Plenty, Prize,
 *   Sheaf, Shelter, Skull, Snort
 *
 * 可用陶片（1.21 试炼密室）：
 * - Flow, Guster, Scrape
 *
 * 砖块可作为空白面使用（对应 Blank 图案）。
 */
class PotterySherdItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param pattern 饰纹陶罐图案类型
     * @param properties 物品属性（最大堆叠数默认64）
     */
    PotterySherdItem(blockentity::DecoratedPotPattern pattern, ItemProperties properties);

    ~PotterySherdItem() override = default;

    /**
     * @brief 获取图案类型
     */
    [[nodiscard]] blockentity::DecoratedPotPattern getPattern() const noexcept { return m_pattern; }

    /**
     * @brief 添加物品提示信息
     *
     * MC Java 中陶片物品没有额外的 tooltip 描述行，
     * 仅显示由 ITEM_NAME 组件提供的物品名称。
     * 此方法保留为空实现以备将来扩展。
     */
    void addInformation(
        const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const override;

private:
    blockentity::DecoratedPotPattern m_pattern;
};

} // namespace item
} // namespace mc
