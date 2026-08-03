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
#include "item/core/ItemStack.hpp"
#include "item/crafting/SpecialRecipe.hpp"
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 地图复制配方
 *
 * 允许玩家使用已填充地图和空地图复制地图。
 * 复制的地图与原地图共享相同的地图数据。
 *
 * 匹配条件：
 * - 必须有一张已填充地图（FilledMapItem）
 * - 必须有至少一张空地图（EmptyMapItem）
 *
 * 结果：
 * - 输出数量 = 空地图数量 + 1（原地图保留）
 */
class MapCloningRecipe : public SpecialRecipe {
public:
    explicit MapCloningRecipe(const ResourceLocation& id);

    /**
     * @brief 检查是否匹配地图复制配方
     * @param inventory 合成网格
     * @return 如果有一张已填充地图和至少一张空地图返回 true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成复制的地图
     * @param inventory 合成网格
     * @return 复制的地图堆（数量 = 空地图数量 + 1）
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 原地图保留在原位置
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 检查物品是否为已填充地图
     * @param stack 物品堆
     * @return 如果是已填充地图返回 true
     */
    [[nodiscard]] static bool _isFilledMap(const ItemStack& stack);

    /**
     * @brief 检查物品是否为空地图
     * @param stack 物品堆
     * @return 如果是空地图返回 true
     */
    [[nodiscard]] static bool _isEmptyMap(const ItemStack& stack);
};

} // namespace crafting
} // namespace mc
