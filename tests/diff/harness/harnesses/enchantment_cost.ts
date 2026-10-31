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

/*
 * enchantment_cost.ts — 附魔等级与代价曲线能力的输入构造。
 *
 * 单元的枚举由 oracle 的 `list` 决定（数据包 `enchantment/` 下有哪些附魔就是全集）。
 * 每个单元回答「这个附魔最高几级、铁砧代价多少、每级的附魔台等级区间是什么」。
 *
 * `minCost` / `maxCost` 是**按等级顺序**的数组，顺序有意义，不能声明为无序。
 */

/** 附魔 ID → 探针与 oracle 共用的一行输入。 */
export function enchantmentCostInput(unit: string): { enchantment: string } {
    return { enchantment: unit };
}