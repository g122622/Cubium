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

#include "item/core/Item.hpp"
#include "item/crafting/Ingredient.hpp"
#include "item/potion/Potion.hpp"
#include <functional>
#include <utility>
#include <vector>

namespace mc {
namespace potion {

/**
 * @brief 酿造配方谓词
 *
 * 定义一个输入到输出的转换规则。
 *
 * @tparam T 输入输出类型（Potion 或 Item）
 */
template <typename T>
struct MixPredicate {
    std::function<const T*()> input;  ///< 输入提供者
    crafting::Ingredient reagent;     ///< 酿造材料
    std::function<const T*()> output; ///< 输出提供者

    MixPredicate(const T* inputIn, crafting::Ingredient reagentIn, const T* outputIn)
        : input([inputIn]() { return inputIn; })
        , reagent(std::move(reagentIn))
        , output([outputIn]() { return outputIn; })
    {}
};

/**
 * @brief 酿造配方管理
 *
 * 管理所有酿造配方的注册和查找。
 * 支持两种酿造转换：
 * 1. 药水类型转换（加入材料改变药水类型）
 * 2. 容器转换（药水→喷溅药水→滞留药水）
 *
 * 参考: net.minecraft.potion.PotionBrewing
 */
class PotionBrewing {
public:
    using PotionMix = MixPredicate<Potion>;
    using ItemMix = MixPredicate<Item>;

    /**
     * @brief 初始化原版酿造配方
     *
     * 必须在物品和药水注册完成后调用。
     */
    static void initialize();

    // ========== 查询方法 ==========

    /**
     * @brief 检查物品是否为药水容器
     * @param stack 物品堆
     * @return 如果是药水容器返回true
     */
    [[nodiscard]] static bool isPotionItem(const ItemStack& stack);

    /**
     * @brief 检查物品是否为酿造材料
     * @param stack 物品堆
     * @return 如果是酿造材料返回true
     */
    [[nodiscard]] static bool isReagent(const ItemStack& stack);

    /**
     * @brief 检查是否可以酿造
     * @param potionStack 药水物品堆
     * @param reagentStack 材料物品堆
     * @return 如果可以酿造返回true
     */
    [[nodiscard]] static bool canBrew(const ItemStack& potionStack, const ItemStack& reagentStack);

    /**
     * @brief 执行酿造
     * @param potionStack 药水物品堆（输入）
     * @param reagentStack 材料物品堆（输入）
     * @return 酿造后的物品堆
     */
    [[nodiscard]] static ItemStack brew(const ItemStack& potionStack, const ItemStack& reagentStack);

    /**
     * @brief 检查药水是否可酿造
     * @param potion 药水
     * @return 如果是可酿造的药水返回true
     */
    [[nodiscard]] static bool isBrewablePotion(const Potion* potion);

private:
    /// 药水类型转换配方
    static std::vector<PotionMix> s_potionMixes;

    /// 容器转换配方（药水→喷溅药水，喷溅药水→滞留药水）
    static std::vector<ItemMix> s_itemMixes;

    /// 药水容器列表
    static std::vector<crafting::Ingredient> s_potionItems;

    /// 是否已初始化
    static bool s_initialized;

    // ========== 辅助方法 ==========

    /**
     * @brief 添加药水容器
     */
    static void _addContainer(const Item* item);

    /**
     * @brief 添加容器转换配方
     */
    static void _addContainerRecipe(const Item* input, const Item* reagent, const Item* output);

    /**
     * @brief 添加药水类型转换配方
     */
    static void _addMix(const Potion* input, const Item* reagent, const Potion* output);

    /**
     * @brief 检查药水类型转换
     */
    [[nodiscard]] static bool _hasPotionConversion(const Potion* potion, const ItemStack& reagent);

    /**
     * @brief 检查容器转换
     */
    [[nodiscard]] static bool _hasItemConversion(const Item* item, const ItemStack& reagent);

    /**
     * @brief 执行药水类型转换
     */
    [[nodiscard]] static const Potion* _doPotionConversion(const Potion* potion, const ItemStack& reagent);

    /**
     * @brief 执行容器转换
     */
    [[nodiscard]] static const Item* _doItemConversion(const Item* item, const ItemStack& reagent);
};

} // namespace potion
} // namespace mc
