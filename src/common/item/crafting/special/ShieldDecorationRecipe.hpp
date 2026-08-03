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
#include <optional>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 盾牌装饰配方
 *
 * 在合成台中将旗帜图案应用到盾牌上。匹配条件：
 * - 恰好1个盾牌+1个旗帜
 * - 盾牌不能已有BlockEntityTag（即无图案）
 * - 合成栏中没有其他物品
 *
 * 结果：将旗帜的BlockEntityTag（含Patterns和Base）复制到盾牌
 *
 * 参考: net.minecraft.item.crafting.ShieldRecipes
 */
class ShieldDecorationRecipe : public SpecialRecipe {
public:
    /**
     * @brief 构造函数
     * @param id 配方资源位置
     */
    explicit ShieldDecorationRecipe(const ResourceLocation& id);

    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 从合成栏中找到盾牌和旗帜对
     * @return 找到的盾牌和旗帜索引，或nullopt
     */
    struct ShieldBannerPair {
        i32 shieldIndex; ///< 盾牌索引
        i32 bannerIndex; ///< 旗帜索引
    };
    [[nodiscard]] std::optional<ShieldBannerPair> _findPair(const CraftingInventory& inventory) const;
};

} // namespace crafting
} // namespace mc
