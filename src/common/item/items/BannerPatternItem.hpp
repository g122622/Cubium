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
 * IMPLIED, WITHOUT BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "item/core/Item.hpp"
#include "world/blockentity/interactive/BannerPattern.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {

namespace text {
class ITextComponent;
}

namespace item {

/**
 * @brief 旗帜图案物品
 *
 * 用于在织布机中应用特殊旗帜图案的物品。
 * 每个图案物品对应一种特殊图案类型。
 *
 * 可用图案：
 * - FLOWER: 花朵图案（纸 + 滨菊合成）
 * - CREEPER: 苦力怕图案（纸 + 苦力怕头颅合成，稀有度Uncommon）
 * - SKULL: 骷髅图案（纸 + 凋灵骷髅头颅合成，稀有度Uncommon）
 * - MOJANG: Mojang图案（纸 + 附魔金苹果合成，稀有度Epic）
 * - GLOBE: 地球图案（无合成配方，战利品获取）
 * - PIGLIN: 猪灵图案（无合成配方，战利品获取）
 *
 * 参考: net.minecraft.item.BannerPatternItem
 */
class BannerPatternItem : public Item {
public:
    /**
     * @brief 构造函数
     * @param pattern 图案类型
     * @param properties 物品属性（最大堆叠数1，杂项标签页）
     */
    BannerPatternItem(blockentity::BannerPatternType pattern, ItemProperties properties);

    ~BannerPatternItem() override = default;

    /**
     * @brief 获取图案类型
     */
    [[nodiscard]] blockentity::BannerPatternType getBannerPattern() const noexcept { return m_pattern; }

    /**
     * @brief 添加物品提示信息
     *
     * 显示图案的翻译名称。
     */
    void addInformation(
        const ItemStack& stack, IWorld* world, std::vector<std::string>& tooltip, bool advanced) const override;

private:
    blockentity::BannerPatternType m_pattern;
};

} // namespace item
} // namespace mc
