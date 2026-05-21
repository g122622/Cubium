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

#include "LootParameterSet.hpp"

namespace mc {
namespace loot {

/**
 * @brief 预定义参数集合工厂
 *
 * 提供每种掉落表类型对应的参数集合，包含正确的必需和可选参数。
 * 参考: net.minecraft.loot.LootParameterSets
 */
namespace LootParameterSets {
extern LootParameterSet empty();
extern LootParameterSet block();
extern LootParameterSet chest();
extern LootParameterSet entity();
extern LootParameterSet fishing();
extern LootParameterSet gift();
extern LootParameterSet barter();
extern LootParameterSet generic();
} // namespace LootParameterSets

} // namespace loot
} // namespace mc
