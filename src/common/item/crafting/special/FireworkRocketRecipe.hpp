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
 * @brief 烟花火箭合成配方（crafting_special_firework_rocket）
 *
 * 对应 MC 1.21.11 的 FireworkRocketRecipe。玩家用火药 + 纸 + 烟花之星合成 3 个烟花火箭：
 * - 纸恰一张；
 * - 火药 1~3 个，数量即 flight_duration（飞行时长等级）；
 * - 烟花之星 0~N 个，各自携带的 firework_explosion 组件依网格顺序累积为火箭的爆炸列表。
 * 结果火箭携带 fireworks 组件承载以上数据。
 *
 * 参考: net.minecraft.world.item.crafting.FireworkRocketRecipe
 */
class FireworkRocketRecipe : public SpecialRecipe {
public:
    explicit FireworkRocketRecipe(const ResourceLocation& id);

    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;
};

} // namespace crafting
} // namespace mc
