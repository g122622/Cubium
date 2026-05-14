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

#include "core/Types.hpp"
#include <vector>

namespace mc {

// 前向声明
class ItemStack;

/**
 * @brief 配方辅助填充器接口
 *
 * 实现此接口的背包可以将内容填充到RecipeItemHelper中，
 * 用于配方书查找可用配方。
 *
 * RecipeItemHelper是一个物品计数器，用于快速检查玩家背包
 * 是否有足够的材料制作某个配方。
 *
 * 参考: net.minecraft.inventory.IRecipeHelperPopulator
 */
class IRecipeHelperPopulator {
public:
    virtual ~IRecipeHelperPopulator() = default;

    /**
     * @brief 将背包内容填充到物品计数器
     * @param itemCounts 物品ID到数量的映射（输出参数）
     *
     * 遍历背包中的所有物品，对于每个物品：
     * - 如果物品没有损坏、没有附魔、没有自定义名称，则计数
     * - 使用物品ID作为key，数量累加
     *
     * 使用示例：
     * @code
     * std::unordered_map<i32, i32> itemCounts;
     * inventory.fillStackedContents(itemCounts);
     * // 然后可以使用itemCounts检查是否有足够材料
     * @endcode
     *
     * 参考: net.minecraft.inventory.IRecipeHelperPopulator.fillStackedContents
     */
    virtual void fillStackedContents(std::unordered_map<i32, i32>& itemCounts) const = 0;
};

} // namespace mc
