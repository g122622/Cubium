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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/crafting/SpecialRecipe.hpp"
#include "util/color/DyeColor.hpp"
#include <optional>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 旗帜复制配方
 *
 * 在合成台中复制旗帜图案。匹配条件：
 * - 恰好2个相同颜色的旗帜
 * - 其中一个有图案（源），另一个无图案（目标）
 * - 源旗帜图案不超过6层
 * - 合成栏中没有其他物品
 *
 * 结果：源旗帜的副本（count=1）
 * 剩余：源旗帜保留返回
 *
 * 参考: net.minecraft.item.crafting.BannerDuplicateRecipe
 */
class BannerDuplicateRecipe : public SpecialRecipe {
public:
    /**
     * @brief 构造函数
     * @param id 配方资源位置
     */
    explicit BannerDuplicateRecipe(const ResourceLocation& id);

    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 从合成栏中找到旗帜对
     * @return 找到的旗帜对信息，或nullopt
     */
    struct BannerPair {
        i32 sourceIndex; ///< 有图案的旗帜索引
        i32 targetIndex; ///< 无图案的旗帜索引
        DyeColor color;  ///< 旗帜颜色
    };
    [[nodiscard]] std::optional<BannerPair> _findBannerPair(const CraftingInventory& inventory) const;
};

} // namespace crafting
} // namespace mc
