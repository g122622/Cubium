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
#include <unordered_map>

namespace mc {

class Item;

namespace blocks {

/**
 * @brief 可堆肥物品注册表
 *
 * 管理可堆肥物品及其堆肥概率。
 *
 * 堆肥概率表：
 * - 30%: 树叶、树苗、种子等
 * - 50%: 干海带块、高草、仙人掌、藤蔓、西瓜片等
 * - 65%: 花、蘑菇、农作物、海带等
 * - 85%: 干草块、面包、曲奇等
 * - 100%: 蛋糕、南瓜派
 */
class CompostableItems {
public:
    /**
     * @brief 初始化可堆肥物品注册表
     *
     * 必须在 Items::initialize() 之后调用。
     */
    static void initialize();

    /**
     * @brief 获取物品的堆肥概率
     *
     * @param item 物品指针
     * @return 堆肥概率 [0.0, 1.0]，返回 0.0 表示不可堆肥
     */
    [[nodiscard]] static f32 getCompostChance(const Item* item);

    /**
     * @brief 检查物品是否可堆肥
     *
     * @param item 物品指针
     * @return true 如果可堆肥
     */
    [[nodiscard]] static bool isCompostable(const Item* item);

    /**
     * @brief 检查注册表是否已初始化
     */
    [[nodiscard]] static bool isInitialized() noexcept { return s_initialized; }

private:
    // 物品到堆肥概率的映射
    static std::unordered_map<const Item*, f32> s_chances;
    static bool s_initialized;

    /**
     * @brief 注册可堆肥物品
     *
     * @param item 物品指针
     * @param chance 堆肥概率 [0.0, 1.0]
     */
    static void _registerCompostable(const Item* item, f32 chance);

    // 按概率等级注册物品
    static void _registerChance30();  // 30% 概率物品
    static void _registerChance50();  // 50% 概率物品
    static void _registerChance65();  // 65% 概率物品
    static void _registerChance85();  // 85% 概率物品
    static void _registerChance100(); // 100% 概率物品
};

} // namespace blocks
} // namespace mc
