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

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/crafting/SpecialRecipe.hpp"

namespace mc {
namespace crafting {

/**
 * @brief 烟花之星合成配方（crafting_special_firework_star）
 *
 * 对应 MC 1.21.11 的 FireworkStarRecipe。玩家用火药 + 染料（+ 可选的形状物品 /
 * 荧石粉 / 钻石）合成烟花之星，结果携带 firework_explosion 组件描述本次爆炸：
 * - 形状物品（火焰弹→大型球、羽毛→爆裂、金粒→星形、各类头颅→苦力怕形）决定形状，
 *   至多一个；
 * - 染料颜色按网格顺序累积为爆炸主色列表；
 * - 荧石粉置 has_twinkle，钻石置 has_trail，各至多一个。
 * 匹配要求网格非空物品数 >= 2，且至少含一个火药与一个染料。
 *
 * 参考: net.minecraft.world.item.crafting.FireworkStarRecipe
 */
class FireworkStarRecipe : public SpecialRecipe {
public:
    explicit FireworkStarRecipe(const ResourceLocation& id);

    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;
};

} // namespace crafting
} // namespace mc
