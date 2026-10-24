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

#include "common/item/core/ItemRarity.hpp"
#include <optional>
#include <string_view>

namespace mc {

/**
 * @brief 查原版物品的稀有度
 *
 * 稀有度决定物品名颜色与战利品品质。它散落在上百处物品注册调用里，逐个手写
 * 极易漏；这里以原版 jar 导出的表为唯一来源（scripts/baking/gen_item_rarity.py
 * 生成 VanillaItemRarity.cpp），Items::initialize() 注册完成后统一套用。
 *
 * @param itemName 物品资源位置，如 "minecraft:netherite_ingot"
 * @return 原版稀有度；表中没有（即 common）时返回 nullopt
 */
[[nodiscard]] std::optional<ItemRarity> vanillaItemRarity(std::string_view itemName);

} // namespace mc